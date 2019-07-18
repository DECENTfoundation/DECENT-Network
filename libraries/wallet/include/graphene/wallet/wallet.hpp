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

#include <graphene/app/api.hpp>
#include <graphene/utilities/key_conversion.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/budget_record_object.hpp>
#include <graphene/chain/global_property_object.hpp>
#include <graphene/chain/transaction_detail_object.hpp>
#include <fc/api.hpp>


using namespace graphene::app;
using namespace graphene::chain;
using namespace graphene::utilities;
using namespace std;

namespace fc
{
   void to_variant(const account_multi_index_type& accts, variant& vo);
   void from_variant(const variant &var, account_multi_index_type &vo);
}

namespace graphene { namespace wallet {

      typedef uint16_t transaction_handle_type;

/**
 * This class takes a variant and turns it into an object
 * of the given type, with the new operator.
 */

      graphene::db::object* create_object( const variant& v );

      struct server_data
      {
         string                    server;
         string                    user;
         string                    password;
      };

      struct wallet_data
      {
         int version = 0;
         /** Chain ID this wallet is used with */
         chain_id_type chain_id;
         account_multi_index_type my_accounts;
         /// @return IDs of all accounts in @ref my_accounts
         vector<object_id_type> my_account_ids()const
         {
            vector<object_id_type> ids;
            ids.reserve(my_accounts.size());
            std::transform(my_accounts.begin(), my_accounts.end(), std::back_inserter(ids),
                           [](const account_object& ao) { return ao.id; });
            return ids;
         }
         /// @brief Add acct to @ref my_accounts, or update it if it is already in @ref my_accounts
         /// @return \c true if the account was newly inserted; \c false if it was only updated
         bool update_account(const account_object& acct)
         {
            auto& idx = my_accounts.get<graphene::db::by_id>();
            auto itr = idx.find(acct.get_id());
            if( itr != idx.end() )
            {
               idx.replace(itr, acct);
               return false;
            } else {
               idx.insert(acct);
               return true;
            }
         }

         /** encrypted keys */
         vector<char>              cipher_keys;


         /** map an account to a set of extra keys that have been imported for that account */
         map<account_id_type, set<public_key_type> >  extra_keys;

         // map of account_name -> base58_private_key for
         //    incomplete account regs
         map<string, vector<string> > pending_account_registrations;
         map<string, string> pending_miner_registrations;


         string                    ws_server = "ws://localhost:8090";
         string                    ws_user;
         string                    ws_password;
         string                    update_time;
      };

      struct wallet_about
      {
         decent::about_info daemon_info;
         decent::about_info wallet_info;
      };

      struct wallet_info
      {
         uint32_t head_block_num;
         block_id_type head_block_id;
         std::string head_block_age;
         std::string next_maintenance_time;
         chain_id_type chain_id;
         double participation;
         vector<miner_id_type> active_miners;
      };

      struct el_gamal_key_pair
      {
         DInteger private_key;
         DInteger public_key;
      };

      struct el_gamal_key_pair_str
      {
         DIntegerString private_key;
         DIntegerString public_key;
      };

      /**
       * Needed for backward compatibility. Old wallet json files use this struct to store encrypted ec keys.
       */
      struct plain_keys
      {
         map<public_key_type, string>  ec_keys;
         fc::sha512                    checksum;
      };

      /**
       * New wallet json files store encrypted ec keys along with derived el gamal keys.
       */
      struct plain_ec_and_el_gamal_keys : public plain_keys
      {
         plain_ec_and_el_gamal_keys& operator=( const plain_keys& pk )
         {
            ec_keys = pk.ec_keys;
            checksum = pk.checksum;
            return *this;
         }

         plain_ec_and_el_gamal_keys() = default;
         vector<el_gamal_key_pair_str> el_gamal_keys;
      };

      struct brain_key_info
      {
         string brain_priv_key;
         string wif_priv_key;
         public_key_type pub_key;
      };

      struct exported_account_keys
      {
         string account_name;
         vector<vector<char>> encrypted_private_keys;
         vector<public_key_type> public_keys;
      };

      struct exported_keys
      {
         fc::sha512 password_checksum;
         vector<exported_account_keys> account_keys;
      };

      struct approval_delta
      {
         vector<string> active_approvals_to_add;
         vector<string> active_approvals_to_remove;
         vector<string> owner_approvals_to_add;
         vector<string> owner_approvals_to_remove;
         vector<string> key_approvals_to_add;
         vector<string> key_approvals_to_remove;
      };

      struct regional_price_info
      {
         string region;
         string amount;
         string asset_symbol;
      };

