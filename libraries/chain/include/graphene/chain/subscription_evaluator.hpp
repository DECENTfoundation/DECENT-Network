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
   private:
      uint32_t period_count;
   };

   class subscribe_by_author_evaluator : public evaluator< subscribe_by_author_evaluator >
   {
   public:
      typedef subscribe_by_author_operation operation_type;

      void_result do_evaluate( const subscribe_by_author_operation& o );
      void_result do_apply( const subscribe_by_author_operation& o );
   private:
      uint32_t period_count;
   };

} } // namespace::chain