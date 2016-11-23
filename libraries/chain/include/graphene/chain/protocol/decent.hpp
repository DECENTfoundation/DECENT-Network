#pragma once
#include <graphene/chain/protocol/base.hpp>
#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/protocol/asset.hpp>

#include <fc/reflect.hpp>
#include <fc/string.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/io/json.hpp>
#include <fc/time.hpp>

#include <stdint.h>
#include <vector>

namespace graphene { namespace chain {

   struct content_submit_operation : public base_operation
   {
      account_id_type author;
      fc::string URI;
      asset price;
      fc::ripemd160 hash;
      vector<account_id_type> seeders;
      fc::time_point_sec expiration;
      asset publishing_fee;
      string synopsis;
   };
   
   struct request_to_buy_operation : public base_operation
   {
      fc::string URI;
      account_id_type consumer;
   };
   
   struct leave_rating_operation : public base_operation
   {
      fc::string URI;
      account_id_type consumer;
      uint64_t rating;
   };
   
   struct ready_to_publish_operation : public base_operation
   {
      account_id_type seeder;
      uint64_t space;
      uint32_t price_per_MByte;
   };
   
   struct proof_of_custody_operation : public base_operation
   {
      account_id_type seeder;
      fc::string URI;
      vector<char> proof;
   };
   
   struct deliver_keys_operation : public base_operation
   {
      account_id_type seeder;
      public_key_type key;
   };
   
} } // graphene::chain

FC_REFLECT(graphene::chain::content_submit_operation,(author)(URI)(price)(hash)(seeders)(expiration)(publishing_fee)(synopsis))
FC_REFLECT(graphene::chain::request_to_buy_operation,(URI)(consumer))
FC_REFLECT(graphene::chain::leave_rating_operation,(URI)(consumer))
FC_REFLECT(graphene::chain::ready_to_publish_operation,(seeder)(space)(price_per_MByte))
FC_REFLECT(graphene::chain::proof_of_custody_operation,(seeder)(URI)(proof))
FC_REFLECT(graphene::chain::deliver_keys_operation,(seeder)(key))
