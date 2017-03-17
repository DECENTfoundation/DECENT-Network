/*
 * Copyright (c) 2017 DECENT Services and contributors.
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


#include <graphene/chain/database.hpp>

#include <graphene/chain/account_object.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/content_object.hpp>
#include <graphene/chain/buying_object.hpp>


namespace graphene { namespace chain {

void database::buying_expire(const buying_object& buying){
   adjust_balance( buying.consumer, buying.price );
   modify<buying_object>(buying, [&](buying_object& bo){
        bo.price.amount = 0;
        bo.expired = true;
        bo.expiration_or_delivery_time = head_block_time();
   });
}

void database::content_expire(const content_object& content){
   adjust_balance( content.author, content.publishing_fee_escrow );
   modify<content_object>(content, [&](content_object& co){
        co.publishing_fee_escrow.amount = 0;
   });
}

void database::decent_housekeeping()
{
   const auto& cidx = get_index_type<content_index>().indices().get<by_expiration>();
   auto citr = cidx.begin();
   while( citr != cidx.end() && citr->expiration <= head_block_time() )
   {
      return_escrow_submission_operation resop;
      resop.escrow = citr->publishing_fee_escrow;

      content_expire(*citr);

      resop.author = citr->author;
      resop.content = citr->id;
      push_applied_operation( resop );

      ++citr;
   }
   const auto& bidx = get_index_type<buying_index>().indices().get<by_expiration_time>();
   auto bitr = bidx.begin();
   while( bitr != bidx.end() && bitr->expiration_time <= head_block_time() )
   {
      if(!bitr->delivered) {
         buying_expire(*bitr);

         return_escrow_buying_operation rebop;
         rebop.escrow = bitr->price;
         rebop.consumer = bitr->consumer;
         rebop.buying = bitr->id;
         push_applied_operation(rebop);

      }
      ++bitr;
   }
}

share_type database::get_witness_budget()
{
   //get age in years
   auto now = head_block_time();
   auto start_time = fetch_block_by_number(1)->timestamp;
   uint32_t age = fc::microseconds(now - start_time).to_seconds() / 3600 / 24 / 365;
   if ( age <=5 )
      return DECENT_REWARDS_YEAR_1 / 365;
   if ( age <=10 )
      return DECENT_REWARDS_YEAR_6 / 365;
   if ( age <=15 )
      return DECENT_REWARDS_YEAR_11 / 365;
   if ( age <=20 )
      return DECENT_REWARDS_YEAR_16 / 365;
   return 0;
}

real_supply database::get_real_supply()const
{
   //walk through account_balances, vesting_balances and escrows in content and buying objects
   real_supply total;
   const auto& abidx = get_index_type<account_balance_index>().indices().get<by_id>();
   auto abitr = abidx.begin();
   while( abitr != abidx.end() ){
      total.account_balances += abitr->balance;
      ++abitr;
   }

   const auto& vbidx = get_index_type<vesting_balance_index>().indices().get<by_id>();
   auto vbitr = vbidx.begin();
   while( vbitr != vbidx.end() ){
      total.vesting_balances += vbitr->balance.amount;
      ++vbitr;
   }

   const auto& cidx = get_index_type<content_index>().indices().get<by_id>();
   auto citr = cidx.begin();
   while( citr != cidx.end() ){
      total.escrows += citr->publishing_fee_escrow.amount;
      ++citr;
   }

   const auto& bidx = get_index_type<buying_index>().indices().get<by_id>();
   auto bitr = bidx.begin();
   while( bitr != bidx.end() ){
      total.escrows += bitr->price.amount;
      ++bitr;
   }
   return total;
}

}
}