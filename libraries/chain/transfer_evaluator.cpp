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
#include <graphene/chain/transfer_evaluator.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/hardfork.hpp>
#include <graphene/chain/transaction_detail_object.hpp>
#include <graphene/chain/content_object.hpp>

#include <boost/multiprecision/cpp_int.hpp>

namespace graphene { namespace chain {
void_result transfer_evaluator::do_evaluate( const transfer_operation& op )
{ try {
   
   const database& d = db();

   const account_object& from_account    = op.from(d);
   const account_object& to_account      = op.to(d);
   const asset_object&   asset_type      = op.amount.asset_id(d);

   try {

      bool insufficient_balance = d.get_balance( from_account, asset_type ).amount >= op.amount.amount;
      FC_ASSERT( insufficient_balance,
                 "Insufficient Balance: ${balance}, unable to transfer '${total_transfer}' from account '${a}' to '${t}'", 
                 ("a",from_account.name)("t",to_account.name)("total_transfer",d.to_pretty_string(op.amount))("balance",d.to_pretty_string(d.get_balance(from_account, asset_type))) );

      return void_result();
   } FC_RETHROW_EXCEPTIONS( error, "Unable to transfer ${a} from ${f} to ${t}", ("a",d.to_pretty_string(op.amount))("f",op.from(d).name)("t",op.to(d).name) );

}  FC_CAPTURE_AND_RETHROW( (op) ) }

void_result transfer_evaluator::do_apply( const transfer_operation& o )
{ try {
   db().adjust_balance( o.from, -o.amount );
   db().adjust_balance( o.to, o.amount );

   auto & d = db();

   db().create<transaction_detail_object>([&o, &d](transaction_detail_object& obj)
                                          {
                                             obj.m_operation_type = (uint8_t)transaction_detail_object::transfer;

                                             obj.m_from_account = o.from;
                                             obj.m_to_account = o.to;
                                             obj.m_transaction_amount = o.amount;
                                             obj.m_transaction_fee = o.fee;
                                             obj.m_transaction_encrypted_memo = o.memo;
                                             obj.m_str_description = "transfer";
                                             obj.m_timestamp = d.head_block_time();
                                          });
   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result transfer2_evaluator::do_evaluate( const transfer2_operation& op )
{ try {
      const database& d = db();

      FC_ASSERT(db().head_block_time() > HARDFORK_2_TIME );
      const account_object& from_account        = op.from(d);
      const asset_object&   asset_type          = op.amount.asset_id(d);

      try {
         bool insufficient_balance = d.get_balance( from_account, asset_type ).amount >= op.amount.amount;
         FC_ASSERT( insufficient_balance,
                    "Insufficient Balance: ${balance}, unable to transfer '${total_transfer}' from account '${a}' to '${t}'",
                    ("a",from_account.name)("t",op.to)("total_transfer",d.to_pretty_string(op.amount))("balance",d.to_pretty_string(d.get_balance(from_account, asset_type))) );

         return void_result();
      } FC_RETHROW_EXCEPTIONS( error, "Unable to transfer ${a} from ${f} to ${t}", ("a",d.to_pretty_string(op.amount))("f",op.from(d).name)("t",op.to) );

   }  FC_CAPTURE_AND_RETHROW( (op) ) }

void_result transfer2_evaluator::do_apply( const transfer2_operation& o )
{ try {
      auto & d = db();
      account_id_type to_acc;

      d.adjust_balance( o.from, -o.amount );
      string suffix;

      if( o.to.is<account_id_type>() )
      {
         to_acc = o.to.as<account_id_type>();
         d.adjust_balance( to_acc, o.amount );
      }
      else
      {
         auto& content_idx = d.get_index_type<content_index>().indices().get<by_id>();
         const auto& content_itr = content_idx.find( o.to );
         asset amount = o.amount;

         if( !content_itr->co_authors.empty() )
         {
            boost::multiprecision::int128_t amount_for_co_author;
            for( auto const &element : content_itr->co_authors )
            {
               amount_for_co_author = ( o.amount.amount.value * element.second ) / 10000ll ;
               d.adjust_balance( element.first, asset( static_cast<share_type>(amount_for_co_author), o.amount.asset_id) );
               amount.amount -= amount_for_co_author;
            }
         }

         to_acc = content_itr->author;
         suffix = " to content " + std::string(content_itr->id);

         if( amount.amount != 0 ) {
            FC_ASSERT( amount.amount > 0 );
            d.adjust_balance(to_acc, amount);
         }
      }

      d.create<transaction_detail_object>([&o, &d, &to_acc, &suffix](transaction_detail_object& obj)
                                             {
                                                obj.m_operation_type = (uint8_t)transaction_detail_object::transfer;

                                                obj.m_from_account = o.from;
                                                obj.m_to_account = to_acc;
                                                obj.m_transaction_amount = o.amount;
                                                obj.m_transaction_fee = o.fee;
                                                obj.m_transaction_encrypted_memo = o.memo;
                                                obj.m_str_description = "transfer" + suffix;
                                                obj.m_timestamp = d.head_block_time();
                                             });
      return void_result();
   } FC_CAPTURE_AND_RETHROW( (o) ) }

} } // graphene::chain