      struct content_download_status
      {
         int          total_key_parts;
         int          received_key_parts;
         int          total_download_bytes;
         int          received_download_bytes;
         std::string  status_text;
      };

      struct operation_detail {
         string                   memo;
         string                   description;
         operation_history_object op;
      };

      struct buying_object_ex : public buying_object, public content_download_status {
         buying_object_ex(const buying_object& obj, const content_download_status& status)
          : buying_object(obj), content_download_status(status)
         {
            // buying_object price is used for other purposes
            // but the GUI relies on buying_object_ex::price
            // so overwrite as a quick fix
            price = paid_price_before_exchange;
            this->id = std::string(obj.id);
         }

         std::string         id;
         std::string         author_account;
         uint32_t            times_bought;
         fc::ripemd160       hash;
         uint64_t            AVG_rating;
      };

      struct rating_object_ex : public buying_object
      {
         rating_object_ex( const buying_object& buying, string author )
         : buying_object( buying ), author(author) {}
         std::string author;
      };

      struct vesting_balance_object_with_info : public vesting_balance_object
      {
         vesting_balance_object_with_info( const vesting_balance_object& vbo, fc::time_point_sec now );
         vesting_balance_object_with_info( const vesting_balance_object_with_info& vbo ) = default;

         /**
          * How much is allowed to be withdrawn.
          */
         asset allowed_withdraw;

         /**
          * The time at which allowed_withdrawal was calculated.
          */
         fc::time_point_sec allowed_withdraw_time;
      };

      struct miner_voting_info
      {
          miner_id_type id;
          string name;
          string url;
          uint64_t total_votes;
          bool voted;
      };


      struct balance_change_result_detail : public balance_change_result
      {
          string                   memo;
          string                   description;
      };

      struct extended_asset : public asset
      {
         extended_asset( const asset& a, const string& pretty_amount )
            : asset( a ), pretty_amount( pretty_amount ) {}
         std::string pretty_amount;
      };

      struct signed_transaction_info : public signed_transaction
      {
         signed_transaction_info(const signed_transaction& tx)
            : signed_transaction( tx ), transaction_id( tx.id() ) {}
         transaction_id_type transaction_id;
      };

      namespace detail {
         class wallet_api_impl;
      }


/**
 * This wallet assumes it is connected to the database server with a high-bandwidth, low-latency connection and
 * performs minimal caching. This API could be provided locally to be used by a web interface.
 *
 * @defgroup WalletCLI CLI Wallet
 * @{
 * @defgroup WalletAPI_Wallet Wallet
 * @defgroup WalletAPI_General General
 * @defgroup WalletAPI_Account Account
 * @defgroup WalletAPI_Asset Asset
 * @defgroup WalletAPI_NonFungibleToken Non Fungible Token
 * @defgroup WalletAPI_Subscription Subscription
 * @defgroup WalletAPI_Content Content
 * @defgroup WalletAPI_Messaging Messaging
 * @defgroup WalletAPI_Monitoring Monitoring
 * @defgroup WalletAPI_Seeding Seeding
 * @defgroup WalletAPI_Mining Mining
 * @defgroup WalletAPI_Proposals Proposals
 * @defgroup WalletAPI_TransactionBuilder Transaction Builder
 * @}
 */

   class wallet_api : public fc::api_base<wallet_api>
      {
      public:
         wallet_api( const fc::api<login_api> &rapi, const chain_id_type &chain_id, const server_data &ws );
         virtual ~wallet_api();


#include "general.hpp"
#include "wallet_file.hpp"
#include "account.hpp"
#include "assets.hpp"
#include "transaction_builder.hpp"
#include "mining.hpp"
#include "seeding.hpp"
#include "proposals.hpp"
#include "content.hpp"
#include "subscription.hpp"
#include "messaging.hpp"
#include "monitoring.hpp"

         ////////////////////////
         // Non Fungible Token //
         ////////////////////////

         /**
          * @brief Lists all non fungible tokens registered on the blockchain.
          * To list all non fungible tokens, pass the empty string \c "" for the \c lowerbound to start
          * at the beginning of the list, and iterate as necessary.
          * @param lowerbound the symbol of the first non fungible token to include in the list
          * @param limit the maximum number of non fungible tokens to return (max: 100)
          * @return the list of non fungible token objects, ordered by symbol
          * @ingroup WalletAPI_NonFungibleToken
          */
         vector<non_fungible_token_object> list_non_fungible_tokens(const string& lowerbound, uint32_t limit) const;

