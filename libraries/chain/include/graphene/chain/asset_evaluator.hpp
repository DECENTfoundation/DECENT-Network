/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once
#include <graphene/chain/evaluator.hpp>

namespace graphene { namespace chain {

   class asset_create_evaluator : public evaluator<asset_create_operation, asset_create_evaluator>
   {
      public:
         void_result do_evaluate( const operation_type& o );
         graphene::db::object_id_type do_apply( const operation_type& o );
   };

   class asset_issue_evaluator : public evaluator<asset_issue_operation, asset_issue_evaluator>
   {
   public:
      void_result do_evaluate( const operation_type& o );
      void_result do_apply( const operation_type& o );

   private:
      const asset_dynamic_data_object* asset_dyn_data = nullptr;
   };

   class monitored_asset_update_evaluator : public evaluator<update_monitored_asset_operation, monitored_asset_update_evaluator>
   {
      public:
         void_result do_evaluate( const operation_type& o );
         void_result do_apply( const operation_type& o );

      private:
         const asset_object* asset_to_update = nullptr;
   };

   class user_issued_asset_update_evaluator : public evaluator<update_user_issued_asset_operation, user_issued_asset_update_evaluator>
   {
   public:
      void_result do_evaluate( const operation_type& o );
      void_result do_apply( const operation_type& o );

   private:
      const asset_object* asset_to_update = nullptr;
   };

   class asset_fund_pools_evaluator : public evaluator<asset_fund_pools_operation, asset_fund_pools_evaluator>
   {
   public:
      void_result do_evaluate(const operation_type& op);
      void_result do_apply(const operation_type& op);

   private:
      const asset_dynamic_data_object* asset_dyn_data = nullptr;
   };

   class asset_reserve_evaluator : public evaluator<asset_reserve_operation, asset_reserve_evaluator>
   {
   public:
      void_result do_evaluate( const operation_type& o );
      void_result do_apply( const operation_type& o );

   private:
      const asset_dynamic_data_object* asset_dyn_data = nullptr;
   };

   class asset_claim_fees_evaluator : public evaluator<asset_claim_fees_operation, asset_claim_fees_evaluator>
   {
   public:
      void_result do_evaluate( const operation_type& o );
      void_result do_apply( const operation_type& o );

   private:
      const asset_dynamic_data_object* asset_dyn_data = nullptr;
   };

   class asset_publish_feeds_evaluator : public evaluator<asset_publish_feed_operation, asset_publish_feeds_evaluator>
   {
   public:
      void_result do_evaluate( const operation_type& o );
      void_result do_apply( const operation_type& o );

   private:
      std::map<std::pair<asset_id_type,asset_id_type>,price_feed> median_feed_values;
   };

   class update_user_issued_asset_advanced_evaluator : public evaluator<update_user_issued_asset_advanced_operation, update_user_issued_asset_advanced_evaluator>
   {
   public:
      void_result do_evaluate( const operation_type& o );
      void_result do_apply( const operation_type& o );

   private:
      const asset_object* asset_to_update = nullptr;
      bool count_fixed_max_supply_ext = false;
      bool set_precision = false;
   };

} } // graphene::chain
