/* (c) 2019 DECENT Services. For details refers to LICENSE.txt */
#pragma once
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/protocol/non_fungible_token.hpp>

namespace graphene { namespace chain {

   class non_fungible_token_create_definition_evaluator : public evaluator<non_fungible_token_create_definition_operation, non_fungible_token_create_definition_evaluator>
   {
      public:
         void_result do_evaluate( const operation_type& op );
         graphene::db::object_id_type do_apply( const operation_type& op );
   };

   class non_fungible_token_update_definition_evaluator : public evaluator<non_fungible_token_update_definition_operation, non_fungible_token_update_definition_evaluator>
   {
      public:
         void_result do_evaluate( const operation_type& op );
         graphene::db::object_id_type do_apply( const operation_type& op );

      private:
         const non_fungible_token_object* nft_to_update = nullptr;
   };

   class non_fungible_token_issue_evaluator : public evaluator<non_fungible_token_issue_operation, non_fungible_token_issue_evaluator>
   {
      public:
         void_result do_evaluate( const operation_type& op );
         graphene::db::object_id_type do_apply( const operation_type& op );

      private:
         const non_fungible_token_object* nft_to_update = nullptr;
   };

   class non_fungible_token_transfer_evaluator : public evaluator<non_fungible_token_transfer_operation, non_fungible_token_transfer_evaluator>
   {
      public:
         void_result do_evaluate( const operation_type& op );
         void_result do_apply( const operation_type& op );

      private:
         const non_fungible_token_data_object* nft_data_to_update = nullptr;
   };

   class non_fungible_token_update_data_evaluator : public evaluator<non_fungible_token_update_data_operation, non_fungible_token_update_data_evaluator>
   {
      public:
         void_result do_evaluate( const operation_type& op );
         void_result do_apply( const operation_type& op );

      private:
         const non_fungible_token_data_object* nft_data_to_update = nullptr;
   };

} } // graphene::chain
