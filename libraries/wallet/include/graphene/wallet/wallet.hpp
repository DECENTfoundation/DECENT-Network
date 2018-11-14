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
#include <decent/encrypt/encryptionutils.hpp>
#include <graphene/chain/transaction_detail_object.hpp>


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

      object* create_object( const variant& v );

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
            auto& idx = my_accounts.get<by_id>();
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
         string                    packages_path = "./packages/";
         string                    libtorrent_config_path;
         string                    update_time;
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

      struct signed_block_with_info : public signed_block
      {
         signed_block_with_info( const signed_block& block );
         signed_block_with_info( const signed_block_with_info& block ) = default;

         block_id_type block_id;
         public_key_type signing_key;
         vector< transaction_id_type > transaction_ids;
         graphene::chain::share_type miner_reward;
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


      struct balance_operation_detail
      {
          string                   memo;
          string                   description;
          operation_history_object hist_object;
          asset_array balance;
          asset fee;
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

      class wallet_api
      {
      public:
         wallet_api( const wallet_data& initial_data, fc::api<login_api> rapi );
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

FC_REFLECT( graphene::wallet::balance_operation_detail, (memo)(description)(hist_object)(balance)(fee) )

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


FC_REFLECT_DERIVED( graphene::wallet::signed_block_with_info, (graphene::chain::signed_block),
                    (block_id)(signing_key)(transaction_ids)(miner_reward) )

FC_REFLECT_DERIVED( graphene::wallet::vesting_balance_object_with_info, (graphene::chain::vesting_balance_object),
                    (allowed_withdraw)(allowed_withdraw_time) )

FC_REFLECT( graphene::wallet::miner_voting_info, (id)(name)(url)(total_votes)(voted) )

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

        //Wallet file
        (list_my_accounts)
        (get_private_key)
        (is_new)
        (is_locked)
        (lock)(unlock)(set_password)
        (load_wallet_file)
        (save_wallet_file)
        (import_key)
        (dump_private_keys)

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
        (suggest_brain_key)
        (register_account)
        (create_account_with_brain_key)
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
        (seeding_startup)
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

#if 0
        //Debug
        (dbg_make_mia)
        (dbg_push_blocks)
        (dbg_generate_blocks)
        (dbg_stream_json_objects)
        (dbg_update_object)
        (set_transfer_logs)
        (sign_buffer)
        (verify_signature)

        //Network
        (flood_network)
#endif


