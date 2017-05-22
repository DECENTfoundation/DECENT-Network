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
      static const uint8_t space_id = implementation_ids;
      static const uint8_t type_id  = impl_buying_object_type;

      account_id_type consumer;
      string URI;
      uint64_t size = uint64_t(-1); // initialized by content.size
      uint64_t rating = uint64_t(-1);  // this is the user rating
      uint64_t average_rating = uint64_t(-1);   // initialized by content_object.AVG_rating
      asset price;  // initialized by request_to_buy_operation.price then reset to 0 for escrow system and inflation calculations
      asset paid_price; // initialized by request_to_buy_operation.price
      std::string synopsis;   // initialized by content.synopsis
      vector<account_id_type> seeders_answered;
      vector<decent::encrypt::CiphertextString> key_particles;
      DIntegerString pubKey;
      time_point_sec expiration_time;
      bool expired = false;
      bool delivered = false;
      time_point_sec expiration_or_delivery_time;
      // User can't add rating and comment in two time-separated steps. For example, if content is already rated by user, he is not
      // allowed to add comment later. If user wants to add both rating and comment, he has to do it in one step.
      bool rated_or_commented = false;
      time_point_sec created; // initialized by content.created
      time_point_sec expiration; // initialized by content.expiration
#ifdef PRICE_REGIONS
      uint32_t region_code_from;
#endif

      bool is_open() const { return !( expired || delivered ); }
      share_type get_price() const { return paid_price.amount; }
   };


   struct by_URI_consumer;
   struct by_consumer_URI;
   struct by_expiration_time;
   struct by_consumer_time;
   struct by_URI_open;
   struct by_open_expiration;
   struct by_consumer_open;
   struct by_size;
   struct by_price;
   struct by_created;
   struct by_purchased;

   template <typename TAG, typename _t_object>
   struct key_extractor;

   template <>
   struct key_extractor<by_size, buying_object>
   {
      static uint64_t get(buying_object const& ob)
      {
         return ob.size;
      }
   };

   template <>
   struct key_extractor<by_price, buying_object>
   {
      static share_type get(buying_object const& ob)
      {
         return ob.get_price();
      }
   };

   template <>
   struct key_extractor<by_created, buying_object>
   {
      static time_point_sec get(buying_object const& ob)
      {
         return ob.created;
      }
   };
   
   template<>
   struct key_extractor<by_purchased, buying_object>
   {
      static time_point_sec get(buying_object const& ob)
      {
         return ob.expiration_or_delivery_time;
      }
   };

   template <>
   struct key_extractor<by_consumer_open, buying_object>
   {
      static boost::tuple<account_id_type, bool> get(buying_object const& ob)
      {
         return boost::make_tuple(ob.consumer, ob.is_open());
      }
   };

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
                  member<buying_object, uint64_t, &buying_object::size>
            >,
            ordered_non_unique< tag< by_price>,
                  const_mem_fun<buying_object, share_type, &buying_object::get_price>
            >,
            ordered_non_unique< tag< by_created>,
                  member<buying_object, time_point_sec, &buying_object::created>
            >,
            ordered_non_unique< tag< by_purchased>,
               member<buying_object, time_point_sec, &buying_object::expiration_or_delivery_time>
            >
         >
   >buying_object_multi_index_type;
   
   typedef generic_index< buying_object, buying_object_multi_index_type > buying_index;


}}

#ifdef PRICE_REGIONS
FC_REFLECT_DERIVED(graphene::chain::buying_object,
                   (graphene::db::object),
                   (consumer)(URI)(synopsis)(price)(paid_price)(seeders_answered)(size)(rating)(average_rating)(expiration_time)(pubKey)(key_particles)
                   (expired)(delivered)(expiration_or_delivery_time)(rated_or_commented)(created)(expiration)(region_code_from) )
#else
FC_REFLECT_DERIVED(graphene::chain::buying_object,
                   (graphene::db::object),
                   (consumer)(URI)(synopsis)(price)(paid_price)(seeders_answered)(size)(rating)(average_rating)(expiration_time)(pubKey)(key_particles)
                   (expired)(delivered)(expiration_or_delivery_time)(rated_or_commented)(created)(expiration) )
#endif
