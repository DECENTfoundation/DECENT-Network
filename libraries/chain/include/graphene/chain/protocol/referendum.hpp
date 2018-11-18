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
#include <graphene/db/generic_index.hpp>
namespace graphene { namespace chain { 
   /**
     * @defgroup proposed_transactions  The Graphene Transaction Proposal Protocol
     * @ingroup operations
     *
     * Graphene allows users to propose a transaction which requires approval of multiple accounts in order to execute.
     * The user proposes a transaction using proposal_create_operation, then signatory accounts use
     * proposal_update_operations to add or remove their approvals from this operation. When a sufficient number of
     * approvals have been granted, the operations in the proposal are used to create a virtual transaction which is
     * subsequently evaluated. Even if the transaction fails, the proposal will be kept until the expiration time, at
     * which point, if sufficient approval is granted, the transaction will be evaluated a final time. This allows
     * transactions which will not execute successfully until a given time to still be executed through the proposal
     * mechanism. The first time the proposed transaction succeeds, the proposal will be regarded as resolved, and all
     * future updates will be invalid.
     *
     * The proposal system allows for arbitrarily complex or recursively nested authorities. If a recursive authority
     * (i.e. an authority which requires approval of 'nested' authorities on other accounts) is required for a
     * proposal, then a second proposal can be used to grant the nested authority's approval. That is, a second
     * proposal can be created which, when sufficiently approved, adds the approval of a nested authority to the first
     * proposal. This multiple-proposal scheme can be used to acquire approval for an arbitrarily deep authority tree.
     *
     * Note that at any time, a proposal can be approved in a single transaction if sufficient signatures are available
     * on the proposal_update_operation, as long as the authority tree to approve the proposal does not exceed the
     * maximum recursion depth. In practice, however, it is easier to use proposals to acquire all approvals, as this
     * leverages on-chain notification of all relevant parties that their approval is required. Off-chain
     * multi-signature approval requires some off-chain mechanism for acquiring several signatures on a single
     * transaction. This off-chain synchronization can be avoided using proposals.
     * @{
     */
   /**
    * op_wrapper is used to get around the circular definition of operation and proposals that contain them.
    */
   struct op_wrapper;
   /**
    * @brief The proposal_create_operation creates a transaction proposal, for use in multi-sig scenarios
    * @ingroup operations
    *
    * Creates a transaction proposal. The operations which compose the transaction are listed in order in proposed_ops,
    * and expiration_time specifies the time by which the proposal must be accepted or it will fail permanently. The
    * expiration_time cannot be farther in the future than the maximum expiration time set in the global properties
    * object.
    */
   struct referendum_create_operation : public base_operation
   {
       struct fee_parameters_type { 
          uint64_t fee            = 1000 * GRAPHENE_HXCHAIN_PRECISION;
       };
       asset              fee;
	   account_id_type    proposer;
       address            fee_paying_account;
       vector<op_wrapper> proposed_ops;
	   optional<guarantee_object_id_type> guarantee_id;
       extensions_type    extensions;

	   optional<guarantee_object_id_type> get_guarantee_id()const { return guarantee_id; }
       address fee_payer()const { return fee_paying_account; }
       void            validate()const;
       share_type calculate_fee(const fee_parameters_type& k)const { return fee.amount; }
	   void get_required_authorities(vector<authority>& a)const
	   {
		   a.push_back(authority(1, fee_payer(), 1));
	   }
   };

        struct citizen_referendum_senator_operation : public base_operation
        {
            optional<guarantee_object_id_type> guarantee_id;
            optional<guarantee_object_id_type> get_guarantee_id()const { return guarantee_id; }
            struct fee_parameters_type { uint64_t fee = 0.001 * GRAPHENE_HXCHAIN_PRECISION; };

            asset                                 fee;
            map<account_id_type, account_id_type> replace_queue;
            share_type      calculate_fee(const fee_parameters_type& k)const { return 0; }
            address fee_payer()const { return address(); }
            void            validate()const {}
        };

   
   struct referendum_update_operation : public base_operation
   {
	   struct fee_parameters_type {
		   uint64_t fee = 0.001 * GRAPHENE_HXCHAIN_PRECISION;
	   };

	   address            fee_paying_account;
	   asset                      fee;
	   referendum_id_type          referendum;
	   flat_set<address>  key_approvals_to_add;
	   flat_set<address>  key_approvals_to_remove;
	   extensions_type            extensions;

	   address fee_payer()const { return fee_paying_account; }
	   void            validate()const;
	   share_type calculate_fee(const fee_parameters_type& k)const { return 0; };
	   void get_required_authorities(vector<authority>& a)const;
	   optional<guarantee_object_id_type> guarantee_id;
	   optional<guarantee_object_id_type> get_guarantee_id()const { return guarantee_id; }
   };
   struct  referendum_accelerate_pledge_operation :public base_operation
   {
	   struct fee_parameters_type
	   {
		   uint64_t fee = 0.001 * GRAPHENE_HXCHAIN_PRECISION;
	   };
	   address          fee_paying_account;
	   asset            fee;
	   referendum_id_type  referendum_id;
	   optional<guarantee_object_id_type> guarantee_id;
	   optional<guarantee_object_id_type> get_guarantee_id()const { return guarantee_id; }
	   address    fee_payer()const { return fee_paying_account; }
	   void            validate()const;
	   share_type calculate_fee(const fee_parameters_type& k)const { return fee.amount; };
	   void get_required_authorities(vector<authority>& a)const
	   {
		   a.push_back(authority(1, fee_payer(), 1));
	   }
   };

  
}} // graphene::chain

FC_REFLECT( graphene::chain::referendum_create_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::referendum_update_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::referendum_accelerate_pledge_operation::fee_parameters_type, (fee))
FC_REFLECT( graphene::chain::referendum_create_operation,(fee)(proposer)(fee_paying_account)
            (proposed_ops)(guarantee_id)(extensions))
FC_REFLECT(graphene::chain::referendum_update_operation, (fee)(fee_paying_account)
	(referendum)(key_approvals_to_add)(key_approvals_to_remove)(extensions))
FC_REFLECT(graphene::chain::referendum_accelerate_pledge_operation,(fee)(fee_paying_account)(guarantee_id)(referendum_id))
FC_REFLECT(graphene::chain::citizen_referendum_senator_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::citizen_referendum_senator_operation,(fee)(replace_queue))