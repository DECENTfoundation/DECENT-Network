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

#include <graphene/app/full_account.hpp>

#include <graphene/chain/protocol/types.hpp>

#include <graphene/chain/database.hpp>

#include <graphene/chain/account_object.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/message_object.hpp>
#include <graphene/chain/chain_property_object.hpp>
#include <graphene/chain/operation_history_object.hpp>
#include <graphene/chain/proposal_object.hpp>
#include <graphene/chain/miner_object.hpp>
#include <graphene/chain/buying_object.hpp>
#include <graphene/chain/content_object.hpp>
#include <graphene/chain/seeder_object.hpp>
#include <graphene/chain/budget_record_object.hpp>
#include <graphene/chain/subscription_object.hpp>
#include <graphene/chain/transaction_detail_object.hpp>
#include <graphene/chain/transaction_history_object.hpp>

#include <fc/api.hpp>
#include <fc/optional.hpp>
#include <fc/variant_object.hpp>

#include <fc/network/ip.hpp>

#include <boost/container/flat_set.hpp>

#include <functional>
#include <map>
#include <memory>
#include <vector>

/**
 * @defgroup DatabaseAPI Database API
 * @{
 * @defgroup DatabaseAPI_Subscription Subscription
 * @defgroup DatabaseAPI_BlockTx Blocks and Transactions
 * @defgroup DatabaseAPI_Globals Globals
 * @defgroup DatabaseAPI_Key Key
 * @defgroup DatabaseAPI_Account Account
 * @defgroup DatabaseAPI_Balance Balance
 * @defgroup DatabaseAPI_Asset Asset
 * @defgroup DatabaseAPI_Mining Mining
 * @defgroup DatabaseAPI_AuthValidation Authority and Validation
 * @defgroup DatabaseAPI_Proposals Proposals
 * @defgroup DatabaseAPI_Decent Decent
 * @}
 */
namespace graphene { namespace app {

      using namespace graphene::chain;
      using namespace std;

      class database_api_impl;

      struct order
      {
         double                     price;
         double                     quote;
         double                     base;
      };

      struct order_book
      {
         string                      base;
         string                      quote;
         vector< order >             bids;
         vector< order >             asks;
      };

      struct market_ticker
      {
         string                     base;
         string                     quote;
         double                     latest;
         double                     lowest_ask;
         double                     highest_bid;
         double                     percent_change;
         double                     base_volume;
         double                     quote_volume;
      };

      struct market_volume
      {
         string                     base;
         string                     quote;
         double                     base_volume;
         double                     quote_volume;
      };

      struct market_trade
      {
         fc::time_point_sec         date;
         double                     price;
         double                     amount;
         double                     value;
      };

      struct miner_voting_info
      {
         miner_id_type id;
         string name;
         string url;
         uint64_t total_votes;
         bool voted;
      };

/**
 * @brief The database_api class implements the RPC API for the chain database.
 *
 * This API exposes accessors on the database which query state tracked by a blockchain validating node. This API is
 * read-only; all modifications to the database must be performed via transactions. Transactions are broadcast via
 * the \c network_broadcast_api.
 */
      class database_api
      {
      public:
         database_api(graphene::chain::database& db);
         ~database_api();

         /////////////
         // Objects //
         /////////////

         /**
          * @brief Get the objects corresponding to the provided IDs.
          * @note If any of the provided IDs does not map to an object, a null variant is returned in its position.
          * @param ids IDs of the objects to retrieve
          * @return the objects retrieved, in the order they are mentioned in ids
          * @ingroup DatabaseAPI_Globals
          */
         fc::variants get_objects(const vector<object_id_type>& ids)const;

         ///////////////////
         // Subscriptions //
         ///////////////////

         /**
          *
          * @param cb
          * @param clear_filter
          * @ingroup DatabaseAPI_Subscription
          */
         void set_subscribe_callback( std::function<void(const variant&)> cb, bool clear_filter );

         /**
          *
          * @param cb
          * @ingroup DatabaseAPI_Subscription
          */
         void set_pending_transaction_callback( std::function<void(const variant&)> cb );

