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
      vector<account_id_type> seeders_answered;
      vector<decent::crypto::ciphertext> key_particles;
      d_integer pubKey;
      time_point_sec expiration_time;
   };

   class buying_history_object : public graphene::db::abstract_object<buying_history_object>
   {
   public:
      static const uint8_t space_id = implementation_ids;
      static const uint8_t type_id  = impl_buying_history_object_type;
      buying_id_type buying;
      account_id_type consumer;
      string URI;
      vector<account_id_type> seeders_answered;
      vector<decent::crypto::ciphertext> key_particles;
      bool delivered;
      time_point_sec time;
      bool rated = false;
   };

   struct by_URI_consumer;
   struct by_consumer_URI;
   struct by_expiration_time;
   
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
            >
         >
   >buying_object_multi_index_type;
   
   typedef generic_index< buying_object, buying_object_multi_index_type > buying_index;

   struct by_consumer_time;
   struct by_buying;

   typedef multi_index_container<
      buying_history_object,
         indexed_by<
             ordered_unique< tag<by_id>,
                member< object, object_id_type, &object::id >
             >,
             ordered_unique< tag<by_buying>,
                member< buying_history_object, buying_id_type, &buying_history_object::buying >
             >,  
             ordered_unique< tag< by_URI_consumer>,
                composite_key< buying_history_object,
                   member<buying_history_object, string, &buying_history_object::URI>,
                   member<buying_history_object, account_id_type, &buying_history_object::consumer>
                >
             >,
             ordered_unique< tag< by_consumer_URI>,
                composite_key< buying_history_object,
                   member<buying_history_object, account_id_type, &buying_history_object::consumer>,
                   member<buying_history_object, string, &buying_history_object::URI>
                >
             >,
             ordered_unique< tag< by_consumer_time>,
                composite_key< buying_history_object,
                   member<buying_history_object, account_id_type, &buying_history_object::consumer>,
                   member<buying_history_object, time_point_sec, &buying_history_object::time>
                >
             >
         >
   >buying_history_object_multi_index_type;

typedef generic_index< buying_history_object, buying_history_object_multi_index_type > buying_history_index;

}}

FC_REFLECT_DERIVED(graphene::chain::buying_object,
                   (graphene::db::object),
                   (consumer)(URI)(seeders_answered)(expiration_time)(pubKey)(key_particles) )

FC_REFLECT_DERIVED(graphene::chain::buying_history_object,
                   (graphene::db::object),
                   (consumer)(URI)(time)(delivered)(seeders_answered)(rated)(key_particles)(buying) )
