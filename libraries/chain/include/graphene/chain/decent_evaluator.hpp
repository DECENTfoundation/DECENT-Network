/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
#pragma once
#include <graphene/chain/evaluator.hpp>

namespace graphene { namespace chain {

   class set_publishing_manager_evaluator : public evaluator<set_publishing_manager_operation, set_publishing_manager_evaluator>
   {
   public:
      operation_result do_evaluate( const operation_type& o );
      operation_result do_apply( const operation_type& o );
   };

   class set_publishing_right_evaluator : public evaluator<set_publishing_right_operation, set_publishing_right_evaluator>
   {
   public:
      operation_result do_evaluate( const operation_type& o );
      operation_result do_apply( const operation_type& o );
   };

   class content_submit_evaluator : public evaluator<content_submit_operation, content_submit_evaluator>
   {
   public:
      operation_result do_evaluate( const operation_type& o );
      operation_result do_apply( const operation_type& o );

   private:
      bool is_resubmit = false;
   };

   class content_cancellation_evaluator : public evaluator<content_cancellation_operation, content_cancellation_evaluator>
   {
   public:
      operation_result do_evaluate( const operation_type& o );
      operation_result do_apply( const operation_type& o );
   };

   class request_to_buy_evaluator : public evaluator<request_to_buy_operation, request_to_buy_evaluator>
   {
   public:
      operation_result do_evaluate( const operation_type& o );
      operation_result do_apply( const operation_type& o );

   private:
      bool is_subscriber = false;
      asset paid_price;
      asset paid_price_after_conversion;
      asset content_price;
   };
   
   class leave_rating_evaluator : public evaluator<leave_rating_and_comment_operation, leave_rating_evaluator>
   {
   public:
      operation_result do_evaluate( const operation_type& o );
      operation_result do_apply( const operation_type& o );
   };
   
   class ready_to_publish_obsolete_evaluator : public evaluator<ready_to_publish_obsolete_operation, ready_to_publish_obsolete_evaluator>
   {
   public:
      operation_result do_evaluate( const operation_type& o );
      operation_result do_apply( const operation_type& o );
   };

   class ready_to_publish_evaluator : public evaluator<ready_to_publish_operation, ready_to_publish_evaluator>
   {
   public:
      operation_result do_evaluate( const operation_type& o );
      operation_result do_apply( const operation_type& o );
   private:
      const seeder_object* s_obj = nullptr;
   };

   class proof_of_custody_evaluator : public evaluator<proof_of_custody_operation, proof_of_custody_evaluator>
   {
   public:
      operation_result do_evaluate( const operation_type& o );
      operation_result do_apply( const operation_type& o );
   };
   
   class deliver_keys_evaluator : public evaluator<deliver_keys_operation, deliver_keys_evaluator>
   {
   public:
      operation_result do_evaluate( const operation_type& o );
      operation_result do_apply( const operation_type& o );
   };

   class report_stats_evaluator : public evaluator<report_stats_operation, report_stats_evaluator>
   {
   public:
      operation_result do_evaluate( const operation_type& o );
      operation_result do_apply( const operation_type& o );
   };

   using return_escrow_submission_evaluator = virtual_evaluator_t<return_escrow_submission_operation>;

   using return_escrow_buying_evaluator = virtual_evaluator_t<return_escrow_buying_operation>;

   using pay_seeder_evaluator = virtual_evaluator_t<pay_seeder_operation>;

   using finish_buying_evaluator = virtual_evaluator_t<finish_buying_operation>;

}}