         /**
          *
          * @param cb
          * @ingroup DatabaseAPI_Subscription
          */
         void set_block_applied_callback( std::function<void(const variant& block_id)> cb );

         /**
          *
          * @param URI content we are monitoring
          * @param cb callback
          * @ingroup DatabaseAPI_Subscription
          */
         void set_content_update_callback( std::function<void()>cb, const string & URI );

         /**
          * @brief Stop receiving any notifications.
          * @note This unsubscribes from all subscribed objects.
          * @ingroup DatabaseAPI_Subscription
          */
         void cancel_all_subscriptions();

         /////////////////////////////
         // Blocks and transactions //
         /////////////////////////////

         /**
          * @brief Retrieve a block header.
          * @param block_num height of the block whose header should be returned
          * @return header of the referenced block, or \c null if no matching block was found
          * @ingroup DatabaseAPI_BlockTx
          */
         optional<block_header> get_block_header(uint32_t block_num)const;

         /**
          * @brief Retrieve a full, signed block.
          * @param block_num height of the block to be returned
          * @return the referenced block, or \c null if no matching block was found
          * @ingroup DatabaseAPI_BlockTx
          */
         optional<signed_block> get_block(uint32_t block_num)const;

         /**
          * @brief Used to fetch an individual transaction.
          * @param block_num id of the block
          * @param trx_in_block specifies the position of the transaction within the block
          * @return the transaction
          * @ingroup DatabaseAPI_BlockTx
          */
         processed_transaction get_transaction( uint32_t block_num, uint32_t trx_in_block )const;

         /**
          * @brief Query the last local block.
          * @return the block time
          * @ingroup DatabaseAPI_BlockTx
          */
         fc::time_point_sec head_block_time()const;

         /**
          * @brief If the transaction has not expired, this method will return the transaction for the given ID or
          * it will return \c null if it is not known. Just because it is not known does not mean it wasn't
          * included in the blockchain.
          * @param id ID of the transaction to retrieve
          * @ingroup DatabaseAPI_BlockTx
          */
         optional<signed_transaction> get_recent_transaction_by_id( const transaction_id_type& id )const;

         /**
          * @brief This method will return the transaction for the given ID (transaction hash) or
          * it will return \c null if it is not known. Just because it is not known does not mean it wasn't
          * included in the blockchain.
          * @note By default these objects are not tracked, the transaction_history_plugin must
          * be loaded for these objects to be maintained.
          * @param id ID of the transaction to retrieve
          * @ingroup DatabaseAPI_BlockTx
          */
         optional<signed_transaction> get_transaction_by_id( const transaction_id_type& id )const;

         /////////////
         // Globals //
         /////////////

         /**
          * @brieg Get the name of the API.
          * @return the name of the API
          * @ingroup DatabaseAPI_Globals
          */
         std::string info()const;

         /**
          * @brief Retrieve the \c chain_property_object associated with the chain.
          * @return chain id and immutable chain parameters
          * @ingroup DatabaseAPI_Globals
          */
         chain_property_object get_chain_properties()const;

         /**
          * @brief Retrieve the current \c global_property_object. This object contains all of the properties
          * of the blockchain that are fixed or that change only once per maintenance interval such as the current
          * list of miners, block interval, etc.
          * @return the global properties
          * @ingroup DatabaseAPI_Globals
          */
         global_property_object get_global_properties()const;

         /**
          * @brief Retrieve compile-time constants.
          * @return configured constants
          * @ingroup DatabaseAPI_Globals
          */
         fc::variant_object get_config()const;

         /**
          * @brief Get the chain ID
          * @return the chain ID identifying blockchain network
          * @ingroup DatabaseAPI_Globals
          */
         chain_id_type get_chain_id()const;

         /**
          * @brief Retrieve the current dynamic_global_property_object. The returned object contains information
          * that changes every block interval such as the head block number, the next miner, etc.
          * @return the dynamic properties
          * @ingroup DatabaseAPI_Globals
          */
         dynamic_global_property_object get_dynamic_global_properties()const;

         //////////
         // Keys //
         //////////

