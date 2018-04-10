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
#include <graphene/chain/protocol/base.hpp>
#include <graphene/chain/protocol/memo.hpp>

namespace graphene { namespace chain { 

   bool is_valid_symbol( const string& symbol );

   /**
    * @brief The asset_options struct contains options available on all assets in the network
    *
    * @note Changes to this struct will break protocol compatibility
    */
   struct asset_options {
      /// The maximum supply of this asset which may exist at any given time. This can be as large as
      /// GRAPHENE_MAX_SHARE_SUPPLY
      share_type max_supply = GRAPHENE_MAX_SHARE_SUPPLY;

      /// When a non-core asset is used to pay a fee, the blockchain must convert that asset to core asset in
      /// order to accept the fee. If this asset's fee pool is funded, the chain will automatically deposite fees
      /// in this asset to its accumulated fees, and withdraw from the fee pool the same amount as converted at
      /// the core exchange rate.
      price core_exchange_rate;

      /// True to allow implicit conversion of this asset to/from core asset.
      bool is_exchangeable = true;

      /// Extensions

      /// False when issuer can change max_supply, otherwise false
      struct fixed_max_supply_struct{
         bool is_fixed_max_supply;
         fixed_max_supply_struct(bool is_fixed=false) : is_fixed_max_supply{ is_fixed } {};
      };

      typedef static_variant<void_t, fixed_max_supply_struct>     asset_options_extensions;
      typedef flat_set<asset_options_extensions> asset_options_extensions_type;
      asset_options_extensions_type extensions;

      /// Perform internal consistency checks.
      /// @throws fc::exception if any check fails
      void validate()const;
   };

   struct monitored_asset_options
   {
      /// Feeds published for this asset. If issuer is not committee, the keys in this map are the feed publishing
      /// accounts; otherwise, the feed publishers are the currently active committee_members and miners and this map
      /// should be treated as an implementation detail. The timestamp on each feed is the time it was published.
      flat_map<account_id_type, pair<time_point_sec, price_feed>> feeds;
      /// This is the currently active price feed, calculated as the median of values from the currently active
      /// feeds.
      price_feed current_feed;
      /// This is the publication time of the oldest feed which was factored into current_feed.
      time_point_sec current_feed_publication_time;

      /// Time before a price feed expires
      uint32_t feed_lifetime_sec = GRAPHENE_DEFAULT_PRICE_FEED_LIFETIME;
      /// Minimum number of unexpired feeds required to extract a median feed from
      uint8_t minimum_feeds = 1;

      time_point_sec feed_expiration_time()const
         { return current_feed_publication_time + feed_lifetime_sec; }
      bool feed_is_expired(time_point_sec current_time)const
         { return feed_expiration_time() <= current_time; }
      void update_median_feeds(time_point_sec current_time);
      bool feed_is_valid(time_point_sec current_time) const{
         if(feed_is_expired(current_time))
            return false;
         if(current_feed.core_exchange_rate.is_null())
            return false;
         return true;
      }

      void validate()const;
   };

   /**
    * @ingroup operations
    * Creates an asset.
    */
   struct asset_create_operation : public base_operation
   {
      struct fee_parameters_type {
         uint64_t basic_fee      = 1*GRAPHENE_BLOCKCHAIN_PRECISION/1000;

      };

      asset                   fee;
      /// This account must sign and pay the fee for this operation. Later, this account may update the asset
      account_id_type         issuer;
      /// The ticker symbol of this asset
      string                  symbol;
      /// Number of digits to the right of decimal point, must be less than or equal to 12
      uint8_t                 precision = 0;

      /**
       * data that describes the meaning/purpose of this asset, fee will be charged proportional to
       * size of description.
       */
      string description;

      asset_options options;

      optional<monitored_asset_options> monitored_asset_opts;

      /// WARNING! Duplicate variable. Do no use it. It does not have any effect.
      bool is_exchangeable = true;

      extensions_type extensions;

      account_id_type fee_payer()const { return monitored_asset_opts.valid() ? account_id_type() : issuer; }
      void            validate()const;
      share_type      calculate_fee( const fee_parameters_type& k )const;
   };

   /**
    * @ingroup operations
    */
   struct asset_issue_operation : public base_operation
   {
      struct fee_parameters_type {
         uint64_t fee = 5 * GRAPHENE_BLOCKCHAIN_PRECISION/1000;
      };

