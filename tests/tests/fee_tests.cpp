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

#include <fc/smart_ref_impl.hpp>
#include <fc/uint128.hpp>

#include <graphene/chain/hardfork.hpp>

#include <graphene/chain/fba_accumulator_id.hpp>

#include <graphene/chain/fba_object.hpp>
#include <graphene/chain/market_object.hpp>
#include <graphene/chain/vesting_balance_object.hpp>
#include <graphene/chain/exceptions.hpp>

#include <boost/test/unit_test.hpp>

#include "../common/database_fixture.hpp"

using namespace graphene::chain;
using namespace graphene::chain::test;

BOOST_FIXTURE_TEST_SUITE( fee_tests, database_fixture )

BOOST_AUTO_TEST_CASE( nonzero_fee_test )
{
   try
   {
      ACTORS((alice)(bob));

      const share_type prec = asset::scaled_precision( asset_id_type()(db).precision );

      // Return number of core shares (times precision)
      auto _core = [&]( int64_t x ) -> asset
      {  return asset( x*prec );    };

      transfer( committee_account, alice_id, _core(1000000) );

      // make sure the database requires our fee to be nonzero
      enable_fees();

      signed_transaction tx;
      transfer_operation xfer_op;
      xfer_op.from = alice_id;
      xfer_op.to = bob_id;
      xfer_op.amount = _core(1000);
      xfer_op.fee = _core(0);
      tx.operations.push_back( xfer_op );
      set_expiration( db, tx );
      sign( tx, alice_private_key );
      GRAPHENE_REQUIRE_THROW( PUSH_TX( db, tx ), insufficient_fee );
   }
   catch( const fc::exception& e )
   {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE(asset_claim_fees_test)
{
   try
   {
      ACTORS((alice)(bob)(izzy)(jill));
      // Izzy issues asset to Alice
      // Jill issues asset to Bob
      // Alice and Bob trade in the market and pay fees
      // Verify that Izzy and Jill can claim the fees

      const share_type core_prec = asset::scaled_precision( asset_id_type()(db).precision );

      // Return number of core shares (times precision)
      auto _core = [&]( int64_t x ) -> asset
      {  return asset( x*core_prec );    };

      transfer( committee_account, alice_id, _core(1000000) );
      transfer( committee_account,   bob_id, _core(1000000) );
      transfer( committee_account,  izzy_id, _core(1000000) );
      transfer( committee_account,  jill_id, _core(1000000) );

      asset_id_type izzycoin_id = create_bitasset( "IZZYCOIN", izzy_id,   GRAPHENE_1_PERCENT, charge_market_fee ).id;
      asset_id_type jillcoin_id = create_bitasset( "JILLCOIN", jill_id, 2*GRAPHENE_1_PERCENT, charge_market_fee ).id;

      const share_type izzy_prec = asset::scaled_precision( asset_id_type(izzycoin_id)(db).precision );
      const share_type jill_prec = asset::scaled_precision( asset_id_type(jillcoin_id)(db).precision );

      auto _izzy = [&]( int64_t x ) -> asset
      {   return asset( x*izzy_prec, izzycoin_id );   };
      auto _jill = [&]( int64_t x ) -> asset
      {   return asset( x*jill_prec, jillcoin_id );   };

      update_feed_producers( izzycoin_id(db), { izzy_id } );
      update_feed_producers( jillcoin_id(db), { jill_id } );

      const asset izzy_satoshi = asset(1, izzycoin_id);
      const asset jill_satoshi = asset(1, jillcoin_id);

      // Izzycoin is worth 100 BTS
      price_feed feed;
      feed.settlement_price = price( _izzy(1), _core(100) );
      feed.maintenance_collateral_ratio = 175 * GRAPHENE_COLLATERAL_RATIO_DENOM / 100;
      feed.maximum_short_squeeze_ratio = 150 * GRAPHENE_COLLATERAL_RATIO_DENOM / 100;
      publish_feed( izzycoin_id(db), izzy, feed );

      // Jillcoin is worth 30 BTS
      feed.settlement_price = price( _jill(1), _core(30) );
      feed.maintenance_collateral_ratio = 175 * GRAPHENE_COLLATERAL_RATIO_DENOM / 100;
      feed.maximum_short_squeeze_ratio = 150 * GRAPHENE_COLLATERAL_RATIO_DENOM / 100;
      publish_feed( jillcoin_id(db), jill, feed );

      enable_fees();

      // Alice and Bob create some coins
      borrow( alice_id, _izzy( 200), _core( 60000) );
      borrow(   bob_id, _jill(2000), _core(180000) );

      // Alice and Bob place orders which match
      create_sell_order( alice_id, _izzy(100), _jill(300) );   // Alice is willing to sell her Izzy's for 3 Jill
      create_sell_order(   bob_id, _jill(700), _izzy(200) );   // Bob is buying up to 200 Izzy's for up to 3.5 Jill

      // 100 Izzys and 300 Jills are matched, so the fees should be
      //   1 Izzy (1%) and 6 Jill (2%).

      auto claim_fees = [&]( account_id_type issuer, asset amount_to_claim )
      {
         asset_claim_fees_operation claim_op;
         claim_op.issuer = issuer;
         claim_op.amount_to_claim = amount_to_claim;
         signed_transaction tx;
         tx.operations.push_back( claim_op );
         db.current_fee_schedule().set_fee( tx.operations.back() );
         set_expiration( db, tx );
         fc::ecc::private_key   my_pk = (issuer == izzy_id) ? izzy_private_key : jill_private_key;
         fc::ecc::private_key your_pk = (issuer == izzy_id) ? jill_private_key : izzy_private_key;
         sign( tx, your_pk );
         GRAPHENE_REQUIRE_THROW( PUSH_TX( db, tx ), fc::exception );
         tx.signatures.clear();
         sign( tx, my_pk );
         PUSH_TX( db, tx );
      };

      {
         const asset_object& izzycoin = izzycoin_id(db);
         const asset_object& jillcoin = jillcoin_id(db);

         //wdump( (izzycoin)(izzycoin.dynamic_asset_data_id(db))((*izzycoin.bitasset_data_id)(db)) );
         //wdump( (jillcoin)(jillcoin.dynamic_asset_data_id(db))((*jillcoin.bitasset_data_id)(db)) );

         // check the correct amount of fees has been awarded
         BOOST_CHECK( izzycoin.dynamic_asset_data_id(db).accumulated_fees == _izzy(1).amount );
         BOOST_CHECK( jillcoin.dynamic_asset_data_id(db).accumulated_fees == _jill(6).amount );

      }

      {
         const asset_object& izzycoin = izzycoin_id(db);
         const asset_object& jillcoin = jillcoin_id(db);

         // can't claim more than balance
         GRAPHENE_REQUIRE_THROW( claim_fees( izzy_id, _izzy(1) + izzy_satoshi ), fc::exception );
         GRAPHENE_REQUIRE_THROW( claim_fees( jill_id, _jill(6) + jill_satoshi ), fc::exception );

         // can't claim asset that doesn't belong to you
         GRAPHENE_REQUIRE_THROW( claim_fees( jill_id, izzy_satoshi ), fc::exception );
         GRAPHENE_REQUIRE_THROW( claim_fees( izzy_id, jill_satoshi ), fc::exception );

         // can claim asset in one go
         claim_fees( izzy_id, _izzy(1) );
         GRAPHENE_REQUIRE_THROW( claim_fees( izzy_id, izzy_satoshi ), fc::exception );
         BOOST_CHECK( izzycoin.dynamic_asset_data_id(db).accumulated_fees == _izzy(0).amount );

         // can claim in multiple goes
         claim_fees( jill_id, _jill(4) );
         BOOST_CHECK( jillcoin.dynamic_asset_data_id(db).accumulated_fees == _jill(2).amount );
         GRAPHENE_REQUIRE_THROW( claim_fees( jill_id, _jill(2) + jill_satoshi ), fc::exception );
         claim_fees( jill_id, _jill(2) );
         BOOST_CHECK( jillcoin.dynamic_asset_data_id(db).accumulated_fees == _jill(0).amount );
      }
   }
   FC_LOG_AND_RETHROW()
}


BOOST_AUTO_TEST_CASE( account_create_fee_scaling )
{ try {
   auto accounts_per_scale = db.get_global_properties().parameters.accounts_per_fee_scale;
   db.modify(global_property_id_type()(db), [](global_property_object& gpo)
   {
      gpo.parameters.current_fees = fee_schedule::get_default();
      gpo.parameters.current_fees->get<account_create_operation>().basic_fee = 1;
   });

   for( int i = db.get_dynamic_global_properties().accounts_registered_this_interval; i < accounts_per_scale; ++i )
   {
      BOOST_CHECK_EQUAL(db.get_global_properties().parameters.current_fees->get<account_create_operation>().basic_fee, 1);
      create_account("shill" + fc::to_string(i));
   }
   for( int i = 0; i < accounts_per_scale; ++i )
   {
      BOOST_CHECK_EQUAL(db.get_global_properties().parameters.current_fees->get<account_create_operation>().basic_fee, 16);
      create_account("moreshills" + fc::to_string(i));
   }
   for( int i = 0; i < accounts_per_scale; ++i )
   {
      BOOST_CHECK_EQUAL(db.get_global_properties().parameters.current_fees->get<account_create_operation>().basic_fee, 256);
      create_account("moarshills" + fc::to_string(i));
   }
   BOOST_CHECK_EQUAL(db.get_global_properties().parameters.current_fees->get<account_create_operation>().basic_fee, 4096);

   generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);
   BOOST_CHECK_EQUAL(db.get_global_properties().parameters.current_fees->get<account_create_operation>().basic_fee, 1);
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( fee_refund_test )
{
   try
   {
      ACTORS((alice)(bob)(izzy));

      int64_t alice_b0 = 1000000, bob_b0 = 1000000;

      transfer( account_id_type(), alice_id, asset(alice_b0) );
      transfer( account_id_type(), bob_id, asset(bob_b0) );

      asset_id_type core_id = asset_id_type();
      asset_id_type usd_id = create_user_issued_asset( "IZZYUSD", izzy_id(db), charge_market_fee ).id;
      issue_uia( alice_id, asset( alice_b0, usd_id ) );
      issue_uia( bob_id, asset( bob_b0, usd_id ) );

      int64_t order_create_fee = 537;
      int64_t order_cancel_fee = 129;

      uint32_t skip = database::skip_witness_signature
                    | database::skip_transaction_signatures
                    | database::skip_transaction_dupe_check
                    | database::skip_block_size_check
                    | database::skip_tapos_check
                    | database::skip_authority_check
                    | database::skip_merkle_check
                    ;

      generate_block( skip );

      for( int i=0; i<2; i++ )
      {
         if( i == 1 )
         {
            generate_block( skip );
         }

         // enable_fees() and change_fees() modifies DB directly, and results will be overwritten by block generation
         // so we have to do it every time we stop generating/popping blocks and start doing tx's
         enable_fees();
         /*
         change_fees({
                       limit_order_create_operation::fee_parameters_type { order_create_fee },
                       limit_order_cancel_operation::fee_parameters_type { order_cancel_fee }
                     });
         */
         // C++ -- The above commented out statement doesn't work, I don't know why
         // so we will use the following rather lengthy initialization instead
         {
            flat_set< fee_parameters > new_fees;
            {
               limit_order_create_operation::fee_parameters_type create_fee_params;
               create_fee_params.fee = order_create_fee;
               new_fees.insert( create_fee_params );
            }
            {
               limit_order_cancel_operation::fee_parameters_type cancel_fee_params;
               cancel_fee_params.fee = order_cancel_fee;
               new_fees.insert( cancel_fee_params );
            }
            change_fees( new_fees );
         }

         // Alice creates order
         // Bob creates order which doesn't match

         // AAAAGGHH create_sell_order reads trx.expiration #469
         set_expiration( db, trx );

         // Check non-overlapping

         limit_order_id_type ao1_id = create_sell_order( alice_id, asset(1000), asset(1000, usd_id) )->id;
         limit_order_id_type bo1_id = create_sell_order(   bob_id, asset(500, usd_id), asset(1000) )->id;

         BOOST_CHECK_EQUAL( get_balance( alice_id, core_id ), alice_b0 - 1000 - order_create_fee );
         BOOST_CHECK_EQUAL( get_balance( alice_id,  usd_id ), alice_b0 );
         BOOST_CHECK_EQUAL( get_balance(   bob_id, core_id ), bob_b0 - order_create_fee );
         BOOST_CHECK_EQUAL( get_balance(   bob_id,  usd_id ), bob_b0 - 500 );

         // Bob cancels order
         cancel_limit_order( bo1_id(db) );

         int64_t cancel_net_fee;
         cancel_net_fee = order_cancel_fee;

         BOOST_CHECK_EQUAL( get_balance( alice_id, core_id ), alice_b0 - 1000 - order_create_fee );
         BOOST_CHECK_EQUAL( get_balance( alice_id,  usd_id ), alice_b0 );
         BOOST_CHECK_EQUAL( get_balance(   bob_id, core_id ), bob_b0 - cancel_net_fee );
         BOOST_CHECK_EQUAL( get_balance(   bob_id,  usd_id ), bob_b0 );

         // Alice cancels order
         cancel_limit_order( ao1_id(db) );

         BOOST_CHECK_EQUAL( get_balance( alice_id, core_id ), alice_b0 - cancel_net_fee );
         BOOST_CHECK_EQUAL( get_balance( alice_id,  usd_id ), alice_b0 );
         BOOST_CHECK_EQUAL( get_balance(   bob_id, core_id ), bob_b0 - cancel_net_fee );
         BOOST_CHECK_EQUAL( get_balance(   bob_id,  usd_id ), bob_b0 );

         // Check partial fill
         const limit_order_object* ao2 = create_sell_order( alice_id, asset(1000), asset(200, usd_id) );
         const limit_order_object* bo2 = create_sell_order(   bob_id, asset(100, usd_id), asset(500) );

         BOOST_CHECK( ao2 != nullptr );
         BOOST_CHECK( bo2 == nullptr );

         BOOST_CHECK_EQUAL( get_balance( alice_id, core_id ), alice_b0 - cancel_net_fee - order_create_fee - 1000 );
         BOOST_CHECK_EQUAL( get_balance( alice_id,  usd_id ), alice_b0 + 100 );
         BOOST_CHECK_EQUAL( get_balance(   bob_id, core_id ),   bob_b0 - cancel_net_fee - order_create_fee + 500 );
         BOOST_CHECK_EQUAL( get_balance(   bob_id,  usd_id ),   bob_b0 - 100 );

         // cancel Alice order, show that entire deferred_fee was consumed by partial match
         cancel_limit_order( *ao2 );

         BOOST_CHECK_EQUAL( get_balance( alice_id, core_id ), alice_b0 - cancel_net_fee - order_create_fee - 500 - order_cancel_fee );
         BOOST_CHECK_EQUAL( get_balance( alice_id,  usd_id ), alice_b0 + 100 );
         BOOST_CHECK_EQUAL( get_balance(   bob_id, core_id ),   bob_b0 - cancel_net_fee - order_create_fee + 500 );
         BOOST_CHECK_EQUAL( get_balance(   bob_id,  usd_id ),   bob_b0 - 100 );

         // TODO: Check multiple fill
         // there really should be a test case involving Alice creating multiple orders matched by single Bob order
         // but we'll save that for future cleanup

         // undo above tx's and reset
         generate_block( skip );
         db.pop_block();
      }
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( stealth_fba_test )
{
   try
   {
      ACTORS( (alice)(bob)(chloe)(dan)(izzy)(philbin)(tom) );
      // Philbin (registrar who registers Rex)

      // Izzy (initial issuer of stealth asset, will later transfer to Tom)
      // Alice, Bob, Chloe, Dan (ABCD)
      // Rex (recycler -- buyback account for stealth asset)
      // Tom (owner of stealth asset who will be set as top_n authority)

      // Izzy creates STEALTH
      asset_id_type stealth_id = create_user_issued_asset( "STEALTH", izzy_id(db),
         disable_confidential | transfer_restricted | override_authority | white_list | charge_market_fee ).id;

      /*
      // this is disabled because it doesn't work, our modify() is probably being overwritten by undo

      //
      // Init blockchain with stealth ID's
      // On a real chain, this would be done with #define GRAPHENE_FBA_STEALTH_DESIGNATED_ASSET
      // causing the designated_asset fields of these objects to be set at genesis, but for
      // this test we modify the db directly.
      //
      auto set_fba_asset = [&]( uint64_t fba_acc_id, asset_id_type asset_id )
      {
         db.modify( fba_accumulator_id_type(fba_acc_id)(db), [&]( fba_accumulator_object& fba )
         {
            fba.designated_asset = asset_id;
         } );
      };

      set_fba_asset( fba_accumulator_id_transfer_to_blind  , stealth_id );
      set_fba_asset( fba_accumulator_id_blind_transfer     , stealth_id );
      set_fba_asset( fba_accumulator_id_transfer_from_blind, stealth_id );
      */

      // Izzy kills some permission bits (this somehow happened to the real STEALTH in production)
      {
         asset_update_operation update_op;
         update_op.issuer = izzy_id;
         update_op.asset_to_update = stealth_id;
         asset_options new_options;
         new_options = stealth_id(db).options;
         new_options.issuer_permissions = charge_market_fee;
         new_options.flags = disable_confidential | transfer_restricted | override_authority | white_list | charge_market_fee;
         // after fixing #579 you should be able to delete the following line
         new_options.core_exchange_rate = price( asset( 1, stealth_id ), asset( 1, asset_id_type() ) );
         update_op.new_options = new_options;
         signed_transaction tx;
         tx.operations.push_back( update_op );
         set_expiration( db, tx );
         sign( tx, izzy_private_key );
         PUSH_TX( db, tx );
      }

      // Izzy transfers issuer duty to Tom
      {
         asset_update_operation update_op;
         update_op.issuer = izzy_id;
         update_op.asset_to_update = stealth_id;
         update_op.new_issuer = tom_id;
         // new_options should be optional, but isn't...the following line should be unnecessary #580
         update_op.new_options = stealth_id(db).options;
         signed_transaction tx;
         tx.operations.push_back( update_op );
         set_expiration( db, tx );
         sign( tx, izzy_private_key );
         PUSH_TX( db, tx );
      }

      // Tom re-enables the permission bits to clear the flags, then clears them again
      // Allowed by #572 when current_supply == 0
      {
         asset_update_operation update_op;
         update_op.issuer = tom_id;
         update_op.asset_to_update = stealth_id;
         asset_options new_options;
         new_options = stealth_id(db).options;
         new_options.issuer_permissions = new_options.flags | charge_market_fee;
         update_op.new_options = new_options;
         signed_transaction tx;
         // enable perms is one op
         tx.operations.push_back( update_op );

         new_options.issuer_permissions = charge_market_fee;
         new_options.flags = charge_market_fee;
         update_op.new_options = new_options;
         // reset wrongly set flags and reset permissions can be done in a single op
         tx.operations.push_back( update_op );

         set_expiration( db, tx );
         sign( tx, tom_private_key );
         PUSH_TX( db, tx );
      }

      // Philbin registers Rex who will be the asset's buyback, including sig from the new issuer (Tom)
      account_id_type rex_id;
      {
         buyback_account_options bbo;
         bbo.asset_to_buy = stealth_id;
         bbo.asset_to_buy_issuer = tom_id;
         bbo.markets.emplace( asset_id_type() );
         account_create_operation create_op = make_account( "rex" );
         create_op.registrar = philbin_id;
         create_op.extensions.value.buyback_options = bbo;
         create_op.owner = authority::null_authority();
         create_op.active = authority::null_authority();

         signed_transaction tx;
         tx.operations.push_back( create_op );
         set_expiration( db, tx );
         sign( tx, philbin_private_key );
         sign( tx, tom_private_key );

         processed_transaction ptx = PUSH_TX( db, tx );
         rex_id = ptx.operation_results.back().get< object_id_type >();
      }

      // Tom issues some asset to Alice and Bob
      set_expiration( db, trx );  // #11
      issue_uia( alice_id, asset( 1000, stealth_id ) );
      issue_uia(   bob_id, asset( 1000, stealth_id ) );

      // Tom sets his authority to the top_n of the asset
      {
         top_holders_special_authority top2;
         top2.num_top_holders = 2;
         top2.asset = stealth_id;

         account_update_operation op;
         op.account = tom_id;
         op.extensions.value.active_special_authority = top2;
         op.extensions.value.owner_special_authority = top2;

         signed_transaction tx;
         tx.operations.push_back( op );

         set_expiration( db, tx );
         sign( tx, tom_private_key );

         PUSH_TX( db, tx );
      }

      // Wait until the next maintenance interval for top_n to take effect
      generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);

      // Do a blind op to add some fees to the pool.
      fund( chloe_id(db), asset( 100000, asset_id_type() ) );

      auto create_transfer_to_blind = [&]( account_id_type account, asset amount, const std::string& key ) -> transfer_to_blind_operation
      {
         fc::ecc::private_key blind_key = fc::ecc::private_key::regenerate( fc::sha256::hash( key+"-privkey" ) );
         public_key_type blind_pub = blind_key.get_public_key();

         fc::sha256 secret = fc::sha256::hash( key+"-secret" );
         fc::sha256 nonce = fc::sha256::hash( key+"-nonce" );

         transfer_to_blind_operation op;
         blind_output blind_out;
         blind_out.owner = authority( 1, blind_pub, 1 );
         blind_out.commitment = fc::ecc::blind( secret, amount.amount.value );
         blind_out.range_proof = fc::ecc::range_proof_sign( 0, blind_out.commitment, secret, nonce, 0, 0, amount.amount.value );

         op.amount = amount;
         op.from = account;
         op.blinding_factor = fc::ecc::blind_sum( {secret}, 1 );
         op.outputs = {blind_out};

         return op;
      };

      {
         transfer_to_blind_operation op = create_transfer_to_blind( chloe_id, asset( 5000, asset_id_type() ), "chloe-key" );
         op.fee = asset( 1000, asset_id_type() );

         signed_transaction tx;
         tx.operations.push_back( op );
         set_expiration( db, tx );
         sign( tx, chloe_private_key );

         PUSH_TX( db, tx );
      }

      // wait until next maint interval
      generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);

      idump( ( get_operation_history( chloe_id ) ) );
      idump( ( get_operation_history( rex_id ) ) );
      idump( ( get_operation_history( tom_id ) ) );
   }
   catch( const fc::exception& e )
   {
      elog( "caught exception ${e}", ("e", e.to_detail_string()) );
      throw;
   }
}

BOOST_AUTO_TEST_SUITE_END()
