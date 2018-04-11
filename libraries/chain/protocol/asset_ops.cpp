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
#include <graphene/chain/protocol/asset_ops.hpp>

namespace graphene { namespace chain {

/**
 *  Valid symbols can contain [A-Z0-9], and '.'
 *  They must start with [A, Z]
 *  They must end with [A, Z]
 *  They can contain a maximum of one '.'
 */
bool is_valid_symbol( const string& symbol )
{
    if( symbol.size() < GRAPHENE_MIN_ASSET_SYMBOL_LENGTH )
        return false;

    if( symbol.size() > GRAPHENE_MAX_ASSET_SYMBOL_LENGTH )
        return false;

    if( !isalpha( symbol.front() ) )
        return false;

    if( !isalpha( symbol.back() ) )
        return false;

    bool dot_already_present = false;
    for( const auto c : symbol )
    {
        if( (isalpha( c ) && isupper( c )) || isdigit(c) )
            continue;

        if( c == '.' )
        {
            if( dot_already_present )
                return false;

            dot_already_present = true;
            continue;
        }

        return false;
    }

    return true;
}

share_type asset_create_operation::calculate_fee(const asset_create_operation::fee_parameters_type& param)const
{

   if( monitored_asset_opts.valid() )
      return param.basic_fee;

   auto fee = 5 * param.basic_fee;

   switch(symbol.size()) {
      case 3: fee = param.basic_fee * 5000;
         break;
      case 4: fee = param.basic_fee * 200;
         break;
      default:
         break;
   }

   return fee;
}

void  asset_create_operation::validate()const
{
   FC_ASSERT( fee.amount >= 0 );
   FC_ASSERT( is_valid_symbol(symbol) );
   FC_ASSERT( precision <= 12 );
   FC_ASSERT( description.length() <= 1000 );

   if( monitored_asset_opts.valid() ) {
      monitored_asset_opts->validate();
      FC_ASSERT(options.max_supply == 0);
   }
   else {
      options.validate();
   }
}

share_type asset_issue_operation::calculate_fee(const fee_parameters_type& k)const
{
   return k.fee + calculate_data_fee( fc::raw::pack_size(memo), k.fee );
}

void asset_issue_operation::validate()const {
   FC_ASSERT(fee.amount >= 0);
   FC_ASSERT(asset_to_issue.amount.value <= GRAPHENE_MAX_SHARE_SUPPLY);
   FC_ASSERT(asset_to_issue.amount.value > 0);
   FC_ASSERT(asset_to_issue.asset_id != asset_id_type(0));
}

void asset_options::validate()const
{
   FC_ASSERT( max_supply > 0 );
   FC_ASSERT( max_supply <= GRAPHENE_MAX_SHARE_SUPPLY );
   core_exchange_rate.validate();
   FC_ASSERT( core_exchange_rate.base.asset_id.instance.value == 0 ||
              core_exchange_rate.quote.asset_id.instance.value == 0 );
}

void monitored_asset_options::validate()const
{
   current_feed.validate();
   FC_ASSERT( feed_lifetime_sec > 0 );
   FC_ASSERT( minimum_feeds > 0 );
}

void update_monitored_asset_operation::validate()const
{
   FC_ASSERT( fee.amount >= 0 );
   FC_ASSERT( new_feed_lifetime_sec >= 0 );
   FC_ASSERT( new_minimum_feeds >= 0 );
}

void update_user_issued_asset_operation::validate()const
{
   FC_ASSERT( fee.amount >= 0 );
   FC_ASSERT( max_supply >= 0 );
   core_exchange_rate.validate();

   if( new_issuer )
      FC_ASSERT(issuer != *new_issuer);

   asset dummy = asset(1, asset_to_update) * core_exchange_rate;
   FC_ASSERT(dummy.asset_id == asset_id_type());
}

void asset_reserve_operation::validate()const
{
   FC_ASSERT( fee.amount >= 0 );
   FC_ASSERT( amount_to_reserve.amount.value <= GRAPHENE_MAX_SHARE_SUPPLY );
   FC_ASSERT( amount_to_reserve.amount.value > 0 );
}

void asset_fund_pools_operation::validate() const
{
   FC_ASSERT( fee.amount >= 0 );
   FC_ASSERT( fee.asset_id == asset_id_type() );
   FC_ASSERT( dct_asset.asset_id == asset_id_type() );
   FC_ASSERT( uia_asset.amount >= 0 && dct_asset.amount >= 0 ); // are not negative
   FC_ASSERT( uia_asset.amount > 0 || dct_asset.amount > 0 );   // at least one amount must be greater than zero
}

void asset_claim_fees_operation::validate()const {
   FC_ASSERT( fee.amount >= 0 );
   FC_ASSERT( dct_asset.asset_id == asset_id_type() );
   FC_ASSERT( uia_asset.amount >= 0 && dct_asset.amount >= 0 ); // are not negative
   FC_ASSERT( uia_asset.amount > 0 || dct_asset.amount > 0 );   // at least one amount must be greater than zero
}

void asset_publish_feed_operation::validate()const
{
   FC_ASSERT( fee.amount >= 0 );
   feed.validate();
   // maybe some of these could be moved to feed.validate()
   if( !feed.core_exchange_rate.is_null() )
   {
      feed.core_exchange_rate.validate();
   }

   FC_ASSERT( !feed.core_exchange_rate.is_null() );
   FC_ASSERT( feed.is_for( asset_id ) );
}


} } // namespace graphene::chain
