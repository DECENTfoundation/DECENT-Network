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

#include <graphene/chain/database.hpp>
#include <graphene/chain/db_with.hpp>

#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/global_property_object.hpp>
#include <graphene/chain/hardfork.hpp>
#include <graphene/chain/proposal_object.hpp>
#include <graphene/chain/transaction_object.hpp>
#include <graphene/chain/withdraw_permission_object.hpp>
#include <graphene/chain/miner_object.hpp>

#include <graphene/chain/protocol/fee_schedule.hpp>

#include <fc/uint128.hpp>

namespace graphene { namespace chain {

void database::update_global_dynamic_data( const signed_block& b )
{
   const dynamic_global_property_object& _dgp =
      dynamic_global_property_id_type(0)(*this);

   uint32_t missed_blocks = get_slot_at_time( b.timestamp );
   assert( missed_blocks != 0 );
   missed_blocks--;
   for( uint32_t i = 0; i < missed_blocks; ++i ) {
      const auto& miner_missed = get_scheduled_miner( i+1 )(*this);
      if(  miner_missed.id != b.miner ) {
         /*
         const auto& miner_account = miner_missed.miner_account(*this);
         if( (fc::time_point::now() - b.timestamp) < fc::seconds(30) )
            wlog( "Miner ${name} missed block ${n} around ${t}", ("name",miner_account.name)("n",b.block_num())("t",b.timestamp) );
            */

         modify( miner_missed, [&]( miner_object& w ) {
           w.total_missed++;
         });
      }
   }

   // dynamic global properties updating
   modify( _dgp, [&]( dynamic_global_property_object& dgp ){
      if( BOOST_UNLIKELY( b.block_num() == 1 ) )
         dgp.recently_missed_count = 0;
         else if( _checkpoints.size() && _checkpoints.rbegin()->first >= b.block_num() )
         dgp.recently_missed_count = 0;
      else if( missed_blocks )
         dgp.recently_missed_count += GRAPHENE_RECENTLY_MISSED_COUNT_INCREMENT*missed_blocks;
      else if( dgp.recently_missed_count > GRAPHENE_RECENTLY_MISSED_COUNT_INCREMENT )
         dgp.recently_missed_count -= GRAPHENE_RECENTLY_MISSED_COUNT_DECREMENT;
      else if( dgp.recently_missed_count > 0 )
         dgp.recently_missed_count--;

      dgp.head_block_number = b.block_num();
      dgp.head_block_id = b.id();
      dgp.time = b.timestamp;
      dgp.current_miner = b.miner;
      dgp.recent_slots_filled = (
           (dgp.recent_slots_filled << 1)
           + 1) << missed_blocks;
      dgp.current_aslot += missed_blocks+1;
   });

   if( !(get_node_properties().skip_flags & skip_undo_history_check) )
   {
      FC_VERIFY_AND_THROW( _dgp.head_block_number - _dgp.last_irreversible_block_num  < GRAPHENE_MAX_UNDO_HISTORY, undo_database_exception,
                 "The database does not have enough undo history to support a blockchain with so many missed blocks. "
                 "Please add a checkpoint if you would like to continue applying blocks beyond this point.",
                 ("last_irreversible_block_num",_dgp.last_irreversible_block_num)("head", _dgp.head_block_number)
                 ("recently_missed",_dgp.recently_missed_count)("max_undo",GRAPHENE_MAX_UNDO_HISTORY) );
   }

   _undo_db.set_max_size( _dgp.head_block_number - _dgp.last_irreversible_block_num + 1 );
   _fork_db.set_max_size( _dgp.head_block_number - _dgp.last_irreversible_block_num + 1 );
}

void database::update_signing_miner(const miner_object& signing_miner, const signed_block& new_block)
{
   const global_property_object& gpo = get_global_properties();
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();
   uint64_t new_block_aslot = dpo.current_aslot + get_slot_at_time( new_block.timestamp );

   int64_t time_to_maint = (dpo.next_maintenance_time - dpo.last_budget_time ).to_seconds();
   uint32_t blocks_in_interval = static_cast<uint32_t>(( uint64_t(time_to_maint) + gpo.parameters.block_interval - 1 ) / gpo.parameters.block_interval);
   //uint32_t blocks_in_interval = ( gpo.parameters.maintenance_interval ) / gpo.parameters.block_interval;

   share_type miner_pay_from_reward = 0;
   share_type miner_pay_from_fees = 0;

   if( blocks_in_interval > 0 ) {
      miner_pay_from_fees = dpo.miner_budget_from_fees / blocks_in_interval ;
   }
   miner_pay_from_reward = get_new_asset_per_block();

   //this should never happen, but better check.
   if(miner_pay_from_fees < share_type(0))
      miner_pay_from_fees = share_type(0);

   //ilog("calculating miner pay; miner budget = ${b}, from fees = ${f}, blocks: ${r}, miner pay = ${p}",("b", dpo.miner_budget_from_rewards)("f", dpo.unspent_fee_budget)("r", blocks_in_interval)("p", miner_pay_from_fees+miner_pay_from_reward));

   modify( dpo, [&]( dynamic_global_property_object& _dpo )
   {
      _dpo.mined_rewards += get_new_asset_per_block();
      _dpo.unspent_fee_budget -= miner_pay_from_fees;
   } );

   deposit_miner_pay( signing_miner, miner_pay_from_fees + miner_pay_from_reward );

   modify( signing_miner, [&]( miner_object& _wit )
   {
      _wit.last_aslot = new_block_aslot;
      _wit.last_confirmed_block_num = new_block.block_num();
   } );
}

void database::update_last_irreversible_block()
{
   const global_property_object& gpo = get_global_properties();
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();

   if(get_node_properties().skip_flags&skip_undo_history_check)
   {
      modify( dpo, [&]( dynamic_global_property_object& _dpo )
      {
           _dpo.last_irreversible_block_num = head_block_num();
      } );
      return;
   }

   std::vector<const miner_object*> wit_objs;
   wit_objs.reserve( gpo.active_miners.size() );
   for( const miner_id_type& wid : gpo.active_miners )
      wit_objs.push_back( &(wid(*this)) );

   static_assert( GRAPHENE_IRREVERSIBLE_THRESHOLD > 0, "irreversible threshold must be nonzero" );
   static_assert( GRAPHENE_IRREVERSIBLE_THRESHOLD_HF5 > 0, "irreversible threshold must be nonzero" );

   // 1 1 1 2 2 2 2 2 2 2 -> 2     .7*10 = 7
   // 1 1 1 1 1 1 1 2 2 2 -> 1
   // 3 3 3 3 3 3 3 3 3 3 -> 3

   size_t irreversible_threshold = head_block_time() > HARDFORK_5_TIME ? GRAPHENE_IRREVERSIBLE_THRESHOLD_HF5 : GRAPHENE_IRREVERSIBLE_THRESHOLD;
   size_t offset = ((GRAPHENE_100_PERCENT - irreversible_threshold) * wit_objs.size() / GRAPHENE_100_PERCENT);

   std::nth_element( wit_objs.begin(), wit_objs.begin() + offset, wit_objs.end(),
      []( const miner_object* a, const miner_object* b )
      {
         return a->last_confirmed_block_num < b->last_confirmed_block_num;
      } );

   uint32_t new_last_irreversible_block_num = wit_objs[offset]->last_confirmed_block_num;

   // add check that there is not only one miner for last DECENT_MAX_BLOCKS_SINGLE_MINER_GROUP
   // where DECENT_SINGLE_MINER_GROUP_SIZE
   size_t miner_group_size_offset = wit_objs.size() - DECENT_SINGLE_MINER_GROUP_SIZE;
   size_t max_blocks_group= DECENT_MAX_BLOCKS_SINGLE_MINER_GROUP;
   std::nth_element( wit_objs.begin(), wit_objs.begin() + miner_group_size_offset, wit_objs.end(),
       []( const miner_object* a, const miner_object* b )
       {
         return a->last_confirmed_block_num < b->last_confirmed_block_num;
       } );

   FC_VERIFY_AND_THROW( head_block_num() <= max_blocks_group + wit_objs[miner_group_size_offset]->last_confirmed_block_num,
         too_many_blocks_by_single_group_exception, "Too many unconfirmed blocks by single miner group", ("head_block_num", head_block_num())
	 ("nth miner last confirmed block", wit_objs[miner_group_size_offset]->last_confirmed_block_num));

   if( new_last_irreversible_block_num > dpo.last_irreversible_block_num )
   {
      modify( dpo, [&]( dynamic_global_property_object& _dpo )
      {
         _dpo.last_irreversible_block_num = new_last_irreversible_block_num;
      } );
   }
}

void database::clear_expired_transactions()
{ try {
   //Look for expired transactions in the deduplication list, and remove them.
   //Transactions must have expired by at least two forking windows in order to be removed.
   auto& transaction_idx = static_cast<transaction_index&>(get_mutable_index(implementation_ids, impl_transaction_object_type));
   const auto& dedupe_index = transaction_idx.indices().get<by_expiration>();
   while( (!dedupe_index.empty()) && (head_block_time() > dedupe_index.rbegin()->expiration) )
      transaction_idx.remove(*dedupe_index.rbegin());
} FC_RETHROW() }

void database::clear_expired_proposals()
{
   const auto& proposal_expiration_index = get_index_type<proposal_index>().indices().get<by_expiration>();
   while( !proposal_expiration_index.empty() && proposal_expiration_index.begin()->expiration_time <= head_block_time() )
   {
      const proposal_object& proposal = *proposal_expiration_index.begin();
      processed_transaction result;
      try {
         if( proposal.is_authorized_to_execute(*this) )
         {
            result = push_proposal(proposal);
            //TODO: Do something with result so plugins can process it.
            continue;
         }
      } catch( const fc::exception& e ) {
         elog("Failed to apply proposed transaction on its expiration. Deleting it.\n${proposal}\n${error}",
              ("proposal", proposal)("error", e.to_detail_string()));
      }
      remove(proposal);
   }
}

void database::update_expired_feeds()
{
   auto& asset_idx = get_index_type<asset_index>().indices().get<by_type>();
   auto itr = asset_idx.lower_bound( true /** market issued */ );
   while( itr != asset_idx.end() ) {
      const asset_object &a = *itr;
      ++itr;
      assert(a.is_monitored_asset());

      bool feed_is_expired;
      feed_is_expired = a.monitored_asset_opts->feed_is_expired(head_block_time());
      if( feed_is_expired ) {
         modify(a, [ this ](asset_object &ao) {
              ao.monitored_asset_opts->update_median_feeds(head_block_time());
         });
      }
   }
}

void database::update_maintenance_flag( bool new_maintenance_flag )
{
   modify( get_dynamic_global_properties(), [&]( dynamic_global_property_object& dpo )
   {
      auto maintenance_flag = dynamic_global_property_object::maintenance_flag;
      dpo.dynamic_flags =
           (dpo.dynamic_flags & ~maintenance_flag)
         | (new_maintenance_flag ? maintenance_flag : 0);
   } );
   return;
}

void database::update_withdraw_permissions()
{
   auto& permit_index = get_index_type<withdraw_permission_index>().indices().get<by_expiration>();
   while( !permit_index.empty() && permit_index.begin()->expiration <= head_block_time() )
      remove(*permit_index.begin());
}

} }
