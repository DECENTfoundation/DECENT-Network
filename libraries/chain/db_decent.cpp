/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
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
#include <graphene/chain/seeder_object.hpp>

#include <algorithm>

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
   if( content.publishing_fee_escrow.amount >= 0 )
      adjust_balance( content.author, content.publishing_fee_escrow );
   else //workaround due to block halt at #404726- this should never happen but if it does again, the remaining amount shall be paid by someone else, in this case by decent6
   {
      elog("applying workaround in content_expire to content ${s}",("s",content.URI));
      adjust_balance(account_id_type(20),content.publishing_fee_escrow );

   }
   modify<content_object>(content, [&](content_object& co){
        co.publishing_fee_escrow.amount = 0;
   });

   const auto& idx = get_index_type<seeding_statistics_index>().indices().get<by_seeder>();
   for( const auto& element : content.key_parts )
   {
      const auto& stats = idx.find( element.first );
      if( content.last_proof.count( element.first ) )
      {
         modify<seeding_statistics_object>( *stats, [&element,&content]( seeding_statistics_object& so){
            so.total_content_seeded -= ( content.size * 1000 * 1000 ) / content.key_parts.size();
            so.num_of_content_seeded--;
         });
      }
      else if(stats->total_content_requested_to_seed > 0)
      {
         modify<seeding_statistics_object>( *stats, []( seeding_statistics_object& so){
            so.total_content_requested_to_seed--;
         });
      }
   }
}

void database::renew_subscription(const subscription_object& subscription, const uint32_t subscription_period, const asset price){
   modify<subscription_object>(subscription, [&](subscription_object& so){
      so.expiration += subscription_period * 24 * 3600 ;
   });

   time_point_sec time = head_block_time();
   create<transaction_detail_object>([&](transaction_detail_object& obj)
      {
         obj.m_operation_type = (uint8_t)transaction_detail_object::subscription;
         obj.m_from_account = subscription.from;
         obj.m_to_account = subscription.to;
         obj.m_transaction_amount = price;
         obj.m_transaction_fee = asset();
         string str_subscription_period = std::to_string(subscription_period) + " day";
         if( subscription_period > 1 )
            str_subscription_period += "s";
         obj.m_str_description = str_subscription_period;
         obj.m_timestamp = time;
      });

   adjust_balance( subscription.from, -price );
   adjust_balance( subscription.to, price );

   renewal_of_subscription_operation ros_vop;
   ros_vop.consumer = subscription.from;
   ros_vop.subscription = subscription.id;
   push_applied_operation(ros_vop);
}

void database::disallow_automatic_renewal_of_subscription(const subscription_object& subscription){
   modify<subscription_object>(subscription, [&](subscription_object& so){
      so.automatic_renewal = false;
   });

   disallow_automatic_renewal_of_subscription_operation daros_vop;
   daros_vop.consumer = subscription.from;
   daros_vop.subscription = subscription.id;
   push_applied_operation(daros_vop);
}

