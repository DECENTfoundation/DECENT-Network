/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
#pragma once
#include <graphene/chain/protocol/asset.hpp>
#include <graphene/db/object.hpp>
#include <graphene/db/generic_index.hpp>

#include <fc/reflect/reflect.hpp>

#include <stdint.h>

namespace graphene { namespace chain {

   class seeder_object : public graphene::db::abstract_object<implementation_ids, impl_publisher_object_type, seeder_object>
   {
   public:
      account_id_type seeder;
      uint64_t free_space;
      asset price;
      fc::time_point_sec expiration;
      decent::encrypt::DIntegerString pubKey;

      std::string ipfs_ID;
      // seeding stats used to compute seeder's rating
      seeding_statistics_id_type stats;
      // seeder's rating
      uint32_t rating = 0;
      // optional ISO 3166-1 alpha-2 two-letter region code
      std::string region_code;
   };

   struct by_seeder;
   struct by_free_space;
   struct by_price;
   struct by_expiration;
   struct by_region;
   struct by_rating;

   typedef multi_index_container<
      seeder_object,
         indexed_by<
            graphene::db::object_id_index,
            ordered_unique< tag<by_seeder>,
               member<seeder_object, account_id_type, &seeder_object::seeder>
            >,
            ordered_non_unique< tag<by_free_space>,
               member<seeder_object, uint64_t, &seeder_object::free_space>
            >,
            ordered_non_unique< tag<by_price>,
               member<seeder_object, asset, &seeder_object::price>
            >,
            ordered_non_unique< tag<by_expiration>,
               member<seeder_object, fc::time_point_sec, &seeder_object::expiration>
            >,
            ordered_non_unique< tag<by_region>,
               member<seeder_object, std::string, &seeder_object::region_code>
            >,
            ordered_non_unique< tag<by_rating>,
               member<seeder_object, uint32_t, &seeder_object::rating>,std::greater<uint32_t>
            >
         >
   >seeder_object_multi_index_type;

   typedef graphene::db::generic_index< seeder_object, seeder_object_multi_index_type > seeder_index;

}} // graphene::chain

FC_REFLECT_DERIVED(graphene::chain::seeder_object,
                   (graphene::db::object),
                   (seeder)(free_space)(price)(expiration)(pubKey)(ipfs_ID)(stats)(rating)(region_code) )
