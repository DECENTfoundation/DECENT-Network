#pragma once

#include <graphene/chain/protocol/base.hpp>
#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/protocol/asset.hpp>

#include <fc/reflect/reflect.hpp>

namespace graphene { namespace chain {

   /**
    * @ingroup transactions
    * @brief Creates a subscription to author. This function is used by consumers..
    */
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

   /**
    * @ingroup transactions
    * @brief Creates a subscription to author. This function is used by author.
    */
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

   /**
    * @ingroup transactions
    * @brief This function is used to allow/disallow automatic renewal of expired subscription.
    */
   struct automatic_renewal_of_subscription_operation : public base_operation
      {
         struct fee_parameters_type { uint64_t fee = 0; };

         asset fee;

         account_id_type consumer;
         subscription_id_type subscription;
         bool automatic_renewal;

         account_id_type fee_payer()const { return consumer; }
         void            validate()const {};
      };

   /**
    * @ingroup transactions
    * @brief This virtual operation disallows automatic renewal of subscription if consumer doesn't have enought balance to renew
          * expired subscription
    */
   struct disallow_automatic_renewal_of_subscription_operation : public base_operation
   {
      struct fee_parameters_type { uint64_t fee = 0; };

      asset fee;
      account_id_type consumer;
      subscription_id_type subscription;

      account_id_type fee_payer()const { return consumer; }
      void            validate()const { FC_ASSERT( !"virtual operation" ); }
   };

   /**
    * @ingroup transactions
    * @brief This virtual operation is used to lengthen expired subscription
    */
   struct renewal_of_subscription_operation : public base_operation
   {
      struct fee_parameters_type { uint64_t fee = 0; };

      asset fee;
      account_id_type consumer;
      subscription_id_type subscription;

      account_id_type fee_payer()const { return consumer; }
      void            validate()const { FC_ASSERT( !"virtual operation" ); }
   };

} } // graphene::chain

FC_REFLECT(graphene::chain::subscribe_operation,(fee)(from)(to)(duration)(price))
FC_REFLECT(graphene::chain::subscribe_by_author_operation,(fee)(from)(to)(duration))
FC_REFLECT(graphene::chain::automatic_renewal_of_subscription_operation,(fee)(consumer)(subscription)(automatic_renewal))
FC_REFLECT(graphene::chain::disallow_automatic_renewal_of_subscription_operation,(fee)(consumer)(subscription))
FC_REFLECT(graphene::chain::renewal_of_subscription_operation,(fee)(consumer)(subscription))

FC_REFLECT( graphene::chain::subscribe_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::subscribe_by_author_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::automatic_renewal_of_subscription_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::disallow_automatic_renewal_of_subscription_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::renewal_of_subscription_operation::fee_parameters_type, (fee) )