void database::set_and_reset_seeding_stats()
{
   const auto& idx = get_index_type<seeder_index>().indices().get<by_id>();

   // total_delivered_keys / (total_delivered_keys + missed_delivered_keys)
   uint32_t key_delivery_ratio;
   // (uploaded - uploaded_till_last_maint) / total_content_seeded
   uint64_t upload_to_data_recent;
   uint32_t num_of_requests_to_buy;
   uint32_t avg_buying_ratio;
   uint32_t seeding_rel_ratio;
   uint32_t seeding_abs_ratio;
   uint32_t num_of_content;
   uint32_t missed_ratio;

   auto itr = idx.begin();
   while( itr != idx.end() )
   {
      const seeding_statistics_object& stats = get<seeding_statistics_object>( itr->stats );

      num_of_requests_to_buy = stats.total_delivered_keys + stats.missed_delivered_keys;
      // interval [0,1]->[0,10000]
      if( num_of_requests_to_buy > 0 )
         key_delivery_ratio = ( stats.total_delivered_keys * 10000 ) / num_of_requests_to_buy;
      else
         key_delivery_ratio = 0;

      // interval [0,~), multiplied by 10.000
      if( stats.total_content_seeded > 0 )
         upload_to_data_recent = ( ( stats.total_upload - stats.uploaded_till_last_maint ) * 10000 ) / stats.total_content_seeded;
      else
         upload_to_data_recent = 0;

      // interval [0,~), multiplied by 10.000
      if( stats.num_of_content_seeded > 0 )
         avg_buying_ratio = ( ( num_of_requests_to_buy * 10000 * 3 ) / stats.num_of_content_seeded ) / 10 + ( stats.avg_buying_ratio * 7 ) / 10;
      else
         avg_buying_ratio = ( stats.avg_buying_ratio * 7 ) / 10;

      // multiplied by 10000
      if( avg_buying_ratio > 0 && upload_to_data_recent > 0)
         seeding_rel_ratio = ( 3 * 10000 * upload_to_data_recent / avg_buying_ratio ) / 10 + ( stats.seeding_rel_ratio * 7 ) / 10;
      else
         seeding_rel_ratio = stats.seeding_rel_ratio;

      if( upload_to_data_recent > 0 )
         seeding_abs_ratio = ( 3 * upload_to_data_recent ) / 10 + ( stats.seeding_abs_ratio * 7 ) / 10;
      else
         seeding_abs_ratio = stats.seeding_abs_ratio;

      num_of_content = stats.total_content_requested_to_seed + stats.num_of_content_seeded;
      // [0,1]->10000
      if( num_of_content > 0 )
         missed_ratio = ( ( 3 * stats.num_of_content_seeded * 10000 ) / num_of_content ) / 10 + ( stats.missed_ratio * 7 ) / 10;
      else
         missed_ratio = ( stats.missed_ratio * 7 ) / 10;

      seeding_rel_ratio = std::min( 20000u, seeding_rel_ratio );
      seeding_abs_ratio = std::min( 40000u, seeding_abs_ratio );

      modify<seeder_object>( *itr, [&](seeder_object& so){
         so.rating = std::min( ( seeding_rel_ratio + seeding_abs_ratio ) / 6, ( missed_ratio + key_delivery_ratio ) / 2);
      });

      modify<seeding_statistics_object>( stats, [&]( seeding_statistics_object& so ){
         so.uploaded_till_last_maint = so.total_upload;
         so.missed_delivered_keys = 0;
         so.total_delivered_keys = 0;
         so.total_content_requested_to_seed = 0;
         so.num_of_pors = 0;
         so.avg_buying_ratio = avg_buying_ratio;
         so.seeding_rel_ratio = seeding_rel_ratio;
         so.seeding_abs_ratio = seeding_abs_ratio;
         so.missed_ratio = missed_ratio;
      });

      itr++;
   }
}

void database::decent_housekeeping()
{
   set_and_reset_seeding_stats();

   const auto& cidx = get_index_type<content_index>().indices().get<by_expiration>();
   auto citr = cidx.lower_bound( get_dynamic_global_properties().last_budget_time + get_global_properties().parameters.block_interval );
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
   auto bitr = bidx.lower_bound( get_dynamic_global_properties().last_budget_time + get_global_properties().parameters.block_interval );
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


   // we need to know the next_maintenance_time
   const auto& dpo = get_dynamic_global_properties();
   time_point_sec next_maintenance_time = dpo.next_maintenance_time;

   const auto& gpo = get_global_properties();
   uint32_t maintenance_interval;
   if( gpo.pending_parameters )
      maintenance_interval = gpo.pending_parameters->maintenance_interval;
   else
      maintenance_interval = gpo.parameters.maintenance_interval;

   if( dpo.head_block_number == 1 )
      next_maintenance_time = time_point_sec() +
         (((head_block_time().sec_since_epoch() / maintenance_interval) + 1) * maintenance_interval);
   else
   {
      auto y = (head_block_time() - next_maintenance_time).to_seconds() / maintenance_interval;
      next_maintenance_time += (y+1) * maintenance_interval;
   }

   const auto& sidx = get_index_type<subscription_index>().indices().get<by_renewal>();
   const auto& aidx = get_index_type<account_index>().indices().get<by_id>();

   auto sitr = sidx.begin();
   while( sitr != sidx.end() && sitr->automatic_renewal )
   {
      if(sitr->expiration <= next_maintenance_time) {
         const auto &author = aidx.find(sitr->to);

         try {
            asset price = author->options.price_per_subscribe;
            auto ao = get( price.asset_id );
            asset dct_price;
            //if the price is in fiat, calculate price in DCT with current exchange rate...
            if( ao.is_monitored_asset() ){
               auto rate = ao.monitored_asset_opts->current_feed.core_exchange_rate;
               FC_ASSERT(!rate.is_null(), "No price feed for asset");
               dct_price = price * rate;
            }else{
               dct_price = price;
            }

            if( dct_price <= get_balance( sitr->from, dct_price.asset_id ))
               renew_subscription(*sitr, author->options.subscription_period, dct_price);
            else
               disallow_automatic_renewal_of_subscription(*sitr);
         }
         catch( fc::assert_exception& e ){
            elog("Failed to automatically renew expired subscription : ${id} . ${error}",
                 ("id", sitr->id)("error", e.to_detail_string()));
            disallow_automatic_renewal_of_subscription(*sitr);
         }
      }
      ++sitr;
   }
}

