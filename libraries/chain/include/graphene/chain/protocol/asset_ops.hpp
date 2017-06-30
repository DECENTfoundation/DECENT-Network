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

      void validate()const;
   };


   /**
    * @ingroup operations
    * Creates an asset. If the asset is MUA, the operation may only be used in a proposed transaction, and a proposed transaction which contains this
    * operation must have a review period specified in the current global parameters before it may be accepted.
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

      /// The maximum supply of this asset which may exist at any given time. This can be as large as
      /// GRAPHENE_MAX_SHARE_SUPPLY
      share_type max_supply = GRAPHENE_MAX_SHARE_SUPPLY;

      /// Time before a price feed expires
      uint32_t feed_lifetime_sec = GRAPHENE_DEFAULT_PRICE_FEED_LIFETIME;
      /// Minimum number of unexpired feeds required to extract a median feed from
      uint8_t minimum_feeds = 1;

      extensions_type extensions;

      account_id_type fee_payer()const { return account_id_type(); }
      void            validate()const;
      share_type      calculate_fee( const fee_parameters_type& k )const;
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
         uint64_t basic_fee      = 1*GRAPHENE_BLOCKCHAIN_PRECISION/1000;
      };

      asset_update_operation(){}

      asset           fee;
      account_id_type issuer;
      asset_id_type   asset_to_update;

      /// If the asset is to be given a new issuer, specify his ID here.
      optional<account_id_type>   new_issuer;
      share_type max_supply = GRAPHENE_MAX_SHARE_SUPPLY;
      string new_description;
      /// Time before a price feed expires
      uint32_t new_feed_lifetime_sec = 0;
      /// Minimum number of unexpired feeds required to extract a median feed from
      uint8_t new_minimum_feeds = 0;

      extensions_type             extensions;

      account_id_type fee_payer()const { return issuer; }
      void            validate()const;
      share_type      calculate_fee(const fee_parameters_type& k)const;
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

FC_REFLECT( graphene::chain::asset_create_operation::fee_parameters_type, (basic_fee) )
FC_REFLECT( graphene::chain::asset_update_operation::fee_parameters_type, (basic_fee) )
FC_REFLECT( graphene::chain::asset_publish_feed_operation::fee_parameters_type, (fee) )

FC_REFLECT( graphene::chain::asset_create_operation,
            (fee)
            (issuer)
            (symbol)
            (precision)
            (description)
            (max_supply)
            (feed_lifetime_sec)
            (minimum_feeds)
            (extensions)
          )
FC_REFLECT( graphene::chain::asset_update_operation,
            (fee)
            (issuer)
            (asset_to_update)
            (new_issuer)
            (new_description)
            (new_feed_lifetime_sec)
            (new_minimum_feeds)
            (max_supply)
            (extensions)
          )

FC_REFLECT( graphene::chain::asset_publish_feed_operation,
            (fee)(publisher)(asset_id)(feed)(extensions) )
