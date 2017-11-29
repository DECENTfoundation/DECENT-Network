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

#include <graphene/chain/account_object.hpp>
#include <graphene/chain/asset_object.hpp>

#include <fc/crypto/digest.hpp>

#include "../common/database_fixture.hpp"

using namespace graphene::chain;
using namespace graphene::chain::test;

BOOST_FIXTURE_TEST_SUITE( uia_tests, database_fixture )

BOOST_AUTO_TEST_CASE( create_advanced_uia )
{
   try {
      ACTORS( (alice) );
      asset_id_type test_asset_id = db.get_index<asset_object>().get_next_id();
      asset_create_operation creator;
      creator.issuer = alice_id;
      creator.fee = asset();
      creator.symbol = "ADVANCED";
      creator.options.max_supply = 100000000;
      creator.precision = 2;
      creator.options.core_exchange_rate = price({asset(2),asset(1, test_asset_id)});

      trx.operations.push_back(std::move(creator));
      ilog("Creating asset ADVANCED");
      PUSH_TX( db, trx, ~0 );

      const asset_object& test_asset = test_asset_id(db);
      BOOST_CHECK(test_asset.symbol == "ADVANCED");
      BOOST_CHECK(asset(1, test_asset_id) * test_asset.options.core_exchange_rate == asset(2));
      BOOST_CHECK(test_asset.options.max_supply == 100000000);
      BOOST_CHECK(!test_asset.monitored_asset_opts.valid());

      const asset_dynamic_data_object& test_asset_dynamic_data = test_asset.dynamic_asset_data_id(db);
      BOOST_CHECK(test_asset_dynamic_data.current_supply == 0);

   } catch(fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE( asset_name_test )
{
   try
   {
      //names must be at least 5 characters long...
      ACTORS( (alice)(bobian) );

      auto has_asset = [&]( std::string symbol ) -> bool
      {
         const auto& assets_by_symbol = db.get_index_type<asset_index>().indices().get<by_symbol>();
         return assets_by_symbol.find( symbol ) != assets_by_symbol.end();
      };

      ilog("Creating asset ALPHA");
      // Alice creates asset "ALPHA"
      BOOST_CHECK( !has_asset("ALPHA") );    BOOST_CHECK( !has_asset("ALPHA.ONE") );
      create_user_issued_asset( "ALPHA", alice_id(db) );
      BOOST_CHECK(  has_asset("ALPHA") );    BOOST_CHECK( !has_asset("ALPHA.ONE") );

      // Nobody can create another asset named ALPHA
      GRAPHENE_REQUIRE_THROW( create_user_issued_asset( "ALPHA",   bobian_id(db) ), fc::exception );
      BOOST_CHECK(  has_asset("ALPHA") );    BOOST_CHECK( !has_asset("ALPHA.ONE") );
      GRAPHENE_REQUIRE_THROW( create_user_issued_asset( "ALPHA", alice_id(db) ), fc::exception );
      BOOST_CHECK(  has_asset("ALPHA") );    BOOST_CHECK( !has_asset("ALPHA.ONE") );

   }
   catch(fc::exception& e)
   {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE( asset_issue_transfer_test )
{
   try
   {
      //names must be at least 5 characters long...
      ACTORS( (alice)(bobian) );
      transfer( miner_account, alice_id, asset(300000000) );

      auto has_asset = [&]( std::string symbol ) -> bool
      {
           const auto& assets_by_symbol = db.get_index_type<asset_index>().indices().get<by_symbol>();
           return assets_by_symbol.find( symbol ) != assets_by_symbol.end();
      };

      ilog("Creating asset ALPHA");
      // Alice creates asset "ALPHA"
      BOOST_CHECK( !has_asset("ALPHA") );    BOOST_CHECK( !has_asset("ALPHA.ONE") );
      asset_id_type alpha_asset_id = db.get_index<asset_object>().get_next_id();
      create_user_issued_asset( "ALPHA", alice_id(db) );
      BOOST_CHECK(  has_asset("ALPHA") );    BOOST_CHECK( !has_asset("ALPHA.ONE") );

      ilog("Issuing asset ALPHA");
      //Alice issues asset "ALPHA"
      issue_uia( bobian_id(db), asset(100000000, alpha_asset_id));
      BOOST_CHECK(get_balance(bobian_id,alpha_asset_id) == 100000000 );
      issue_uia( alice_id(db), asset(250000000, alpha_asset_id));
      BOOST_CHECK(get_balance(bobian_id,alpha_asset_id) == 100000000 );
      BOOST_CHECK(get_balance(alice_id,alpha_asset_id) == 250000000 );

      enable_fees();
      transfer(alice_id,bobian_id,asset(50000000,alpha_asset_id),asset(500000));
      BOOST_CHECK(get_balance(bobian_id,alpha_asset_id) == 150000000 );
      BOOST_CHECK(get_balance(alice_id,alpha_asset_id) == 200000000 );
      GRAPHENE_REQUIRE_THROW(transfer(alice_id,bobian_id,asset(5,alpha_asset_id),asset(500000, alpha_asset_id)), fc::exception);
      trx.clear();
      idump((alpha_asset_id(db)));
      ilog("Alice has ${s} DCTs, ",("s",get_balance(alice_id, asset_id_type())));
      //Fill the pools
      asset_fund_pools_operation filler;
      filler.dct_asset = asset(50000000);
      filler.fee = asset(500000, asset_id_type());
      filler.from_account = alice_id;
      filler.uia_asset = asset(50000000, alpha_asset_id);

      trx.operations.clear();
      trx.operations.push_back(std::move(filler));
      sign( trx, alice_private_key );
      ilog("Filling asset pools");
      PUSH_TX( db, trx);
      trx.clear();
      idump((alpha_asset_id(db)));
      transfer(alice_id,bobian_id,asset(50000000,alpha_asset_id),asset(500000, alpha_asset_id));
      BOOST_CHECK(get_balance(bobian_id,alpha_asset_id) == 200000000 );
      BOOST_CHECK(get_balance(alice_id,alpha_asset_id) == 99500000 );

      for(int i=0; i<99; i++)
         transfer(alice_id,bobian_id,asset(10000,alpha_asset_id),asset(500000, alpha_asset_id));

      GRAPHENE_REQUIRE_THROW(transfer(alice_id,bobian_id,asset(10000,alpha_asset_id),asset(500000, alpha_asset_id)), fc::exception);
      trx.clear();
   }
   catch(fc::exception& e)
   {
      edump((e.to_detail_string()));
      throw;
   }
}


BOOST_AUTO_TEST_CASE( asset_content_in_uia_test )
{
   try
   {
      //names must be at least 5 characters long...
      ACTORS( (alice)(bobian)(cecil) );
      transfer( miner_account, alice_id, asset(400000000) );
      transfer( miner_account, cecil_id, asset(400000000) );


      auto has_asset = [&]( std::string symbol ) -> bool
      {
           const auto& assets_by_symbol = db.get_index_type<asset_index>().indices().get<by_symbol>();
           return assets_by_symbol.find( symbol ) != assets_by_symbol.end();
      };

      ilog("Creating asset ALPHA");
      // Alice creates asset "ALPHA"
      BOOST_CHECK( !has_asset("ALPHA") );    BOOST_CHECK( !has_asset("ALPHA.ONE") );
      asset_id_type alpha_asset_id = db.get_index<asset_object>().get_next_id();
      create_user_issued_asset( "ALPHA", alice_id(db) );
      BOOST_CHECK(  has_asset("ALPHA") );    BOOST_CHECK( !has_asset("ALPHA.ONE") );

      //create content
      create_content(alice_id, "http://abcd", asset(100, alpha_asset_id));
      GRAPHENE_REQUIRE_THROW(buy_content(cecil_id, "http://abcd", asset(100)), fc::exception);
      trx.clear();
      ilog("Issuing asset ALPHA");
      //Alice issues asset "ALPHA"
      issue_uia( bobian_id(db), asset(100000000, alpha_asset_id));
      issue_uia( alice_id(db), asset(250000000, alpha_asset_id));

      //Fill the pools
      fill_pools(alpha_asset_id, alice_id, asset(50000000), asset(50000000, alpha_asset_id));


      ilog("Alice has ${s} DCTs, ",("s",get_balance(alice_id, asset_id_type())));
      ilog("Alice has ${s} ALPHAs, ",("s",get_balance(alice_id, alpha_asset_id)));
      idump((alpha_asset_id(db).dynamic_asset_data_id(db)));

      buy_content(bobian_id, "http://abcd", asset(100, alpha_asset_id));
      buy_content(cecil_id, "http://abcd", asset(100));
      ilog("Alice has ${s} DCTs, ",("s",get_balance(alice_id, asset_id_type())));
      ilog("Alice has ${s} ALPHAs, ",("s",get_balance(alice_id, alpha_asset_id)));
      idump((alpha_asset_id(db).dynamic_asset_data_id(db)));
   }
   catch(fc::exception& e)
   {
      edump((e.to_detail_string()));
      throw;
   }
}


BOOST_AUTO_TEST_CASE( asset_content_in_dct_test )
{
   try
   {
      //names must be at least 5 characters long...
      ACTORS( (alice)(bobian)(cecil) );
      transfer( miner_account, alice_id, asset(400000000) );
      transfer( miner_account, cecil_id, asset(400000000) );


      auto has_asset = [&]( std::string symbol ) -> bool
      {
           const auto& assets_by_symbol = db.get_index_type<asset_index>().indices().get<by_symbol>();
           return assets_by_symbol.find( symbol ) != assets_by_symbol.end();
      };

      ilog("Creating asset ALPHA");
      // Alice creates asset "ALPHA"
      BOOST_CHECK( !has_asset("ALPHA") );    BOOST_CHECK( !has_asset("ALPHA.ONE") );
      asset_id_type alpha_asset_id = db.get_index<asset_object>().get_next_id();
      create_user_issued_asset( "ALPHA", alice_id(db) );
      BOOST_CHECK(  has_asset("ALPHA") );    BOOST_CHECK( !has_asset("ALPHA.ONE") );

      //create content
      create_content(alice_id, "http://abcd", asset(100));
      GRAPHENE_REQUIRE_THROW(buy_content(bobian_id, "http://abcd", asset(100, alpha_asset_id)), fc::exception);
      trx.clear();
      //try to buy
      ilog("Issuing asset ALPHA");
      //Alice issues asset "ALPHA"
      issue_uia( bobian_id(db), asset(100000000, alpha_asset_id));
      issue_uia( alice_id(db), asset(250000000, alpha_asset_id));

      //Fill the pools
      fill_pools(alpha_asset_id, alice_id, asset(50000000), asset(50000000, alpha_asset_id));


      ilog("Alice has ${s} DCTs, ",("s",get_balance(alice_id, asset_id_type())));
      ilog("Alice has ${s} ALPHAs, ",("s",get_balance(alice_id, alpha_asset_id)));
      idump((alpha_asset_id(db).dynamic_asset_data_id(db)));

      buy_content(bobian_id, "http://abcd", asset(100, alpha_asset_id));
      buy_content(cecil_id, "http://abcd", asset(100));
      ilog("Alice has ${s} DCTs, ",("s",get_balance(alice_id, asset_id_type())));
      ilog("Alice has ${s} ALPHAs, ",("s",get_balance(alice_id, alpha_asset_id)));
      idump((alpha_asset_id(db).dynamic_asset_data_id(db)));
   }
   catch(fc::exception& e)
   {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_SUITE_END()
