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
#include <graphene/chain/protocol/asset.hpp>
#include <graphene/chain/protocol/chain_parameters.hpp>

namespace graphene { namespace chain {

  /**
    * @brief Create a miner object, as a bid to hold a miner position on the network.
    * @ingroup operations
    *
    * Accounts which wish to become miners may use this operation to create a miner object which stakeholders may
    * vote on to approve its position as a miner.
    */
   struct miner_create_operation : public base_operation<false>
   {
      struct fee_parameters_type { uint64_t fee = 50 * GRAPHENE_BLOCKCHAIN_PRECISION/100; };

      asset             fee;
      /// The account which owns the miner. This account pays the fee for this operation.
      account_id_type   miner_account;
      std::string       url;
      public_key_type   block_signing_key;

      account_id_type fee_payer()const { return miner_account; }
      void            validate()const;
   };

  /**
    * @brief Update a miner object's URL and block signing key.
    * @ingroup operations
    */
   struct miner_update_operation : public base_operation<false>
   {
      struct fee_parameters_type
      {
         share_type fee = 1 * GRAPHENE_BLOCKCHAIN_PRECISION/1000;
      };

      asset             fee;
      /// The miner object to update.
      miner_id_type   miner;
      /// The account which owns the miner. This account pays the fee for this operation.
      account_id_type   miner_account;
      /// The new URL.
      fc::optional<std::string> new_url;
      /// The new block signing key.
      fc::optional<public_key_type> new_signing_key;

      account_id_type fee_payer()const { return miner_account; }
      void            validate()const;
   };

   /**
    * @brief Used by miners to update the global parameters of the blockchain.
    * @ingroup operations
    *
    * This operation allows the miners to update the global parameters on the blockchain. These control various
    * tunable aspects of the chain, including block and maintenance intervals, maximum data sizes, the fees charged by
    * the network, etc.
    *
    * This operation may only be used in a proposed transaction, and a proposed transaction which contains this
    * operation must have a review period specified in the current global parameters before it may be accepted.
    */
   struct miner_update_global_parameters_operation : public base_operation<false>
   {
      struct fee_parameters_type { uint64_t fee = 10; };

      asset             fee;
      chain_parameters  new_parameters;

      account_id_type fee_payer()const { return account_id_type(); }
      void            validate()const;
   };

   /// TODO: miner_resign_operation : public base_operation

} } // graphene::chain

FC_REFLECT( graphene::chain::miner_create_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::miner_create_operation, (fee)(miner_account)(url)(block_signing_key) )

FC_REFLECT( graphene::chain::miner_update_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::miner_update_operation, (fee)(miner)(miner_account)(new_url)(new_signing_key) )

FC_REFLECT( graphene::chain::miner_update_global_parameters_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::miner_update_global_parameters_operation, (fee)(new_parameters) )
