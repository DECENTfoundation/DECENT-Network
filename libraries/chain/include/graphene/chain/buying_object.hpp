#pragma once
#include <graphene/chain/protocol/types.hpp>
#include <graphene/db/object.hpp>
#include <graphene/db/generic_index.hpp>
#include <decent/encrypt/crypto_types.hpp>

#include <fc/time.hpp>
#include <fc/reflect/reflect.hpp>

#include <boost/multi_index/composite_key.hpp>

namespace graphene { namespace chain {
using decent::crypto::d_integer;

   class buying_object : public graphene::db::abstract_object<buying_object>
   {
   public:
      static const uint8_t space_id = implementation_ids;
      static const uint8_t type_id  = impl_buying_object_type;

      account_id_type consumer;
      string URI;
      
      uint64_t size;
      uint64_t rating;
       
      asset price;
      std::string synopsis;
       
      vector<account_id_type> seeders_answered;
      vector<decent::crypto::ciphertext_string> key_particles;
      d_integer_string pubKey;
      time_point_sec expiration_time;

      bool expired = false;
      bool delivered = false;
      bool is_open()const { return !( expired || delivered ); }
      time_point_sec expiration_or_delivery_time;
      bool rated = false;
   };


   struct by_URI_consumer;
   struct by_consumer_URI;
   struct by_expiration_time;
   struct by_consumer_time;
   struct by_URI_open;
   struct by_open_expiration;
   struct by_consumer_open;
   
   typedef multi_index_container<
      buying_object,
         indexed_by<
            ordered_unique< tag<by_id>,
               member< object, object_id_type, &object::id >
            >,
            ordered_unique< tag< by_URI_consumer>,
               composite_key< buying_object,
                  member<buying_object, string, &buying_object::URI>,
                  member<buying_object, account_id_type, &buying_object::consumer>
               >
            >,
            ordered_unique< tag< by_consumer_URI>,
               composite_key< buying_object,
                  member<buying_object, account_id_type, &buying_object::consumer>,
                  member<buying_object, string, &buying_object::URI>
               >
            >,
            ordered_non_unique<tag<by_expiration_time>,
               member<buying_object, time_point_sec, &buying_object::expiration_time>
            >,
            ordered_non_unique< tag< by_consumer_time>,
               composite_key< buying_object,
                  member<buying_object, account_id_type, &buying_object::consumer>,
                  member<buying_object, time_point_sec, &buying_object::expiration_or_delivery_time>
               >
            >,
            ordered_non_unique< tag< by_URI_open>,
               composite_key< buying_object,
                  member<buying_object, string, &buying_object::URI>,
                  const_mem_fun<buying_object, bool, &buying_object::is_open>
               >
            >,
            ordered_non_unique< tag< by_open_expiration>,
               composite_key< buying_object,
                  const_mem_fun<buying_object, bool, &buying_object::is_open>,
                  member<buying_object, time_point_sec, &buying_object::expiration_time>
               >
            >,
            ordered_non_unique< tag< by_consumer_open>,
               composite_key< buying_object,
                  member<buying_object, account_id_type, &buying_object::consumer>,
                  const_mem_fun<buying_object, bool, &buying_object::is_open>
               >
            >
         >
   >buying_object_multi_index_type;
   
   typedef generic_index< buying_object, buying_object_multi_index_type > buying_index;

}}

FC_REFLECT_DERIVED(graphene::chain::buying_object,
                   (graphene::db::object),
                   (consumer)(URI)(synopsis)(price)(seeders_answered)(size)(rating)(expiration_time)(pubKey)(key_particles)
                   (expired)(delivered)(expiration_or_delivery_time)(rated) )
