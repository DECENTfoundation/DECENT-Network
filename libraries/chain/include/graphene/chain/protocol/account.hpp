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
#include <graphene/chain/protocol/ext.hpp>
#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/protocol/vote.hpp>
#include <set>


namespace graphene { namespace chain {

   bool is_valid_name( const string& s );
   bool is_cheap_name( const string& n );

   struct publishing_rights
   {
      /// True if account can give or remove right to publish a content
      bool is_publishing_manager = false;
      /// Rights to publish a content received from publishing managers.
      /// An account can publish a content if has at least one right from publishing managers.
      std::set<account_id_type> publishing_rights_received;
      /// List of accounts that get publishing right from this account. This list is empty if account does not have publishing manager status.
      std::set<account_id_type> publishing_rights_forwarded;
   };

   /// These are the fields which can be updated by the active authority.
   struct account_options
   {
      /// The memo key is the key this account will typically use to encrypt/sign transaction memos and other non-
      /// validated account activities. This field is here to prevent confusion if the active authority has zero or
      /// multiple keys in it.
      public_key_type  memo_key;
      /// If this field is set to an account ID other than GRAPHENE_PROXY_TO_SELF_ACCOUNT,
      /// then this account's votes will be ignored; its stake
      /// will be counted as voting for the referenced account's selected votes instead.
      account_id_type voting_account = GRAPHENE_PROXY_TO_SELF_ACCOUNT;

      /// The number of active witnesses this account votes the blockchain should appoint
      /// Must not exceed the actual number of witnesses voted for in @ref votes
      uint16_t num_witness = 0;
      /// This is the list of vote IDs this account votes for. The weight of these votes is determined by this
      /// account's balance of core asset.
      flat_set<vote_id_type> votes;
      extensions_type        extensions;

      /// True if account (author) allows subscription
      bool allow_subscription = false;
      /// Price for subscription per one subscription period
      asset price_per_subscribe;
      /// Minimal duration of subscription in days
      uint32_t subscription_period = 0;

      void validate()const;
   };

   /**
    *  @ingroup operations
    */
   struct account_create_operation : public base_operation
   {
      struct ext
      {
         optional< void_t >            null_ext;
      };

      struct fee_parameters_type
      {
         uint64_t basic_fee      = 1*GRAPHENE_BLOCKCHAIN_PRECISION/1000; ///< the cost to register the cheapest non-free account
      };

      asset           fee;
      /// This account pays the fee. Must be a lifetime member.
      account_id_type registrar;

      string          name;
      authority       owner;
      authority       active;

      account_options options;
      extension< ext > extensions;

      account_id_type fee_payer()const { return registrar; }
      void            validate()const;
      share_type      calculate_fee(const fee_parameters_type& )const;

      void get_required_active_authorities( flat_set<account_id_type>& a )const
      {
         // registrar should be required anyway as it is the fee_payer(), but we insert it here just to be sure
         a.insert( registrar );
      }
   };

   /**
    * @ingroup operations
    * @brief Update an existing account
    *
    * This operation is used to update an existing account. It can be used to update the authorities, or adjust the options on the account.
    * See @ref account_object::options_type for the options which may be updated.
    */
   struct account_update_operation : public base_operation
   {
      struct ext
      {
         optional< void_t >            null_ext;
      };

      struct fee_parameters_type
      {
         share_type fee             = 1 * GRAPHENE_BLOCKCHAIN_PRECISION/1000;
      };

      asset fee;
      /// The account to update
      account_id_type account;

      /// New owner authority. If set, this operation requires owner authority to execute.
      optional<authority> owner;
      /// New active authority. This can be updated by the current active authority.
      optional<authority> active;

      /// New account options
      optional<account_options> new_options;
      extension< ext > extensions;

      account_id_type fee_payer()const { return account; }
      void       validate()const;
      share_type calculate_fee( const fee_parameters_type& k )const;

      bool is_owner_update()const
      { return owner.valid(); }

      void get_required_owner_authorities( flat_set<account_id_type>& a )const
      { if( is_owner_update() ) a.insert( account ); }

      void get_required_active_authorities( flat_set<account_id_type>& a )const
      { if( !is_owner_update() ) a.insert( account ); }
   };


} } // graphene::chain


FC_REFLECT(graphene::chain::account_options, (memo_key)(voting_account)(num_witness)(votes)(extensions)
           (allow_subscription)(price_per_subscribe)(subscription_period))

FC_REFLECT(graphene::chain::publishing_rights, (is_publishing_manager)(publishing_rights_received)(publishing_rights_forwarded))

FC_REFLECT(graphene::chain::account_create_operation::ext, (null_ext))
FC_REFLECT( graphene::chain::account_create_operation,
            (fee)(registrar)(name)(owner)(active)(options)(extensions)
          )

FC_REFLECT(graphene::chain::account_update_operation::ext, (null_ext))
FC_REFLECT( graphene::chain::account_update_operation,
            (fee)(account)(owner)(active)(new_options)(extensions)
          )

FC_REFLECT( graphene::chain::account_create_operation::fee_parameters_type, (basic_fee) )
FC_REFLECT( graphene::chain::account_update_operation::fee_parameters_type, (fee) )

