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

#include <fc/exception/exception.hpp>
#include <graphene/chain/protocol/protocol.hpp>

#define GRAPHENE_ASSERT( expr, exc_type, FORMAT, ... )                \
   FC_MULTILINE_MACRO_BEGIN                                           \
   if( !(expr) )                                                      \
      FC_THROW_EXCEPTION( exc_type, FORMAT, __VA_ARGS__ );            \
   FC_MULTILINE_MACRO_END


#define GRAPHENE_DECLARE_OP_BASE_EXCEPTIONS( op_name )                \
   FC_DECLARE_DERIVED_EXCEPTION(                                      \
      op_name ## _validate_exception,                                 \
      graphene::chain::operation_validate_exception,                  \
      3040000 + 100 * operation::tag< op_name ## _operation >::value, \
      #op_name "_operation validation exception"                      \
      )                                                               \
   FC_DECLARE_DERIVED_EXCEPTION(                                      \
      op_name ## _evaluate_exception,                                 \
      graphene::chain::operation_evaluate_exception,                  \
      3050000 + 100 * operation::tag< op_name ## _operation >::value, \
      #op_name "_operation evaluation exception"                      \
      )

#define GRAPHENE_DECLARE_OP_VALIDATE_EXCEPTION( exc_name, op_name, seqnum, msg ) \
   FC_DECLARE_DERIVED_EXCEPTION(                                      \
      op_name ## _ ## exc_name,                                       \
      graphene::chain::op_name ## _validate_exception,                \
      3040000 + 100 * operation::tag< op_name ## _operation >::value  \
         + seqnum,                                                    \
      msg                                                             \
      )

#define GRAPHENE_DECLARE_OP_EVALUATE_EXCEPTION( exc_name, op_name, seqnum, msg ) \
   FC_DECLARE_DERIVED_EXCEPTION(                                      \
      op_name ## _ ## exc_name,                                       \
      graphene::chain::op_name ## _evaluate_exception,                \
      3050000 + 100 * operation::tag< op_name ## _operation >::value  \
         + seqnum,                                                    \
      msg                                                             \
      )

namespace graphene { namespace chain {

   enum chain_exception_code
   {
      transaction_exception_code               = 1,
      operation_validate_exception_code        = 2,
      operation_evaluate_exception_code        = 3,
      utility_exception_code                   = 4,
      undo_database_exception_code             = 5,
      unlinkable_block_exception_code          = 6,
      tx_missing_active_auth_code              = 7,
      tx_missing_owner_auth_code               = 8,
      tx_missing_other_auth_code               = 9,
      tx_irrelevant_sig_code                   = 10,
      tx_duplicate_sig_code                    = 11,
      invalid_committee_approval_code          = 12,
      insufficient_fee_code                    = 13,
      trx_must_have_at_least_one_op_code       = 14,
      invalid_pts_address_code                 = 15,
      pop_empty_chain_code                     = 16,
   };

   FC_DECLARE_EXCEPTION( chain_exception, fc::chain_exception_base_code, "blockchain exception" )

   FC_DECLARE_DERIVED_EXCEPTION( transaction_exception,             graphene::chain::chain_exception, fc::chain_exception_base_code + transaction_exception_code, "Transaction validation exception." )
   FC_DECLARE_DERIVED_EXCEPTION( operation_validate_exception,      graphene::chain::chain_exception, fc::chain_exception_base_code + operation_validate_exception_code, "Operation validation exception." )
   FC_DECLARE_DERIVED_EXCEPTION( operation_evaluate_exception,      graphene::chain::chain_exception, fc::chain_exception_base_code + operation_evaluate_exception_code, "Operation evaluation exception." )
   FC_DECLARE_DERIVED_EXCEPTION( utility_exception,                 graphene::chain::chain_exception, fc::chain_exception_base_code + utility_exception_code, "Utility method exception." )
   FC_DECLARE_DERIVED_EXCEPTION( undo_database_exception,           graphene::chain::chain_exception, fc::chain_exception_base_code + undo_database_exception_code, "Undo database exception." )
   FC_DECLARE_DERIVED_EXCEPTION( unlinkable_block_exception,        graphene::chain::chain_exception, fc::chain_exception_base_code + unlinkable_block_exception_code, "Unlinkable block." )

