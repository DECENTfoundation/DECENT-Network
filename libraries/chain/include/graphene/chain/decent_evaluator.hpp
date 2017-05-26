#pragma once
#include <graphene/chain/evaluator.hpp>
#include <decent/encrypt/custodyutils.hpp>
// return type?

namespace graphene { namespace chain {

   static decent::encrypt::CustodyUtils _custody_utils;

   class content_submit_evaluator : public evaluator<content_submit_evaluator>
   {
   public:
      typedef content_submit_operation operation_type;

      bool is_resubmit = false;

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
      typedef leave_rating_and_comment_operation operation_type;
      
      void_result do_evaluate( const leave_rating_and_comment_operation& o );
      void_result do_apply( const leave_rating_and_comment_operation& o );
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
   private:
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

   class return_escrow_submission_evaluator : public evaluator<return_escrow_submission_evaluator>
   {
   public:
      typedef return_escrow_submission_operation operation_type;

      void_result do_evaluate( const return_escrow_submission_operation& o );
      void_result do_apply( const return_escrow_submission_operation& o );
   };

   class return_escrow_buying_evaluator : public evaluator<return_escrow_buying_evaluator>
   {
   public:
      typedef return_escrow_buying_operation operation_type;

      void_result do_evaluate( const return_escrow_buying_operation& o );
      void_result do_apply( const return_escrow_buying_operation& o );
   };

   class report_stats_evaluator : public evaluator<report_stats_evaluator>
   {
   public:
      typedef report_stats_operation operation_type;

      void_result do_evaluate( const report_stats_operation& o );
      void_result do_apply( const report_stats_operation& o );
   };

   class pay_seeder_evaluator : public evaluator<pay_seeder_evaluator>
   {
   public:
      typedef pay_seeder_operation operation_type;

      void_result do_evaluate( const pay_seeder_operation& o );
      void_result do_apply( const pay_seeder_operation& o );
   };

   class finish_buying_evaluator : public evaluator<finish_buying_evaluator>
   {
   public:
      typedef finish_buying_operation operation_type;

      void_result do_evaluate( const finish_buying_operation& o );
      void_result do_apply( const finish_buying_operation& o );
   };

}}