         /**
          * @brief Returns information about the given non fungible token.
          * @param nft_symbol_or_id the name or id of the non fungible token symbol in question
          * @return the information about the non fungible token stored in the block chain
          * @ingroup WalletAPI_NonFungibleToken
          */
         non_fungible_token_object get_non_fungible_token(const string& nft_symbol_or_id) const;

         /**
          * @brief Creates a new non fungible token definition.
          * @param issuer the name or id of the account who will pay the fee and become the issuer of the new non fungible token
          * @param symbol the ticker symbol of the new non fungible token
          * @param description non fungible token description (max: 1000)
          * @param definitions non fungible token data definitions
          * @param max_supply the maximum supply of this non fungible token which may exist at any given time
          * @param fixed_max_supply true to deny future modifications of 'max_supply' otherwise false
          * @param transferable true to allow token transfer to other account otherwise false
          * @param broadcast \c true to broadcast the transaction on the network
          * @return the signed transaction creating the new non fungible token
          * @ingroup WalletAPI_NonFungibleToken
          */
         signed_transaction_info create_non_fungible_token(const string& issuer,
                                                           const string& symbol,
                                                           const string& description,
                                                           const non_fungible_token_data_definitions& definitions,
                                                           uint32_t max_supply,
                                                           bool fixed_max_supply,
                                                           bool transferable,
                                                           bool broadcast = false);

         /**
          * @brief Updates the non fungible token definition.
          * @note Maximum supply will be changed only if fixed_max_supply is not set.
          * @param issuer the name or id of the account who will become the new issuer (or pass empty string)
          * @param symbol the ticker symbol of the non fungible token to update
          * @param description non fungible token description (max: 1000)
          * @param max_supply the maximum supply of this non fungible token which may exist at any given time
          * @param fixed_max_supply true to deny future modifications of 'max_supply'
          * @param broadcast \c true to broadcast the transaction on the network
          * @return the signed transaction creating the new non fungible token
          * @ingroup WalletAPI_NonFungibleToken
          */
         signed_transaction_info update_non_fungible_token(const string& issuer,
                                                           const string& symbol,
                                                           const string& description,
                                                           uint32_t max_supply,
                                                           bool fixed_max_supply,
                                                           bool broadcast = false);

         /**
          * @brief Issues new instance of non fungible token.
          * @param to_account the name or id of the account to receive the new instance
          * @param symbol the ticker symbol of the non fungible token to issue
          * @param data non fungible token instance data
          * @param memo a memo to include in the transaction, readable by the recipient
          * @param broadcast \c true to broadcast the transaction on the network
          * @return the signed transaction issuing the new instance
          * @ingroup WalletAPI_NonFungibleToken
          */
         signed_transaction_info issue_non_fungible_token(const string& to_account,
                                                          const string& symbol,
                                                          const fc::variants& data,
                                                          const string& memo,
                                                          bool broadcast = false);

         /**
          * @brief Gets non fungible token instances by registered token symbol.
          * @param nft_symbol_or_id the name or id of the non fungible token symbol in question
          * @return the non fungible token data objects found
          * @ingroup WalletAPI_NonFungibleToken
          */
         vector<non_fungible_token_data_object> list_non_fungible_token_data(const string& nft_symbol_or_id) const;

         /**
          * @brief Get account's summary of various non fungible tokens.
          * @param account the name or id of the account
          * @return a summary of non fungible token ids
          * @ingroup DatabaseAPI_Balance
          */
         map<non_fungible_token_id_type,uint32_t> get_non_fungible_token_summary(const string& account) const;

         /**
          * @brief Gets account's balances in various non fungible tokens.
          * @param account the name or id of the account
          * @param symbols_or_ids set of symbol names or non fungible token ids to filter retrieved tokens (to disable filtering pass empty set)
          * @return the list of non fungible token data objects
          * @ingroup WalletAPI_NonFungibleToken
          */
         vector<non_fungible_token_data_object> get_non_fungible_token_balances(const string& account,
                                                                                const set<string>& symbols_or_ids) const;

         /**
          * @brief Gets non fungible token data object transfer history.
          * @param nft_data_id the non fungible token data object id to search history for
          * @return a list of transaction detail objects
          * @ingroup WalletAPI_NonFungibleToken
          */
         vector<transaction_detail_object> search_non_fungible_token_history(non_fungible_token_data_id_type nft_data_id) const;

         /**
          * @brief Transfers ownership of token instance.
          * @param to_account the name or id of the account to receive the token instance
          * @param nft_data_id the token instance id to transfer
          * @param memo a memo to include in the transaction, readable by the recipient
          * @param broadcast \c true to broadcast the transaction on the network
          * @return the signed transaction transfering the token instance
          * @ingroup WalletAPI_NonFungibleToken
          */
         signed_transaction_info transfer_non_fungible_token_data(const string& to_account,
                                                                  const non_fungible_token_data_id_type nft_data_id,
                                                                  const string& memo,
                                                                  bool broadcast = false);

