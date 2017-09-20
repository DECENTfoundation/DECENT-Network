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
#include <graphene/chain/protocol/operations.hpp>
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/database.hpp>

namespace graphene { namespace chain {

   class asset_create_evaluator : public evaluator<asset_create_evaluator>
   {
      public:
         typedef asset_create_operation operation_type;

         void_result do_evaluate( const asset_create_operation& o );
         object_id_type do_apply( const asset_create_operation& o );
   };

   class asset_issue_evaluator : public evaluator<asset_issue_evaluator>
   {
   public:
      typedef asset_issue_operation operation_type;
      void_result do_evaluate( const asset_issue_operation& o );
      void_result do_apply( const asset_issue_operation& o );

      const asset_dynamic_data_object* asset_dyn_data = nullptr;
   };

   class monitored_asset_update_evaluator : public evaluator<monitored_asset_update_evaluator>
   {
      public:
         typedef update_monitored_asset_operation operation_type;

         void_result do_evaluate( const update_monitored_asset_operation& o );
         void_result do_apply( const update_monitored_asset_operation& o );

         const asset_object* asset_to_update = nullptr;
   };

   class user_issued_asset_update_evaluator : public evaluator<user_issued_asset_update_evaluator>
   {
   public:
      typedef update_user_issued_asset_operation operation_type;

      void_result do_evaluate( const update_user_issued_asset_operation& o );
      void_result do_apply( const update_user_issued_asset_operation& o );

      const asset_object* asset_to_update = nullptr;
   };

   class asset_fund_pools_evaluator : public evaluator<asset_fund_pools_evaluator>
   {
   public:
      typedef asset_fund_pools_operation operation_type;

      void_result do_evaluate(const asset_fund_pools_operation& op);
      void_result do_apply(const asset_fund_pools_operation& op);

      const asset_dynamic_data_object* asset_dyn_data = nullptr;
   };

   class asset_reserve_evaluator : public evaluator<asset_reserve_evaluator>
   {
   public:
      typedef asset_reserve_operation operation_type;
      void_result do_evaluate( const asset_reserve_operation& o );
      void_result do_apply( const asset_reserve_operation& o );

      const asset_dynamic_data_object* asset_dyn_data = nullptr;
   };

   class asset_claim_fees_evaluator : public evaluator<asset_claim_fees_evaluator>
   {
   public:
      typedef asset_claim_fees_operation operation_type;

      void_result do_evaluate( const asset_claim_fees_operation& o );
      void_result do_apply( const asset_claim_fees_operation& o );
      const asset_dynamic_data_object* asset_dyn_data = nullptr;
   };

   class asset_publish_feeds_evaluator : public evaluator<asset_publish_feeds_evaluator>
   {
   public:
      typedef asset_publish_feed_operation operation_type;

      void_result do_evaluate( const asset_publish_feed_operation& o );
      void_result do_apply( const asset_publish_feed_operation& o );

      std::map<std::pair<asset_id_type,asset_id_type>,price_feed> median_feed_values;
   };

} } // graphene::chain
