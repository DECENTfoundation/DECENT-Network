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
#include <graphene/chain/protocol/config.hpp>
#include <graphene/chain/protocol/types.hpp>

namespace graphene { namespace chain {

   extern const int64_t scaled_precision_lut[];

   struct asset
   {
      asset( share_type a = 0, asset_id_type id = asset_id_type() )
      :amount(a),asset_id(id){}

      share_type    amount;
      asset_id_type asset_id;

      asset& operator += ( const asset& o )
      {
         FC_ASSERT( asset_id == o.asset_id );
         amount += o.amount;
         return *this;
      }
      asset& operator -= ( const asset& o )
      {
         FC_ASSERT( asset_id == o.asset_id );
         amount -= o.amount;
         return *this;
      }
      asset operator -()const { return asset( -amount, asset_id ); }

      friend bool operator == ( const asset& a, const asset& b )
      {
         return std::tie( a.asset_id, a.amount ) == std::tie( b.asset_id, b.amount );
      }
      friend bool operator < ( const asset& a, const asset& b )
      {
         FC_ASSERT( a.asset_id == b.asset_id );
         return a.amount < b.amount;
      }
      friend bool operator <= ( const asset& a, const asset& b )
      {
         return (a == b) || (a < b);
      }

      friend bool operator != ( const asset& a, const asset& b )
      {
         return !(a == b);
      }
      friend bool operator > ( const asset& a, const asset& b )
      {
         return !(a <= b);
      }
      friend bool operator >= ( const asset& a, const asset& b )
      {
         return !(a < b);
      }

      friend asset operator - ( const asset& a, const asset& b )
      {
         FC_ASSERT( a.asset_id == b.asset_id );
         return asset( a.amount - b.amount, a.asset_id );
      }
      friend asset operator + ( const asset& a, const asset& b )
      {
         FC_ASSERT( a.asset_id == b.asset_id );
         return asset( a.amount + b.amount, a.asset_id );
      }
      friend asset operator * ( const uint32_t a, const asset& b )
      {
         return asset( b.amount * a, b.asset_id );
      }
      static share_type scaled_precision( uint8_t precision )
      {
         FC_ASSERT( precision < 19 );
         return scaled_precision_lut[ precision ];
      }
   };

   /**
    * @brief The price struct stores asset prices in the Graphene system.
    *
    * A price is defined as a ratio between two assets, and represents a possible exchange rate between those two
    * assets. prices are generally not stored in any simplified form, i.e. a price of (1000 CORE)/(20 USD) is perfectly
    * normal.
    *
    * The assets within a price are labeled base and quote. Throughout the Graphene code base, the convention used is
    * that the base asset is the asset being sold, and the quote asset is the asset being purchased, where the price is
    * represented as base/quote, so in the example price above the seller is looking to sell CORE asset and get USD in
    * return.
    */
   struct price
   {
      price(const asset& base = asset(), const asset quote = asset())
         : base(base),quote(quote){}

      asset base;
      asset quote;

      static price max(asset_id_type base, asset_id_type quote );
      static price min(asset_id_type base, asset_id_type quote );

      static price call_price(const asset& debt, const asset& collateral, uint16_t collateral_ratio);

      /// The unit price for an asset type A is defined to be a price such that for any asset m, m*A=m
      static price unit_price(asset_id_type a = asset_id_type()) { return price(asset(1, a), asset(1, a)); }

      price max()const { return price::max( base.asset_id, quote.asset_id ); }
      price min()const { return price::min( base.asset_id, quote.asset_id ); }

      double to_real()const { return double(base.amount.value)/double(quote.amount.value); }

      bool is_null()const;
      void validate()const;
   };

   price operator / ( const asset& base, const asset& quote );
   inline price operator~( const price& p ) { return price{p.quote,p.base}; }

   bool  operator <  ( const asset& a, const asset& b );
   bool  operator <= ( const asset& a, const asset& b );
   bool  operator <  ( const price& a, const price& b );
   bool  operator <= ( const price& a, const price& b );
   bool  operator >  ( const price& a, const price& b );
   bool  operator >= ( const price& a, const price& b );
   bool  operator == ( const price& a, const price& b );
   bool  operator != ( const price& a, const price& b );
   asset operator *  ( const asset& a, const price& b );

   /**
    *  @class price_feed
    *  @brief defines market parameters for margin positions
    */
   struct price_feed
   {
      /**
       *  Required maintenance collateral is defined
       *  as a fixed point number with a maximum value of 10.000
       *  and a minimum value of 1.000.  (denominated in GRAPHENE_COLLATERAL_RATIO_DENOM)
       *
       *  A black swan event occurs when value_of_collateral equals
       *  value_of_debt, to avoid a black swan a margin call is
       *  executed when value_of_debt * required_maintenance_collateral
       *  equals value_of_collateral using rate.
       *
       *  Default requirement is $1.75 of collateral per $1 of debt
       *
       *  BlackSwan ---> SQR ---> MCR ----> SP
       */
      ///@{

      /// Price at which automatically exchanging this asset for CORE from fee pool occurs (used for paying fees)
      price core_exchange_rate;

      void validate() const { FC_ASSERT( core_exchange_rate.quote.asset_id == asset_id_type(0) );};
      bool is_for( asset_id_type asset_id ) const;
   };

} }

FC_REFLECT( graphene::chain::asset, (amount)(asset_id) )
FC_REFLECT( graphene::chain::price, (base)(quote) )

#define GRAPHENE_PRICE_FEED_FIELDS (core_exchange_rate)

FC_REFLECT( graphene::chain::price_feed, GRAPHENE_PRICE_FEED_FIELDS )
