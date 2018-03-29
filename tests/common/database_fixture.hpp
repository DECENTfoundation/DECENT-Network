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
#pragma once

#include <graphene/app/application.hpp>
#include <graphene/chain/database.hpp>
#include <fc/io/json.hpp>
#include <fc/smart_ref_impl.hpp>

#include <graphene/chain/operation_history_object.hpp>

#include <iostream>

using namespace graphene::db;

extern uint32_t GRAPHENE_TESTING_GENESIS_TIMESTAMP;

#define PUSH_TX \
   graphene::chain::test::_push_transaction

#define PUSH_BLOCK \
   graphene::chain::test::_push_block

// See below
#define REQUIRE_OP_VALIDATION_SUCCESS( op, field, value ) \
{ \
   const auto temp = op.field; \
   op.field = value; \
   op.validate(); \
   op.field = temp; \
}
#define REQUIRE_OP_EVALUATION_SUCCESS( op, field, value ) \
{ \
   const auto temp = op.field; \
   op.field = value; \
   trx.operations.back() = op; \
   op.field = temp; \
   db.push_transaction( trx, ~0 ); \
}

#define GRAPHENE_REQUIRE_THROW( expr, exc_type )          \
{                                                         \
   std::string req_throw_info = fc::json::to_string(      \
      fc::mutable_variant_object()                        \
      ("source_file", __FILE__)                           \
      ("source_lineno", __LINE__)                         \
      ("expr", #expr)                                     \
      ("exc_type", #exc_type)                             \
      );                                                  \
   if( fc::enable_record_assert_trip )                    \
      std::cout << "GRAPHENE_REQUIRE_THROW begin "        \
         << req_throw_info << std::endl;                  \
   BOOST_REQUIRE_THROW( expr, exc_type );                 \
   if( fc::enable_record_assert_trip )                    \
      std::cout << "GRAPHENE_REQUIRE_THROW end "          \
         << req_throw_info << std::endl;                  \
}

#define GRAPHENE_CHECK_THROW( expr, exc_type )            \
{                                                         \
   std::string req_throw_info = fc::json::to_string(      \
      fc::mutable_variant_object()                        \
      ("source_file", __FILE__)                           \
      ("source_lineno", __LINE__)                         \
      ("expr", #expr)                                     \
      ("exc_type", #exc_type)                             \
      );                                                  \
   if( fc::enable_record_assert_trip )                    \
      std::cout << "GRAPHENE_CHECK_THROW begin "          \
         << req_throw_info << std::endl;                  \
   BOOST_CHECK_THROW( expr, exc_type );                   \
   if( fc::enable_record_assert_trip )                    \
      std::cout << "GRAPHENE_CHECK_THROW end "            \
         << req_throw_info << std::endl;                  \
}

#define REQUIRE_OP_VALIDATION_FAILURE_2( op, field, value, exc_type ) \
{ \
   const auto temp = op.field; \
   op.field = value; \
   GRAPHENE_REQUIRE_THROW( op.validate(), exc_type ); \
   op.field = temp; \
}
#define REQUIRE_OP_VALIDATION_FAILURE( op, field, value ) \
   REQUIRE_OP_VALIDATION_FAILURE_2( op, field, value, fc::exception )

#define REQUIRE_THROW_WITH_VALUE_2(op, field, value, exc_type) \
{ \
   auto bak = op.field; \
   op.field = value; \
   trx.operations.back() = op; \
   op.field = bak; \
   GRAPHENE_REQUIRE_THROW(db.push_transaction(trx, ~0), exc_type); \
}

#define REQUIRE_THROW_WITH_VALUE( op, field, value ) \
   REQUIRE_THROW_WITH_VALUE_2( op, field, value, fc::exception )

///This simply resets v back to its default-constructed value. Requires v to have a working assingment operator and
/// default constructor.
#define RESET(v) v = decltype(v)()
///This allows me to build consecutive test cases. It's pretty ugly, but it works well enough for unit tests.
/// i.e. This allows a test on update_account to begin with the database at the end state of create_account.
#define INVOKE(test) ((struct test*)this)->test_method(); trx.clear()

#define PREP_ACTOR(name) \
   fc::ecc::private_key name ## _private_key = generate_private_key(BOOST_PP_STRINGIZE(name));   \
   public_key_type name ## _public_key = name ## _private_key.get_public_key();

#define ACTOR(name) \
   PREP_ACTOR(name) \
   const auto& name = create_account(BOOST_PP_STRINGIZE(name), name ## _public_key); \
   account_id_type name ## _id = name.id; (void)name ## _id;

#define GET_ACTOR(name) \
   fc::ecc::private_key name ## _private_key = generate_private_key(BOOST_PP_STRINGIZE(name)); \
   const account_object& name = get_account(BOOST_PP_STRINGIZE(name)); \
   account_id_type name ## _id = name.id; \
   (void)name ##_id

#define ACTORS_IMPL(r, data, elem) ACTOR(elem)
#define ACTORS(names) BOOST_PP_SEQ_FOR_EACH(ACTORS_IMPL, ~, names)

namespace graphene { namespace chain {

struct database_fixture {

   database_fixture();
   ~database_fixture();

   static fc::ecc::private_key generate_private_key(string seed);
   static void verify_asset_supplies( const database& db );

   digest_type digest( const transaction& tx );

   const account_object& create_account(
         const string& name,
         const public_key_type& key = public_key_type()
   );

   const account_object& create_account(
         const string& name,
         const account_object& registrar,
         const account_object& referrer,
         uint8_t referrer_percent = 100,
         const public_key_type& key = public_key_type()
   );

   const account_object& create_account(
         const string& name,
         const private_key_type& key,
         const account_id_type& registrar_id = account_id_type(),
         const account_id_type& referrer_id = account_id_type(),
         uint8_t referrer_percent = 100
   );

   account_create_operation make_account(
         const std::string& name = "nathan",
         public_key_type = public_key_type()
   );

   account_create_operation make_account(
         const std::string& name,
         const account_object& registrar,
         const account_object& referrer,
         uint8_t referrer_percent = 100,
         public_key_type key = public_key_type()
   );

   void open_database();

   signed_block generate_block(uint32_t skip = ~0,
                               const fc::ecc::private_key& key = generate_private_key("null_key"),
                               int miss_blocks = 0);

   /**
    * @brief Generates block_count blocks
    * @param block_count number of blocks to generate
    */
   void generate_blocks(uint32_t block_count);

   /**
    * @brief Generates blocks until the head block time matches or exceeds timestamp
    * @param timestamp target time to generate blocks until
    */
   void generate_blocks(fc::time_point_sec timestamp, bool miss_intermediate_blocks = true, uint32_t skip = ~0);


   const asset_object& create_user_issued_asset( const string& name );
   const asset_object& create_user_issued_asset( const string& name,
                                                 const account_object& issuer );
   const asset_object& create_monitored_asset(const string& name,
                                              account_id_type issuer = GRAPHENE_MINER_ACCOUNT );

   void transfer( account_id_type from, account_id_type to, const asset& amount, const asset& fee = asset() );
   void transfer( const account_object& from, const account_object& to, const asset& amount, const asset& fee = asset() );
   void sign( signed_transaction& trx, const fc::ecc::private_key& key );

   const miner_object& create_miner(account_id_type owner,
                                    const fc::ecc::private_key& signing_private_key = generate_private_key("null_key"));
   const miner_object& create_miner(const account_object& owner,
                                    const fc::ecc::private_key& signing_private_key = generate_private_key("null_key"));

   int64_t get_balance( account_id_type account, asset_id_type a )const;
   int64_t get_balance( const account_object& account, const asset_object& a )const;

   const asset_object& get_asset( const string& symbol )const;
   const account_object& get_account( const string& name )const;
   const account_object& get_account_by_id(account_id_type id)const;
   const miner_object& get_miner(account_id_type id)const;

   void enable_fees();

   uint64_t fund( const account_object& account, const asset& amount = asset(500000) );
   string generate_anon_acct_name();

   void issue_uia( const account_object& recipient, asset amount );
   void issue_uia( account_id_type recipient_id, asset amount );

   void publish_feed(asset_id_type mia, account_id_type by, const price_feed& f)
   { publish_feed(mia(db), by(db), f); }
   void publish_feed(const asset_object& mia, const account_object& by, const price_feed& f);

   void fill_pools(asset_id_type uia, account_id_type by, asset to_core_pool, asset to_asset_pool);

   void create_content(account_id_type by, string url, asset price, map<account_id_type, uint32_t> co_authors={});
   void buy_content(account_id_type by, string url, asset price);


   // the reason we use an app is to exercise the indexes of built-in
   //   plugins
   graphene::app::application app;
   genesis_state_type genesis_state;
   chain::database &db;
   signed_transaction trx;
   signed_transaction trx2;
   fc::ecc::private_key private_key = fc::ecc::private_key::generate();
   fc::ecc::private_key init_account_priv_key = fc::ecc::private_key::regenerate(fc::sha256::hash(string("null_key")) );
   public_key_type init_account_pub_key;

   public_key_type miner_key;
   account_id_type miner_account;
   uint32_t anon_acct_count;

   optional<fc::temp_directory> data_dir;


#if 0  //OLD CODE..




   bool skip_key_index_test = false;


   database_fixture();
   ~database_fixture();

   static fc::ecc::private_key generate_private_key(string seed);

   static void verify_asset_supplies( const database& db );
   void verify_account_history_plugin_index( )const;

   const limit_order_object* create_sell_order( account_id_type user, const asset& amount, const asset& recv );
   const limit_order_object* create_sell_order( const account_object& user, const asset& amount, const asset& recv );





   //asset cancel_limit_order( const limit_order_object& order );
   void fund_fee_pool( const account_object& from, const asset_object& asset_to_fund, const share_type amount );
   //void change_fees( const flat_set< fee_parameters >& new_params, uint32_t new_scale = 0 );
   void print_market( const string& syma, const string& symb )const;
   string pretty( const asset& a )const;
   //void print_limit_order( const limit_order_object& cur )const;
   void print_joint_market( const string& syma, const string& symb )const;
   //vector< operation_history_object > get_operation_history( account_id_type account_id )const;
#endif

};



namespace test {
/// set a reasonable expiration time for the transaction
void set_expiration( const database& db, transaction& tx );

bool _push_block( database& db, const signed_block& b, uint32_t skip_flags = 0 );
processed_transaction _push_transaction( database& db, const signed_transaction& tx, uint32_t skip_flags = 0 );
}

} }
