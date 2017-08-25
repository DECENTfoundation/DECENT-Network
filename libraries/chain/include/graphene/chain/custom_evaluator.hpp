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

   enum custom_operation_subtype
   {
      custom_operation_subtype_messaging = 0,
      custom_operation_subtype_max,
   };

   class custom_operation_interpreter
   {
   public:
      virtual void_result do_evaluate(const custom_operation& o) = 0;
      virtual void_result do_apply(const custom_operation& o) = 0;
   };

   class custom_operation_interpreter_messaging : public custom_operation_interpreter
   {
   public:
      void_result do_evaluate(const custom_operation& o) override;// { return void_result(); };
   
      void_result do_apply(const custom_operation& o) override;// { return void_result(); };
   };

   class custom_evaluator : public evaluator<custom_evaluator>
   {
      public:
         typedef custom_operation operation_type;

         void_result do_evaluate(const custom_operation& o);
         void_result do_apply(const custom_operation& o);
   private:
         static std::map<custom_operation_subtype, std::unique_ptr<custom_operation_interpreter>> create_callbacks_map();
         static std::map<custom_operation_subtype, std::unique_ptr<custom_operation_interpreter>> operation_subtypes;
   };
} }
