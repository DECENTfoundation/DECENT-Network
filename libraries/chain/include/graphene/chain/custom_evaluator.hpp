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
#include <graphene/chain/message_object.hpp>

namespace graphene { namespace chain {

   class custom_evaluator : public evaluator<custom_operation, custom_evaluator>
   {
     public:
        enum custom_operation_subtype : uint16_t
        {
           custom_operation_subtype_undefined = 0,
           custom_operation_subtype_messaging
        };

        struct message_payload {
           account_id_type from;
           public_key_type pub_from;
           std::vector<message_object_receivers_data> receivers_data;

           void get_messaging_payload(const custom_operation& o);
           void set_messaging_payload(custom_operation& o) const;
        };

        operation_result do_evaluate(const operation_type& o);
        operation_result do_apply(const operation_type& o);
   };

} }

FC_REFLECT( graphene::chain::custom_evaluator::message_payload, (from)(pub_from)(receivers_data) )