   FC_DECLARE_DERIVED_EXCEPTION( tx_missing_active_auth_exception,  graphene::chain::chain_exception, fc::chain_exception_base_code + tx_missing_active_auth_code, "Missing required active authority." )
   FC_DECLARE_DERIVED_EXCEPTION( tx_missing_owner_auth_exception,   graphene::chain::chain_exception, fc::chain_exception_base_code + tx_missing_owner_auth_code, "Missing required owner authority." )
   FC_DECLARE_DERIVED_EXCEPTION( tx_missing_other_auth_exception,             graphene::chain::chain_exception, fc::chain_exception_base_code + tx_missing_other_auth_code, "Missing required other authority." )
   FC_DECLARE_DERIVED_EXCEPTION( tx_irrelevant_sig_exception,       graphene::chain::chain_exception, fc::chain_exception_base_code + tx_irrelevant_sig_code, "Irrelevant signature included." )
   FC_DECLARE_DERIVED_EXCEPTION( tx_duplicate_sig_exception,        graphene::chain::chain_exception, fc::chain_exception_base_code + tx_duplicate_sig_code, "Duplicate signature included." )
   FC_DECLARE_DERIVED_EXCEPTION( invalid_committee_approval_exception, graphene::chain::chain_exception, fc::chain_exception_base_code + invalid_committee_approval_code, "Committee account cannot directly approve transaction." )
   FC_DECLARE_DERIVED_EXCEPTION( insufficient_fee_exception,        graphene::chain::chain_exception, fc::chain_exception_base_code + insufficient_fee_code, "Insufficient fee." )
   
   FC_DECLARE_DERIVED_EXCEPTION( trx_must_have_at_least_one_op_exception,     graphene::chain::chain_exception, fc::chain_exception_base_code + trx_must_have_at_least_one_op_code, "Transaction must have at least one operation.")

   FC_DECLARE_DERIVED_EXCEPTION( invalid_pts_address_exception,     graphene::chain::chain_exception, fc::chain_exception_base_code + invalid_pts_address_code, "Invalid pts address." )
   FC_DECLARE_DERIVED_EXCEPTION( pop_empty_chain_exception,         graphene::chain::chain_exception, fc::chain_exception_base_code + pop_empty_chain_code, "There are no blocks to pop." )
     

   GRAPHENE_DECLARE_OP_BASE_EXCEPTIONS( account_create );
   GRAPHENE_DECLARE_OP_EVALUATE_EXCEPTION( max_auth_exceeded, account_create, 1, "Exceeds max authority fan-out" )
   GRAPHENE_DECLARE_OP_EVALUATE_EXCEPTION( auth_account_not_found, account_create, 2, "Auth account not found" )

   GRAPHENE_DECLARE_OP_BASE_EXCEPTIONS( account_update );
   GRAPHENE_DECLARE_OP_EVALUATE_EXCEPTION( max_auth_exceeded, account_update, 1, "Exceeds max authority fan-out" )
   GRAPHENE_DECLARE_OP_EVALUATE_EXCEPTION( auth_account_not_found, account_update, 2, "Auth account not found" )

   GRAPHENE_DECLARE_OP_BASE_EXCEPTIONS( asset_reserve );
   GRAPHENE_DECLARE_OP_EVALUATE_EXCEPTION( invalid_on_mia, asset_reserve, 1, "invalid on mia" )

   GRAPHENE_DECLARE_OP_BASE_EXCEPTIONS( proposal_create );
   GRAPHENE_DECLARE_OP_EVALUATE_EXCEPTION( review_period_required, proposal_create, 1, "review_period required" )
   GRAPHENE_DECLARE_OP_EVALUATE_EXCEPTION( review_period_insufficient, proposal_create, 2, "review_period insufficient" )

   #define GRAPHENE_RECODE_EXC( cause_type, effect_type ) \
      catch( const cause_type& e ) \
      { throw( effect_type( e.what(), e.get_log() ) ); }

} } // graphene::chain
