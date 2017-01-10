#pragma once

#include <graphene/chain/protocol/base.hpp>
#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/protocol/asset.hpp>

#include <fc/reflect/reflect.hpp>

namespace graphene { namespace chain {

   struct subscribe_operation : public base_operation
   {
      struct fee_parameters_type { uint64_t fee = 0; };

      asset fee;

      account_id_type from;
      account_id_type to;
      uint32_t duration;
      asset price;

      account_id_type fee_payer()const { return from; }
      void            validate()const;
   };

   struct subscribe_by_author_operation : public base_operation
   {
      struct fee_parameters_type { uint64_t fee = 0; };

      asset fee;

      account_id_type from;
      account_id_type to;
      uint32_t duration;

      account_id_type fee_payer()const { return from; }
      void            validate()const;
   };

   struct allow_subscription_operation : public base_operation
   {
      struct fee_parameters_type { uint64_t fee = 0; };

      asset fee;

      account_id_type account;
      bool allow_subscription;
      uint32_t duration;
      asset price;

      account_id_type fee_payer()const { return account; }
      void            validate()const;
   };

} } // graphene::chain

FC_REFLECT(graphene::chain::subscribe_operation,(fee)(from)(to)(duration)(price))
FC_REFLECT(graphene::chain::subscribe_by_author_operation,(fee)(from)(to)(duration))
FC_REFLECT(graphene::chain::allow_subscription_operation,(fee)(allow_subscription)(duration)(price))

FC_REFLECT( graphene::chain::subscribe_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::subscribe_by_author_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::allow_subscription_operation::fee_parameters_type, (fee) )