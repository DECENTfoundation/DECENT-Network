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

#include <graphene/chain/account_object.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/global_property_object.hpp>
#include <graphene/chain/vesting_balance_object.hpp>
#include <graphene/chain/miner_object.hpp>

namespace graphene { namespace chain {

asset database::get_balance(account_id_type owner, asset_id_type asset_id) const
{
   auto& index = get_index_type<account_balance_index>().indices().get<by_account_asset>();
   auto itr = index.find(boost::make_tuple(owner, asset_id));
   if( itr == index.end() )
      return asset(0, asset_id);
   return itr->get_balance();
}

asset database::get_balance(const account_object& owner, const asset_object& asset_obj) const
{
   return get_balance(owner.get_id(), asset_obj.get_id());
}

std::string database::to_pretty_string(const asset& a) const
{
   return a.asset_id(*this).amount_to_pretty_string(a.amount);
}

void database::adjust_balance(account_id_type account, asset delta )
{ try {
   if( delta.amount == 0 )
      return;

   auto& index = get_index_type<account_balance_index>().indices().get<by_account_asset>();
   auto itr = index.find(boost::make_tuple(account, delta.asset_id));
   if(itr == index.end())
   {
      FC_ASSERT( delta.amount > 0, "Insufficient Balance: ${a}'s balance of ${b} is less than required ${r}",
                 ("a",account(*this).name)
                 ("b",to_pretty_string(asset(0,delta.asset_id)))
                 ("r",to_pretty_string(-delta)));
      create<account_balance_object>([account,&delta](account_balance_object& b) {
         b.owner = account;
         b.asset_type = delta.asset_id;
         b.balance = delta.amount.value;
      });
   } else {
      if( delta.amount < 0 )
         FC_ASSERT( itr->get_balance() >= -delta, "Insufficient Balance: ${a}'s balance of ${b} is less than required ${r}", ("a",account(*this).name)("b",to_pretty_string(itr->get_balance()))("r",to_pretty_string(-delta)));
      modify(*itr, [delta](account_balance_object& b) {
         b.adjust_balance(delta);
      });
   }

} FC_CAPTURE_AND_RETHROW( (account)(delta) ) }

fc::optional<vesting_balance_id_type> database::deposit_lazy_vesting(
   const fc::optional<vesting_balance_id_type>& ovbid,
   share_type amount, uint32_t req_vesting_seconds,
   account_id_type req_owner,
   bool require_vesting )
{
   if( amount == 0 )
      return {};

   fc::time_point_sec now = head_block_time();

   while( true )
   {
      if( !ovbid.valid() )
         break;
      const vesting_balance_object& vbo = (*ovbid)(*this);
      if( vbo.owner != req_owner )
         break;
      if( vbo.policy.which() != vesting_policy::tag< cdd_vesting_policy >::value )
         break;
      if( vbo.policy.get< cdd_vesting_policy >().vesting_seconds != req_vesting_seconds )
         break;
      modify( vbo, [&]( vesting_balance_object& _vbo )
      {
         if( require_vesting )
            _vbo.deposit(now, amount);
         else
            _vbo.deposit_vested(now, amount);
      } );
      return {};
   }

   const vesting_balance_object& vbo = create< vesting_balance_object >( [&]( vesting_balance_object& _vbo )
   {
      _vbo.owner = req_owner;
      _vbo.balance = amount;

      cdd_vesting_policy policy;
      policy.vesting_seconds = req_vesting_seconds;
      policy.coin_seconds_earned = require_vesting ? 0 : amount.value * policy.vesting_seconds;
      policy.coin_seconds_earned_last_update = now;

      _vbo.policy = policy;
   } );

   return vbo.id;
}

void database::deposit_miner_pay(const miner_object& wit, share_type amount)
{
   if( amount == 0 )
      return;

   fc::optional<vesting_balance_id_type> new_vbid = deposit_lazy_vesting(
      wit.pay_vb,
      amount,
      get_global_properties().parameters.miner_pay_vesting_seconds,
      wit.miner_account,
      true );

   if( new_vbid.valid() )
   {
      modify( wit, [&]( miner_object& _wit )
      {
         _wit.pay_vb = *new_vbid;
      } );
   }

   return;
}

asset database::price_to_dct(asset price){
   if(price.asset_id == asset_id_type() )
      return price;

   asset_object ao = price.asset_id(*this);
   graphene::chain::price exchange_rate;

   if(ao.is_monitored_asset())
   {
      exchange_rate = ao.monitored_asset_opts->current_feed.core_exchange_rate;
      FC_ASSERT(!exchange_rate.is_null(), "unable to determine DCT price without price feeds");
   }
   else
   {
      exchange_rate = ao.options.core_exchange_rate;
   }

   asset price_in_dct = price * exchange_rate;
   FC_ASSERT( price_in_dct.asset_id == asset_id_type() );

   return price_in_dct;
}

bool database::are_assets_exchangeable( const asset_object& payment, const asset_object& price )
{
   // DCT -> DCT  ok
   // DCT -> MIA  ok
   // DCT -> UIA  must be exchangeable

   // MIA -> DCT | UIA | MIA not allowed

   // UIA -> DCT  must be exchangeable
   // UIA -> MIA  must be exchangeable
   // UIA -> UIA  ok, but UIAs must be the same

   if( payment.is_monitored_asset() )
      return false;
   else if( payment.id == price.id )
      return true;
   else if( !price.is_monitored_asset() &&
            payment.id != asset_id_type() && price.id != asset_id_type() ) // exchange between two different UIAs is not allowed
      return false;
   else if( payment.options.is_exchangeable && price.options.is_exchangeable ) // DCT and MIA assets are always exchangeable
      return true;
   else
      return false;
}

} }
