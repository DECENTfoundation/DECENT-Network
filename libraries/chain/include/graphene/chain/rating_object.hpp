#pragma once
#include <graphene/chain/protocol/types.hpp>
#include <graphene/db/object.hpp>
#include <graphene/db/generic_index.hpp>

#include <fc/reflect/reflect.hpp>

#include <boost/multi_index/composite_key.hpp>


namespace graphene { namespace chain {
   
   class rating_object : public graphene::db::abstract_object<rating_object>
   {
   public:
      static const uint8_t space_id = implementation_ids;
      static const uint8_t type_id  = impl_rating_object_type;
      
      account_id_type consumer;
      string URI;
      uint64_t rating;
      buying_id_type buying;
   };
   
   struct by_URI_consumer;
   struct by_consumer_URI;
   struct by_rating;
   
   typedef multi_index_container<
      rating_object,
         indexed_by<
            ordered_unique< tag<by_id>,
               member< object, object_id_type, &object::id >
            >,
            ordered_unique< tag< by_URI_consumer>,
               composite_key< rating_object,
                  member<rating_object, string, &rating_object::URI>,
                  member<rating_object, account_id_type, &rating_object::consumer>
               >
            >,
            ordered_unique< tag< by_consumer_URI>,
               composite_key< rating_object,
                  member<rating_object, account_id_type, &rating_object::consumer>,
                  member<rating_object, string, &rating_object::URI>
               >
            >
         >
   >rating_object_multi_index_type;
   
   
   typedef generic_index< rating_object, rating_object_multi_index_type > rating_index;

}}

FC_REFLECT_DERIVED(graphene::chain::rating_object,
                   (graphene::db::object),
                   (consumer)(URI)(rating)(buying) )