         /**
          * @brief Get all accounts that refer to the key in their owner or active authorities.
          * @param key list of public keys
          * @return list of lists of account IDs. One list of account IDs per public key
          * @ingroup DatabaseAPI_Key
          */
         vector<vector<account_id_type>> get_key_references( vector<public_key_type> key )const;

         //////////////
         // Accounts //
         //////////////

         /**
          * @brief Get a list of accounts by ID.
          * @note This function has semantics identical to \c get_objects().
          * @param account_ids IDs of the accounts to retrieve
          * @return the accounts corresponding to the provided IDs
          * @ingroup DatabaseAPI_Account
          */
         vector<optional<account_object>> get_accounts(const vector<account_id_type>& account_ids)const;

         /**
          * @brief Fetch all objects relevant to the specified accounts and subscribe to updates.
          * @note This function fetches all relevant objects for the given accounts, and subscribes to updates to the given
          * accounts. If any of the \c strings in \c names_or_ids cannot be tied to an account, that input will be
          * ignored. All other accounts will be retrieved and subscribed.
          * @param subscribe \c true to subscribe to updates
          * @param names_or_ids each item must be the name or ID of an account to retrieve
          * @return map of \c string from \c names_or_ids to the corresponding account
          * @ingroup DatabaseAPI_Account
          */
         std::map<string,full_account> get_full_accounts( const vector<string>& names_or_ids, bool subscribe );

         /**
          * @brief Get an account by name.
          * @param name name of the account to retrieve
          * @return the \c account_object corresponding to the provided \c name, or \c null if no matching content was found
          * @ingroup DatabaseAPI_Account
          */
         optional<account_object> get_account_by_name( string name )const;

         /**
          * @brief Get all accounts that refer to the account id in their owner or active authorities.
          * @param account_id ID of the account
          * @return a list of account IDs
          * @ingroup DatabaseAPI_Account
          */
         vector<account_id_type> get_account_references( account_id_type account_id )const;

         /**
          * @brief Get a list of accounts by name.
          * @note This function has semantics identical to \c get_objects().
          * @param account_names names of the accounts to retrieve
          * @return the accounts holding the provided names
          * @ingroup DatabaseAPI_Account
          */
         vector<optional<account_object>> lookup_account_names(const vector<string>& account_names)const;

         /**
          * @brief Get names and IDs for registered accounts.
          * @param lower_bound_name lower bound of the first name to return
          * @param limit maximum number of results to return ( must not exceed 1000 )
          * @return map of account names to corresponding IDs
          * @ingroup DatabaseAPI_Account
          */
         map<string,account_id_type> lookup_accounts(const string& lower_bound_name, uint32_t limit)const;

         /**
          * @brief Get names and IDs for registered accounts that match search term.
          * @param search_term will try to partially match account name or id
          * @param order sort data by field
          * @param id \c object_id to start searching from
          * @param limit maximum number of results to return ( must not exceed 1000 )
          * @return map of account names to corresponding IDs
          * @ingroup DatabaseAPI_Account
          */
         vector<account_object> search_accounts(const string& search_term, const string order, const object_id_type& id, uint32_t limit) const;

         /**
          * @brief Returns the operations on the named account.
          * @note This returns a list of transaction detail objects, which describe activity on the account.
          * @param account the account to search
          * @param order sort data by field
          * @param id \c object_id to start searching from
          * @param limit the number of entries to return (starting from the most recent) (max 100)
          * @return a list of transaction detail objects
          * @ingroup DatabaseAPI_Account
          */
         vector<class transaction_detail_object> search_account_history(account_id_type const& account,
                                                                        string const& order,
                                                                        object_id_type const& id,
                                                                        int limit) const;

         //////////////
         // Balances //
         //////////////

         /**
          * @brief Get an account's balances in various assets.
          * @param id ID of the account to get balances for
          * @param assets IDs of the assets to get balances of; if empty, get all assets account has a balance in
          * @return a list of the given account's balances
          * @ingroup DatabaseAPI_Balance
          */
         vector<asset> get_account_balances(account_id_type id, const flat_set<asset_id_type>& assets)const;

