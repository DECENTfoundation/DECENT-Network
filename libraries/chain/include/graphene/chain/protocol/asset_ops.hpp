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

   struct monitored_asset_options
   {
      /// Feeds published for this asset. If issuer is not committee, the keys in this map are the feed publishing
      /// accounts; otherwise, the feed publishers are the currently active committee_members and witnesses and this map
      /// should be treated as an implementation detail. The timestamp on each feed is the time it was published.
      flat_map<account_id_type, pair<time_point_sec,price_feed>> feeds;
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

      void validate()const;
   };

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

      optional<monitored_asset_options> monitored_asset_opts;

      /**
       * data that describes the meaning/purpose of this asset, fee will be charged proportional to
       * size of description.
       */
      string description;
      extensions_type extensions;

      /// Perform internal consistency checks.
      /// @throws fc::exception if any check fails
      void validate()const;
   };

   /**
    * @ingroup operations
    */
   struct asset_create_operation : public base_operation
   {
      struct fee_parameters_type { 
         uint64_t symbol3        = 500000 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint64_t symbol4        = 300000 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint64_t long_symbol    = 5000   * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint32_t price_per_kbyte = 10; /// only required for large memos.
      };

      asset                   fee;
      /// This account must sign and pay the fee for this operation. Later, this account may update the asset
      account_id_type         issuer;
      /// The ticker symbol of this asset
      string                  symbol;
      /// Number of digits to the right of decimal point, must be less than or equal to 12
      uint8_t                 precision = 0;

      /// Options common to all assets.
      ///
      /// @note common_options.core_exchange_rate technically needs to store the asset ID of this new asset. Since this
      /// ID is not known at the time this operation is created, create this price as though the new asset has instance
      /// ID 1, and the chain will overwrite it with the new asset's ID.
      asset_options              common_options;
      extensions_type extensions;

      account_id_type fee_payer()const { return issuer; }
      void            validate()const;
      share_type      calculate_fee( const fee_parameters_type& k )const;
   };

   /**
    * @ingroup operations
    */
   struct asset_fund_fee_pool_operation : public base_operation
   {
      struct fee_parameters_type { uint64_t fee =  GRAPHENE_BLOCKCHAIN_PRECISION; };

      asset           fee; ///< core asset
      account_id_type from_account;
      asset_id_type   asset_id;
      share_type      amount; ///< core asset
      extensions_type extensions;

      account_id_type fee_payer()const { return from_account; }
      void       validate()const;
   };

   /**
    * @brief Update options common to all assets
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
   struct asset_update_operation : public base_operation
   {
      struct fee_parameters_type { 
         uint64_t fee            = 500 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint32_t price_per_kbyte = 10;
      };

      asset_update_operation(){}

      asset           fee;
      account_id_type issuer;
      asset_id_type   asset_to_update;

      /// If the asset is to be given a new issuer, specify his ID here.
      optional<account_id_type>   new_issuer;
      asset_options               new_options;
      extensions_type             extensions;

      account_id_type fee_payer()const { return issuer; }
      void            validate()const;
      share_type      calculate_fee(const fee_parameters_type& k)const;
   };

      /**
       * @brief Update options specific to BitAssets
       * @ingroup operations
       *
       * BitAssets have some options which are not relevant to other asset types. This operation is used to update those
       * options an an existing BitAsset.
       *
       * @pre @ref issuer MUST be an existing account and MUST match asset_object::issuer on @ref asset_to_update
       * @pre @ref asset_to_update MUST be a BitAsset, i.e. @ref asset_object::is_monitored_asset() returns true
       * @pre @ref fee MUST be nonnegative, and @ref issuer MUST have a sufficient balance to pay it
       * @pre @ref new_options SHALL be internally consistent, as verified by @ref validate()
       * @post @ref asset_to_update will have BitAsset-specific options matching those of new_options
       */
      struct asset_update_monitored_asset_operation : public base_operation
      {
         struct fee_parameters_type { uint64_t fee = 500 * GRAPHENE_BLOCKCHAIN_PRECISION; };

         asset           fee;
         account_id_type issuer;
         asset_id_type   asset_to_update;

         monitored_asset_options new_options;
         extensions_type  extensions;

         account_id_type fee_payer()const { return issuer; }
         void            validate()const;
      };

   /**
    * @ingroup operations
    */
   struct asset_issue_operation : public base_operation
   {
      struct fee_parameters_type { 
         uint64_t fee = 20 * GRAPHENE_BLOCKCHAIN_PRECISION; 
         uint32_t price_per_kbyte = GRAPHENE_BLOCKCHAIN_PRECISION;
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
    * @brief used to transfer accumulated fees back to the issuer's balance.
    */
   struct asset_claim_fees_operation : public base_operation
   {
      struct fee_parameters_type {
         uint64_t fee = 20 * GRAPHENE_BLOCKCHAIN_PRECISION;
      };

      asset           fee;
      account_id_type issuer;
      asset           amount_to_claim; /// amount_to_claim.asset_id->issuer must == issuer
      extensions_type extensions;

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
      struct fee_parameters_type { uint64_t fee = GRAPHENE_BLOCKCHAIN_PRECISION; };

      asset                  fee; ///< paid for by publisher
      account_id_type        publisher;
      asset_id_type          asset_id; ///< asset for which the feed is published
      price_feed             feed;
      extensions_type        extensions;

      account_id_type fee_payer()const { return publisher; }
      void            validate()const;
   };

} } // graphene::chain

FC_REFLECT( graphene::chain::asset_claim_fees_operation, (fee)(issuer)(amount_to_claim)(extensions) )
FC_REFLECT( graphene::chain::asset_claim_fees_operation::fee_parameters_type, (fee) )

FC_REFLECT( graphene::chain::asset_options,
            (max_supply)
            (core_exchange_rate)
            (monitored_asset_opts)
            (description)
            (extensions)
          )

FC_REFLECT( graphene::chain::monitored_asset_options,
            (feeds)
            (current_feed)
            (current_feed_publication_time)
            (feed_lifetime_sec)
            (minimum_feeds)
)

FC_REFLECT( graphene::chain::asset_create_operation::fee_parameters_type, (symbol3)(symbol4)(long_symbol)(price_per_kbyte) )
FC_REFLECT( graphene::chain::asset_fund_fee_pool_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::asset_update_operation::fee_parameters_type, (fee)(price_per_kbyte) )
FC_REFLECT( graphene::chain::asset_issue_operation::fee_parameters_type, (fee)(price_per_kbyte) )
FC_REFLECT( graphene::chain::asset_publish_feed_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::asset_update_monitored_asset_operation::fee_parameters_type, (fee) )

FC_REFLECT( graphene::chain::asset_create_operation,
            (fee)
            (issuer)
            (symbol)
            (precision)
            (common_options)
            (extensions)
          )
FC_REFLECT( graphene::chain::asset_update_operation,
            (fee)
            (issuer)
            (asset_to_update)
            (new_issuer)
            (new_options)
            (extensions)
          )
FC_REFLECT( graphene::chain::asset_issue_operation,
            (fee)(issuer)(asset_to_issue)(issue_to_account)(memo)(extensions) )
FC_REFLECT( graphene::chain::asset_fund_fee_pool_operation, (fee)(from_account)(asset_id)(amount)(extensions) );
FC_REFLECT( graphene::chain::asset_publish_feed_operation,
            (fee)(publisher)(asset_id)(feed)(extensions) )
FC_REFLECT( graphene::chain::asset_update_monitored_asset_operation, (fee)(issuer)(asset_to_update)(new_options)(extensions) )