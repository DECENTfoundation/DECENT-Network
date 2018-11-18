/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
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
      asset price;

      account_id_type fee_payer()const { return from; }
      void            validate()const;

       optional<guarantee_object_id_type> guarantee_id;
       optional<guarantee_object_id_type> get_guarantee_id()const { return guarantee_id; }
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

      account_id_type fee_payer()const { return from; }
      void            validate()const;

       optional<guarantee_object_id_type> guarantee_id;
       optional<guarantee_object_id_type> get_guarantee_id()const { return guarantee_id; }
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

       optional<guarantee_object_id_type> guarantee_id;
       optional<guarantee_object_id_type> get_guarantee_id()const { return guarantee_id; }
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

       optional<guarantee_object_id_type> guarantee_id;
       optional<guarantee_object_id_type> get_guarantee_id()const { return guarantee_id; }
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

       optional<guarantee_object_id_type> guarantee_id;
       optional<guarantee_object_id_type> get_guarantee_id()const { return guarantee_id; }
   };

} } // graphene::chain

FC_REFLECT(graphene::chain::subscribe_operation,(fee)(from)(to)(price))
FC_REFLECT(graphene::chain::subscribe_by_author_operation,(fee)(from)(to))
FC_REFLECT(graphene::chain::automatic_renewal_of_subscription_operation,(fee)(consumer)(subscription)(automatic_renewal))
FC_REFLECT(graphene::chain::disallow_automatic_renewal_of_subscription_operation,(fee)(consumer)(subscription))
FC_REFLECT(graphene::chain::renewal_of_subscription_operation,(fee)(consumer)(subscription))

FC_REFLECT( graphene::chain::subscribe_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::subscribe_by_author_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::automatic_renewal_of_subscription_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::disallow_automatic_renewal_of_subscription_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::renewal_of_subscription_operation::fee_parameters_type, (fee) )
