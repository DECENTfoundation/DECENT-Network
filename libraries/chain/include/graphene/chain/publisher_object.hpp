#pragma once
#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/protocol/asset.hpp>
#include <graphene/db/object.hpp>
#include <graphene/db/generic_index.hpp>

#include <fc/reflect/reflect.hpp>

#include <stdint.h>

namespace graphene { namespace chain {
   
   class publisher_object : public graphene::db::abstract_object<publisher_object>
   {
   public:
      static const uint8_t space_id = implementation_ids;
      static const uint8_t type_id  = impl_publisher_object_type;
      
      account_id_type publisher;
      uint64_t free_space;
      asset price;
   };
   
   struct by_publisher;
   struct by_free_space;
   struct by_price;
   
   typedef multi_index_container<
      publisher_object,
         indexed_by<
            ordered_unique< tag<by_id>,
               member< object, object_id_type, &object::id >
            >,
            ordered_unique< tag<by_publisher>,
               member<publisher_object, account_id_type, &publisher_object::publisher>
            >,
            ordered_non_unique< tag<by_free_space>,
               member<publisher_object, uint64_t, &publisher_object::free_space>
            >,
            ordered_non_unique< tag<by_price>,
               member<publisher_object, asset, &publisher_object::price>
            >
         >
   >publisher_object_multi_index_type;
   
   typedef generic_index< publisher_object, publisher_object_multi_index_type > publisher_index;
   
}}

FC_REFLECT_DERIVED(graphene::chain::publisher_object,
                   (graphene::db::object),
                   (publisher)(free_space)(price) )
