#pragma once
#include <graphene/chain/evaluator.hpp>

// return type?

namespace graphene { namespace chain {
 
   class content_submit_evaluator : public evaluator<content_submit_evaluator>
   {
   public:
      typedef content_submit_operation operation_type;
      
      void_result do_evaluate( const content_submit_operation& o );
      void_result do_apply( const content_submit_operation& o );
   };
   
   class request_to_buy_evaluator : public evaluator<request_to_buy_evaluator>
   {
   public:
      typedef request_to_buy_operation operation_type;
      
      void_result do_evaluate( const request_to_buy_operation& o );
      void_result do_apply( const request_to_buy_operation& o );
   };
   
   class leave_rating_evaluator : public evaluator<leave_rating_evaluator>
   {
   public:
      typedef leave_rating_operation operation_type;
      
      void_result do_evaluate( const leave_rating_operation& o );
      void_result do_apply( const leave_rating_operation& o );
   };
   
   class ready_to_publish_evaluator : public evaluator<ready_to_publish_evaluator>
   {
   public:
      typedef ready_to_publish_operation operation_type;
      
      void_result do_evaluate( const ready_to_publish_operation& o );
      void_result do_apply( const ready_to_publish_operation& o );
   };
   
   class proof_of_custody_evaluator : public evaluator<proof_of_custody_evaluator>
   {
   public:
      typedef proof_of_custody_operation operation_type;
      
      void_result do_evaluate( const proof_of_custody_operation& o );
      void_result do_apply( const proof_of_custody_operation& o );
   };
   
   class deliver_keys_evaluator : public evaluator<deliver_keys_evaluator>
   {
   public:
      typedef deliver_keys_operation operation_type;
      
      void_result do_evaluate( const deliver_keys_operation& o );
      void_result do_apply( const deliver_keys_operation& o );
   };
   
}}
