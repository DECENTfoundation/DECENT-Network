#pragma once
#include <graphene/chain/protocol/types.hpp>
#include <graphene/db/object.hpp>
#include <graphene/chain/database.hpp>

#include <graphene/db/generic_index.hpp>
#include <decent/encrypt/crypto_types.hpp>

#include <fc/time.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/io/json.hpp>
#include <boost/multi_index/composite_key.hpp>


namespace graphene { namespace chain {

using decent::encrypt::DInteger;

   class buying_object : public graphene::db::abstract_object<buying_object>
   {
   public:
      
      void set_db(const graphene::chain::database*  obj_db) {
         db = obj_db;
      };

      
      const graphene::chain::database* db = nullptr;
      
      static const uint8_t space_id = implementation_ids;
      static const uint8_t type_id  = impl_buying_object_type;

      account_id_type consumer;
      string URI;
      
      
      uint64_t get_size() const {
         FC_ASSERT(db);
         const auto& idx = db->get_index_type<content_index>().indices().get<by_URI>();
         auto itr = idx.find(URI);
         if (itr != idx.end())
            return (*itr).size;
         
         return 0;
      }
      
      uint64_t get_rating() const {
         FC_ASSERT(db);
         const auto& idx = db->get_index_type<content_index>().indices().get<by_URI>();
         auto itr = idx.find(URI);
         if (itr != idx.end())
            return (*itr).AVG_rating;
         
         return 0;
      }

      
      uint64_t size;
      uint64_t rating;
      
      share_type get_price() const {
         FC_ASSERT(db);
         const auto& idx = db->get_index_type<content_index>().indices().get<by_URI>();
         auto itr = idx.find(URI);
         if (itr != idx.end())
            return (*itr).get_price_amount();
         
         return 0;
      }
      
      time_point_sec get_created_time() const {
         FC_ASSERT(db);
         const auto& idx = db->get_index_type<content_index>().indices().get<by_URI>();
         auto itr = idx.find(URI);
         if (itr != idx.end())
            return (*itr).created;
         
         return time_point();
      }
      


      
      asset price;
      std::string synopsis;

#ifdef PRICE_REGIONS
      uint32_t region_code_from;
#endif

      vector<account_id_type> seeders_answered;

      vector<decent::encrypt::CiphertextString> key_particles;
      DIntegerString pubKey;

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
   struct by_size;
   struct by_rating;
   struct by_price;
   struct by_created;

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
            >,
            ordered_non_unique< tag< by_size>,
                  const_mem_fun<buying_object, uint64_t, &buying_object::get_size>
            >,
            ordered_non_unique< tag< by_rating>,
                  member<buying_object, uint64_t, &buying_object::rating>
            >,
            ordered_non_unique< tag< by_price>,
                  const_mem_fun<buying_object, share_type, &buying_object::get_price>
            >,
            ordered_non_unique< tag< by_created>,
                  const_mem_fun<buying_object, time_point_sec, &buying_object::get_created_time>
            >
         >
   >buying_object_multi_index_type;
   
   typedef generic_index< buying_object, buying_object_multi_index_type > buying_index;


}}

#ifdef PRICE_REGIONS
FC_REFLECT_DERIVED(graphene::chain::buying_object,
                   (graphene::db::object),
                   (consumer)(URI)(synopsis)(region_code_from)(price)(seeders_answered)(size)(rating)(expiration_time)(pubKey)(key_particles)
                   (expired)(delivered)(expiration_or_delivery_time)(rated) )
#else
FC_REFLECT_DERIVED(graphene::chain::buying_object,
                   (graphene::db::object),
                   (consumer)(URI)(synopsis)(price)(seeders_answered)(size)(rating)(expiration_time)(pubKey)(key_particles)
                   (expired)(delivered)(expiration_or_delivery_time)(rated) )
#endif
