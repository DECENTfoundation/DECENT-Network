/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <boost/test/unit_test.hpp>

#include <graphene/chain/database.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/hardfork.hpp>

#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/budget_record_object.hpp>
#include <graphene/chain/miner_object.hpp>
//#include <graphene/chain/market_object.hpp>
#include <graphene/chain/vesting_balance_object.hpp>
#include <graphene/chain/withdraw_permission_object.hpp>
#include <graphene/chain/miner_object.hpp>

#include "../common/tempdir.hpp"

#include "../common/database_fixture.hpp"

using namespace graphene::chain;
using namespace graphene::chain::test;

BOOST_FIXTURE_TEST_SUITE( operation_tests, database_fixture )

BOOST_AUTO_TEST_CASE( withdraw_permission_create )
{ try {
   auto nathan_private_key = generate_private_key("nathan");
   auto dan_private_key = generate_private_key("danian");
   account_id_type nathan_id = create_account("nathan", nathan_private_key.get_public_key()).id;
   account_id_type dan_id = create_account("danian", dan_private_key.get_public_key()).id;

   transfer(account_id_type(), nathan_id, asset(1000));
   generate_block();
   set_expiration( db, trx );

   {
      withdraw_permission_create_operation op;
      op.authorized_account = dan_id;
      op.withdraw_from_account = nathan_id;
      op.withdrawal_limit = asset(5);
      op.withdrawal_period_sec = fc::hours(1).to_seconds();
      op.periods_until_expiration = 5;
      op.period_start_time = db.head_block_time() + db.get_global_properties().parameters.block_interval*5;
      trx.operations.push_back(op);
      REQUIRE_OP_VALIDATION_FAILURE(op, withdrawal_limit, asset());
      REQUIRE_OP_VALIDATION_FAILURE(op, periods_until_expiration, 0);
      REQUIRE_OP_VALIDATION_FAILURE(op, withdraw_from_account, dan_id);
      REQUIRE_OP_VALIDATION_FAILURE(op, withdrawal_period_sec, 0);
      REQUIRE_THROW_WITH_VALUE(op, withdrawal_limit, asset(10, asset_id_type(10)));
      REQUIRE_THROW_WITH_VALUE(op, authorized_account, account_id_type(1000));
      REQUIRE_THROW_WITH_VALUE(op, period_start_time, fc::time_point_sec(10000));
      REQUIRE_THROW_WITH_VALUE(op, withdrawal_period_sec, 1);
      trx.operations.back() = op;
   }
   sign( trx, nathan_private_key );
   db.push_transaction( trx );
   trx.clear();
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( withdraw_permission_test )
{ try {
   INVOKE(withdraw_permission_create);

   auto nathan_private_key = generate_private_key("nathan");
   auto dan_private_key = generate_private_key("danian");
   account_id_type nathan_id = get_account("nathan").id;
   account_id_type dan_id = get_account("danian").id;
   withdraw_permission_id_type permit;
   set_expiration( db, trx );

   fc::time_point_sec first_start_time;
   {
      const withdraw_permission_object& permit_object = permit(db);
      BOOST_CHECK(permit_object.authorized_account == dan_id);
      BOOST_CHECK(permit_object.withdraw_from_account == nathan_id);
      BOOST_CHECK(permit_object.period_start_time > db.head_block_time());
      first_start_time = permit_object.period_start_time;
      BOOST_CHECK(permit_object.withdrawal_limit == asset(5));
      BOOST_CHECK(permit_object.withdrawal_period_sec == fc::hours(1).to_seconds());
      BOOST_CHECK(permit_object.expiration == first_start_time + permit_object.withdrawal_period_sec*5 );
   }

   generate_blocks(2);

   {
      withdraw_permission_claim_operation op;
      op.withdraw_permission = permit;
      op.withdraw_from_account = nathan_id;
      op.withdraw_to_account = dan_id;
      op.amount_to_withdraw = asset(1);
      set_expiration( db, trx );

      trx.operations.push_back(op);
      //Throws because we haven't entered the first withdrawal period yet.
      GRAPHENE_REQUIRE_THROW(PUSH_TX( db, trx ), fc::exception);
      //Get to the actual withdrawal period
      generate_blocks(permit(db).period_start_time);

      REQUIRE_THROW_WITH_VALUE(op, withdraw_permission, withdraw_permission_id_type(5));
      REQUIRE_THROW_WITH_VALUE(op, withdraw_from_account, dan_id);
      REQUIRE_THROW_WITH_VALUE(op, withdraw_from_account, account_id_type());
      REQUIRE_THROW_WITH_VALUE(op, withdraw_to_account, nathan_id);
      REQUIRE_THROW_WITH_VALUE(op, withdraw_to_account, account_id_type());
      REQUIRE_THROW_WITH_VALUE(op, amount_to_withdraw, asset(10));
      REQUIRE_THROW_WITH_VALUE(op, amount_to_withdraw, asset(6));
      set_expiration( db, trx );
      trx.clear();
      trx.operations.push_back(op);
      sign( trx, dan_private_key );
      PUSH_TX( db, trx );

      // would be legal on its own, but doesn't work because trx already withdrew
      REQUIRE_THROW_WITH_VALUE(op, amount_to_withdraw, asset(5));

      // Make sure we can withdraw again this period, as long as we're not exceeding the periodic limit
      trx.clear();
      // withdraw 1
      trx.operations = {op};
      // make it different from previous trx so it's non-duplicate
      trx.expiration += fc::seconds(1);
      sign( trx, dan_private_key );
      PUSH_TX( db, trx );
      trx.clear();
   }

   BOOST_CHECK_EQUAL(get_balance(nathan_id, asset_id_type()), 998);
   BOOST_CHECK_EQUAL(get_balance(dan_id, asset_id_type()), 2);

   {
      const withdraw_permission_object& permit_object = permit(db);
      BOOST_CHECK(permit_object.authorized_account == dan_id);
      BOOST_CHECK(permit_object.withdraw_from_account == nathan_id);
      BOOST_CHECK(permit_object.period_start_time == first_start_time);
      BOOST_CHECK(permit_object.withdrawal_limit == asset(5));
      BOOST_CHECK(permit_object.withdrawal_period_sec == fc::hours(1).to_seconds());
      BOOST_CHECK_EQUAL(permit_object.claimed_this_period.value, 2 );
      BOOST_CHECK(permit_object.expiration == first_start_time + 5*permit_object.withdrawal_period_sec);
      generate_blocks(first_start_time + permit_object.withdrawal_period_sec);
      // lazy update:  verify period_start_time isn't updated until new trx occurs
      BOOST_CHECK(permit_object.period_start_time == first_start_time);
   }

   {
      transfer(nathan_id, dan_id, asset(997));
      withdraw_permission_claim_operation op;
      op.withdraw_permission = permit;
      op.withdraw_from_account = nathan_id;
      op.withdraw_to_account = dan_id;
      op.amount_to_withdraw = asset(5);
      trx.operations.push_back(op);
      set_expiration( db, trx );
      sign( trx, dan_private_key );
      //Throws because nathan doesn't have the money
      GRAPHENE_CHECK_THROW(PUSH_TX( db, trx ), fc::exception);
      op.amount_to_withdraw = asset(1);
      trx.clear();
      trx.operations = {op};
      set_expiration( db, trx );
      sign( trx, dan_private_key );
      PUSH_TX( db, trx );
   }

   BOOST_CHECK_EQUAL(get_balance(nathan_id, asset_id_type()), 0);
   BOOST_CHECK_EQUAL(get_balance(dan_id, asset_id_type()), 1000);
   trx.clear();
   transfer(dan_id, nathan_id, asset(1000));

   {
      const withdraw_permission_object& permit_object = permit(db);
      BOOST_CHECK(permit_object.authorized_account == dan_id);
      BOOST_CHECK(permit_object.withdraw_from_account == nathan_id);
      BOOST_CHECK(permit_object.period_start_time == first_start_time + permit_object.withdrawal_period_sec);
      BOOST_CHECK(permit_object.expiration == first_start_time + 5*permit_object.withdrawal_period_sec);
      BOOST_CHECK(permit_object.withdrawal_limit == asset(5));
      BOOST_CHECK(permit_object.withdrawal_period_sec == fc::hours(1).to_seconds());
      generate_blocks(permit_object.expiration);
   }
   // Ensure the permit object has been garbage collected
   BOOST_CHECK(db.find_object(permit) == nullptr);

   {
      withdraw_permission_claim_operation op;
      op.withdraw_permission = permit;
      op.withdraw_from_account = nathan_id;
      op.withdraw_to_account = dan_id;
      op.amount_to_withdraw = asset(5);
      trx.operations.push_back(op);
      set_expiration( db, trx );
      sign( trx, dan_private_key );
      //Throws because the permission has expired
      GRAPHENE_CHECK_THROW(PUSH_TX( db, trx ), fc::exception);
   }
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( withdraw_permission_nominal_case )
{ try {
   INVOKE(withdraw_permission_create);

   auto nathan_private_key = generate_private_key("nathan");
   auto dan_private_key = generate_private_key("danian");
   account_id_type nathan_id = get_account("nathan").id;
   account_id_type dan_id = get_account("danian").id;
   withdraw_permission_id_type permit;

   while(true)
   {
      const withdraw_permission_object& permit_object = permit(db);
      //wdump( (permit_object) );
      withdraw_permission_claim_operation op;
      op.withdraw_permission = permit;
      op.withdraw_from_account = nathan_id;
      op.withdraw_to_account = dan_id;
      op.amount_to_withdraw = asset(5);
      trx.operations.push_back(op);
      set_expiration( db, trx );
      sign( trx, dan_private_key );
      PUSH_TX( db, trx );
      // tx's involving withdraw_permissions can't delete it even
      // if no further withdrawals are possible
      BOOST_CHECK(db.find_object(permit) != nullptr);
      BOOST_CHECK( permit_object.claimed_this_period == 5 );
      trx.clear();
      generate_blocks(
           permit_object.period_start_time
         + permit_object.withdrawal_period_sec );
      if( db.find_object(permit) == nullptr )
         break;
   }

   BOOST_CHECK_EQUAL(get_balance(nathan_id, asset_id_type()), 975);
   BOOST_CHECK_EQUAL(get_balance(dan_id, asset_id_type()), 25);
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( withdraw_permission_update )
{ try {
   INVOKE(withdraw_permission_create);

   auto nathan_private_key = generate_private_key("nathan");
   account_id_type nathan_id = get_account("nathan").id;
   account_id_type dan_id = get_account("danian").id;
   withdraw_permission_id_type permit;
   set_expiration( db, trx );

   {
      withdraw_permission_update_operation op;
      op.permission_to_update = permit;
      op.authorized_account = dan_id;
      op.withdraw_from_account = nathan_id;
      op.periods_until_expiration = 2;
      op.period_start_time = db.head_block_time() + 10;
      op.withdrawal_period_sec = 10;
      op.withdrawal_limit = asset(12);
      trx.operations.push_back(op);
      REQUIRE_THROW_WITH_VALUE(op, periods_until_expiration, 0);
      REQUIRE_THROW_WITH_VALUE(op, withdrawal_period_sec, 0);
      REQUIRE_THROW_WITH_VALUE(op, withdrawal_limit, asset(1, asset_id_type(12)));
      REQUIRE_THROW_WITH_VALUE(op, withdrawal_limit, asset(0));
      REQUIRE_THROW_WITH_VALUE(op, withdraw_from_account, account_id_type(0));
      REQUIRE_THROW_WITH_VALUE(op, authorized_account, account_id_type(0));
      REQUIRE_THROW_WITH_VALUE(op, period_start_time, db.head_block_time() - 50);
      trx.operations.back() = op;
      sign( trx, nathan_private_key );
      PUSH_TX( db, trx );
   }

   {
      const withdraw_permission_object& permit_object = db.get(permit);
      BOOST_CHECK(permit_object.authorized_account == dan_id);
      BOOST_CHECK(permit_object.withdraw_from_account == nathan_id);
      BOOST_CHECK(permit_object.period_start_time == db.head_block_time() + 10);
      BOOST_CHECK(permit_object.withdrawal_limit == asset(12));
      BOOST_CHECK(permit_object.withdrawal_period_sec == 10);
      // BOOST_CHECK(permit_object.remaining_periods == 2);
   }
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( withdraw_permission_delete )
{ try {
   INVOKE(withdraw_permission_update);

   withdraw_permission_delete_operation op;
   op.authorized_account = get_account("danian").id;
   op.withdraw_from_account = get_account("nathan").id;
   set_expiration( db, trx );
   trx.operations.push_back(op);
   sign( trx, generate_private_key("nathan" ));
   PUSH_TX( db, trx );
} FC_LOG_AND_RETHROW() }

#if 0 //not working now
BOOST_AUTO_TEST_CASE( mia_feeds )
{ try {
   ACTORS((nathan)(danian)(benian)(vikram));
   asset_id_type bit_usd_id = create_monitored_asset("USDBIT").id;

   {
      asset_update_operation op;
      const asset_object& obj = bit_usd_id(db);
      op.asset_to_update = bit_usd_id;
      op.issuer = obj.issuer;
      //op.new_issuer = nathan_id;
      op.new_options = obj.options;
      trx.operations.push_back(op);
      PUSH_TX( db, trx, ~0 );
      generate_block();
      trx.clear();
   }
   {
      const asset_object& obj = bit_usd_id(db);
      BOOST_CHECK_EQUAL(obj.options.monitored_asset_opts->feeds.size(), 3);
   }
   {
      const asset_object& bit_usd = bit_usd_id(db);
      asset_publish_feed_operation op;
      op.publisher = vikram_id;
      op.asset_id = bit_usd_id;
      op.feed.core_exchange_rate = ~price(asset(GRAPHENE_BLOCKCHAIN_PRECISION),bit_usd.amount(30));

      // We'll expire margins after a month
      // Accept defaults for required collateral
      trx.operations.emplace_back(op);
      PUSH_TX( db, trx, ~0 );

      op.publisher = ben_id;
      op.feed.core_exchange_rate = ~price(asset(GRAPHENE_BLOCKCHAIN_PRECISION),bit_usd.amount(25));
      trx.operations.back() = op;
      PUSH_TX( db, trx, ~0 );

      op.publisher = dan_id;
      op.feed.core_exchange_rate = ~price(asset(GRAPHENE_BLOCKCHAIN_PRECISION),bit_usd.amount(40));
      trx.operations.back() = op;
      PUSH_TX( db, trx, ~0 );

      op.publisher = nathan_id;
      trx.operations.back() = op;
      GRAPHENE_CHECK_THROW(PUSH_TX( db, trx, ~0 ), fc::exception);
   }
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( feed_limit_test )
{ try {
   INVOKE( mia_feeds );
   const asset_object& bit_usd = get_asset("USDBIT");
   GET_ACTOR(nathan);

   BOOST_TEST_MESSAGE("Setting minimum feeds to 4");
   asset_update_monitored_asset_operation op;
   op.new_options.minimum_feeds = 4;
   op.asset_to_update = bit_usd.get_id();
   op.issuer = bit_usd.issuer;
   trx.operations = {op};
   sign( trx, nathan_private_key );
   db.push_transaction(trx);

   BOOST_TEST_MESSAGE("Setting minimum feeds to 3");
   op.new_options.minimum_feeds = 3;
   trx.clear();
   trx.operations = {op};
   sign( trx, nathan_private_key );
   db.push_transaction(trx);

} FC_LOG_AND_RETHROW() }
#endif

BOOST_AUTO_TEST_CASE( miner_create )
{ try {
   ACTOR(nathan);
   trx.clear();
   miner_id_type nathan_miner_id = create_miner(nathan_id, nathan_private_key).id;
   // Give nathan some voting stake
   transfer(miner_account, nathan_id, asset(10000000));
   generate_block();
   set_expiration( db, trx );

   {
      account_update_operation op;
      op.account = nathan_id;
      op.new_options = nathan_id(db).options;
      op.new_options->votes.insert(nathan_miner_id(db).vote_id);
      op.new_options->num_miner = std::count_if(op.new_options->votes.begin(), op.new_options->votes.end(),
                                                  [](vote_id_type id) { return id.type() == vote_id_type::miner; });
      trx.operations.push_back(op);
      sign( trx, nathan_private_key );
      PUSH_TX( db, trx );
      trx.clear();
   }

   generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);
   const auto& miners = db.get_global_properties().active_miners;

   // make sure we're in active_miners
   auto itr = std::find(miners.begin(), miners.end(), nathan_miner_id);
   BOOST_CHECK(itr != miners.end());

   // generate blocks until we are at the beginning of a round
   while( ((db.get_dynamic_global_properties().current_aslot + 1) % miners.size()) != 0 )
      generate_block();

   int produced = 0;
   // Make sure we get scheduled at least once in miners.size()*2 blocks
   // may take this many unless we measure where in the scheduling round we are
   // TODO:  intense_test that repeats this loop many times
   for( size_t i=0, n=miners.size()*2; i<n; i++ )
   {
      signed_block block = generate_block();
      if( block.miner == nathan_miner_id )
         produced++;
   }
   BOOST_CHECK_GE( produced, 1 );
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( assert_op_test )
{
   try {
   // create some objects
   auto nathan_private_key = generate_private_key("nathan");
   public_key_type nathan_public_key = nathan_private_key.get_public_key();
   account_id_type nathan_id = create_account("nathan", nathan_public_key).id;

   assert_operation op;

   // nathan checks that his public key is equal to the given value.
   op.fee_paying_account = nathan_id;
   op.predicates.emplace_back(account_name_eq_lit_predicate{ nathan_id, "nathan" });
   trx.operations.push_back(op);
   sign( trx, nathan_private_key );
   PUSH_TX( db, trx );

   // nathan checks that his public key is not equal to the given value (fail)
   trx.clear();
   op.predicates.emplace_back(account_name_eq_lit_predicate{ nathan_id, "danian" });
   trx.operations.push_back(op);
   sign( trx, nathan_private_key );
   GRAPHENE_CHECK_THROW( PUSH_TX( db, trx ), fc::exception );
   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(transfer_with_memo) {
   try {
      ACTOR(alice);
      ACTOR(bobian);
      transfer(account_id_type(), alice_id, asset(1000));
      BOOST_CHECK_EQUAL(get_balance(alice_id, asset_id_type()), 1000);

      transfer_operation op;
      op.from = alice_id;
      op.to = bobian_id;
      op.amount = asset(500);
      op.memo = memo_data();
      op.memo->set_message(alice_private_key, bobian_public_key, "Dear Bob,\n\nMoney!\n\nLove, Alice");
      trx.operations = {op};
      trx.sign(alice_private_key, db.get_chain_id());
      db.push_transaction(trx);

      BOOST_CHECK_EQUAL(get_balance(alice_id, asset_id_type()), 500);
      BOOST_CHECK_EQUAL(get_balance(bobian_id, asset_id_type()), 500);

      auto memo = db.get_recent_transaction(trx.id()).operations.front().get<transfer_operation>().memo;
      BOOST_CHECK(memo);
      BOOST_CHECK_EQUAL(memo->get_message(bobian_private_key, alice_public_key), "Dear Bob,\n\nMoney!\n\nLove, Alice");
   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(zero_second_vbo)
{
   try
   {
      ACTOR(alice);
      // don't pay miners so we have some budget to work with

      transfer(account_id_type(), alice_id, asset(int64_t(100000) * 100 * 1000 * 1000));

      enable_fees();
      generate_block();

      // Wait for a maintenance interval to ensure we have a full day's budget to work with.
      // Otherwise we may not have enough to feed the miners and the worker will end up starved if we start late in the day.
      generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);
      generate_block();

      auto check_vesting_1b = [&](vesting_balance_id_type vbid)
      {
         // this function checks that Alice can't draw any right now,
         // but one block later, she can withdraw it all.

         vesting_balance_withdraw_operation withdraw_op;
         withdraw_op.vesting_balance = vbid;
         withdraw_op.owner = alice_id;
         withdraw_op.amount = asset(1);

         signed_transaction withdraw_tx;
         withdraw_tx.operations.push_back( withdraw_op );
         sign(withdraw_tx, alice_private_key);
         GRAPHENE_REQUIRE_THROW( PUSH_TX( db, withdraw_tx ), fc::exception );

         generate_block();
         withdraw_tx = signed_transaction();
         withdraw_op.amount = asset(500);
         withdraw_tx.operations.push_back( withdraw_op );
         set_expiration( db, withdraw_tx );
         sign(withdraw_tx, alice_private_key);
         PUSH_TX( db, withdraw_tx );
      };

      // This block creates a zero-second VBO with a vesting_balance_create_operation.
      {
         cdd_vesting_policy_initializer pinit;
         pinit.vesting_seconds = 0;

         vesting_balance_create_operation create_op;
         create_op.creator = alice_id;
         create_op.owner = alice_id;
         create_op.amount = asset(500);
         create_op.policy = pinit;

         signed_transaction create_tx;
         create_tx.operations.push_back( create_op );
         set_expiration( db, create_tx );
         sign(create_tx, alice_private_key);

         processed_transaction ptx = PUSH_TX( db, create_tx );
         vesting_balance_id_type vbid = ptx.operation_results[0].get<object_id_type>();
         check_vesting_1b( vbid );
      }

  } FC_LOG_AND_RETHROW()
}

// TODO:  Write linear VBO tests


BOOST_AUTO_TEST_SUITE_END()
