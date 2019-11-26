/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
#pragma once
#include <graphene/chain/protocol/decent.hpp>

#include <graphene/db/object.hpp>
#include <graphene/db/generic_index.hpp>

namespace graphene { namespace chain {

   struct PriceRegions
   {
      std::map<uint32_t, asset> map_price;

      fc::optional<asset> GetPrice(uint32_t region_code) const;
      void ClearPrices();
      void SetSimplePrice(asset const& price);
      void SetRegionPrice(uint32_t region_code, asset const& price);
      bool Valid(uint32_t region_code) const;
      bool Valid(const std::string& region_code) const;
   };

   class content_object : public graphene::db::abstract_object<implementation_ids, impl_content_object_type, content_object>
   {
   public:
      account_id_type author;
      // If co_authors map is not empty, payout will be splitted.
      // Maps co-authors to split based on basis points.
      std::map<account_id_type, uint32_t> co_authors;

      fc::time_point_sec expiration;
      fc::time_point_sec created;
      PriceRegions price;

      std::string synopsis;
      uint64_t size;
      uint32_t quorum;
      std::string URI;
      std::map<account_id_type, decent::encrypt::CiphertextString> key_parts;
      std::map<account_id_type, fc::time_point_sec> last_proof;
      std::map<account_id_type, share_type> seeder_price;
      bool is_blocked = false;

      fc::ripemd160 _hash;
      uint32_t AVG_rating = 0;
      uint32_t num_of_ratings = 0;
      uint32_t times_bought = 0;
      asset publishing_fee_escrow;
      fc::optional<decent::encrypt::CustodyData> cd;

      template <RegionCodes::RegionCode code>
      share_type get_price_amount_template() const
      {
         // this function is used only by db index
         // later will avoid using index for prices
         // but anyway, make sure the index will not throw
         // for regions not having a price
         // use fallback to any other region price that is defined
         // (at least one price will be defined)
         // corresponding code that uses the index "by_price" will check
         // for such wrongly placed content items and skip those
         fc::optional<asset> region_price = price.GetPrice(code);
         if (false == region_price.valid())
         {
            FC_ASSERT(false == price.map_price.empty());
            auto it_any_price = price.map_price.begin();
            region_price = it_any_price->second;
         }

         return region_price->amount;
      }

      bool recent_proof( uint64_t validity_seconds ) const {
         auto  now = fc::time_point::now();
         for( auto i: last_proof ){
            if( i.second > now - fc::seconds(validity_seconds) )
               return true;
         }
         return false;
      }
   };

   struct by_author;
   struct by_URI;
   struct by_AVG_rating;
   struct by_size;
   struct by_price;
   struct by_times_bought;
   struct by_expiration;
   struct by_created;

   template <typename TAG, typename _t_object>
   struct key_extractor;

   template <>
   struct key_extractor<by_author, content_object>
   {
      static account_id_type get(content_object const& ob)
      {
         return ob.author;
      }
   };

   template <>
   struct key_extractor<by_URI, content_object>
   {
      static std::string get(content_object const& ob)
      {
         return ob.URI;
      }
   };

   template <>
   struct key_extractor<by_AVG_rating, content_object>
   {
      static uint32_t get(content_object const& ob)
      {
         return ob.AVG_rating;
      }
   };

   template <>
   struct key_extractor<by_size, content_object>
   {
      static uint64_t get(content_object const& ob)
      {
         return ob.size;
      }
   };

   template <>
   struct key_extractor<by_price, content_object>
   {
      static share_type get(content_object const& ob)
      {
         return ob.get_price_amount_template<RegionCodes::OO_none>();
      }
   };

   template <>
   struct key_extractor<by_expiration, content_object>
   {
      static fc::time_point_sec get(content_object const& ob)
      {
         return ob.expiration;
      }
   };

   template <>
   struct key_extractor<by_created, content_object>
   {
      static fc::time_point_sec get(content_object const& ob)
      {
         return ob.created;
      }
   };

   typedef boost::multi_index_container<
      content_object,
      db::mi::indexed_by<
         db::object_id_index,
         db::mi::ordered_non_unique<db::mi::tag<by_author>,
            db::mi::member<content_object, account_id_type, &content_object::author>
         >,
         db::mi::ordered_unique<db::mi::tag<by_URI>,
            db::mi::member<content_object, std::string, &content_object::URI>
         >,
         db::mi::ordered_non_unique<db::mi::tag<by_price>,
            db::mi::const_mem_fun<content_object, share_type, &content_object::get_price_amount_template<RegionCodes::OO_none>>
         >,
         db::mi::ordered_non_unique<db::mi::tag<by_size>,
            db::mi::member<content_object, uint64_t, &content_object::size>
         >,
         db::mi::ordered_non_unique<db::mi::tag<by_AVG_rating>,
            db::mi::member<content_object, uint32_t, &content_object::AVG_rating>
         >,
         db::mi::ordered_non_unique<db::mi::tag<by_times_bought>,
            db::mi::member<content_object, uint32_t, &content_object::times_bought>, std::greater<uint32_t>
         >,
         db::mi::ordered_non_unique<db::mi::tag<by_expiration>,
               db::mi::member<content_object, fc::time_point_sec, &content_object::expiration>
         >,
         db::mi::ordered_non_unique<db::mi::tag<by_created>,
            db::mi::member<content_object, fc::time_point_sec, &content_object::created>
         >
      >
   > content_object_multi_index_type;

   typedef graphene::db::generic_index< content_object, content_object_multi_index_type > content_index;

}}

FC_REFLECT_DERIVED(graphene::chain::content_object,
                   (graphene::db::object),
                   (author)(co_authors)(expiration)(created)(price)(size)(synopsis)
                   (URI)(quorum)(key_parts)(_hash)(last_proof)(is_blocked)
                   (AVG_rating)(num_of_ratings)(times_bought)(publishing_fee_escrow)(cd)(seeder_price) )

FC_REFLECT( graphene::chain::PriceRegions, (map_price) )