         /**
          * @brief Semantically equivalent to \c get_account_balances(), but takes a name instead of an ID.
          * @param name the name of the account to get balances for
          * @param assets IDs of the assets to get balances of; if empty, get all assets account has a balance in
          * @return a list of the given account's balances
          * @ingroup DatabaseAPI_Balance
          */
         vector<asset> get_named_account_balances(const std::string& name, const flat_set<asset_id_type>& assets)const;

         /**
          * @brief Get information about a vesting balance object.
          * @param account_id an account ID
          * @return a list of vesting balances with additional information
          * @ingroup DatabaseAPI_Balance
          */
         vector<vesting_balance_object> get_vesting_balances( account_id_type account_id )const;

         /**
          * @brief Get the total number of accounts registered on the blockchain.
          * @return the total number of registered accounts
          * @ingroup DatabaseAPI_Account
          */
         uint64_t get_account_count()const;

         ////////////
         // Assets //
         ////////////

         /**
          * @brief Get a list of assets by ID.
          * @note This function has semantics identical to \c get_objects().
          * @param asset_ids IDs of the assets to retrieve
          * @return the assets corresponding to the provided IDs
          * @ingroup DatabaseAPI_Asset
          */
         vector<optional<asset_object>> get_assets(const vector<asset_id_type>& asset_ids)const;

         /**
          * @brief Get assets alphabetically by symbol name.
          * @param lower_bound_symbol lower bound of symbol names to retrieve
          * @param limit maximum number of assets to fetch (must not exceed 100)
          * @return the assets found
          * @ingroup DatabaseAPI_Asset
          */
         vector<asset_object> list_assets(const string& lower_bound_symbol, uint32_t limit)const;

         /**
          * @brief Get a list of assets by symbol.
          * @note This function has semantics identical to \c get_objects().
          * @param symbols_or_ids symbols or stringified IDs of the assets to retrieve
          * @return the assets corresponding to the provided symbols or IDs
          * @ingroup DatabaseAPI_Asset
          */
         vector<optional<asset_object>> lookup_asset_symbols(const vector<string>& symbols_or_ids)const;

         /**
          * @brief Returns a reward for a miner from the most recent block.
          * @return amount of newly generated DCT
          * @ingroup DatabaseAPI_Mining
          */
         share_type get_new_asset_per_block() const;

         /**
          * @brief Returns a reward for a miner from a specified block.
          * @note A reward from transaction fees is not included.
          * @param block_num the block number
          * @return amount of newly generated DCT
          * @ingroup DatabaseAPI_Mining
          */
         share_type get_asset_per_block_by_block_num(uint32_t block_num) const;

         /**
          * @brief Converts asset into DCT, using actual price feed.
          * @param price asset in DCT, monitored asset or user issued asset
          * @return price in DCT
          * @ingroup DatabaseAPI_Asset
          */
         asset price_to_dct( asset price )const;

         ///////////////
         // Miners //
         ///////////////

         /**
          * @brief Get a list of miners by ID.
          * @note This function has semantics identical to \c get_objects().
          * @param miner_ids IDs of the miners to retrieve
          * @return the miners corresponding to the provided IDs
          * @ingroup DatabaseAPI_Mining
          */
         vector<optional<miner_object>> get_miners(const vector<miner_id_type>& miner_ids)const;

         /**
          * @brief Get the miner owned by a given account.
          * @param account the ID of the account whose miner should be retrieved
          * @return the miner object, or \c null if the account does not have a miner
          * @ingroup DatabaseAPI_Mining
          */
         fc::optional<miner_object> get_miner_by_account(account_id_type account)const;

         /**
          * @brief Get names and IDs for registered miners.
          * @param lower_bound_name lower bound of the first name to return
          * @param limit maximum number of results to return ( must not exceed 1000 )
          * @return map of miner names to corresponding IDs
          * @ingroup DatabaseAPI_Mining
          */
         map<string, miner_id_type> lookup_miner_accounts(const string& lower_bound_name, uint32_t limit)const;

         /**
          * @brief Get the total number of miners registered with the blockchain.
          * @return the total number of registered miners
          * @ingroup DatabaseAPI_Mining
          */
         uint64_t get_miner_count()const;

