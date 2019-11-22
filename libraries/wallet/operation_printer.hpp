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

#include <graphene/chain/protocol/operations.hpp>

namespace graphene { namespace wallet { namespace detail {

class wallet_api_impl;

// BLOCK  TRX  OP  VOP
struct operation_printer : fc::visitor<std::string>
{
private:
   struct result_visitor : fc::visitor<std::string>
   {
      std::string operator()(const chain::void_result& x) const;
      std::string operator()(const db::object_id_type& oid) const;
   };

   std::ostream& out;
   const wallet_api_impl& wallet;
   chain::operation_result result;

   void fee(const chain::asset& a) const;
   std::string memo(const fc::optional<chain::memo_data>& data, const chain::account_object& from, const chain::account_object& to) const;

   std::string generic(std::string op_name, const chain::asset& fee, chain::account_id_type payer_id) const;

public:
   operation_printer(std::ostream& out, const wallet_api_impl& wallet, const chain::operation_result& r = chain::operation_result());

   template<typename T>
   std::string operator()(const T& op) const
   {
      return generic(fc::get_typename<T>::name(), op.fee, op.fee_payer());
   }

   std::string operator()(const chain::transfer_obsolete_operation& op) const;
   std::string operator()(const chain::transfer_operation& op) const;
   std::string operator()(const chain::non_fungible_token_issue_operation& op) const;
   std::string operator()(const chain::non_fungible_token_transfer_operation& op) const;
   std::string operator()(const chain::account_create_operation& op) const;
   std::string operator()(const chain::account_update_operation& op) const;
   std::string operator()(const chain::asset_create_operation& op) const;
   std::string operator()(const chain::content_submit_operation& op) const;
   std::string operator()(const chain::request_to_buy_operation& op) const;
   std::string operator()(const chain::leave_rating_and_comment_operation& op) const;
   std::string operator()(const chain::ready_to_publish_operation& op) const;
   std::string operator()(const chain::custom_operation& op) const;
};

struct op_prototype_visitor : fc::visitor<void>
{
   boost::container::flat_map<std::string, chain::operation>& name2op;

   op_prototype_visitor( boost::container::flat_map<std::string, chain::operation>& _prototype_ops) : name2op(_prototype_ops) {}

   template<typename Type>
   result_type operator()( const Type& op )const
   {
      std::string name = fc::get_typename<Type>::name();
      size_t p = name.rfind(':');
      if( p != std::string::npos )
         name = name.substr( p+1 );
      name2op[ name ] = Type();
   }
};

} } } // namespace graphene::wallet::detail
