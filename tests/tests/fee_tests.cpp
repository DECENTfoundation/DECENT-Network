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

#include <fc/smart_ref_impl.hpp>
#include <fc/uint128.hpp>

#include <graphene/chain/hardfork.hpp>
//#include <graphene/chain/market_object.hpp>
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
      ACTORS((alice)(bobian));

      const share_type prec = asset::scaled_precision( asset_id_type()(db).precision );

      // Return number of core shares (times precision)
      auto _core = [&]( int64_t x ) -> asset
      {  return asset( x*prec );    };

      transfer( miner_account, alice_id, _core(1000000) );

      // make sure the database requires our fee to be nonzero
      enable_fees();

      signed_transaction tx;
      transfer_operation xfer_op;
      xfer_op.from = alice_id;
      xfer_op.to = bobian_id;
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
      ACTORS((alice)(bobian)(izzyt)(jilly));
      // Izzy issues asset to Alice
      // Jill issues asset to Bob
      // Alice and Bob trade in the market and pay fees
      // Verify that Izzy and Jill can claim the fees

      const share_type core_prec = asset::scaled_precision( asset_id_type()(db).precision );

      // Return number of core shares (times precision)
      auto _core = [&]( int64_t x ) -> asset
      {  return asset( x*core_prec );    };

      transfer( miner_account, alice_id, _core(1000000) );
      transfer( miner_account, bobian_id, _core(1000000) );
      transfer( miner_account, izzyt_id, _core(1000000) );
      transfer( miner_account, jilly_id, _core(1000000) );

      asset_id_type izzycoin_id = create_monitored_asset( "IZZYCOIN", izzyt_id ).id;
      asset_id_type jillcoin_id = create_monitored_asset( "JILLCOIN", jilly_id ).id;

      const share_type izzy_prec = asset::scaled_precision( asset_id_type(izzycoin_id)(db).precision );
      const share_type jill_prec = asset::scaled_precision( asset_id_type(jillcoin_id)(db).precision );

      auto _izzy = [&]( int64_t x ) -> asset
      {   return asset( x*izzy_prec, izzycoin_id );   };
      auto _jill = [&]( int64_t x ) -> asset
      {   return asset( x*jill_prec, jillcoin_id );   };

      const asset izzy_satoshi = asset(1, izzycoin_id);
      const asset jill_satoshi = asset(1, jillcoin_id);

      // Izzycoin is worth 100 BTS
      price_feed feed;
      publish_feed( izzycoin_id(db), izzyt, feed );

      // Jillcoin is worth 30 BTS
      publish_feed( jillcoin_id(db), jilly, feed );

      enable_fees();

      // Alice and Bob issue some coins
      issue_uia( alice_id, asset( 200*izzy_prec, izzycoin_id ) );
      issue_uia(bobian_id, asset( 2000*jill_prec, jillcoin_id ) );


#if 0
      // Alice and Bob place orders which match
      create_sell_order( alice_id, _izzy(100), _jill(300) );   // Alice is willing to sell her Izzy's for 3 Jill
      create_sell_order(bobian_id, _jill(700), _izzy(200) );   // Bob is buying up to 200 Izzy's for up to 3.5 Jill

      // 100 Izzys and 300 Jills are matched, so the fees should be
      //   1 Izzy (1%) and 6 Jill (2%).

      {
         const asset_object& izzycoin = izzycoin_id(db);
         const asset_object& jillcoin = jillcoin_id(db);

         //wdump( (izzycoin)(izzycoin.dynamic_asset_data_id(db))((*izzycoin.bitasset_data_id)(db)) );
         //wdump( (jillcoin)(jillcoin.dynamic_asset_data_id(db))((*jillcoin.bitasset_data_id)(db)) );

         // check the correct amount of fees has been awarded
//         BOOST_CHECK( izzycoin.dynamic_asset_data_id(db).accumulated_fees == _izzy(1).amount );
//         BOOST_CHECK( jillcoin.dynamic_asset_data_id(db).accumulated_fees == _jill(6).amount );

      }
#endif


   }
   FC_LOG_AND_RETHROW()
}

#if 0
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
#endif

#if 0
BOOST_AUTO_TEST_CASE( fee_refund_test )
{
   try
   {
      ACTORS((alice)(bob)(izzy));

      int64_t alice_b0 = 1000000, bob_b0 = 1000000;

      transfer( account_id_type(), alice_id, asset(alice_b0) );
      transfer( account_id_type(), bob_id, asset(bob_b0) );

      asset_id_type core_id = asset_id_type();
      asset_id_type usd_id = create_user_issued_asset( "IZZYUSD", izzy_id(db) ).id;
      issue_uia( alice_id, asset( alice_b0, usd_id ) );
      issue_uia( bob_id, asset( bob_b0, usd_id ) );

      int64_t order_create_fee = 537;
      int64_t order_cancel_fee = 129;

      uint32_t skip = database::skip_miner_signature
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
#endif

BOOST_AUTO_TEST_SUITE_END()