         /**
          * @brief Get a list of published price feeds by a miner.
          * @param account_id id of the account
          * @param count maximum number of price feeds to fetch (must not exceed 100)
          * @returns a list of price feeds published by the miner
          * @ingroup DatabaseAPI_Mining
          */
         multimap< time_point_sec, price_feed> get_feeds_by_miner(const account_id_type account_id,
                                                                  const uint32_t count)const;

         ///////////
         // Votes //
         ///////////

         /**
          * @brief Given a set of votes, return the objects they are voting for.
          * @note The results will be in the same order as the votes. \c null will be returned for
          * any vote ids that are not found.
          * @param votes set of votes
          * @ingroup DatabaseAPI_Mining
          */
         vector<variant> lookup_vote_ids( const vector<vote_id_type>& votes )const;

         ////////////////////////////
         // Authority / validation //
         ////////////////////////////

         /**
          * @brief Get a hexdump of the serialized binary form of a transaction.
          * @param trx the signed transaction
          * @return hexadecimal string
          * @ingroup DatabaseAPI_AuthValidation
          */
         std::string get_transaction_hex(const signed_transaction& trx)const;

         /**
          *  @brief This API will take a partially signed transaction and a set of public keys that the owner has the ability to sign for
          *  and return the minimal subset of public keys that should add signatures to the transaction.
          *  @param trx the signed transaction
          *  @param available_keys set of available public keys
          *  @ingroup DatabaseAPI_AuthValidation
          */
         set<public_key_type> get_required_signatures( const signed_transaction& trx, const flat_set<public_key_type>& available_keys )const;

         /**
          *  @brief This method will return the set of all public keys that could possibly sign for a given transaction. This call can
          *  be used by wallets to filter their set of public keys to just the relevant subset prior to calling \c get_required_signatures()
          *  to get the minimum subset.
          *  @param trx the signed transaction
          *  @ingroup DatabaseAPI_AuthValidation
          */
         set<public_key_type> get_potential_signatures( const signed_transaction& trx )const;

         /**
          * @brief Verifies required signatures of a transaction.
          * @param trx the signed transaction
          * @return \c true if the \c trx has all of the required signatures, otherwise throws an exception
          * @ingroup DatabaseAPI_AuthValidation
          */
         bool           verify_authority( const signed_transaction& trx )const;

         /**
          * @brief Verifies if the signers have enough authority to authorize an account.
          * @param name_or_id the name or id of the account
          * @param signers set of public keys
          * @return \c true if the signers have enough authority
          * @ingroup DatabaseAPI_AuthValidation
          */
         bool           verify_account_authority( const string& name_or_id, const flat_set<public_key_type>& signers )const;

         /**
          *  @brief Validates a transaction against the current state without broadcasting it on the network.
          *  @param trx the signed transaction
          *  @ingroup DatabaseAPI_AuthValidation
          */
         processed_transaction validate_transaction( const signed_transaction& trx )const;

         /**
          *  @brief For each operation calculates the required fee in the specified asset type.
          *  @param ops the set of operations
          *  @param id the asset ID
          *  @return the required fees
          *  @ingroup DatabaseAPI_AuthValidation
          */
         fc::variants get_required_fees( const vector<operation>& ops, asset_id_type id )const;

         ///////////////////////////
         // Proposed transactions //
         ///////////////////////////

         /**
          * @brief Get the set of proposed transactions relevant to the specified account id.
          * @param id the account ID
          * @return a set of proposed transactions
          * @ingroup DatabaseAPI_Proposals
          */
         vector<proposal_object> get_proposed_transactions( account_id_type id )const;

         ////////////
         // Decent //
         ////////////


         /**
          * @brief Return current core asset supply.
          * @return current supply
          * @ingroup DatabaseAPI_Asset
          */
         real_supply get_real_supply()const;

         /**
          * @brief Get a list of accounts holding publishing manager status.
          * @param lower_bound_name the name of the first account to return. If the named account does not exist,
          * the list will start at the account that comes after \c lowerbound
          * @param limit the maximum number of accounts to return (max: 100)
          * @return a list of publishing managers
          * @ingroup DatabaseAPI_Decent
          */
         vector<account_id_type> list_publishing_managers( const string& lower_bound_name, uint32_t limit  )const;

