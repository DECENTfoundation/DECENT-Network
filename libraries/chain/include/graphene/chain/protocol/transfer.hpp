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
#include <graphene/chain/protocol/base.hpp>
#include <graphene/chain/protocol/memo.hpp>

namespace graphene { namespace chain {

   /**
    * @ingroup operations
    *
    * @brief Transfers an amount of one asset from one account to another.
    * Fees are paid by the "from" account.
    *
    *  @warning Obsolete operation. Use \c transfer2_operation instead.
    *  @pre amount.amount > 0
    *  @pre fee.amount >= 0
    *  @pre from != to
    *  @post from account's balance will be reduced by fee and amount
    *  @post to account's balance will be increased by amount
    *  @return n/a
    */
   struct transfer_operation : public base_operation<false>
   {
      struct fee_parameters_type {
      uint64_t fee       = GRAPHENE_BLOCKCHAIN_PRECISION / 1000;
   };

      asset            fee;
      /// Account to transfer asset from
      account_id_type  from;
      /// Account to transfer asset to
      account_id_type  to;
      /// The amount of asset to transfer from @ref from to @ref to
      asset            amount;

      /// User provided data encrypted to the memo key of the "to" account
      optional<memo_data> memo;
      extensions_type   extensions;

      account_id_type fee_payer()const { return from; }
      void            validate()const;

      bool is_partner_account_id(account_id_type acc_id) const;
   };

   /**
    * @ingroup operations
    *
    * @brief Transfers an amount of one asset from one account to another account or to content.
    * In the case of transferring to a content, amount is transferred to author and co-authors of the content,
    * if they are specified.
    *
    *  Fees are paid by the "from" account
    *
    *  @pre amount.amount > 0
    *  @pre fee.amount >= 0
    *  @pre from != to
    *  @post from account's balance will be reduced by fee and amount
    *  @post to account's balance will be increased by amount
    *  @return n/a
    */
   struct transfer2_operation : public base_operation<false>
   {
      struct fee_parameters_type {
         uint64_t fee       = GRAPHENE_BLOCKCHAIN_PRECISION / 1000;
      };

      asset            fee;
      /// Account to transfer asset from
      account_id_type  from;
      /// Account or content to transfer asset to
      graphene::db::object_id_type  to;
      /// The amount of asset to transfer from @ref from to @ref to
      asset            amount;

      /// User provided data encrypted to the memo key of the "to" account
      optional<memo_data> memo;
      extensions_type   extensions;

      account_id_type fee_payer()const { return from; }
      void            validate()const;

      bool is_partner_account_id(account_id_type acc_id) const;

   };

}} // graphene::chain

FC_REFLECT( graphene::chain::transfer_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::transfer_operation, (fee)(from)(to)(amount)(memo)(extensions) )
FC_REFLECT( graphene::chain::transfer2_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::transfer2_operation, (fee)(from)(to)(amount)(memo)(extensions) )
