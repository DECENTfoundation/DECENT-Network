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
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/transaction_evaluation_state.hpp>

namespace graphene { namespace chain {

   class generic_evaluator
   {
   public:
      virtual ~generic_evaluator() = default;

      virtual operation_result start_evaluate(transaction_evaluation_state& eval_state, const operation& op, bool apply);

      /**
       * @note derived classes should ASSUME that the default validation that is
       * indepenent of chain state should be performed by op.validate() and should
       * not perform these extra checks.
       */
      virtual operation_result evaluate(const operation& op) = 0;
      virtual operation_result apply(const operation& op) = 0;

      /**
       * Routes the fee to where it needs to go.  The default implementation
       * routes the fee to the asset accumulated_fees.
       *
       * Before pay_fee() is called, the fee is computed by prepare_fee() and has been
       * moved out of the fee_paying_account and (if paid in a non-CORE asset) converted
       * by the asset's fee pool.
       *
       * Therefore, when pay_fee() is called, the fee only exists in this->core_fee_paid.
       * So pay_fee() need only increment the receiving balance.
       */
      virtual void pay_fee();

      database& db()const;

      //void check_required_authorities(const operation& op);
   protected:
      /**
       * @brief Fetch objects relevant to fee payer and set pointer members
       * @param account_id Account which is paying the fee
       * @param fee The fee being paid. May be in assets other than core.
       *
       * This method verifies that the fee is valid and sets the object pointer members and the fee fields. It should
       * be called during do_evaluate.
       *
       * In particular, core_fee_paid field is set by prepare_fee().
       */
      void prepare_fee(account_id_type account_id, asset fee);

      /**
       * Convert the fee into BTS through the exchange pool.
       *
       * Reads core_fee_paid field for how much CORE is deducted from the exchange pool,
       * and fee_from_account for how much USD is added to the pool.
       *
       * Since prepare_fee() does the validation checks ensuring the account and fee pool
       * have sufficient balance and the exchange rate is correct,
       * those validation checks are not replicated here.
       *
       * Rather than returning a value, this method fills in core_fee_paid field.
       */
      void convert_fee();

      graphene::db::object_id_type get_relative_id( graphene::db::object_id_type rel_id ) const;

      // the next two functions are helpers that allow template functions declared in this 
      // header to call db() without including database.hpp, which would
      // cause a circular dependency
      share_type calculate_fee_for_operation(const operation& op) const;
      void db_adjust_balance(const account_id_type& fee_payer, asset fee_from_account);

      asset                            fee_from_account;
      share_type                       core_fee_paid;
      const account_object*            fee_paying_account = nullptr;
      const account_statistics_object* fee_paying_account_statistics = nullptr;
      const asset_object*              fee_asset          = nullptr;
      const asset_dynamic_data_object* fee_asset_dyn_data = nullptr;
      transaction_evaluation_state*    trx_state;
   };

   class op_evaluator
   {
   public:
      virtual ~op_evaluator() = default;
      virtual operation_result evaluate(transaction_evaluation_state& eval_state, const operation& op, bool apply) = 0;
   };

   template<typename T>
   class op_evaluator_impl : public op_evaluator
   {
   public:
      virtual operation_result evaluate(transaction_evaluation_state& eval_state, const operation& op, bool apply) override
      {
         T eval;
         return eval.start_evaluate(eval_state, op, apply);
      }
   };

   template<typename Operation, typename Evaluator>
   class evaluator : public generic_evaluator
   {
   public:
      typedef Operation operation_type;
      typedef Evaluator evaluator_type;
      static_assert(!operation_type::value, "Operation is virtual");

      virtual operation_result evaluate(const operation& o) final override
      {
         auto* eval = static_cast<evaluator_type*>(this);
         const auto& op = o.get<operation_type>();

         prepare_fee(op.fee_payer(), op.fee);
         if( !trx_state->skip_fee_schedule_check )
         {
            share_type required_fee = calculate_fee_for_operation(op);
            FC_VERIFY_AND_THROW( core_fee_paid >= required_fee,
                       insufficient_fee_exception,
                       "Insufficient Fee Paid",
                       ("core_fee_paid",core_fee_paid)("required", required_fee) );
         }

         return eval->do_evaluate(op);
      }

      virtual operation_result apply(const operation& o) final override
      {
         auto* eval = static_cast<evaluator_type*>(this);
         const auto& op = o.get<operation_type>();

         convert_fee();
         pay_fee();

         auto result = eval->do_apply(op);

         db_adjust_balance(op.fee_payer(), -fee_from_account);

         return result;
      }
   };

   template<typename Operation>
   class evaluator<Operation, void> : public generic_evaluator
   {
   public:
      typedef Operation operation_type;
      static_assert(operation_type::value, "Operation is not virtual");

      virtual operation_result evaluate(const operation& o) final override
      {
         return void_result();
      }

      virtual operation_result apply(const operation& o) final override
      {
         return void_result();
      }
   };

   template<typename Operation>
   using virtual_evaluator_t = evaluator<Operation, void>;

} }
