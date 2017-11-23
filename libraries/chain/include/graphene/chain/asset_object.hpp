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
#pragma once
#include <graphene/chain/protocol/asset_ops.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <graphene/db/flat_index.hpp>
#include <graphene/db/generic_index.hpp>

namespace graphene { namespace chain {
   class account_object;
   class database;
   using namespace graphene::db;

   /**
    *  @brief tracks the asset information that changes frequently
    *  @ingroup object
    *  @ingroup implementation
    *
    *  Because the asset_object is very large it doesn't make sense to save an undo state
    *  for all of the parameters that never change.   This object factors out the parameters
    *  of an asset that change in almost every transaction that involves the asset.
    *
    *  This object exists as an implementation detail and its ID should never be referenced by
    *  a blockchain operation.
    */
   class asset_dynamic_data_object : public abstract_object<asset_dynamic_data_object>
   {
      public:
         static const uint8_t space_id = implementation_ids;
         static const uint8_t type_id  = impl_asset_dynamic_data_type;

         /// The number of shares currently in existence
         share_type current_supply = 0;
         share_type asset_pool = 0; ///< pool for this asset
         share_type core_pool = 0;  ///< pool for core asset
   };

   /**
    *  @brief tracks the parameters of an asset
    *  @ingroup object
    *
    *  All assets have a globally unique symbol name that controls how they are traded and an issuer who
    *  has authority over the parameters of the asset.
    */
   class asset_object : public graphene::db::abstract_object<asset_object>
   {
      public:
         static const uint8_t space_id = protocol_ids;
         static const uint8_t type_id  = asset_object_type;

         /// This function does not check if any registered asset has this symbol or not; it simply checks whether the
         /// symbol would be valid.
         /// @return true if symbol is a valid ticker symbol; false otherwise.
         static bool is_valid_symbol( const string& symbol );

         /// @return true if this is monitored asset; false otherwise.
         bool is_monitored_asset()const { return monitored_asset_opts.valid(); }

         /// Helper function to get an asset object with the given amount in this asset's type
         asset amount(share_type a)const { return asset(a, id); }
         /// Convert a string amount (i.e. "123.45") to an asset object with this asset's type
         /// The string may have a decimal and/or a negative sign.
         asset amount_from_string(string amount_string)const;
         /// Convert an asset to a textual representation, i.e. "123.45"
         string amount_to_string(share_type amount)const;
         /// Convert an asset to a textual representation, i.e. "123.45"
         string amount_to_string(const asset& amount)const
         { FC_ASSERT(amount.asset_id == id); return amount_to_string(amount.amount); }
         /// Convert an asset to a textual representation with symbol, i.e. "123.45 USD"
         string amount_to_pretty_string(share_type amount)const
         { return amount_to_string(amount) + " " + symbol; }
         /// Convert an asset to a textual representation with symbol, i.e. "123.45 USD"
         string amount_to_pretty_string(const asset &amount)const
         { FC_ASSERT(amount.asset_id == id); return amount_to_pretty_string(amount.amount); }

         /// Ticker symbol for this asset, i.e. "USD"
         string symbol;
         /// Maximum number of digits after the decimal point (must be <= 12)
         uint8_t precision = 0;
         /// ID of the account which issued this asset.
         account_id_type issuer;

         /// The meaning/purpose of this asset
         string description;

         /// set for monitored assets
         optional<monitored_asset_options> monitored_asset_opts;

         /// set for user issued asset
         asset_options options;

         /// Current supply, fee pool, and collected fees are stored in a separate object as they change frequently.
         asset_dynamic_data_id_type  dynamic_asset_data_id;

         asset_id_type get_id()const { return id; }

         void validate()const {}

         /*
          * converts asset of ID type to core asset or vice versa. Pools are adjusted accordingly.
          *
          *
          */
         template<class DB>
         asset convert( asset from, DB& db )const{
            const auto& ao = db.template get<asset_object>(id);

            if( id == asset_id_type() )
            {
               FC_ASSERT(from.asset_id == asset_id_type(), "Convert can't be called on DCT");
               return from;
            }

            if( ao.is_monitored_asset() )
               return convert_mia(from, db);
            return convert_uia(from, db);
         };