      asset            fee;
      account_id_type  issuer; ///< Must be asset_to_issue->asset_id->issuer
      asset            asset_to_issue;
      account_id_type  issue_to_account;


      /** user provided data encrypted to the memo key of the "to" account */
      optional<memo_data>  memo;
      extensions_type      extensions;

      account_id_type fee_payer()const { return issuer; }
      void            validate()const;
      share_type      calculate_fee(const fee_parameters_type& k)const;
   };

   /**
    * @brief Update the options specific to a user issued asset.
    * @ingroup operations
    *
    * There are a number of options which all assets in the network use. These options are enumerated in the @ref
    * asset_options struct. This operation is used to update these options for an existing asset.
    *
    * @pre @ref issuer SHALL be an existing account and MUST match asset_object::issuer on @ref asset_to_update
    * @pre @ref fee SHALL be nonnegative, and @ref issuer MUST have a sufficient balance to pay it
    * @pre @ref new_options SHALL be internally consistent, as verified by @ref validate()
    * @post @ref asset_to_update will have options matching those of new_options
    */
   struct update_user_issued_asset_operation : public base_operation
   {
      struct fee_parameters_type {
         uint64_t fee      = 5*GRAPHENE_BLOCKCHAIN_PRECISION/1000;
      };

      asset           fee;
      account_id_type issuer;
      asset_id_type   asset_to_update;

      string new_description;
      /// If the asset is to be given a new issuer, specify his ID here.
      optional<account_id_type>   new_issuer;
      uint64_t max_supply;
      price core_exchange_rate;
      /// True to allow implicit conversion of this asset to/from core asset.
      bool is_exchangeable;

      extensions_type             extensions;

      account_id_type fee_payer()const { return issuer; }
      void            validate()const;
   };

    /**
    * @ingroup operations
    */
   struct asset_fund_pools_operation : public base_operation
   {
      struct fee_parameters_type { uint64_t fee =  5*GRAPHENE_BLOCKCHAIN_PRECISION/1000; };

      asset           fee; ///< core asset
      account_id_type from_account;
      asset           uia_asset; ///< this asset
      asset           dct_asset; ///< core asset
      extensions_type extensions;

      account_id_type fee_payer()const { return from_account; }
      void       validate()const;
   };

    /**
    * @brief used to take an asset out of circulation, returning to the issuer
    * @ingroup operations
    *
    * @note You cannot use this operation on market-issued assets.
    */
   struct asset_reserve_operation : public base_operation
   {
      struct fee_parameters_type { uint64_t fee = 5 * GRAPHENE_BLOCKCHAIN_PRECISION/1000; };

      asset             fee;
      account_id_type   payer;
      asset             amount_to_reserve;
      extensions_type   extensions;

      account_id_type fee_payer()const { return payer; }
      void            validate()const;
   };

   /**
    * @brief used to transfer accumulated fees back to the issuer's balance.
    */
   struct asset_claim_fees_operation : public base_operation
   {
      struct fee_parameters_type {
         uint64_t fee = 5 * GRAPHENE_BLOCKCHAIN_PRECISION/1000;
      };

      asset           fee;
      account_id_type issuer;
      asset           uia_asset; /// uia_asset.asset_id->issuer must == issuer
      asset           dct_asset;
      extensions_type extensions;

      account_id_type fee_payer()const { return issuer; }
      void            validate()const;
   };

   /**
    * @brief Update options common to monitored assets
    * @ingroup operations
    *
    * There are a number of options which all assets in the network use. These options are enumerated in the @ref
    * asset_options struct. This operation is used to update these options for an existing asset.
    *
    * @pre @ref issuer SHALL be an existing account and MUST match asset_object::issuer on @ref asset_to_update
    * @pre @ref fee SHALL be nonnegative, and @ref issuer MUST have a sufficient balance to pay it
    * @pre @ref new_options SHALL be internally consistent, as verified by @ref validate()
    * @post @ref asset_to_update will have options matching those of new_options
    */
   struct update_monitored_asset_operation : public base_operation
   {
      struct fee_parameters_type {
         uint64_t fee      = 5*GRAPHENE_BLOCKCHAIN_PRECISION/1000;
      };

      update_monitored_asset_operation(){}

      asset           fee;
      account_id_type issuer;
      asset_id_type   asset_to_update;

