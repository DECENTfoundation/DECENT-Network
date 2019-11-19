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

#include <boost/multiprecision/integer.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/uint128.hpp>

#include <graphene/chain/database.hpp>
#include <graphene/chain/hardfork.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/budget_record_object.hpp>
#include <graphene/chain/chain_property_object.hpp>
#include <graphene/chain/global_property_object.hpp>
#include <graphene/chain/vesting_balance_object.hpp>
#include <graphene/chain/vote_count.hpp>
#include <graphene/chain/miner_object.hpp>

namespace graphene { namespace chain {

template<class Index>
std::vector<std::reference_wrapper<const typename Index::object_type>> database::sort_votable_objects(const std::vector<uint64_t> &vote_tally_buffer) const
{
   using ObjectType = typename Index::object_type;
   const auto& all_objects = get_index_type<Index>().indices();
   std::vector<std::reference_wrapper<const ObjectType>> refs;
   refs.reserve(all_objects.size());
   std::transform(all_objects.begin(), all_objects.end(),
                  std::back_inserter(refs),
                  [](const ObjectType& o) { return std::cref(o); });
   std::stable_sort(refs.begin(), refs.end(),
                   [&](const ObjectType& a, const ObjectType& b)->bool {
      share_type oa_vote = vote_tally_buffer[a.vote_id];
      share_type ob_vote = vote_tally_buffer[b.vote_id];
      if( oa_vote != ob_vote )
         return oa_vote > ob_vote;
      return a.vote_id < b.vote_id;
   });

   return refs;
}

/**
 * Update the budget for miners.
 */
void database::process_budget()
{
   try
   {
      const global_property_object& gpo = get_global_properties();
      const dynamic_global_property_object& dpo = get_dynamic_global_properties();
      const asset_dynamic_data_object& core =
         asset_id_type(0)(*this).dynamic_asset_data_id(*this);
      fc::time_point_sec now = head_block_time();

      int64_t time_to_maint = (dpo.next_maintenance_time - now).to_seconds();
      //
      // The code that generates the next maintenance time should
      //    only produce a result in the future.  If this assert
      //    fails, then the next maintenance time algorithm is buggy.
      //
      assert( time_to_maint > 0 );
      //
      // Code for setting chain parameters should validate
      //    block_interval > 0 (as well as the humans proposing /
      //    voting on changes to block interval).
      //
      assert( gpo.parameters.block_interval > 0 );
      uint32_t blocks_to_maint = static_cast<uint32_t>((uint64_t(time_to_maint) + gpo.parameters.block_interval - 1) / gpo.parameters.block_interval - gpo.parameters.maintenance_skip_slots);

      // blocks_to_maint > 0 because time_to_maint > 0,
      // which means numerator is at least equal to block_interval

      budget_record rec;
      {
         const asset_object& core_asset = asset_id_type(0)(*this);
         rec.from_initial_reserve = core_asset.reserved(*this);
         rec.from_accumulated_fees = core.asset_pool + dpo.unspent_fee_budget;
         rec._real_supply = get_real_supply();
         if(    (dpo.last_budget_time == fc::time_point_sec())
                || (now <= dpo.last_budget_time) )
         {
            rec.time_since_last_budget = 0;
         }else{
            rec.time_since_last_budget = uint64_t( (now - dpo.last_budget_time).to_seconds() );
         }
         rec.next_maintenance_time = dpo.next_maintenance_time;
         rec.block_interval = gpo.parameters.block_interval;
      }

      share_type planned_for_mining = get_miner_budget(blocks_to_maint) ;
      rec.planned_for_mining = planned_for_mining + rec.from_accumulated_fees;
      rec.generated_in_last_interval = dpo.mined_rewards + dpo.miner_budget_from_fees - dpo.unspent_fee_budget;

      rec.supply_delta = rec.generated_in_last_interval - core.asset_pool;

      modify(core, [&]( asset_dynamic_data_object& _core )
      {
         _core.current_supply = (_core.current_supply + rec.supply_delta );

         assert( _core.current_supply == rec._real_supply.total() );

         _core.asset_pool = 0;
      });

      modify(dpo, [&]( dynamic_global_property_object& _dpo )
      {
         // Since initial miner_budget was rolled into
         // available_funds, we replace it with miner_budget
         // instead of adding it.
         _dpo.miner_budget_from_fees = rec.from_accumulated_fees;
         _dpo.miner_budget_from_rewards = planned_for_mining;
         _dpo.unspent_fee_budget = _dpo.miner_budget_from_fees;
         _dpo.mined_rewards = 0;
         _dpo.last_budget_time = now;
      });

      create< budget_record_object >( [&]( budget_record_object& _rec )
      {
         _rec.time = head_block_time();
         _rec.record = rec;
      });

      // available_funds is money we could spend, but don't want to.
      // we simply let it evaporate back into the reserve.
   }
   FC_RETHROW()
}

uint64_t database::get_voting_stake(const account_object& acct) const
{
   const auto& stats = acct.statistics(*this);
   return stats.total_core_in_orders.value
      + (acct.cashback_vb.valid() ? (*acct.cashback_vb)(*this).balance.amount.value: 0)
      + get_balance(acct.get_id(), asset_id_type()).amount.value;
}

void database::perform_chain_maintenance(const signed_block& next_block)
{
   const global_property_object& gpo = get_global_properties();
   std::vector<uint64_t> vote_tally_buffer(gpo.next_available_vote_id);
   std::vector<uint64_t> miner_count_histogram_buffer(gpo.parameters.maximum_miner_count / 2 + 1);
   std::map<vote_id_type, std::vector<std::pair<account_id_type, uint64_t>>> miner_votes_gained;
   uint64_t total_voting_stake = 0;

   for( const account_object& a : get_index_type<account_index>().indices().get<by_name>() ) {
      // There may be a difference between the account whose stake is voting and the one specifying opinions.
      // Usually they're the same, but if the stake account has specified a voting_account, that account is the one
      // specifying the opinions.
      const account_object& opinion_account = (a.options.voting_account == GRAPHENE_PROXY_TO_SELF_ACCOUNT) ? a : get(a.options.voting_account);
      uint64_t voting_stake = get_voting_stake(a);

      for( vote_id_type id : opinion_account.options.votes )
      {
         uint32_t offset = id.instance();
         // if they somehow managed to specify an illegal offset, ignore it.
         if( offset < vote_tally_buffer.size() ) {
            vote_tally_buffer[offset] += voting_stake;
            miner_votes_gained[id].push_back(std::make_pair(a.get_id(), voting_stake));
         }
      }

      if( opinion_account.options.num_miner <= gpo.parameters.maximum_miner_count )
      {
         auto offset = std::min(size_t(opinion_account.options.num_miner/2), miner_count_histogram_buffer.size() - 1);
         // votes for a number greater than maximum_miner_count
         // are turned into votes for maximum_miner_count.
         //
         // in particular, this takes care of the case where a
         // member was voting for a high number, then the
         // parameter was lowered.
         miner_count_histogram_buffer[offset] += voting_stake;
      }

      total_voting_stake += voting_stake;
      modify(get(a.statistics), [&](account_statistics_object& stats) {
         stats.voting_stake = voting_stake;
         stats.votes = opinion_account.options.votes;
      });
   }

   FC_ASSERT( !miner_count_histogram_buffer.empty() );
   share_type stake_target = (total_voting_stake - miner_count_histogram_buffer.front()) / 2;

   /// accounts that vote for 0 or 1 miner do not get to express an opinion on
   /// the number of miners to have (they abstain and are non-voting accounts)

   share_type stake_tally = 0;
   size_t miner_count = 0;
   if( stake_target > 0 )
   {
      while( (miner_count < miner_count_histogram_buffer.size() - 1) && (stake_tally <= stake_target) )
      {
         stake_tally += miner_count_histogram_buffer[++miner_count];
      }
   }

   const chain_property_object& cpo = get_chain_properties();
   auto wits = sort_votable_objects<miner_index>(vote_tally_buffer);
   size_t count = std::min(std::max(miner_count*2+1, (size_t)cpo.immutable_parameters.min_miner_count), wits.size());

   uint32_t ranking = 0;
   for( const miner_object& wit : wits )
   {
      modify( wit, [&]( miner_object& obj ){
         obj.total_votes = vote_tally_buffer[wit.vote_id];
         obj.vote_ranking = ranking++;
         obj.votes_gained = miner_votes_gained[obj.vote_id];
      });
   }

   // Update miner authority
   modify( get(GRAPHENE_MINER_ACCOUNT), [&]( account_object& a )
   {
      vote_counter vc;
      std::for_each( wits.begin(), wits.begin() + count,
         [&vote_tally_buffer, &vc](const miner_object& miner){ vc.add( miner.miner_account, vote_tally_buffer[miner.vote_id] ); } );
      vc.finish( a.active );
   } );

   modify(gpo, [&]( global_property_object& gp ){
      gp.active_miners.clear();
      gp.active_miners.reserve(wits.size());
      std::transform(wits.begin(), wits.begin() + count,
                     std::inserter(gp.active_miners, gp.active_miners.end()),
                     [](const miner_object& w) {
         return w.id;
      });
   });

   decent_housekeeping();

   modify(gpo, [&](global_property_object& p) {
      if( p.pending_parameters )
      {
         p.parameters = std::move(*p.pending_parameters);
         p.pending_parameters.reset();
      }
   });

   auto next_maintenance_time = get<dynamic_global_property_object>(dynamic_global_property_id_type()).next_maintenance_time;
   auto maintenance_interval = gpo.parameters.maintenance_interval;
   //uint32_t maintenance_interval_in_blocks = 0; DEBUG
   //if( gpo.parameters.block_interval )
     // maintenance_interval_in_blocks = maintenance_interval / gpo.parameters.block_interval;

   if( next_maintenance_time <= next_block.timestamp )
   {
      if( next_block.block_num() == 1 )
         next_maintenance_time = fc::time_point_sec() +
               (((next_block.timestamp.sec_since_epoch() / maintenance_interval) + 1) * maintenance_interval);
      else
      {
         // We want to find the smallest k such that next_maintenance_time + k * maintenance_interval > head_block_time()
         //  This implies k > ( head_block_time() - next_maintenance_time ) / maintenance_interval
         //
         // Let y be the right-hand side of this inequality, i.e.
         // y = ( head_block_time() - next_maintenance_time ) / maintenance_interval
         //
         // and let the fractional part f be y-floor(y).  Clearly 0 <= f < 1.
         // We can rewrite f = y-floor(y) as floor(y) = y-f.
         //
         // Clearly k = floor(y)+1 has k > y as desired.  Now we must
         // show that this is the least such k, i.e. k-1 <= y.
         //
         // But k-1 = floor(y)+1-1 = floor(y) = y-f <= y.
         // So this k suffices.
         //
         auto y = (head_block_time() - next_maintenance_time).to_seconds() / maintenance_interval;
         next_maintenance_time += static_cast<uint32_t>( (y+1) * maintenance_interval );
      }
   }

   const dynamic_global_property_object& dgpo = get_dynamic_global_properties();

   modify(dgpo, [next_maintenance_time](dynamic_global_property_object& d) {
      d.next_maintenance_time = next_maintenance_time;
      d.accounts_registered_this_interval = 0;
   });

   // process_budget needs to run at the bottom because
   //   it needs to know the next_maintenance_time
   //TODO_DECENT rework
   process_budget();
}

share_type database::get_miner_pay_from_fees_by_block_time(fc::time_point_sec block_time) const
{
   const auto& idx = get_index_type<budget_record_index>().indices().get<by_time>();
   FC_ASSERT(idx.crbegin()->record.next_maintenance_time > block_time);
   miner_reward_input rwd;

   fc::time_point_sec next_time = (fc::time_point_sec)0;
   fc::time_point_sec prev_time = (fc::time_point_sec)0;

   auto itr = idx.cbegin();
   for (auto itr_stop = idx.cend(); itr != itr_stop && (next_time == (fc::time_point_sec)0); ++itr)
   {
      if (itr->record.next_maintenance_time > block_time)
      {
         next_time = itr->record.next_maintenance_time;
         rwd.from_accumulated_fees = itr->record.from_accumulated_fees;
         rwd.block_interval = itr->record.block_interval;
      }
   }

   FC_ASSERT(next_time != (fc::time_point_sec)0);

   itr--;

   if (itr == idx.begin())
   {
      fc::optional<signed_block> first_block = fetch_block_by_number(1);
      prev_time = first_block->timestamp;
      rwd.time_to_maint = (next_time - prev_time).to_seconds();
   }
   else
   {
      itr--;

      prev_time = (*itr).record.next_maintenance_time;
      rwd.time_to_maint = (next_time - prev_time).to_seconds();
   }

   auto blocks_in_interval = (rwd.time_to_maint + rwd.block_interval - 1) / rwd.block_interval;
   return blocks_in_interval > 0 ? rwd.from_accumulated_fees / blocks_in_interval : 0;
}

miner_reward_input database::get_time_to_maint_by_block_time(fc::time_point_sec block_time) const
{
   const auto& idx = get_index_type<budget_record_index>().indices().get<by_time>();
   FC_ASSERT(idx.crbegin()->record.next_maintenance_time > block_time);
   miner_reward_input rwd;

   fc::time_point_sec next_time = (fc::time_point_sec)0;
   for (auto itr = idx.cbegin(), itr_stop = idx.cend(); itr != itr_stop && (next_time == (fc::time_point_sec)0); ++itr )
   {
      if (itr->record.next_maintenance_time > block_time)
      {
         next_time = itr->record.next_maintenance_time;
         rwd.from_accumulated_fees = itr->record.from_accumulated_fees;
         rwd.block_interval = itr->record.block_interval;
      }
   }

   FC_ASSERT(next_time != (fc::time_point_sec)0);
   rwd.time_to_maint = (next_time - block_time).to_seconds();
   return rwd;
}

} }