         /**
          * @brief Get a list of open buyings.
          * @return a list of buying objects
          * @ingroup DatabaseAPI_Decent
          */
         vector<buying_object> get_open_buyings()const;

         /**
          * @brief Get a list of open buyings by URI.
          * @param URI URI of the buyings to retrieve
          * @return a list of buyings corresponding to the provided URI
          * @ingroup DatabaseAPI_Decent
          */
         vector<buying_object> get_open_buyings_by_URI( const string& URI )const;

         /**
          * @brief Get a list of open buyings by consumer.
          * @param consumer consumer of the buyings to retrieve
          * @return a list of buyings corresponding to the provided consumer
          * @ingroup DatabaseAPI_Decent
          */
         vector<buying_object> get_open_buyings_by_consumer( const account_id_type& consumer )const;

         /**
          * @brief Get history buying objects by consumer.
          * @param consumer consumer of the history buyings to retrieve
          * @return history buying objects corresponding to the provided consumer
          * @ingroup DatabaseAPI_Decent
          */
         vector<buying_object> get_buying_history_objects_by_consumer( const account_id_type& consumer )const;

         /**
          * @brief Get buying objects (open or history) by consumer.
          * @param consumer consumer of the buyings to retrieve
          * @param order ordering field. Available options are defined in 'database_api.cpp'
          * @param id the id of buying object to start searching from
          * @param term search term
          * @param count maximum number of contents to fetch (must not exceed 100)
          * @return buying objects corresponding to the provided consumer
          * @ingroup DatabaseAPI_Decent
          */
         vector<buying_object> get_buying_objects_by_consumer(const account_id_type& consumer,
                                                              const string& order,
                                                              const object_id_type& id,
                                                              const string& term,
                                                              uint32_t count)const;

         /**
          * @brief Get buying (open or history) by consumer and URI.
          * @param consumer consumer of the buying to retrieve
          * @param URI URI of the buying to retrieve
          * @return buying object corresponding to the provided consumer and URI
          * @ingroup DatabaseAPI_Decent
          */
         optional<buying_object> get_buying_by_consumer_URI( const account_id_type& consumer, const string& URI )const;

         /**
          * @brief Search for term in contents (author, title and description).
          * @param user feedback author
          * @param URI the content object uri
          * @param id the id of feedback object to start searching from
          * @param count maximum number of feedbacks to fetch
          * @return the feedback found
          * @ingroup DatabaseAPI_Decent
          */
         vector<buying_object> search_feedback(const string& user,
                                               const string& URI,
                                               const object_id_type& id,
                                               uint32_t count) const;

         /**
          * @brief Get a content by URI.
          * @param URI URI of the content to retrieve
          * @return the content corresponding to the provided URI, or \c null if no matching content was found
          * @ingroup DatabaseAPI_Decent
          */
         optional<content_object> get_content( const string& URI )const;
         
         /**
          * @brief Generate keys for new content submission.
          * @param seeders list of seeder account IDs
          * @return generated key, key parts and quorum
          * @ingroup DatabaseAPI_Decent
          */
         content_keys generate_content_keys(vector<account_id_type> const& seeders)const;
         
         /**
          * @brief Restores encryption key from key parts stored in buying object.
          * @param el_gamal_priv_key_string the private El Gamal key
          * @param buying the buying object
          * @return AES encryption key
          * @ingroup DatabaseAPI_Decent
          */
         fc::sha256 restore_encryption_key(DIntegerString el_gamal_priv_key_string, buying_id_type buying ) const;

         /**
          * @brief Search for term in contents (author, title and description).
          * @param term search term
          * @param order ordering field. Available options are defined in 'database_api.cpp'
          * @param user content owner
          * @param region_code two letter region code
          * @param id the id of content object to start searching from
          * @param type the application and content type to be filtered
          * @param count maximum number of contents to fetch (must not exceed 100)
          * @return the contents found
          * @ingroup DatabaseAPI_Decent
          */
         vector<content_summary> search_content(const string& term,
                                                const string& order,
                                                const string& user,
                                                const string& region_code,
                                                const object_id_type& id,
                                                const string& type,
                                                uint32_t count )const;