         /**
          * @brief Burns (destroys) the token instance.
          * @param nft_data_id the token instance id to destroy
          * @param broadcast \c true to broadcast the transaction on the network
          * @ingroup WalletAPI_NonFungibleToken
          */
         signed_transaction_info burn_non_fungible_token_data(const non_fungible_token_data_id_type nft_data_id,
                                                              bool broadcast = false);

         /**
          * @brief Updates data of token instance.
          * @param modifier the name or id of the modifier account
          * @param nft_data_id the token instance id to update
          * @param data name to value pairs to be updated
          * @param broadcast \c true to broadcast the transaction on the network
          * @return the signed transaction updating the token instance
          * @ingroup WalletAPI_NonFungibleToken
          */
         signed_transaction_info update_non_fungible_token_data(const string& modifier,
                                                                const non_fungible_token_data_id_type nft_data_id,
                                                                const vector<pair<string, fc::variant>>& data,
                                                                bool broadcast = false);

         std::map<string,std::function<string(fc::variant,const fc::variants&)>> get_result_formatters() const;

         fc::signal<void(bool)> lock_changed;
         std::shared_ptr<detail::wallet_api_impl> my;
      };
} }

FC_REFLECT( graphene::wallet::wallet_data,
            (chain_id)
            (my_accounts)
            (cipher_keys)
            (extra_keys)
            (pending_account_registrations)(pending_miner_registrations)
            (ws_server)
            (ws_user)
            (ws_password)
          )

FC_REFLECT( graphene::wallet::wallet_about,
            (daemon_info)
            (wallet_info)
          )

FC_REFLECT( graphene::wallet::wallet_info,
            (head_block_num)
            (head_block_id)
            (head_block_age)
            (next_maintenance_time)
            (chain_id)
            (participation)
            (active_miners)
          )

FC_REFLECT( graphene::wallet::el_gamal_key_pair, (private_key)(public_key) )

FC_REFLECT( graphene::wallet::el_gamal_key_pair_str, (private_key)(public_key) )

FC_REFLECT( graphene::wallet::plain_keys, (ec_keys)(checksum) )

FC_REFLECT_DERIVED( graphene::wallet::plain_ec_and_el_gamal_keys, (graphene::wallet::plain_keys), (el_gamal_keys) )

FC_REFLECT( graphene::wallet::brain_key_info, (brain_priv_key)(wif_priv_key)(pub_key) )

FC_REFLECT( graphene::wallet::exported_account_keys, (account_name)(encrypted_private_keys)(public_keys) )

FC_REFLECT( graphene::wallet::exported_keys, (password_checksum)(account_keys) )

FC_REFLECT( graphene::wallet::approval_delta,
            (active_approvals_to_add)
            (active_approvals_to_remove)
            (owner_approvals_to_add)
            (owner_approvals_to_remove)
            (key_approvals_to_add)
            (key_approvals_to_remove)
          )

FC_REFLECT( graphene::wallet::regional_price_info, (region)(amount)(asset_symbol) )

FC_REFLECT( graphene::wallet::content_download_status,
            (total_key_parts)
            (received_key_parts)
            (total_download_bytes)
            (received_download_bytes)
            (status_text)
          )

FC_REFLECT( graphene::wallet::operation_detail, (memo)(description)(op) )

FC_REFLECT_DERIVED( graphene::wallet::balance_change_result_detail, (graphene::app::balance_change_result), (memo)(description) )

FC_REFLECT_DERIVED( graphene::wallet::buying_object_ex,
                    (graphene::chain::buying_object)
                    (graphene::wallet::content_download_status),
                    (id)
                    (author_account)
                    (AVG_rating)
                    (times_bought)
                    (hash)
                  )

FC_REFLECT_DERIVED( graphene::wallet::rating_object_ex, (graphene::chain::buying_object),(author) )

FC_REFLECT_DERIVED( graphene::wallet::vesting_balance_object_with_info, (graphene::chain::vesting_balance_object),
                    (allowed_withdraw)(allowed_withdraw_time) )

FC_REFLECT( graphene::wallet::miner_voting_info, (id)(name)(url)(total_votes)(voted) )

FC_REFLECT_DERIVED( graphene::wallet::extended_asset, (graphene::chain::asset),(pretty_amount))

FC_REFLECT_DERIVED( graphene::wallet::signed_transaction_info,(graphene::chain::signed_transaction),(transaction_id))

