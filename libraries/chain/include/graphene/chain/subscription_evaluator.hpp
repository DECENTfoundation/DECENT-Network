#pragma once

#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/subscription_object.hpp>

namespace graphene { namespace chain {

   class subscribe_evaluator : public evaluator< subscribe_evaluator >
   {
   public:
      typedef subscribe_operation operation_type;

      void_result do_evaluate( const subscribe_operation& o );
      void_result do_apply( const subscribe_operation& o );
   };

   class subscribe_by_author_evaluator : public evaluator< subscribe_by_author_evaluator >
   {
   public:
      typedef subscribe_by_author_operation operation_type;

      void_result do_evaluate( const subscribe_by_author_operation& o );
      void_result do_apply( const subscribe_by_author_operation& o );
   };

   class automatic_renewal_of_subscription_evaluator : public evaluator<automatic_renewal_of_subscription_evaluator>
   {
   public:
      typedef automatic_renewal_of_subscription_operation operation_type;

      void_result do_evaluate( const automatic_renewal_of_subscription_operation& o );
      void_result do_apply( const automatic_renewal_of_subscription_operation& o );
   };

   class disallow_automatic_renewal_of_subscription_evaluator : public evaluator<disallow_automatic_renewal_of_subscription_evaluator>
   {
   public:
      typedef disallow_automatic_renewal_of_subscription_operation operation_type;

      void_result do_evaluate( const disallow_automatic_renewal_of_subscription_operation& o );
      void_result do_apply( const disallow_automatic_renewal_of_subscription_operation& o );
   };

   class renewal_of_subscription_evaluator : public evaluator<renewal_of_subscription_evaluator>
   {
   public:
      typedef renewal_of_subscription_operation operation_type;

      void_result do_evaluate( const renewal_of_subscription_operation& o );
      void_result do_apply( const renewal_of_subscription_operation& o );
   };

} } // namespace::chain