/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
#pragma once

#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/protocol/subscription.hpp>

namespace graphene { namespace chain {

   class subscribe_evaluator : public evaluator<subscribe_operation, subscribe_evaluator>
   {
   public:
      operation_result do_evaluate( const operation_type& o );
      operation_result do_apply( const operation_type& o );
   };

   class subscribe_by_author_evaluator : public evaluator<subscribe_by_author_operation, subscribe_by_author_evaluator>
   {
   public:
      operation_result do_evaluate( const operation_type& o );
      operation_result do_apply( const operation_type& o );
   };

   class automatic_renewal_of_subscription_evaluator : public evaluator<automatic_renewal_of_subscription_operation, automatic_renewal_of_subscription_evaluator>
   {
   public:
      operation_result do_evaluate( const operation_type& o );
      operation_result do_apply( const operation_type& o );
   };

   using disallow_automatic_renewal_of_subscription_evaluator = virtual_evaluator_t<disallow_automatic_renewal_of_subscription_operation>;

   using renewal_of_subscription_evaluator = virtual_evaluator_t<renewal_of_subscription_operation>;

} } // namespace::chain
