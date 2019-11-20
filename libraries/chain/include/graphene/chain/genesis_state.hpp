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

#include <graphene/chain/protocol/chain_parameters.hpp>
#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/immutable_chain_parameters.hpp>

#include <fc/crypto/sha256.hpp>

namespace graphene { namespace chain {

struct genesis_state_type {
   struct initial_account_type {
      initial_account_type(const std::string& name = std::string(),
                           const public_key_type& owner_key = public_key_type(),
                           const public_key_type& active_key = public_key_type()
                           )
         : name(name),
           owner_key(owner_key),
           active_key(active_key == public_key_type()? owner_key : active_key)
      {}
      std::string name;
      public_key_type owner_key;
      fc::optional<public_key_type> owner_key2;
      fc::optional<public_key_type> owner_key3;
      fc::optional<uint32_t> owner_threshold;
      public_key_type active_key;
      fc::optional<public_key_type> active_key2;
      fc::optional<public_key_type> active_key3;
      fc::optional<uint32_t> active_threshold;
   };
   struct initial_asset_type {
      std::string symbol;
      std::string issuer_name;
      std::string description;
      uint8_t precision = GRAPHENE_BLOCKCHAIN_PRECISION_DIGITS;

      share_type max_supply;
      share_type accumulated_fees;

      bool is_monitored_asset = false;
   };
   struct initial_balance_type {
      std::string owner;
      std::string asset_symbol;
      share_type amount;
   };
   struct initial_miner_type {
      /// Must correspond to one of the initial accounts
      std::string owner_name;
      public_key_type block_signing_key;
   };

   fc::time_point_sec                       initial_timestamp;
   share_type                               max_core_supply = GRAPHENE_MAX_SHARE_SUPPLY;
   chain_parameters                         initial_parameters;
   immutable_chain_parameters               immutable_parameters;
   std::vector<initial_account_type>        initial_accounts;
   std::vector<initial_asset_type>          initial_assets;
   std::vector<initial_balance_type>        initial_balances;
   uint64_t                                 initial_active_miners = GRAPHENE_DEFAULT_MIN_MINER_COUNT;
   std::vector<initial_miner_type>          initial_miner_candidates;

   /**
    * Temporary, will be moved elsewhere.
    */
   chain_id_type                            initial_chain_id;

   /**
    * Get the chain_id corresponding to this genesis state.
    *
    * This is the SHA256 serialization of the genesis_state.
    */
   chain_id_type compute_chain_id() const;
};

} } // namespace graphene::chain

FC_REFLECT(graphene::chain::genesis_state_type::initial_account_type, (name)(owner_key)(active_key)(owner_key2)(owner_key3)(active_key2)(active_key3)(owner_threshold)(active_threshold))

FC_REFLECT(graphene::chain::genesis_state_type::initial_asset_type,
           (symbol)(issuer_name)(description)(precision)(max_supply)(accumulated_fees)
                   (is_monitored_asset))

FC_REFLECT(graphene::chain::genesis_state_type::initial_miner_type, (owner_name)(block_signing_key))
FC_REFLECT(graphene::chain::genesis_state_type::initial_balance_type, (owner)(asset_symbol)(amount))


FC_REFLECT(graphene::chain::genesis_state_type,
           (initial_timestamp)(max_core_supply)(initial_parameters)(initial_accounts)(initial_assets)
           (initial_active_miners)(initial_miner_candidates)
           (initial_chain_id)(initial_balances)
           (immutable_parameters))
