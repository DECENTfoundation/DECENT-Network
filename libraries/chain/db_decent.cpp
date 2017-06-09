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
#include <graphene/chain/subscription_object.hpp>


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

void database::renew_subscription(const subscription_object& subscription, const uint32_t subscription_period){
   // head_block_time rounded up to midnight
   uint32_t head_block_time_rounded_to_days = head_block_time().sec_since_epoch() / ( 24 * 3600 );
   head_block_time_rounded_to_days += 24 * 3600;

   modify<subscription_object>(subscription, [&](subscription_object& so){
      so.expiration = time_point_sec( head_block_time_rounded_to_days ) + subscription_period * 24 * 3600;
   });
}

void database::disallow_automatic_renewal_of_subscription(const subscription_object& subscription){
   modify<subscription_object>(subscription, [&](subscription_object& so){
      so.automatic_renewal= false;
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

   const auto& sidx = get_index_type<subscription_index>().indices().get<by_renewal>();
   const auto& aidx = get_index_type<account_index>().indices().get<by_id>();
   auto sitr = sidx.begin();
   while( sitr != sidx.end() && sitr->automatic_renewal )
   {
      if(sitr->expiration <= head_block_time()) {
         const auto &author = aidx.find(sitr->to);
         if( author->options.price_per_subscribe <= get_balance( sitr->from, author->options.price_per_subscribe.asset_id ))
         {
            renew_subscription(*sitr, author->options.subscription_period);

            adjust_balance( sitr->from, -author->options.price_per_subscribe );
            adjust_balance( sitr->to, author->options.price_per_subscribe );

            renewal_of_subscription_operation ros_vop;
            ros_vop.consumer = sitr->from;
            ros_vop.subscription = sitr->id;
            push_applied_operation(ros_vop);
         }
         else
         {
            disallow_automatic_renewal_of_subscription(*sitr);

            disallow_automatic_renewal_of_subscription_operation daros_vop;
            daros_vop.consumer = sitr->from;
            daros_vop.subscription = sitr->id;
            push_applied_operation(daros_vop);
         }
      }
      ++sitr;
   }
}

bool database::is_reward_switch_time() const
{
   auto now = head_block_num();
   return ( now == DECENT_SPLIT_1 || now == DECENT_SPLIT_2 || now == DECENT_SPLIT_3 || now == DECENT_SPLIT_4 );
}

share_type database::get_new_asset_per_block()
{
   //get age in blocks
   auto now = head_block_num();

   uint64_t block_reward;
   if( now < DECENT_SPLIT_1 )
      block_reward = DECENT_BLOCK_REWARD_1;
   else if( now < DECENT_SPLIT_2 )
      block_reward = DECENT_BLOCK_REWARD_2;
   else if( now < DECENT_SPLIT_3 )
      block_reward = DECENT_BLOCK_REWARD_3;
   else if( now < DECENT_SPLIT_4 )
      block_reward = DECENT_BLOCK_REWARD_4;
   else
      block_reward = DECENT_BLOCK_REWARD_5;

   return block_reward;
}

share_type database::get_witness_budget(uint32_t blocks_to_maint)
{

   const global_property_object& gpo = get_global_properties();

   share_type block_reward = get_new_asset_per_block();

   //uint64_t blocks_per_maintenance_interval = gpo.parameters.maintenance_interval / gpo.parameters.block_interval;

   return blocks_to_maint * block_reward;
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
