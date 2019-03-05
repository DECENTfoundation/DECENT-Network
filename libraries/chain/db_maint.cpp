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
vector<std::reference_wrapper<const typename Index::object_type>> database::sort_votable_objects() const
{
   using ObjectType = typename Index::object_type;
   const auto& all_objects = get_index_type<Index>().indices();
   vector<std::reference_wrapper<const ObjectType>> refs;
   refs.reserve(all_objects.size());
   std::transform(all_objects.begin(), all_objects.end(),
                  std::back_inserter(refs),
                  [](const ObjectType& o) { return std::cref(o); });
   std::stable_sort(refs.begin(), refs.end(),
                   [this](const ObjectType& a, const ObjectType& b)->bool {
      share_type oa_vote = _vote_tally_buffer[a.vote_id];
      share_type ob_vote = _vote_tally_buffer[b.vote_id];
      if( oa_vote != ob_vote )
         return oa_vote > ob_vote;
      return a.vote_id < b.vote_id;
   });

   return refs;
}

template<class... Types>
void database::perform_account_maintenance(std::tuple<Types...> helpers)
{
   const auto& idx = get_index_type<account_index>().indices().get<by_name>();
   for( const account_object& a : idx )
      detail::for_each(helpers, a, detail::gen_seq<sizeof...(Types)>());
}
void database::update_active_miners()
{ try {
   assert( _miner_count_histogram_buffer.size() > 0 );
   share_type stake_target = (_total_voting_stake-_miner_count_histogram_buffer[0]) / 2;

   /// accounts that vote for 0 or 1 miner do not get to express an opinion on
   /// the number of miners to have (they abstain and are non-voting accounts)

   share_type stake_tally = 0; 

   size_t miner_count = 0;
   if( stake_target > 0 )
   {
      while( (miner_count < _miner_count_histogram_buffer.size() - 1)
             && (stake_tally <= stake_target) )
      {
         stake_tally += _miner_count_histogram_buffer[++miner_count];
      }
   }

   const chain_property_object& cpo = get_chain_properties();
   auto wits = sort_votable_objects<miner_index>();
   size_t count = std::min(std::max(miner_count*2+1, (size_t)cpo.immutable_parameters.min_miner_count), wits.size());

   const global_property_object& gpo = get_global_properties();

   uint32_t ranking = 0;
   for( const miner_object& wit : wits )
   {
      modify( wit, [&]( miner_object& obj ){
              obj.total_votes = _vote_tally_buffer[wit.vote_id];
              obj.vote_ranking = ranking++;
              });
   }

   // Update miner authority
   modify( get(GRAPHENE_MINER_ACCOUNT), [&]( account_object& a )
   {
      vote_counter vc;
      for( const miner_object& wit : wits )
         vc.add( wit.miner_account, _vote_tally_buffer[wit.vote_id] );
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

} FC_RETHROW() }


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
      uint64_t blocks_to_maint = (uint64_t(time_to_maint) + gpo.parameters.block_interval - 1) / gpo.parameters.block_interval - gpo.parameters.maintenance_skip_slots;

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


void database::perform_chain_maintenance(const signed_block& next_block, const global_property_object& global_props)
{
   const auto& gpo = get_global_properties();

   struct vote_tally_helper {
      database& d;
      const global_property_object& props;

      vote_tally_helper(database& d, const global_property_object& gpo)
         : d(d), props(gpo)
      {
         d._vote_tally_buffer.resize(props.next_available_vote_id);
         d._miner_count_histogram_buffer.resize(props.parameters.maximum_miner_count / 2 + 1);
         d._total_voting_stake = 0;
      }

      void operator()(const account_object& stake_account) {
         // There may be a difference between the account whose stake is voting and the one specifying opinions.
         // Usually they're the same, but if the stake account has specified a voting_account, that account is the one
         // specifying the opinions.
         const account_object& opinion_account = (stake_account.options.voting_account == GRAPHENE_PROXY_TO_SELF_ACCOUNT) ? stake_account : d.get(stake_account.options.voting_account);
         
         const auto& stats = stake_account.statistics(d);
         uint64_t voting_stake = stats.total_core_in_orders.value
            + (stake_account.cashback_vb.valid() ? (*stake_account.cashback_vb)(d).balance.amount.value: 0)
            + d.get_balance(stake_account.get_id(), asset_id_type()).amount.value;
         
         for( vote_id_type id : opinion_account.options.votes )
         {
            uint32_t offset = id.instance();
            // if they somehow managed to specify an illegal offset, ignore it.
            if( offset < d._vote_tally_buffer.size() )
               d._vote_tally_buffer[offset] += voting_stake;
         }
         
         if( opinion_account.options.num_miner <= props.parameters.maximum_miner_count )
         {
            uint16_t offset = std::min(size_t(opinion_account.options.num_miner/2),
                                       d._miner_count_histogram_buffer.size() - 1);
            // votes for a number greater than maximum_miner_count
            // are turned into votes for maximum_miner_count.
            //
            // in particular, this takes care of the case where a
            // member was voting for a high number, then the
            // parameter was lowered.
            d._miner_count_histogram_buffer[offset] += voting_stake;
         }
         
         d._total_voting_stake += voting_stake;
      }
      
   } tally_helper(*this, gpo);

   perform_account_maintenance(std::tie(
      tally_helper
   ));

   struct clear_canary {
      clear_canary(vector<uint64_t>& target): target(target){}
      ~clear_canary() { target.clear(); }
   private:
      vector<uint64_t>& target;
   };
   clear_canary a(_miner_count_histogram_buffer),
                b(_vote_tally_buffer);

   update_active_miners();
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
         next_maintenance_time = time_point_sec() +
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
         next_maintenance_time += (y+1) * maintenance_interval;
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

} }
