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
#include <graphene/chain/transaction_evaluation_state.hpp>

namespace graphene { namespace chain {

   class referendum_create_evaluator : public evaluator<referendum_create_evaluator>
   {
      public:
         typedef referendum_create_operation operation_type;

         void_result do_evaluate( const referendum_create_operation& o );
         object_id_type do_apply( const referendum_create_operation& o );
		 void pay_fee() override;
         transaction _proposed_trx;
		 fc::uint128_t _pledge =0;
   };

   class referendum_update_evaluator :public evaluator<referendum_update_evaluator>
   {
   public:
	   typedef referendum_update_operation operation_type;

	   void_result do_evaluate(const referendum_update_operation& o);
	   void_result do_apply(const referendum_update_operation& o);
	   const referendum_object* _referendum = nullptr;
	   processed_transaction _processed_transaction;
	   bool _executed_referendum = false;
	   bool _referendum_failed = false;
   };
   class referendum_accelerate_pledge_evaluator : public evaluator<referendum_accelerate_pledge_evaluator>
   {
   public:
	   typedef referendum_accelerate_pledge_operation operation_type;
	   void_result do_evaluate(const referendum_accelerate_pledge_operation& o);
	   void_result do_apply(const referendum_accelerate_pledge_operation& o);
	   void pay_fee() override;
   };
} } // graphene::chain
