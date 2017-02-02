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
#include <graphene/chain/witness_object.hpp>
#include <graphene/chain/market_object.hpp>
#include <graphene/chain/vesting_balance_object.hpp>
#include <graphene/chain/withdraw_permission_object.hpp>
#include <graphene/chain/witness_object.hpp>

#include <graphene/utilities/tempdir.hpp>

#include <fc/crypto/digest.hpp>

#include "../common/database_fixture.hpp"

using namespace graphene::chain;
using namespace graphene::chain::test;

BOOST_FIXTURE_TEST_SUITE( operation_tests, database_fixture )

BOOST_AUTO_TEST_CASE( withdraw_permission_create )
{ try {
   auto nathan_private_key = generate_private_key("nathan");
   auto dan_private_key = generate_private_key("dan");
   account_id_type nathan_id = create_account("nathan", nathan_private_key.get_public_key()).id;
   account_id_type dan_id = create_account("dan", dan_private_key.get_public_key()).id;

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
   auto dan_private_key = generate_private_key("dan");
   account_id_type nathan_id = get_account("nathan").id;
   account_id_type dan_id = get_account("dan").id;
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
   auto dan_private_key = generate_private_key("dan");
   account_id_type nathan_id = get_account("nathan").id;
   account_id_type dan_id = get_account("dan").id;
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
   account_id_type dan_id = get_account("dan").id;
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
   op.authorized_account = get_account("dan").id;
   op.withdraw_from_account = get_account("nathan").id;
   set_expiration( db, trx );
   trx.operations.push_back(op);
   sign( trx, generate_private_key("nathan" ));
   PUSH_TX( db, trx );
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( mia_feeds )
{ try {
   ACTORS((nathan)(dan)(ben)(vikram));
   asset_id_type bit_usd_id = create_monitored_asset("USDBIT").id;

   {
      asset_update_operation op;
      const asset_object& obj = bit_usd_id(db);
      op.asset_to_update = bit_usd_id;
      op.issuer = obj.issuer;
      op.new_issuer = nathan_id;
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

BOOST_AUTO_TEST_CASE( witness_create )
{ try {
   ACTOR(nathan);
   trx.clear();
   witness_id_type nathan_witness_id = create_witness(nathan_id, nathan_private_key).id;
   // Give nathan some voting stake
   transfer(witness_account, nathan_id, asset(10000000));
   generate_block();
   set_expiration( db, trx );

   {
      account_update_operation op;
      op.account = nathan_id;
      op.new_options = nathan_id(db).options;
      op.new_options->votes.insert(nathan_witness_id(db).vote_id);
      op.new_options->num_witness = std::count_if(op.new_options->votes.begin(), op.new_options->votes.end(),
                                                  [](vote_id_type id) { return id.type() == vote_id_type::witness; });
      trx.operations.push_back(op);
      sign( trx, nathan_private_key );
      PUSH_TX( db, trx );
      trx.clear();
   }

   generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);
   const auto& witnesses = db.get_global_properties().active_witnesses;

   // make sure we're in active_witnesses
   auto itr = std::find(witnesses.begin(), witnesses.end(), nathan_witness_id);
   BOOST_CHECK(itr != witnesses.end());

   // generate blocks until we are at the beginning of a round
   while( ((db.get_dynamic_global_properties().current_aslot + 1) % witnesses.size()) != 0 )
      generate_block();

   int produced = 0;
   // Make sure we get scheduled at least once in witnesses.size()*2 blocks
   // may take this many unless we measure where in the scheduling round we are
   // TODO:  intense_test that repeats this loop many times
   for( size_t i=0, n=witnesses.size()*2; i<n; i++ )
   {
      signed_block block = generate_block();
      if( block.witness == nathan_witness_id )
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
   op.predicates.emplace_back(account_name_eq_lit_predicate{ nathan_id, "dan" });
   trx.operations.push_back(op);
   sign( trx, nathan_private_key );
   GRAPHENE_CHECK_THROW( PUSH_TX( db, trx ), fc::exception );
   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(transfer_with_memo) {
   try {
      ACTOR(alice);
      ACTOR(bob);
      transfer(account_id_type(), alice_id, asset(1000));
      BOOST_CHECK_EQUAL(get_balance(alice_id, asset_id_type()), 1000);

      transfer_operation op;
      op.from = alice_id;
      op.to = bob_id;
      op.amount = asset(500);
      op.memo = memo_data();
      op.memo->set_message(alice_private_key, bob_public_key, "Dear Bob,\n\nMoney!\n\nLove, Alice");
      trx.operations = {op};
      trx.sign(alice_private_key, db.get_chain_id());
      db.push_transaction(trx);

      BOOST_CHECK_EQUAL(get_balance(alice_id, asset_id_type()), 500);
      BOOST_CHECK_EQUAL(get_balance(bob_id, asset_id_type()), 500);

      auto memo = db.get_recent_transaction(trx.id()).operations.front().get<transfer_operation>().memo;
      BOOST_CHECK(memo);
      BOOST_CHECK_EQUAL(memo->get_message(bob_private_key, alice_public_key), "Dear Bob,\n\nMoney!\n\nLove, Alice");
   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(zero_second_vbo)
{
   try
   {
      ACTOR(alice);
      // don't pay witnesses so we have some budget to work with

      transfer(account_id_type(), alice_id, asset(int64_t(100000) * 100 * 1000 * 1000));

      enable_fees();
      generate_block();

      // Wait for a maintenance interval to ensure we have a full day's budget to work with.
      // Otherwise we may not have enough to feed the witnesses and the worker will end up starved if we start late in the day.
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

BOOST_AUTO_TEST_CASE( top_n_special )
{
   ACTORS( (alice)(bob)(chloe)(dan)(izzy)(stan) );


   try
   {
      {
         //
         // Izzy (issuer)
         // Stan (special authority)
         // Alice, Bob, Chloe, Dan (ABCD)
         //

         asset_id_type topn_id = create_user_issued_asset( "TOPN", izzy_id(db) ).id;
         authority stan_owner_auth = stan_id(db).owner;
         authority stan_active_auth = stan_id(db).active;

         // wait for maint interval
         // make sure we don't have any authority as account hasn't gotten distributed yet
         generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);

         BOOST_CHECK( stan_id(db).owner  == stan_owner_auth );
         BOOST_CHECK( stan_id(db).active == stan_active_auth );

         // issue some to Alice, make sure she gets control of Stan

         // we need to set_expiration() before issue_uia() because the latter doens't call it #11
         set_expiration( db, trx );  // #11
         issue_uia( alice_id, asset( 1000, topn_id ) );

         BOOST_CHECK( stan_id(db).owner  == stan_owner_auth );
         BOOST_CHECK( stan_id(db).active == stan_active_auth );

         generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);

         /*  NOTE - this was an old check from an earlier implementation that only allowed SA for LTM's
         // no boost yet, we need to upgrade to LTM before mechanics apply to Stan
         BOOST_CHECK( stan_id(db).owner  == stan_owner_auth );
         BOOST_CHECK( stan_id(db).active == stan_active_auth );

         set_expiration( db, trx );  // #11
         generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);
         */

         BOOST_CHECK( stan_id(db).owner  == authority(  501, alice_id, 1000 ) );
         BOOST_CHECK( stan_id(db).active == authority(  501, alice_id, 1000 ) );

         // give asset to Stan, make sure owner doesn't change at all
         set_expiration( db, trx );  // #11
         transfer( alice_id, stan_id, asset( 1000, topn_id ) );

         generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);

         BOOST_CHECK( stan_id(db).owner  == authority(  501, alice_id, 1000 ) );
         BOOST_CHECK( stan_id(db).active == authority(  501, alice_id, 1000 ) );

         set_expiration( db, trx );  // #11
         issue_uia( chloe_id, asset( 131000, topn_id ) );

         // now Chloe has 131,000 and Stan has 1k.  Make sure change occurs at next maintenance interval.
         // NB, 131072 is a power of 2; the number 131000 was chosen so that we need a bitshift, but
         // if we put the 1000 from Stan's balance back into play, we need a different bitshift.

         // we use Chloe so she can be displaced by Bob later (showing the tiebreaking logic).

         // Check Alice is still in control, because we're deferred to next maintenance interval
         BOOST_CHECK( stan_id(db).owner  == authority(  501, alice_id, 1000 ) );
         BOOST_CHECK( stan_id(db).active == authority(  501, alice_id, 1000 ) );

         generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);

         BOOST_CHECK( stan_id(db).owner  == authority( 32751, chloe_id, 65500 ) );
         BOOST_CHECK( stan_id(db).active == authority( 32751, chloe_id, 65500 ) );

         // put Alice's stake back in play
         set_expiration( db, trx );  // #11
         transfer( stan_id, alice_id, asset( 1000, topn_id ) );

         generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);

         BOOST_CHECK( stan_id(db).owner  == authority( 33001, alice_id, 500, chloe_id, 65500 ) );
         BOOST_CHECK( stan_id(db).active == authority( 33001, alice_id, 500, chloe_id, 65500 ) );

         // issue 200,000 to Dan to cause another bitshift.
         set_expiration( db, trx );  // #11
         issue_uia( dan_id, asset( 200000, topn_id ) );
         generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);

         // 200000 Dan
         // 131000 Chloe
         // 1000 Alice

         BOOST_CHECK( stan_id(db).owner  == authority( 41376,                chloe_id, 32750, dan_id, 50000 ) );
         BOOST_CHECK( stan_id(db).active == authority( 41501, alice_id, 250, chloe_id, 32750, dan_id, 50000 ) );

         // have Alice send all but 1 back to Stan, verify that we clamp Alice at one vote
         set_expiration( db, trx );  // #11
         transfer( alice_id, stan_id, asset( 999, topn_id ) );
         generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);

         BOOST_CHECK( stan_id(db).owner  == authority( 41376,                chloe_id, 32750, dan_id, 50000 ) );
         BOOST_CHECK( stan_id(db).active == authority( 41376, alice_id,   1, chloe_id, 32750, dan_id, 50000 ) );

         // send 131k to Bob so he's tied with Chloe, verify he displaces Chloe in top2
         set_expiration( db, trx );  // #11
         issue_uia( bob_id, asset( 131000, topn_id ) );
         generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);

         BOOST_CHECK( stan_id(db).owner  == authority( 41376, bob_id, 32750,                  dan_id, 50000 ) );
         BOOST_CHECK( stan_id(db).active == authority( 57751, bob_id, 32750, chloe_id, 32750, dan_id, 50000 ) );

         // TODO more rounding checks
      }

   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( buyback )
{
   ACTORS( (alice)(bob)(chloe)(dan)(izzy)(philbin) );


   try
   {
      {
         //
         // Izzy (issuer)
         // Alice, Bob, Chloe, Dan (ABCD)
         // Rex (recycler -- buyback account)
         // Philbin (registrar)
         //

         asset_id_type nono_id = create_user_issued_asset( "NONO", izzy_id(db) ).id;
         asset_id_type buyme_id = create_user_issued_asset( "BUYME", izzy_id(db) ).id;

         // Create a buyback account
         account_id_type rex_id;
         {
            buyback_account_options bbo;
            bbo.asset_to_buy = buyme_id;
            bbo.asset_to_buy_issuer = izzy_id;
            bbo.markets.emplace( asset_id_type() );
            account_create_operation create_op = make_account( "rex" );
            create_op.registrar = philbin_id;
            create_op.extensions.value.buyback_options = bbo;
            create_op.owner = authority::null_authority();
            create_op.active = authority::null_authority();

            // Let's break it...

            signed_transaction tx;
            tx.operations.push_back( create_op );
            set_expiration( db, tx );

            tx.operations.back().get< account_create_operation >().extensions.value.buyback_options->asset_to_buy_issuer = alice_id;
            sign( tx, alice_private_key );
            sign( tx, philbin_private_key );

            // Alice and Philbin signed, but asset issuer is invalid
            GRAPHENE_CHECK_THROW( db.push_transaction(tx), account_create_buyback_incorrect_issuer );

            tx.signatures.clear();
            tx.operations.back().get< account_create_operation >().extensions.value.buyback_options->asset_to_buy_issuer = izzy_id;
            sign( tx, philbin_private_key );

            // Izzy didn't sign
            GRAPHENE_CHECK_THROW( db.push_transaction(tx), tx_missing_active_auth );
            sign( tx, izzy_private_key );

            // OK
            processed_transaction ptx = db.push_transaction( tx );
            rex_id = ptx.operation_results.back().get< object_id_type >();

            // Try to create another account rex2 which is bbo on same asset
            tx.signatures.clear();
            tx.operations.back().get< account_create_operation >().name = "rex2";
            sign( tx, izzy_private_key );
            sign( tx, philbin_private_key );
            GRAPHENE_CHECK_THROW( db.push_transaction(tx), account_create_buyback_already_exists );
         }

         // issue some BUYME to Alice
         // we need to set_expiration() before issue_uia() because the latter doens't call it #11
         set_expiration( db, trx );  // #11
         issue_uia( alice_id, asset( 1000, buyme_id ) );
         issue_uia( alice_id, asset( 1000, nono_id ) );

         // Alice wants to sell 100 BUYME for 1000 BTS, a middle price.
         limit_order_id_type order_id_mid = create_sell_order( alice_id, asset( 100, buyme_id ), asset( 1000, asset_id_type() ) )->id;

         generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);
         generate_block();

         // no success because buyback has none for sale
         BOOST_CHECK( order_id_mid(db).for_sale == 100 );

         // but we can send some to buyback
         fund( rex_id(db), asset( 100, asset_id_type() ) );
         // no action until next maint
         BOOST_CHECK( order_id_mid(db).for_sale == 100 );
         generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);
         generate_block();

         // partial fill, Alice now sells 90 BUYME for 900 BTS.
         BOOST_CHECK( order_id_mid(db).for_sale == 90 );

         // TODO check burn amount

         // aagh more state in trx
         set_expiration( db, trx );  // #11

         // Selling 10 BUYME for 50 BTS, a low price.
         limit_order_id_type order_id_low  = create_sell_order( alice_id, asset( 10, buyme_id ), asset(  50, asset_id_type() ) )->id;
         // Selling 10 BUYME for 150 BTS, a high price.
         limit_order_id_type order_id_high = create_sell_order( alice_id, asset( 10, buyme_id ), asset( 150, asset_id_type() ) )->id;

         fund( rex_id(db), asset( 250, asset_id_type() ) );
         generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);
         generate_block();

         BOOST_CHECK( db.find( order_id_low  ) == nullptr );
         BOOST_CHECK( db.find( order_id_mid  ) != nullptr );
         BOOST_CHECK( db.find( order_id_high ) != nullptr );

         // 250 CORE in rex                 90 BUYME in mid order    10 BUYME in low order
         //  50 CORE goes to low order, buy 10 for 50 CORE
         // 200 CORE goes to mid order, buy 20 for 200 CORE
         //                                 70 BUYME in mid order     0 BUYME in low order

         idump( (order_id_mid(db)) );
         BOOST_CHECK( order_id_mid(db).for_sale == 70 );
         BOOST_CHECK( order_id_high(db).for_sale == 10 );

         BOOST_CHECK( get_balance( rex_id, asset_id_type() ) == 0 );

         // clear out the books -- 700 left on mid order, 150 left on high order, so 2000 BTS should result in 1150 left over

         fund( rex_id(db), asset( 2000, asset_id_type() ) );
         generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);

         idump( (get_balance( rex_id, asset_id_type() )) );

         BOOST_CHECK( get_balance( rex_id, asset_id_type() ) == 1150 );

         GRAPHENE_CHECK_THROW( transfer( alice_id, rex_id, asset( 1, nono_id ) ), fc::exception );
         // TODO: Check cancellation works for account which is BTS-restricted
      }

   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