         /**
          * @brief Get a list of seeders by price, in increasing order.
          * @param count maximum number of seeders to retrieve
          * @return the seeders found
          * @ingroup DatabaseAPI_Decent
          */
         vector<seeder_object> list_seeders_by_price( uint32_t count )const;

         /**
          * @brief Get a list of seeders by price, in increasing order. Same method as list_seeders_by_price, kept for compatibility.
          * @param count maximum number of seeders to retrieve
          * @return the seeders found
          * @ingroup DatabaseAPI_Decent
          */
         vector<seeder_object> list_publishers_by_price( uint32_t count )const;

         /**
          * @brief Get a seeder by ID.
          * @param aid ID of the seeder to retrieve
          * @return the seeder corresponding to the provided ID, or \c null if no matching content was found
          * @ingroup DatabaseAPI_Decent
          */
         optional<seeder_object> get_seeder(account_id_type aid) const;

         /**
          * @brief Get a list of seeders by total upload, in decreasing order.
          * @param count maximum number of seeders to retrieve
          * @return the seeders found
          * @ingroup DatabaseAPI_Decent
          */
         optional<vector<seeder_object>> list_seeders_by_upload( const uint32_t count )const;

         /**
          * @brief Get a list of seeders by region code.
          * @param region_code region code of seeders to retrieve
          * @return the seeders found
          * @ingroup DatabaseAPI_Decent
          */
         vector<seeder_object> list_seeders_by_region( const string region_code )const;

         /**
          * @brief Get a list of seeders by rating, in decreasing order.
          * @param count maximum number of seeders to retrieve
          * @return the seeders found
          * @ingroup DatabaseAPI_Decent
          */
         vector<seeder_object> list_seeders_by_rating( const uint32_t count )const;

         /**
          * @brief Get a subscription object by ID.
          * @param sid ID of the subscription to retrieve
          * @return the subscription object corresponding to the provided ID, or null if no matching subscription was found
          * @ingroup DatabaseAPI_Decent
          */
         optional<subscription_object> get_subscription( const subscription_id_type& sid)const;

         /**
          * @brief Get a list of active (not expired) subscriptions subscribed by account (consumer).
          * @param account ID of the account to get subscriptions for
          * @param count maximum number of subscription objects to fetch (must not exceed 100)
          * @return a list of subscription objects corresponding to the provided consumer
          * @ingroup DatabaseAPI_Decent
          */
         vector<subscription_object> list_active_subscriptions_by_consumer( const account_id_type& account, const uint32_t count )const;

         /**
          * @brief Get a list of subscriptions subscribed by account (consumer).
          * @param account ID of the account to get subscriptions for
          * @param count maximum number of subscription objects to fetch (must not exceed 100)
          * @return the contents found
          * @ingroup DatabaseAPI_Decent
          */
         vector<subscription_object> list_subscriptions_by_consumer( const account_id_type& account, const uint32_t count )const;

         /**
          * @brief Get a list of active (not expired) subscriptions to account (author).
          * @param account ID of the account to get subscriptions for
          * @param count maximum number of subscription objects to fetch (must not exceed 100)
          * @return the contents found
          * @ingroup DatabaseAPI_Decent
          */
         vector<subscription_object> list_active_subscriptions_by_author( const account_id_type& account, const uint32_t count )const;

         /**
          * @brief Get a list of  subscriptions subscribed to account (author).
          * @param account ID of the account to get subscriptions for
          * @param count maximum number of subscription objects to fetch (must not exceed 100)
          * @return the contents found
          * @ingroup DatabaseAPI_Decent
          */
         vector<subscription_object> list_subscriptions_by_author( const account_id_type& account, const uint32_t count )const;

         /**
          * @brief Get remaining time to next maintenance interval from given time.
          * @param block_time reference time
          * @return remaining time to next maintenance interval along with some additional data
          * @ingroup DatabaseAPI_Globals
          */
         miner_reward_input get_time_to_maint_by_block_time(fc::time_point_sec block_time) const;

