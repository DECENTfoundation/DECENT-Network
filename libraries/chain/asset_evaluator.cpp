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
#include <graphene/chain/asset_evaluator.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/market_object.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/hardfork.hpp>

#include <functional>

namespace graphene { namespace chain {

void_result asset_create_evaluator::do_evaluate( const asset_create_operation& op )
{ try {

   database& d = db();

   const auto& chain_parameters = d.get_global_properties().parameters;

   auto& asset_indx = d.get_index_type<asset_index>().indices().get<by_symbol>();
   auto asset_symbol_itr = asset_indx.find( op.symbol );
   FC_ASSERT( asset_symbol_itr == asset_indx.end() );

   auto dotpos = op.symbol.rfind( '.' );
   if( dotpos != std::string::npos )
   {
      auto prefix = op.symbol.substr( 0, dotpos );
      auto asset_symbol_itr = asset_indx.find( prefix );
      FC_ASSERT( asset_symbol_itr != asset_indx.end(), "Asset ${s} may only be created by issuer of ${p}, but ${p} has not been registered",
            ("s",op.symbol)("p",prefix) );
      FC_ASSERT( asset_symbol_itr->issuer == op.issuer, "Asset ${s} may only be created by issuer of ${p}, ${i}",
            ("s",op.symbol)("p",prefix)("i", op.issuer(d).name) );
   }

   core_fee_paid -= core_fee_paid.value/2;

   if(op.monitored_asset_opts.valid()) {
      op.monitored_asset_opts->validate();
      FC_ASSERT( op.max_supply == 0 );
   }
   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

object_id_type asset_create_evaluator::do_apply( const asset_create_operation& op )
{ try {
   const asset_dynamic_data_object& dyn_asset =
      db().create<asset_dynamic_data_object>( [&]( asset_dynamic_data_object& a ) {
         a.current_supply = 0;
      });

   auto next_asset_id = db().get_index_type<asset_index>().get_next_id();

   const asset_object& new_asset =
     db().create<asset_object>( [&]( asset_object& a ) {
         a.issuer = op.issuer;
         a.symbol = op.symbol;
         a.precision = op.precision;
         a.description = op.description;
         a.monitored_asset_opts = op.monitored_asset_opts;
         a.max_supply = op.max_supply;

         a.dynamic_asset_data_id = dyn_asset.id;
      });
   assert( new_asset.id == next_asset_id );

   return new_asset.id;
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result asset_issue_evaluator::do_evaluate( const asset_issue_operation& o )
{ try {
   const database& d = db();

   const asset_object& a = o.asset_to_issue.asset_id(d);
   FC_ASSERT( o.issuer == a.issuer );
   FC_ASSERT( !a.is_monitored_asset(), "Cannot manually issue a market-issued asset." );

   asset_dyn_data = &a.dynamic_asset_data_id(d);
   FC_ASSERT( (asset_dyn_data->current_supply + o.asset_to_issue.amount) <= a.max_supply );

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result asset_issue_evaluator::do_apply( const asset_issue_operation& o )
{ try {
   db().adjust_balance( o.issue_to_account, o.asset_to_issue );

   db().modify( *asset_dyn_data, [&]( asset_dynamic_data_object& data ){
        data.current_supply += o.asset_to_issue.amount;
   });

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }



void_result asset_update_evaluator::do_evaluate(const asset_update_operation& o)
{ try {
   database& d = db();

   const asset_object& a = o.asset_to_update(d);
   auto a_copy = a;
   a_copy.description = o.new_description;
   a_copy.max_supply = o.max_supply;
   a_copy.validate();

   if( o.new_issuer )
      FC_ASSERT(d.find_object(*o.new_issuer));
   asset_to_update = &a;
   FC_ASSERT( o.issuer == a.issuer, "", ("o.issuer", o.issuer)("a.issuer", a.issuer) );

   return void_result();
} FC_CAPTURE_AND_RETHROW((o)) }

void_result asset_update_evaluator::do_apply(const asset_update_operation& o)
{ try {
   database& d = db();

   d.modify(*asset_to_update, [&](asset_object& a) {
      if( o.new_issuer )
         a.issuer = *o.new_issuer;
      a.description = o.new_description;
      a.max_supply = o.max_supply;
   });

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result asset_update_monitored_asset_evaluator::do_evaluate(const asset_update_monitored_asset_operation& o)
{ try {
      database& d = db();

      const asset_object& a = o.asset_to_update(d);

      FC_ASSERT(a.is_monitored_asset(), "Cannot update MonitoredAsset-specific settings on a user issued asset.");

      FC_ASSERT( o.issuer == a.issuer, "", ("o.issuer", o.issuer)("a.issuer", a.issuer) );

      return void_result();
   } FC_CAPTURE_AND_RETHROW( (o) ) }

void_result asset_update_monitored_asset_evaluator::do_apply(const asset_update_monitored_asset_operation& o)
{ try {
      bool should_update_feeds = false;

      auto &idx = db().get_index_type<asset_index>().indices().get<by_id>();
      const auto &ao = idx.find(o.asset_to_update);

      // If the minimum number of feeds to calculate a median has changed, we need to recalculate the median
      if( o.new_options.minimum_feeds != ao->monitored_asset_opts->minimum_feeds )
         should_update_feeds = true;

      db().modify(*ao, [&](asset_object& b) {
         b.monitored_asset_opts = o.new_options;

         if( should_update_feeds )
            b.monitored_asset_opts->update_median_feeds(db().head_block_time());
      });

      return void_result();
   } FC_CAPTURE_AND_RETHROW( (o) ) }


void_result asset_publish_feeds_evaluator::do_evaluate(const asset_publish_feed_operation& o)
{ try {
   database& d = db();

   const asset_object& base = o.asset_id(d);
   //Verify that this feed is for a monitored asset and that asset is backed by the base
   FC_ASSERT( base.is_monitored_asset());
   FC_ASSERT( d.get(GRAPHENE_WITNESS_ACCOUNT).active.account_auths.count(o.publisher) );
   FC_ASSERT( o.feed.core_exchange_rate.base.asset_id == asset_id_type() || o.feed.core_exchange_rate.quote.asset_id == asset_id_type() );
   FC_ASSERT( o.feed.core_exchange_rate.base.asset_id == o.asset_id || o.feed.core_exchange_rate.quote.asset_id == o.asset_id );

   return void_result();
} FC_CAPTURE_AND_RETHROW((o)) }

void_result asset_publish_feeds_evaluator::do_apply(const asset_publish_feed_operation& o)
{ try {

   database& d = db();

   const asset_object& base = o.asset_id(d);
   auto old_feed =  base.monitored_asset_opts->current_feed;
   // Store medians for this asset
   d.modify(base , [&o,&d](asset_object& a) {
      a.monitored_asset_opts->feeds[o.publisher] = make_pair(d.head_block_time(), o.feed);
      a.monitored_asset_opts->update_median_feeds(d.head_block_time());
   });

   return void_result();
} FC_CAPTURE_AND_RETHROW((o)) }


} } // graphene::chain
