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

#include <graphene/chain/account_object.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/proposal_object.hpp>
#include <graphene/chain/global_property_object.hpp>
#include <graphene/chain/miner_object.hpp>
#include <graphene/chain/message_object.hpp>

#include <graphene/db/simple_index.hpp>

#include "../common/database_fixture.hpp"

using namespace graphene::chain;
using namespace graphene::chain::test;

BOOST_FIXTURE_TEST_SUITE( messaging_tests, database_fixture )


BOOST_AUTO_TEST_CASE( messaging )
{ try {

   ACTOR(nathan);
   ACTOR(bobian);
   ACTOR(alice);

   transfer(account_id_type()(db), nathan, asset(200000));
   enable_fees();

   std::vector<std::string> to = { "alice", "bobian" };
   std::string text_sent = "Hello from nathan to alice and bobian";

   std::vector<account_object> to_accounts;
   account_id_type from_id = nathan_id;

   message_payload pl;

   message_payload_receivers_data receivers_data_item;

   // message for bobian
   pl.receivers_data.emplace_back(text_sent, nathan_private_key, bobian.options.memo_key, bobian_id);

   // message for alice
   pl.receivers_data.emplace_back(text_sent, nathan_private_key, alice.options.memo_key, alice_id);

   custom_operation cust_op;
   cust_op.id = graphene::chain::custom_operation::custom_operation_subtype_messaging;
   cust_op.payer = from_id;

   pl.from = from_id;
   pl.pub_from = nathan.options.memo_key;
   cust_op.set_messaging_payload(pl);

   trx.operations.push_back(cust_op);

   fee_schedule s = db.get_global_properties().parameters.current_fees;
   s.set_fee(trx.operations.back(), db.head_block_time());

   const auto& global_params = db.get_global_properties().parameters;
   trx.expiration = db.head_block_time() + global_params.maximum_time_until_expiration;
   //custom_operation& result_op = (trx.operations.back()).get<custom_operation>();
   trx.validate();

   sign(trx, nathan_private_key);

   PUSH_TX(db, trx);

   auto msg_itr_found = db.get_index_type<message_index>().indices().get<by_id>().end();
   const auto& idx = db.get_index_type<message_index>();
   const auto& aidx = dynamic_cast<const primary_index<message_index>&>(idx);
   const auto& refs = aidx.get_secondary_index<graphene::chain::message_receiver_index>();
   auto itr = refs.message_to_receiver_memberships.find(bobian_id);

   if (itr != refs.message_to_receiver_memberships.end())
   {
      for (const object_id_type& item : itr->second) {

         auto msg_itr = db.get_index_type<message_index>().indices().get<by_id>().find(item);
         if (msg_itr != db.get_index_type<message_index>().indices().get<by_id>().end()) {
            message_object o = *msg_itr;
            if ((*msg_itr).sender == nathan_id) {
               msg_itr_found = msg_itr;
               break;
            }
         }
      }
   }
   BOOST_REQUIRE(msg_itr_found != db.get_index_type<message_index>().indices().get<by_id>().end());
   std::string received_text_bobian = (*msg_itr_found).receivers_data[0].get_message(bobian_private_key, nathan_public_key);
   std::string received_text_alice = (*msg_itr_found).receivers_data[1].get_message(alice_private_key, nathan_public_key);
   BOOST_REQUIRE(received_text_bobian == received_text_alice);
   BOOST_REQUIRE(received_text_bobian == received_text_alice);
   BOOST_REQUIRE(received_text_bobian == text_sent);

} FC_LOG_AND_RETHROW() }


BOOST_AUTO_TEST_SUITE_END()