      string new_description;
      /// Time before a price feed expires
      uint32_t new_feed_lifetime_sec = 0;
      /// Minimum number of unexpired feeds required to extract a median feed from
      uint8_t new_minimum_feeds = 0;

      extensions_type             extensions;

      account_id_type fee_payer()const { return issuer; }
      void            validate()const;
   };

   /**
    * @brief Publish price feeds for market-issued assets
    * @ingroup operations
    *
    * Price feed providers use this operation to publish their price feeds for market-issued assets. A price feed is
    * used to tune the market for a particular market-issued asset. For each value in the feed, the median across all
    * committee_member feeds for that asset is calculated and the market for the asset is configured with the median of that
    * value.
    *
    * The feed in the operation contains three prices: a call price limit, a short price limit, and a settlement price.
    * The call limit price is structured as (collateral asset) / (debt asset) and the short limit price is structured
    * as (asset for sale) / (collateral asset). Note that the asset IDs are opposite to eachother, so if we're
    * publishing a feed for USD, the call limit price will be CORE/USD and the short limit price will be USD/CORE. The
    * settlement price may be flipped either direction, as long as it is a ratio between the market-issued asset and
    * its collateral.
    */
   struct asset_publish_feed_operation : public base_operation
   {
      struct fee_parameters_type { uint64_t fee = 1 * GRAPHENE_BLOCKCHAIN_PRECISION/10000000;  };

      asset                  fee; ///< paid for by publisher
      account_id_type        publisher;
      asset_id_type          asset_id; ///< asset for which the feed is published
      price_feed             feed;
      extensions_type        extensions;

      account_id_type fee_payer()const { return publisher; }
      void            validate()const;
   };

} } // graphene::chain


FC_REFLECT( graphene::chain::monitored_asset_options,
            (feeds)
            (current_feed)
            (current_feed_publication_time)
            (feed_lifetime_sec)
            (minimum_feeds)
)

FC_REFLECT( graphene::chain::asset_options::fixed_max_supply_struct, (is_fixed_max_supply) )
FC_REFLECT_TYPENAME( graphene::chain::asset_options::asset_options_extensions )
FC_REFLECT( graphene::chain::asset_options,
            (max_supply)
            (core_exchange_rate)
            (is_exchangeable)
            (extensions)
)

FC_REFLECT( graphene::chain::asset_create_operation::fee_parameters_type, (basic_fee) )
FC_REFLECT( graphene::chain::asset_issue_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::update_monitored_asset_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::update_user_issued_asset_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::asset_fund_pools_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::asset_reserve_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::asset_publish_feed_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::asset_claim_fees_operation::fee_parameters_type, (fee) )

FC_REFLECT( graphene::chain::asset_create_operation,
            (fee)
            (issuer)
            (symbol)
            (precision)
            (description)
            (options)
            (monitored_asset_opts)
            (is_exchangeable)
            (extensions)
          )

FC_REFLECT( graphene::chain::asset_issue_operation,
            (fee)
            (issuer)
            (asset_to_issue)
            (issue_to_account)
            (memo)
            (extensions)
          )

FC_REFLECT( graphene::chain::update_monitored_asset_operation,
            (fee)
            (issuer)
            (asset_to_update)
            (new_description)
            (new_feed_lifetime_sec)
            (new_minimum_feeds)
            (extensions)
          )

FC_REFLECT( graphene::chain::update_user_issued_asset_operation,
            (fee)
            (issuer)
            (asset_to_update)
            (new_description)
            (new_issuer)
            (max_supply)
            (core_exchange_rate)
            (is_exchangeable)
            (extensions)
          )

FC_REFLECT( graphene::chain::asset_reserve_operation,
            (fee)
            (payer)
            (amount_to_reserve)
            (extensions)
          )

FC_REFLECT( graphene::chain::asset_fund_pools_operation,
            (fee)
            (from_account)
            (uia_asset)
            (dct_asset)
            (extensions)
          )

FC_REFLECT( graphene::chain::asset_claim_fees_operation,
            (fee)
            (issuer)
            (uia_asset)
            (dct_asset)
            (extensions)
          )

FC_REFLECT( graphene::chain::asset_publish_feed_operation,
            (fee)
            (publisher)
            (asset_id)
            (feed)
            (extensions)
          )
