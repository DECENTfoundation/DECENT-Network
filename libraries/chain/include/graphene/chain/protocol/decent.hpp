#pragma once
#include <graphene/chain/protocol/base.hpp>
#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/protocol/asset.hpp>

#include <fc/reflect/reflect.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/time.hpp>

#include <stdint.h>
#include <vector>

#include <decent/encrypt/crypto_types.hpp>

namespace graphene { namespace chain {

using decent::crypto::delivery_proof;
using decent::crypto::ciphertext;

   struct content_submit_operation : public base_operation
   {
      struct fee_parameters_type { uint64_t fee = 0; };

      asset fee;
      account_id_type author;
      string URI;
      asset price;
      fc::ripemd160 hash;
      vector<account_id_type> seeders;
      fc::time_point_sec expiration;
      asset publishing_fee;
      string synopsis;
      
      account_id_type fee_payer()const { return author; }
   };
   
   struct request_to_buy_operation : public base_operation
   {
      struct fee_parameters_type { uint64_t fee = 0; };
      
      asset fee;
      string URI;
      account_id_type consumer;
      
      account_id_type fee_payer()const { return consumer; }
   };
   
   struct leave_rating_operation : public base_operation
   {
      struct fee_parameters_type { uint64_t fee = 0; };

      asset fee;
      string URI;
      account_id_type consumer;
      uint64_t rating;
      
      account_id_type fee_payer()const { return consumer; }
   };
   
   struct ready_to_publish_operation : public base_operation
   {
      struct fee_parameters_type { uint64_t fee = 0; };
      
      asset fee;
      account_id_type seeder;
      uint64_t space;
      uint32_t price_per_MByte;
      
      account_id_type fee_payer()const { return seeder; }
   };
   
   struct proof_of_custody_operation : public base_operation
   {
      struct fee_parameters_type { uint64_t fee = 0; };

      asset fee;
      account_id_type seeder;
      string URI;
      vector<char> proof;
      
      account_id_type fee_payer()const { return seeder; }
   };
   
   struct deliver_keys_operation : public base_operation
   {
      struct fee_parameters_type { uint64_t fee = 0; };

      asset fee;
      account_id_type seeder;
      delivery_proof proof;
      ciphertext key;
      
      account_id_type fee_payer()const { return seeder; }
   };
   
} } // graphene::chain

FC_REFLECT(graphene::chain::content_submit_operation,(fee)(author)(URI)(price)(hash)(seeders)(expiration)(publishing_fee)(synopsis))
FC_REFLECT(graphene::chain::request_to_buy_operation,(fee)(URI)(consumer))
FC_REFLECT(graphene::chain::leave_rating_operation,(fee)(URI)(consumer)(rating))
FC_REFLECT(graphene::chain::ready_to_publish_operation,(fee)(seeder)(space)(price_per_MByte))
FC_REFLECT(graphene::chain::proof_of_custody_operation,(fee)(seeder)(URI)(proof))
FC_REFLECT(graphene::chain::deliver_keys_operation,(fee)(seeder)(proof)(key))

FC_REFLECT( graphene::chain::content_submit_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::request_to_buy_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::leave_rating_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::ready_to_publish_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::proof_of_custody_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::deliver_keys_operation::fee_parameters_type, (fee) )
