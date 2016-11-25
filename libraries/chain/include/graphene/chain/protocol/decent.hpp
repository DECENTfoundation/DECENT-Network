#pragma once
#include <graphene/chain/protocol/base.hpp>
#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/protocol/asset.hpp>

#include <fc/reflect/reflect.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/time.hpp>

#include <stdint.h>
#include <vector>

namespace graphene { namespace chain {

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
      public_key_type key;
      
      account_id_type fee_payer()const { return seeder; }
   };
   
} } // graphene::chain

FC_REFLECT(graphene::chain::content_submit_operation,(author)(URI)(price)(hash)(seeders)(expiration)(publishing_fee)(synopsis))
FC_REFLECT(graphene::chain::request_to_buy_operation,(URI)(consumer))
FC_REFLECT(graphene::chain::leave_rating_operation,(URI)(consumer)(rating))
FC_REFLECT(graphene::chain::ready_to_publish_operation,(seeder)(space)(price_per_MByte))
FC_REFLECT(graphene::chain::proof_of_custody_operation,(seeder)(URI)(proof))
FC_REFLECT(graphene::chain::deliver_keys_operation,(seeder)(key))

FC_REFLECT( graphene::chain::content_submit_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::request_to_buy_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::leave_rating_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::ready_to_publish_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::proof_of_custody_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::deliver_keys_operation::fee_parameters_type, (fee) )