         template<class DB>
         asset convert_uia( asset from, DB& db ) const{
            asset to;
            const auto& add = db.template get<asset_dynamic_data_object>(dynamic_asset_data_id);
            const auto& ao = db.template get<asset_object>(id);
            FC_ASSERT(ao.options.is_exchangeable && !ao.options.core_exchange_rate.is_null());

            asset core_pool_diff;
            asset asset_pool_diff;
            price rate = ao.options.core_exchange_rate;


            if( from.asset_id == id ){
               to.asset_id = asset_id_type();
               to = from * rate;

               core_pool_diff = -to;
               asset_pool_diff = from;

            }else{
               FC_ASSERT(from.asset_id == asset_id_type(), "Unsupported conversion");

               to.asset_id = id;
               to = from * rate;

               core_pool_diff = -from;
               asset_pool_diff = to;
            }

            FC_ASSERT( add.asset_pool + asset_pool_diff.amount >= share_type(0), "Insufficient funds in asset pool to perform conversion" );
            FC_ASSERT( add.core_pool + core_pool_diff.amount >= share_type(0), "Insufficient funds in core pool to perform conversion"  );

            db.template modify<asset_dynamic_data_object>(add,[&](asset_dynamic_data_object& ado){
                 ado.asset_pool +=asset_pool_diff.amount;
                 ado.core_pool +=core_pool_diff.amount;
            });
            return to;
         };

         template<class DB>
         asset convert_mia( asset from, DB& db ) const{
            const auto& ao = db.template get<asset_object>(id);
            FC_ASSERT( ao.is_monitored_asset() && ao.monitored_asset_opts->feed_is_valid(db.head_block_time()) );
            FC_ASSERT( from.asset_id == asset_id_type() || from.asset_id == id );

            return from * ao.monitored_asset_opts->current_feed.core_exchange_rate;
         };

         template<class DB>
         bool can_convert( asset from, asset& to, const DB& db ) const{
            const auto& add = db.template get<asset_dynamic_data_object>(dynamic_asset_data_id);
            const auto& ao = db.template get<asset_object>(id);

            if( id == asset_id_type() ) {
               if( from.asset_id == asset_id_type()) {
                  to = from;
                  return true;
               }
               return false;
            }

            if(!ao.options.is_exchangeable || ao.options.core_exchange_rate.is_null())
               return false;

            if( ao.is_monitored_asset()  ){
               if( ao.monitored_asset_opts->feed_is_valid(db.head_block_time()) && ( from.asset_id == asset_id_type() || from.asset_id == id )){
                  to = from * ao.monitored_asset_opts->current_feed.core_exchange_rate;
                  return true;
               }
               return false;
            }

            asset core_pool_diff;
            asset asset_pool_diff;
            price rate = ao.options.core_exchange_rate;
            if( from.asset_id == id ){
               to.asset_id = asset_id_type();
               to = from * rate;

               core_pool_diff = -to;
               asset_pool_diff = from;
            }else{
               if ( from.asset_id != asset_id_type() ) //unknown conversion
                  return false;
               to.asset_id = id;
               to = from * rate;

               core_pool_diff = -from;
               asset_pool_diff = to;
            }

            if ( add.asset_pool + asset_pool_diff.amount < share_type(0) || add.core_pool + core_pool_diff.amount < share_type(0) ) {
               return false;
            }

            return true;

         };

         template<class DB>
         const asset_dynamic_data_object& dynamic_data(const DB& db)const
         { return db.get(dynamic_asset_data_id); }

         /**
          *  The total amount of an asset that is reserved for future issuance. 
          */
         template<class DB>
         share_type reserved( const DB& db )const
         { return options.max_supply - dynamic_data(db).current_supply; }
   };

   struct by_symbol;
      struct by_type;
   typedef multi_index_container<
      asset_object,
      indexed_by<
         ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
         ordered_unique< tag<by_symbol>, member<asset_object, string, &asset_object::symbol> >,
            ordered_unique< tag<by_type>,
               composite_key< asset_object,
                  const_mem_fun<asset_object, bool, &asset_object::is_monitored_asset>,
                  member< object, object_id_type, &object::id >
               >
            >
        >
   > asset_object_multi_index_type;
   typedef generic_index<asset_object, asset_object_multi_index_type> asset_index;

} } // graphene::chain

FC_REFLECT_DERIVED( graphene::chain::asset_dynamic_data_object, (graphene::db::object),
                    (current_supply)(asset_pool)(core_pool) )

FC_REFLECT_DERIVED( graphene::chain::asset_object, (graphene::db::object),
                    (symbol)
                    (precision)
                    (issuer)
                    (description)
                    (monitored_asset_opts)
                    (options)
                    (dynamic_asset_data_id)
                  )