         /**
          * @brief Get the number of votes each miner actually has.
          * @return a list mapping account names to the number of votes
          * @ingroup DatabaseAPI_Mining
          */
         vector<database::votes_gained> get_actual_votes() const;

         /**
          * @brief Get miner voting info list by account that match search term.
          * @param account_id account name or empty when search without account
          * @param term search term - miner name
          * @param only_my_votes when \c true it selects only votes given by account
          * @param order order field. Available options are 'name\link|votes'
          * @param id the id of the miner to start searching from, or empty when start from beginning
          * @param count maximum number of miners info to fetch (must not exceed 1000)
          * @return the contents found
          * @ingroup DatabaseAPI_Mining
          */
         vector<miner_voting_info> search_miner_voting(const string& account_id,
                                                       const string& term,
                                                       bool only_my_votes,
                                                       const string& order,
                                                       const string& id,
                                                       uint32_t count ) const;

      private:
         std::shared_ptr< database_api_impl > my;
      };

   } }

FC_REFLECT( graphene::app::order, (price)(quote)(base) );
FC_REFLECT( graphene::app::order_book, (base)(quote)(bids)(asks) );
FC_REFLECT( graphene::app::market_ticker, (base)(quote)(latest)(lowest_ask)(highest_bid)(percent_change)(base_volume)(quote_volume) );
FC_REFLECT( graphene::app::market_volume, (base)(quote)(base_volume)(quote_volume) );
FC_REFLECT( graphene::app::market_trade, (date)(price)(amount)(value) );
FC_REFLECT( graphene::app::miner_voting_info, (id)(name)(url)(total_votes)(voted) );

FC_API(graphene::app::database_api,
          // Objects
          (get_objects)

          // Subscriptions
          (set_subscribe_callback)
          (set_pending_transaction_callback)
          (set_block_applied_callback)
          (cancel_all_subscriptions)
          (set_content_update_callback)

          // Blocks and transactions
          (get_block_header)
          (get_block)
          (get_transaction)
          (head_block_time)
          (get_recent_transaction_by_id)
          (get_new_asset_per_block)
          (get_asset_per_block_by_block_num)
          (get_time_to_maint_by_block_time)
          (get_transaction_by_id)

          // Globals
          (info)
          (get_chain_properties)
          (get_global_properties)
          (get_config)
          (get_chain_id)
          (get_dynamic_global_properties)

          // Keys
          (get_key_references)

          // Accounts
          (get_accounts)
          (get_full_accounts)
          (get_account_by_name)
          (get_account_references)
          (lookup_account_names)
          (lookup_accounts)
          (search_accounts)
          (get_account_count)
          (search_account_history)

          // Balances
          (get_account_balances)
          (get_named_account_balances)
          (get_vesting_balances)

          // Assets
          (get_assets)
          (list_assets)
          (lookup_asset_symbols)
          (price_to_dct)

          // Miners
          (get_miners)
          (get_miner_by_account)
          (lookup_miner_accounts)
          (get_miner_count)
          (get_feeds_by_miner)

          // Votes
          (lookup_vote_ids)
          (search_miner_voting)

          // Authority / validation
          (get_transaction_hex)
          (get_required_signatures)
          (get_potential_signatures)
          (verify_authority)
          (verify_account_authority)
          (validate_transaction)
          (get_required_fees)

          // Proposed transactions
          (get_proposed_transactions)

          // Decent
          (list_publishing_managers)
          (get_open_buyings)
          (get_open_buyings_by_URI)
          (get_open_buyings_by_consumer)
          (get_buying_by_consumer_URI)
          (get_buying_history_objects_by_consumer)
          (get_buying_objects_by_consumer)
          (search_feedback)
          (get_content)
          (generate_content_keys)
          (restore_encryption_key)
          (search_content)
          (list_publishers_by_price)
          (list_seeders_by_price)
          (list_seeders_by_upload)
          (list_seeders_by_region)
          (list_seeders_by_rating)
          (get_seeder)
          (get_real_supply)
          (get_subscription)
          (list_active_subscriptions_by_consumer)
          (list_subscriptions_by_consumer)
          (list_active_subscriptions_by_author)
          (list_subscriptions_by_author)
          (get_actual_votes)

)
