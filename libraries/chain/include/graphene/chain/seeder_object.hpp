#pragma once
#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/protocol/asset.hpp>
#include <graphene/db/object.hpp>
#include <graphene/db/generic_index.hpp>

#include <fc/reflect/reflect.hpp>

#include <stdint.h>

namespace graphene { namespace chain {
   
   class seeder_object : public graphene::db::abstract_object<seeder_object>
   {
   public:
      static const uint8_t space_id = implementation_ids;
      static const uint8_t type_id  = impl_publisher_object_type;
      
      account_id_type seeder;
      uint64_t free_space;
      asset price;
      decent::crypto::d_integer_string pubKey;

   };
   
   struct by_seeder;
   struct by_free_space;
   struct by_price;
   
   typedef multi_index_container<
      seeder_object,
         indexed_by<
            ordered_unique< tag<by_id>,
               member< object, object_id_type, &object::id >
            >,
            ordered_unique< tag<by_seeder>,
               member<seeder_object, account_id_type, &seeder_object::seeder>
            >,
            ordered_non_unique< tag<by_free_space>,
               member<seeder_object, uint64_t, &seeder_object::free_space>
            >,
            ordered_non_unique< tag<by_price>,
               member<seeder_object, asset, &seeder_object::price>
            >
         >
   >seeder_object_multi_index_type;
   
   typedef generic_index< seeder_object, seeder_object_multi_index_type > seeder_index;
   
}}

FC_REFLECT_DERIVED(graphene::chain::seeder_object,
                   (graphene::db::object),
                   (seeder)(free_space)(price)(pubKey) )
