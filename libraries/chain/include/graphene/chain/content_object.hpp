#pragma once
#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/protocol/asset.hpp>
#include <graphene/db/object.hpp>
#include <graphene/db/generic_index.hpp>

#include <fc/crypto/ripemd160.hpp>
#include <fc/reflect/reflect.hpp>

#include <stdint.h>
#include <vector>

namespace graphene { namespace chain {
using namespace decent::encrypt;

   struct content_summary
   {
      string author;
      asset price;
      string synopsis;
      string URI;
      uint32_t AVG_rating;
      uint64_t size;
      time_point_sec expiration;
      time_point_sec created;
      uint32_t times_bought;


      content_summary& set( const content_object& co, const account_object& ao );
   };

   class content_object : public graphene::db::abstract_object<content_object>
   {
   public:
      static const uint8_t space_id = implementation_ids;
      static const uint8_t type_id  = impl_content_object_type;
      
      account_id_type author;
      time_point_sec expiration;
      time_point_sec created;
      asset price;
      string synopsis;
      uint64_t size;
      uint32_t quorum;
      string URI;
      map<account_id_type, CiphertextString> key_parts;
      map<account_id_type, time_point_sec> last_proof;

      fc::ripemd160 _hash;
      uint64_t AVG_rating;
      uint32_t total_rating;
      uint32_t times_bought;
      asset publishing_fee_escrow;
      decent::encrypt::CustodyData cd;

   };
   
   struct by_author;
   struct by_URI;
   struct by_AVG_rating;
   struct by_times_bought;
   struct by_expiration;
   
   
   typedef multi_index_container<
      content_object,
         indexed_by<
            ordered_unique< tag<by_id>,
               member< object, object_id_type, &object::id >
            >,
            ordered_non_unique<tag<by_author>,
               member<content_object, account_id_type, &content_object::author>
            >,
            ordered_unique<tag<by_URI>,
               member<content_object, string, &content_object::URI>
            >,
            ordered_non_unique<tag<by_AVG_rating>,
               member<content_object, uint64_t, &content_object::AVG_rating>
            >,
            ordered_non_unique<tag<by_times_bought>,
               member<content_object, uint32_t, &content_object::times_bought>,
               std::greater<uint32_t>>,
            ordered_non_unique<tag<by_expiration>,
               member<content_object, time_point_sec, &content_object::expiration>
            >
         >
   > content_object_multi_index_type;
   
   
   typedef generic_index< content_object, content_object_multi_index_type > content_index;
   
}}

FC_REFLECT_DERIVED(graphene::chain::content_object,
                   (graphene::db::object),
                   (author)(expiration)(created)(price)(size)(synopsis)
                   (URI)(quorum)(key_parts)(_hash)(last_proof)
                         (AVG_rating)(total_rating)(times_bought)(publishing_fee_escrow)(cd) )

FC_REFLECT( graphene::chain::content_summary, (author)(price)(synopsis)(URI)(AVG_rating)(size)(expiration)(created)(times_bought) )