FC_API( graphene::wallet::wallet_api,
        //General
        (about)
        (get_block)
        (get_global_properties)
        (get_dynamic_global_properties)
        (get_object)
        (info)
        (help)
        (get_help)
        (head_block_time)
        (network_add_nodes)
        (network_get_connected_peers)
        (get_transaction_id)
        (get_transaction_by_id)
        (from_command_file)

        //Wallet file
        (list_my_accounts)
        (get_wallet_filename)
        (get_private_key)
        (is_new)
        (is_locked)
        (lock)(unlock)(set_password)
        (load_wallet_file)
        (save_wallet_file)
        (import_key)
        (import_single_key)
        (dump_private_keys)
        (list_operations)

        //Account
        (get_account_count)
        (list_accounts)
        (search_accounts)
        (list_account_balances)
        (search_account_history)
        (get_account_history)
        (search_account_balance_history)
        (get_account_balance_for_transaction)
        (get_relative_account_history)
        (get_account)
        (derive_private_key)
        (get_public_key)
        (suggest_brain_key)
        (register_account_with_keys)
        (register_account)
        (register_multisig_account)
        (create_account_with_brain_key)
        (update_account_keys)
        (update_account_keys_to_multisig)
        (transfer)
        (transfer2)
        (generate_el_gamal_keys)
        (get_el_gammal_key)
        (generate_brain_key_el_gamal_key)
        (get_brain_key_info)

        //Assets
        (list_assets)
        (get_asset)
        (get_monitored_asset_data)
        (create_monitored_asset)
        (update_monitored_asset)
        (create_user_issued_asset)
        (update_user_issued_asset)
        (issue_asset)
        (fund_asset_pools)
        (reserve_asset)
        (claim_fees)
        (price_to_dct)
        (publish_asset_feed)
        (get_feeds_by_miner)
        (get_real_supply)

        //Non fungible token
        (list_non_fungible_tokens)
        (get_non_fungible_token)
        (create_non_fungible_token)
        (update_non_fungible_token)
        (issue_non_fungible_token)
        (list_non_fungible_token_data)
        (get_non_fungible_token_summary)
        (get_non_fungible_token_balances)
        (search_non_fungible_token_history)
        (transfer_non_fungible_token_data)
        (burn_non_fungible_token_data)
        (update_non_fungible_token_data)

        //Transaction builder
        (begin_builder_transaction)
        (add_operation_to_builder_transaction)
        (replace_operation_in_builder_transaction)
        (set_fees_on_builder_transaction)
        (preview_builder_transaction)
        (sign_builder_transaction)
        (propose_builder_transaction)
        (propose_builder_transaction2)
        (remove_builder_transaction)
        (serialize_transaction)
        (sign_transaction)
        (get_prototype_operation)

        //Mining
        (list_miners)
        (get_miner)
        (create_miner)
        (update_miner)
        (get_vesting_balances)
        (withdraw_vesting)
        (vote_for_miner)
        (set_voting_proxy)
        (set_desired_miner_count)
        (search_miner_voting)

        //Seeding
        (list_seeders_by_price)
        (list_seeders_by_upload)
        (list_seeders_by_region)
        (list_seeders_by_rating)

        //Proposals
        (get_proposed_transactions)
        (propose_transfer)
        (propose_parameter_change)
        (propose_fee_change)
        (approve_proposal)

        //Content
        (submit_content)
        (submit_content_async)
        (content_cancellation)
        (download_content)
        (get_download_status)
        (request_to_buy)
        (leave_rating_and_comment)
        (restore_encryption_key)
        (generate_encryption_key)
        (get_open_buyings)
        (get_open_buyings_by_URI)
        (get_open_buyings_by_consumer)
        (get_buying_history_objects_by_consumer)
        (search_my_purchases)
        (get_buying_by_consumer_URI)
        (search_feedback)
        (get_content)
        (search_content)
        (search_user_content)
        (get_author_and_co_authors_by_URI)
        (create_package)
        (extract_package)
        (download_package)
        (upload_package)
        (remove_package)
        (generate_content_keys)
        (is_package_manager_task_waiting)

        //Subscription
        (subscribe_to_author)
        (subscribe_by_author)
        (set_subscription)
        (set_automatic_renewal_of_subscription)
        (list_active_subscriptions_by_consumer)
        (list_subscriptions_by_consumer)
        (list_active_subscriptions_by_author)
        (list_subscriptions_by_author)

        //Messaging
        (send_message)
        (send_unencrypted_message)
        (get_message_objects)
        (get_messages)
        (get_sent_messages)

        //Monitoring
        (reset_counters)
        (get_counters)
   )