bool database::is_reward_switch_time() const
{
   auto now = head_block_num();
   return ( now == DECENT_SPLIT_0 || now == DECENT_SPLIT_1 || now == DECENT_SPLIT_2 || now == DECENT_SPLIT_3 || now == DECENT_SPLIT_4 );
}

bool database::is_reward_switch_in_interval(uint64_t a, uint64_t b)const
{
   if(a>=b)
      return false;
   if (a <= DECENT_SPLIT_0 && b >= DECENT_SPLIT_0)
      return true;
   if (a <= DECENT_SPLIT_1 && b >= DECENT_SPLIT_1)
      return true;
   if (a <= DECENT_SPLIT_2 && b >= DECENT_SPLIT_2)
      return true;
   if (a <= DECENT_SPLIT_3 && b >= DECENT_SPLIT_3)
      return true;
   if (a <= DECENT_SPLIT_4 && b >= DECENT_SPLIT_4)
      return true;
   return false;
}

uint64_t database::get_next_reward_switch_block(uint64_t start)const
{
   if(start <= DECENT_SPLIT_0 )
      return DECENT_SPLIT_0;
   if(start <= DECENT_SPLIT_1 )
      return DECENT_SPLIT_1;
   if(start <= DECENT_SPLIT_2 )
      return DECENT_SPLIT_2;
   if(start <= DECENT_SPLIT_3 )
      return DECENT_SPLIT_3;
   if(start <= DECENT_SPLIT_4 )
      return DECENT_SPLIT_4;
   return 0;
}

share_type database::get_asset_per_block_by_block_num(uint32_t block_num)
{
   //this method is called AFTER the update of head_block_num in gpo or when user calls get_block.
   //If user calls get_block calculation of miner_reward needs to search backward for block_reward. 
   uint64_t block_reward;
   if (block_num < DECENT_SPLIT_0)
      block_reward = DECENT_BLOCK_REWARD_0;
   else if (block_num < DECENT_SPLIT_1)
      block_reward = DECENT_BLOCK_REWARD_1;
   else if (block_num < DECENT_SPLIT_2)
      block_reward = DECENT_BLOCK_REWARD_2;
   else if (block_num < DECENT_SPLIT_3)
      block_reward = DECENT_BLOCK_REWARD_3;
   else if (block_num < DECENT_SPLIT_4)
      block_reward = DECENT_BLOCK_REWARD_4;
   else
      block_reward = DECENT_BLOCK_REWARD_5;

   return block_reward;
}

share_type database::get_new_asset_per_block()
{
   //get age in blocks
   auto now = head_block_num();

   return get_asset_per_block_by_block_num(now);
}

share_type database::get_miner_budget(uint32_t blocks_to_maint)
{

   const global_property_object& gpo = get_global_properties();

   uint64_t next_switch = get_next_reward_switch_block( head_block_num() );
   if( head_block_num()+1 + blocks_to_maint >= next_switch )
   {
      uint64_t to_switch = next_switch - head_block_num() - 1;
      if( next_switch == DECENT_SPLIT_0 ){
         return get_new_asset_per_block() * to_switch + DECENT_BLOCK_REWARD_1 * ( blocks_to_maint - to_switch );
      }
      if( next_switch == DECENT_SPLIT_1 ) {
         return get_new_asset_per_block() * to_switch + DECENT_BLOCK_REWARD_2 * ( blocks_to_maint - to_switch );
      }
      if( next_switch == DECENT_SPLIT_2 ) {
         return get_new_asset_per_block() * to_switch + DECENT_BLOCK_REWARD_3 * ( blocks_to_maint - to_switch );
      }
      if( next_switch == DECENT_SPLIT_3 ) {
         return get_new_asset_per_block() * to_switch + DECENT_BLOCK_REWARD_4 * ( blocks_to_maint - to_switch );
      }
      if( next_switch == DECENT_SPLIT_4 ) {
         return get_new_asset_per_block() * to_switch + DECENT_BLOCK_REWARD_5 * ( blocks_to_maint - to_switch );
      }
      return get_new_asset_per_block() * to_switch + get_new_asset_per_block() / 2 * ( blocks_to_maint - to_switch );
   }

   return blocks_to_maint * get_new_asset_per_block();
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
