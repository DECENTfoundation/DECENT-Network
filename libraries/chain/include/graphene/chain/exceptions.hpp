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

#include <fc/exception.hpp>
#include <graphene/chain/protocol/operations.hpp>

#define GRAPHENE_DECLARE_OP_BASE_EXCEPTIONS( op_name )                \
   FC_DECLARE_DERIVED_EXCEPTION(                                      \
      op_name ## _validate_exception,                                 \
      operation_validate_exception,                                   \
      100 * operation::tag< op_name ## _operation >::value,           \
      #op_name "_operation validation exception"                      \
      )                                                               \
   FC_DECLARE_DERIVED_EXCEPTION(                                      \
      op_name ## _evaluate_exception,                                 \
      operation_evaluate_exception,                                   \
      100 * operation::tag< op_name ## _operation >::value,           \
      #op_name "_operation evaluation exception"                      \
      )

#define GRAPHENE_DECLARE_OP_VALIDATE_EXCEPTION( exc_name, op_name, seqnum, msg ) \
   FC_DECLARE_DERIVED_EXCEPTION(                                      \
      op_name ## _ ## exc_name,                                       \
      op_name ## _validate_exception,                                 \
      seqnum,                                                         \
      msg                                                             \
      )

#define GRAPHENE_DECLARE_OP_EVALUATE_EXCEPTION( exc_name, op_name, seqnum, msg ) \
   FC_DECLARE_DERIVED_EXCEPTION(                                      \
      op_name ## _ ## exc_name,                                       \
      op_name ## _evaluate_exception,                                 \
      seqnum,                                                         \
      msg                                                             \
      )

#define GRAPHENE_DECLARE_INTERNAL_EXCEPTION( exc_name, seqnum, msg )  \
   FC_DECLARE_DERIVED_EXCEPTION(                                      \
      internal_ ## exc_name,                                          \
      internal_exception,                                             \
      seqnum,                                                         \
      msg                                                             \
      )

#define GRAPHENE_RECODE_EXC( cause_type, effect_type ) \
   catch( const cause_type& e ) \
   { throw effect_type( fc::log_messages(e.get_log()), e.code(), e.name(), e.what() ); }

namespace graphene { namespace chain {

   enum chain_exception_code
   {
      transaction_exception_code               = 1,
      utility_exception_code                   = 2,
      undo_database_exception_code             = 3,
      unlinkable_block_exception_code          = 4,
      tx_missing_active_auth_code              = 5,
      tx_missing_owner_auth_code               = 6,
      tx_missing_other_auth_code               = 7,
      tx_irrelevant_sig_code                   = 8,
      tx_duplicate_sig_code                    = 9,
      invalid_committee_approval_code          = 10,
      insufficient_fee_code                    = 11,
      trx_must_have_at_least_one_op_code       = 12,
      invalid_pts_address_code                 = 13,
      pop_empty_chain_code                     = 14,
      too_many_blocks_by_single_group_code     = 15,
   };

   FC_DECLARE_EXCEPTION( chain_exception, 400, "blockchain exception" )

#define FC_DECLARE_CHAIN_EXCEPTION(TYPE, OFFSET, WHAT) \
   FC_DECLARE_DERIVED_EXCEPTION(TYPE, chain_exception, OFFSET, WHAT)

   FC_DECLARE_CHAIN_EXCEPTION( operation_validate_exception, 3030600, "operation validation exception" )
   FC_DECLARE_CHAIN_EXCEPTION( operation_evaluate_exception, 3040600, "operation evaluation exception" )
   FC_DECLARE_CHAIN_EXCEPTION( internal_exception, 3980600, "internal exception" )

   FC_DECLARE_CHAIN_EXCEPTION( transaction_exception, transaction_exception_code, "Transaction validation exception." )
   FC_DECLARE_CHAIN_EXCEPTION( utility_exception, utility_exception_code, "Utility method exception." )
   FC_DECLARE_CHAIN_EXCEPTION( undo_database_exception, undo_database_exception_code, "Undo database exception." )
   FC_DECLARE_CHAIN_EXCEPTION( unlinkable_block_exception, unlinkable_block_exception_code, "Unlinkable block." )
   FC_DECLARE_CHAIN_EXCEPTION( tx_missing_active_auth_exception, tx_missing_active_auth_code, "Missing required active authority." )
   FC_DECLARE_CHAIN_EXCEPTION( tx_missing_owner_auth_exception, tx_missing_owner_auth_code, "Missing required owner authority." )
   FC_DECLARE_CHAIN_EXCEPTION( tx_missing_other_auth_exception, tx_missing_other_auth_code, "Missing required other authority." )
   FC_DECLARE_CHAIN_EXCEPTION( tx_irrelevant_sig_exception, tx_irrelevant_sig_code, "Irrelevant signature included." )
   FC_DECLARE_CHAIN_EXCEPTION( tx_duplicate_sig_exception, tx_duplicate_sig_code, "Duplicate signature included." )
   FC_DECLARE_CHAIN_EXCEPTION( invalid_committee_approval_exception, invalid_committee_approval_code, "Committee account cannot directly approve transaction." )
   FC_DECLARE_CHAIN_EXCEPTION( insufficient_fee_exception, insufficient_fee_code, "Insufficient fee." )
   FC_DECLARE_CHAIN_EXCEPTION( trx_must_have_at_least_one_op_exception, trx_must_have_at_least_one_op_code, "Transaction must have at least one operation.")
   FC_DECLARE_CHAIN_EXCEPTION( invalid_pts_address_exception, invalid_pts_address_code, "Invalid pts address." )
   FC_DECLARE_CHAIN_EXCEPTION( pop_empty_chain_exception, pop_empty_chain_code, "There are no blocks to pop." )
   FC_DECLARE_CHAIN_EXCEPTION( too_many_blocks_by_single_group_exception, too_many_blocks_by_single_group_code, "There are too many unfonfirmed blocks by single group." )

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

   GRAPHENE_DECLARE_INTERNAL_EXCEPTION( verify_auth_max_auth_exceeded, 1, "Exceeds max authority fan-out" )
   GRAPHENE_DECLARE_INTERNAL_EXCEPTION( verify_auth_account_not_found, 2, "Auth account not found" )

} } // graphene::chain
