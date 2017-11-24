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

      struct plain_keys
      {
         map<public_key_type, string>  keys;
         fc::sha512                    checksum;
      };

      struct brain_key_info
      {
         string brain_priv_key;
         string wif_priv_key;
         public_key_type pub_key;
      };

      struct regional_price_info
      {
         string region;
         string amount;
         string asset_symbol;
      };


      struct operation_detail {
         string                   memo;
         string                   description;
         operation_history_object op;
      };

      struct wallet_data
      {
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
   
   

      struct approval_delta
      {
         vector<string> active_approvals_to_add;
         vector<string> active_approvals_to_remove;
         vector<string> owner_approvals_to_add;
         vector<string> owner_approvals_to_remove;
         vector<string> key_approvals_to_add;
         vector<string> key_approvals_to_remove;
      };

      struct content_download_status
      {
         int          total_key_parts;
         int          received_key_parts;
         int          total_download_bytes;
         int          received_download_bytes;
         std::string  status_text;
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
 * @defgroup WalletAPI_Seeding Seeding
 * @defgroup WalletAPI_Mining Mining
 * @defgroup WalletAPI_Proposals Proposals
 * @defgroup WalletAPI_TransactionBuilder Transaction Builder
 * @defgroup WalletAPI_Network Network
 * @defgroup WalletAPI_Debug Debug
 * @}
 */

      class wallet_api
      {
      public:
         wallet_api( const wallet_data& initial_data, fc::api<login_api> rapi );
         virtual ~wallet_api();

         /**
          * @brief Copy wallet file to a new file.
          * @param destination_filename
          * @return \c true if the wallet is copied,\c false otherwise
          * @ingroup WalletAPI_Wallet
          */
         bool copy_wallet_file( string destination_filename );

         /**
          * @brief Derive private key from given prefix and sequence.
          * @param prefix_string
          * @param sequence_number
          * @return derived private key
          * @ingroup WalletAPI_Account
          */
         fc::ecc::private_key derive_private_key(const std::string& prefix_string, int sequence_number) const;

         /**
          * @brief Lists all available commands.
          * @return a list of all available commands
          * @ingroup WalletAPI_General
          */
         variant                           info();

         /**
          * @brief Returns info such as client version, git version of graphene/fc, version of boost, openssl.
          * @return compile time info and client and dependencies versions
          * @ingroup WalletAPI_General
          */
         variant_object                    about() const;

         /**
          * @brief Retrieve a full, signed block with info.
          * @param num ID/height of the block
          * @return the referenced block with info, or \c null if no matching block was found
          * @ingroup WalletAPI_General
          */
         optional<signed_block_with_info>    get_block( uint32_t num );

         /**
          * @brief Returns the number of accounts registered on the blockchain.
          * @return the number of registered accounts
          * @ingroup WalletAPI_Account
          */
         uint64_t                          get_account_count()const;

         /**
          * @brief Lists all accounts controlled by this wallet.
          * This returns a list of the full account objects for all accounts whose private keys
          * we possess.
          * @return a list of accounts imported in the wallet
          * @ingroup WalletAPI_Wallet
          */
         vector<account_object>            list_my_accounts();

         /**
          * @brief Lists all accounts registered in the blockchain.
          * This returns a list of all account names and their account ids, sorted by account name.
          * Use the \c lowerbound and \c limit parameters to page through the list.  To retrieve all accounts,
          * start by setting \c lowerbound to the empty string \c "", and then each iteration, pass
          * the last account name returned as the \c lowerbound for the next \c list_accounts() call.
          * @param lowerbound the name of the first account to return.  If the named account does not exist,
          *                   the list will start at the account that comes after \c lowerbound
          * @param limit the maximum number of accounts to return (max: 1000)
          * @return a list of accounts mapping account names to account ids
          * @ingroup WalletAPI_Account
          */
         map<string,account_id_type>       list_accounts(const string& lowerbound, uint32_t limit);

         /**
          * @brief Get registered accounts that match search term
          * @param term will try to partially match account name or id
          * @param limit maximum number of results to return ( must not exceed 1000 )
          * @param order sort data by field
          * @param id object_id to start searching from
          * @return map of account names to corresponding IDs
          * @ingroup WalletAPI_Account
          */
         vector<account_object>       search_accounts(const string& term, const string& order, const string& id, uint32_t limit);

         /**
          * @brief List the balances of an account.
          * Each account can have multiple balances, one for each type of asset owned by that
          * account.
          * @param id the name or id of the account whose balances you want
          * @return a list of the given account's balances
          * @ingroup WalletAPI_Account
          */
         vector<asset>                     list_account_balances(const string& id);

         /**
          * @brief Lists all assets registered on the blockchain.
          * To list all assets, pass the empty string \c "" for the \c lowerbound to start
          * at the beginning of the list, and iterate as necessary.
          * @param lowerbound  the symbol of the first asset to include in the list
          * @param limit the maximum number of assets to return (max: 100)
          * @return the list of asset objects, ordered by symbol
          * @ingroup WalletAPI_Asset
          */
         vector<asset_object>              list_assets(const string& lowerbound, uint32_t limit)const;

         /**
          * @brief Returns the operations on the named account.
          * This returns a list of transaction detail objects, which describe past the past activity on the account.
          * @param account_name the name or id of the account
          * @param order sort data by field
          * @param id object_id to start searching from
          * @param limit the number of entries to return (starting from the most recent) (max 100)
          * @return a list of transaction detail objects
          * @ingroup WalletAPI_Account
          */
         vector<class transaction_detail_object> search_account_history(string const& account_name,
                                                                        string const& order,
                                                                        string const& id,
                                                                        int limit) const;


         /** 
          * @brief Returns the most recent operations on the named account.
          * This returns a list of operation history objects, which describe activity on the account.
          * @note this API doesn't give a way to retrieve more than the most recent 100 transactions
          * @param name the name or id of the account
          * @param limit the number of entries to return (starting from the most recent)
          * @return a list of operation history objects
          * @ingroup WalletAPI_Account
          */
         vector<operation_detail>  get_account_history(string name, int limit)const;


         /**
          * @brief Returns the blockchain's slowly-changing properties.
          * This object contains all of the properties of the blockchain that are fixed
          * or that change only once per maintenance interval such as the
          * current list of miners, block interval, etc.
          * @see \c get_dynamic_global_properties() for frequently changing properties
          * @return the global properties
          * @ingroup WalletAPI_General
          */
         global_property_object            get_global_properties() const;

         /**
          * @brief Returns the blockchain's rapidly-changing properties.
          * The returned object contains information that changes every block interval
          * such as the head block number, the next miner, etc.
          * @see \c get_global_properties() for less-frequently changing properties
          * @return the dynamic global properties
          * @ingroup WalletAPI_General
          */
         dynamic_global_property_object    get_dynamic_global_properties() const;

         /**
          * @brief Returns information about the given account.
          *
          * @param account_name_or_id the name or id of the account to provide information about
          * @return the public account data stored in the blockchain
          * @ingroup WalletAPI_Account
          */
         account_object                    get_account(string account_name_or_id) const;

         /**
          * @brief Returns information about the given asset.
          * @param asset_name_or_id the symbol or id of the asset in question
          * @return the information about the asset stored in the block chain
          * @ingroup WalletAPI_Asset
          */
         asset_object                      get_asset(string asset_name_or_id) const;

         /**
          * @brief Returns the specific data for a given monitored asset.
          * @see \c get_asset() 
          * @param asset_name_or_id the symbol or id of the monitored asset in question
          * @return the specific data for this monitored asset
          * @ingroup WalletAPI_Asset
          */
         monitored_asset_options        get_monitored_asset_data(string asset_name_or_id)const;

         /**
          * @brief Lookup the id of a named account.
          * @param account_name_or_id the name of the account to look up
          * @return the id of the named account
          * @ingroup WalletAPI_Account
          */
         account_id_type                   get_account_id(string account_name_or_id) const;

         /**
          * @brief Lookup the id of a named asset.
          * @param asset_name_or_id the symbol of an asset to look up
          * @return the id of the given asset
          * @ingroup WalletAPI_Asset
          */
         asset_id_type                     get_asset_id(string asset_name_or_id) const;

         /**
          * @brief Returns the blockchain object corresponding to the given id.
          * This generic function can be used to retrieve any object from the blockchain
          * that is assigned an ID.  Certain types of objects have specialized convenience
          * functions to return their objects -- e.g., assets have \c get_asset(), accounts
          * have \c get_account(), but this function will work for any object.
          * @param id the id of the object to return
          * @return the requested object
          * @ingroup WalletAPI_General
          */
         variant                           get_object(object_id_type id) const;

         /** @brief Returns the current wallet filename.
          * @note This is the filename that will be used when automatically saving the wallet.
          * @see set_wallet_filename()
          * @return the wallet filename
          * @ingroup WalletAPI_Wallet
          */
         string                            get_wallet_filename() const;

         /**
          * @brief Get the WIF private key corresponding to a public key.  The
          * private key must already be imported in the wallet.
          * @param pubkey public key
          * @return WIF private key corresponding to a public key
          * @ingroup WalletAPI_Wallet
          */
         string                            get_private_key( public_key_type pubkey )const;

         /**
          * @brief Allows creation of customized transactions and fill them with operation/s.
          * @return identifier allowing to construct several transactions in parallel and identify them
          * @ingroup WalletAPI_TransactionBuilder
          */
         transaction_handle_type begin_builder_transaction();

         /**
          * @brief Adds an operation to a transaction in transaction builder.
          * @see \c begin_builder_transaction()
          * @param transaction_handle the number indetifying transaction under construction process
          * @param op the operation
          * @ingroup WalletAPI_TransactionBuilder
          */
         void add_operation_to_builder_transaction(transaction_handle_type transaction_handle, const operation& op);

         /**
          * @brief Replace existing operation in specified transaction in transaction builder.
          * @see \c add_operation_to_builder_transaction()
          * @param handle the number identifying transaction under contruction process
          * @param operation_index index of the operation to replace
          * @param new_op the new operation replacing the existing one
          * @ingroup WalletAPI_TransactionBuilder
          */
         void replace_operation_in_builder_transaction(transaction_handle_type handle,
                                                       unsigned operation_index,
                                                       const operation& new_op);
         /**
          * @brief Set fees on all operations in a transaction
          * @see \c begin_builder_transaction()
          * @param handle the number identifying transaction under contruction process
          * @param fee_asset the asset in which fees are calculated
          * @return total fee in specified asset
          * @ingroup WalletAPI_TransactionBuilder
          */
         asset set_fees_on_builder_transaction(transaction_handle_type handle, string fee_asset = GRAPHENE_SYMBOL);

         /**
          * @brief Previews a transaction from transaction builder.
          * @see \c begin_builder_transaction()
          * @param handle the number identifying transaction under contruction process
          * @return the transaction to preview
          * @ingroup WalletAPI_TransactionBuilder
          */
         transaction preview_builder_transaction(transaction_handle_type handle);

         /**
          * @brief Signs a transaction from transaction builder
          * @see \c prewiev_builder_transaction()
          * @param transaction_handle the number identifying transaction under contruction process
          * @param broadcast \c true to broadcast the transaction on the network
          * @return the signed transaction
          * @ingroup WalletAPI_TransactionBuilder
          */
         signed_transaction sign_builder_transaction(transaction_handle_type transaction_handle, bool broadcast = true);

         /**
          * @brief Allows creation of a proposed transaction suitable for miner-account. Proposed transaction requires approval of multiple accounts in order to execute.
          * @param handle the number identifying transaction under contruction process
          * @param expiration the expiration time of the transaction
          * @param review_period_seconds the time reserved for reviewing the proposal transaction. It's not allowed to vote for the proposal when the transaction is under review
          * @param broadcast \c true to broadcast the transaction on the network
          * @return the signed transaction
          * @ingroup WalletAPI_TransactionBuilder
          */
         signed_transaction propose_builder_transaction(
            transaction_handle_type handle,
            time_point_sec expiration = time_point::now() + fc::minutes(1),
            uint32_t review_period_seconds = 0,
            bool broadcast = true
         );

         /**
          * @brief Allows creation of a proposed transaction. Proposed transaction requires approval of multiple accounts in order to execute. 
          * @see \c propose_builder_transaction()
          * @param handle the number identifying transaction under contruction process
          * @param account_name_or_id the account which will pay the fee to propose the transaction
          * @param expiration the expiration time of the transaction
          * @param review_period_seconds the time reserved for reviewing the proposal transaction. It's not allowed to vote for the proposal when the transaction is under review
          * @param broadcast \c true to broadcast the transaction on the network
          * @return the signed transaction
          * @ingroup WalletAPI_TransactionBuilder
          */
         signed_transaction propose_builder_transaction2(
            transaction_handle_type handle,
            string account_name_or_id,
            time_point_sec expiration = time_point::now() + fc::minutes(1),
            uint32_t review_period_seconds = 0,
            bool broadcast = true
         );

         /**
          * @brief Removes a transaction from transaction builder
          * @param handle the number identifying transaction under contruction process
          * @ingroup WalletAPI_TransactionBuilder
          */
         void remove_builder_transaction(transaction_handle_type handle);

         /**
          * @brief Lists proposed transactions relevant to a user
          * @param account_or_id the name or id of the account
          * @return a list of proposed transactions
          * @ingroup WalletAPI_Proposals
          */
         vector<proposal_object> get_proposed_transactions( string account_or_id )const;

         /**
          * @brief Checks whether the wallet has just been created and has not yet had a password set.
          * Calling \c set_password() will transition the wallet to the locked state.
          * @return \c true if the wallet is new
          * @ingroup WalletAPI_Wallet
          */
         bool    is_new()const;

         /**
          * @brief Checks whether the wallet is locked (is unable to use its private keys).
          * This state can be changed by calling \c lock() or \c unlock().
          * @see \c unlock()
          * @return \c true if the wallet is locked
          * @ingroup WalletAPI_Wallet
          */
         bool    is_locked()const;

         /**
          * @brief Locks the wallet immediately.
          * @see \c unlock()
          * @ingroup WalletAPI_Wallet
          */
         void    lock();

         /**
          * @brief Unlocks the wallet.
          * The wallet remain unlocked until the \c lock() is called
          * or the program exits.
          * @param password the password previously set with \c set_password()
          * @ingroup WalletAPI_Wallet
          */
         void    unlock(string password);

         /**
          * @brief Sets a new password on the wallet.
          * The wallet must be either \c new or \c unlocked to execute this command.
          * @param password
          * @ingroup WalletAPI_Wallet
          */
         void    set_password(string password);

         /**
          * @brief Dumps all private keys successfully imported in the wallet.
          * @note The keys are printed in WIF format.  You can import these keys into another wallet
          * using \c import_key()
          * @return a map containing the private keys and corresponding public keys
          * @ingroup WalletAPI_Wallet
          */
         map<public_key_type, string> dump_private_keys();

         /**
          * @brief Returns a list of all commands supported by the wallet API.
          * This lists each command, along with its arguments and return types.
          * For more detailed help on a single command, use \c get_help()
          * @return a multi-line string suitable for displaying on a terminal
          * @ingroup WalletAPI_General
          */
         string  help()const;

         /**
          * @brief Returns detailed help on a single API command.
          * @param method the name of the API command you want help with
          * @return a multi-line string suitable for displaying on a terminal
          * @ingroup WalletAPI_General
          */
         string  get_help(const string& method)const;

         /**
          * @brief Loads a specified wallet file.
          * The current wallet is closed before the new wallet is loaded.
          * @warning This does not change the filename that will be used for future
          * wallet writes, so this may cause you to overwrite your original
          * wallet unless you also call \c set_wallet_filename()
          * @param wallet_filename the filename of the wallet JSON file to load.
          *                        If \c wallet_filename is empty, it reloads the
          *                        existing wallet file
          * @return \c true if the specified wallet is loaded
          * @ingroup WalletAPI_Wallet
          */
         bool    load_wallet_file(string wallet_filename = "");

         /**
          * @brief Saves the current wallet to the given filename.
          * @warning This does not change the wallet filename that will be used for future
          * writes, so think of this function as 'Save a Copy As...' instead of
          * 'Save As...'.  Use \c set_wallet_filename() to make the filename
          * persist.
          * @param wallet_filename the filename of the new wallet JSON file to create
          *                        or overwrite.  If \c wallet_filename is empty,
          *                        save to the current filename.
          * @ingroup WalletAPI_Wallet
          */
         void    save_wallet_file(string wallet_filename = "");

         /**
          * @brief Sets the wallet filename used for future writes.
          * This does not trigger a save, it only changes the default filename
          * that will be used the next time a save is triggered.
          * @param wallet_filename the new filename to use for future saves
          * @ingroup WalletAPI_Wallet
          */
         void    set_wallet_filename(string wallet_filename);

         /**
          * @brief Suggests a safe brain key to use for creating your account.
          * \c create_account_with_brain_key() requires you to specify a brain key,
          * a long passphrase that provides enough entropy to generate cyrptographic
          * keys.  This function will suggest a suitably random string that should
          * be easy to write down (and, with effort, memorize).
          * @return a suggested brain key
          * @ingroup WalletAPI_Account
          */
         brain_key_info suggest_brain_key()const;

         /**
          * @brief Suggests a safe brain key to use for creating your account. This funcion also 
          * generates \c el_gamal_key_pair corresponding to the brain key.
          * @note \c create_account_with_brain_key() requires you to specify a brain key,
          * a long passphrase that provides enough entropy to generate cyrptographic
          * keys.  This function will suggest a suitably random string that should
          * be easy to write down (and, with effort, memorize).
          * @return a suggested brain key and corresponding El Gamal key pair
          * @ingroup WalletAPI_Account
          */
         pair<brain_key_info, el_gamal_key_pair> generate_brain_key_el_gamal_key() const;

         /**
          * @brief Calculates the private key and public key corresponding to any brain key
          * @param brain_key the brain key to be used for calculation
          * @return the corresponding \c brain_key_info
          * @ingroup WalletAPI_Account
          */
         brain_key_info get_brain_key_info(string const& brain_key) const;

          // TODO: I don't see a broadcast_transaction() function, do we need one?
         /**
          * @brief Converts a signed_transaction in JSON form to its binary representation.
          * @param tx the transaction to serialize
          * @return the binary form of the transaction.  It will not be hex encoded,
          *         this returns a raw string that may have null characters embedded in it
          * @ingroup WalletAPI_TransactionBuilder
          */
         string serialize_transaction(signed_transaction tx) const;

         /**
          * @brief Imports the private key for an existing account.
          * The private key should match either an owner key or an active key for the
          * named account.
          * @see dump_private_keys()
          * @see list_my_accounts()
          * @param account_name_or_id the account owning the key
          * @param wif_key the private key in WIF format
          * @return \c true if the key was imported
          * @ingroup WalletAPI_Wallet
          */
         bool import_key(string account_name_or_id, string wif_key);

         /**
          * @brief Imports accounts from the other wallet file.
          * @param filename the filename of the wallet JSON file
          * @param password user's password to the wallet
          * @return mapped account names to boolean values indicating whether the account was successfully imported
          * @ingroup WalletAPI_Wallet
          */
         map<string, bool> import_accounts( string filename, string password );

         /**
          * @brief Imports account keys from particular account from another wallet file to desired account located in wallet file currently used.
          * @param filename the filename of the wallet JSON file
          * @param password user's password to the wallet
          * @param src_account_name name of the source account
          * @param dest_account_name name of the destination account
          * @return \c true if the keys were imported
          * @ingroup WalletAPI_Wallet
          */
         bool import_account_keys( string filename, string password, string src_account_name, string dest_account_name );

         /**
          * @brief Transforms a brain key to reduce the chance of errors when re-entering the key from memory.
          * This takes a user-supplied brain key and normalizes it into the form used
          * for generating private keys.  In particular, this upper-cases all ASCII characters
          * and collapses multiple spaces into one.
          * @param s the brain key as supplied by the user
          * @return the brain key in its normalized form
          * @ingroup WalletAPI_Account
          */
         string normalize_brain_key(string s) const;

         /**
          * @brief Registers a third party's account on the blockckain.
          * This function is used to register an account for which you do not own the private keys.
          * When acting as a registrar, an end user will generate their own private keys and send
          * you the public keys.  The registrar will use this function to register the account
          * on behalf of the end user.
          * @note The owner key represents absolute control over the account. Generally, the only time the owner key is required
          * is to update the active key.
          * @note The active key represents the hot key of the account. This key has control over nearly all
          * operations the account may perform.
          * @see suggest_brain_key()
          * @param name the name of the account, must be unique on the blockchain and contains at least 5 characters
          * @param owner the owner key for the new account
          * @param active the active key for the new account
          * @param registrar_account the account which will pay the fee to register the user
          * @param broadcast \c true to broadcast the transaction on the network
          * @return the signed transaction registering the account
          * @ingroup WalletAPI_Account
          */
         signed_transaction register_account(string name,
                                             public_key_type owner,
                                             public_key_type active,
                                             string  registrar_account,
                                             bool broadcast = false);

         /**
          * @brief Creates a new account and registers it on the blockchain.
          * @see suggest_brain_key()
          * @see register_account()
          * @param brain_key the brain key used for generating the account's private keys
          * @param account_name the name of the account, must be unique on the blockchain and contains at least 5 characters
          * @param registrar_account the account which will pay the fee to register the user
          * @param broadcast \c true to broadcast the transaction on the network
          * @return the signed transaction registering the account
          * @ingroup WalletAPI_Account
          */
         signed_transaction create_account_with_brain_key(string brain_key,
                                                          string account_name,
                                                          string registrar_account,
                                                          bool broadcast = false);
         /**
          * @brief Creates a new account and registers it on the blockchain, but does not import the key to the wallet.
          * @see suggest_brain_key()
          * @see register_account()
          * @param brain_key the brain key used for generating the account's private keys
          * @param account_name the name of the account, must be unique on the blockchain and contains at least 5 characters
          * @param registrar_account the account which will pay the fee to register the user
          * @param broadcast \c true to broadcast the transaction on the network
          * @return the signed transaction registering the account
          * @ingroup WalletAPI_Account
          */
         signed_transaction create_account_with_brain_key_noimport(string brain_key,
                                                                   string account_name,
                                                                   string registrar_account,
                                                                   bool broadcast = false);

         /** 
          * @brief Transfer an amount from one account to another.
          * @param from the name or id of the account sending the funds
          * @param to the name or id of the account receiving the funds
          * @param amount the amount to send (in nominal units -- to send half of a DCT, specify 0.5)
          * @param asset_symbol the symbol or id of the asset to send
          * @param memo a memo to attach to the transaction.  The memo will be encrypted in the
          *             transaction and readable for the receiver. There is no length limit
          *             other than the limit imposed by maximum transaction size.
          * @note transaction fee is fixed and does not depend on the length of the memo
          * @param broadcast \c true to broadcast the transaction on the network
          * @return the signed transaction transferring funds
          * @ingroup WalletAPI_General
          */
         signed_transaction transfer(string from,
                                     string to,
                                     string amount,
                                     string asset_symbol,
                                     string memo,
                                     bool broadcast = false);

         /**
          * @brief This method works just like transfer, except it always broadcasts the transaction.
          * @param from the name or id of the account sending the funds
          * @param to the name or id of the account receiving the funds
          * @param amount the amount to send (in nominal units -- to send half of a DCT, specify 0.5)
          * @param asset_symbol the symbol or id of the asset to send
          * @param memo a memo to attach to the transaction.  The memo will be encrypted in the
          *            transaction and readable for the receiver. There is no length limit
          *            other than the limit imposed by maximum transaction size.
          * @note transaction fee is fixed and does not depend on the lenght of the memo
          * @return the transaction ID along with the signed transaction.
          * @ingroup WalletAPI_General
          */
         pair<transaction_id_type,signed_transaction> transfer2(string from,
                                                                string to,
                                                                string amount,
                                                                string asset_symbol,
                                                                string memo ) {
            auto trx = transfer( from, to, amount, asset_symbol, memo, true );
            return std::make_pair(trx.id(),trx);
         }

        /**
         * @brief Encapsulates begin_builder_transaction(), add_operation_to_builder_transaction(),
         * propose_builder_transaction2(), set_fees_on_builder_transaction() functions for transfer operation.
         * @param proposer proposer
         * @param from the name or id of the account sending the funds
         * @param to the name or id of the account receiving the funds
         * @param amount the amount to send (in nominal units -- to send half of a DCT, specify 0.5)
         * @param asset_symbol the symbol or id of the asset to send
         * @param memo a memo to attach to the transaction.  The memo will be encrypted in the
         *             transaction and readable for the receiver. There is no length limit
         *             other than the limit imposed by maximum transaction size.
         * @param expiration expiration time
         * @ingroup WalletAPI_Proposals
         */
         void propose_transfer(string proposer,
                               string from,
                               string to,
                               string amount,
                               string asset_symbol,
                               string memo,
                               time_point_sec expiration
         );
         
         /**
          * @brief This method is used to convert a JSON transaction to its transaction ID.
          * @param trx signed transaction
          * @return transaction ID
          * @ingroup WalletAPI_General
          */
         transaction_id_type get_transaction_id( const signed_transaction& trx )const { return trx.id(); }



      /**
       * @brief Creates a new monitored asset.
       * @note some parameters can be changed later using \c update_monitored_asset()
       * @param issuer the name or id of the account who will pay the fee and become the
       *               issuer of the new asset.  This can be updated later
       * @param symbol the ticker symbol of the new asset
       * @param precision the number of digits of precision to the right of the decimal point,
       *                  must be less than or equal to 12
       * @param description asset description. Maximal length is 1000 chars
       * @param feed_lifetime_sec time before a price feed expires
       * @param minimum_feeds minimum number of unexpired feeds required to extract a median feed from
       * @param broadcast \c true to broadcast the transaction on the network
       * @return the signed transaction creating a new asset
       * @ingroup WalletAPI_Asset
       */
      signed_transaction create_monitored_asset(string issuer,
                                                string symbol,
                                                uint8_t precision,
                                                string description,
                                                uint32_t feed_lifetime_sec,
                                                uint8_t minimum_feeds,
                                                bool broadcast = false);

      /**
       * @brief Update the parameters specific to a monitored asset.
       * @param symbol the name or id of the asset to update, which must be a monitored asset
       * @param description asset description
       * @param feed_lifetime_sec time before a price feed expires
       * @param minimum_feeds minimum number of unexpired feeds required to extract a median feed from
       * @param broadcast \c true to broadcast the transaction on the network
       * @return the signed transaction updating the monitored asset
       * @ingroup WalletAPI_Asset
       */
      signed_transaction update_monitored_asset(string symbol,
                                                string description,
                                                uint32_t feed_lifetime_sec,
                                                uint8_t minimum_feeds,
                                                bool broadcast = false);

      /**
       * @brief Creates a new user-issued asset.
       * @note Some parameters can be changed later using \c update_user_issued_asset()
       * @see \c issue_asset()
       * @param issuer the name or id of the account who will pay the fee and become the
       *               issuer of the new asset.  This can be updated later
       * @param symbol the ticker symbol of the new asset
       * @param precision the number of digits of precision to the right of the decimal point,
       *               must be less than or equal to 12
       * @param description asset description. Maximal length is 1000 chars
       * @param max_supply the maximum supply of this asset which may exist at any given time
       * @param core_exchange_rate core_exchange_rate technically needs to store the asset ID of
       *               this new asset. Since this ID is not known at the time this operation is
       *               created, create this price as though the new asset has instance ID 1, and
       *               the chain will overwrite it with the new asset's ID
       * @param is_exchangeable \c true to allow implicit conversion of this asset to/from core asset
       * @param broadcast \c true to broadcast the transaction on the network
       * @return the signed transaction creating a new asset
       * @ingroup WalletAPI_Asset
       */
      signed_transaction create_user_issued_asset(string issuer,
                                                  string symbol,
                                                  uint8_t precision,
                                                  string description,
                                                  uint64_t max_supply,
                                                  price core_exchange_rate,
                                                  bool is_exchangeable,
                                                  bool broadcast = false);

      /** 
       * @brief Issue new shares of an asset.
       * @param to_account the name or id of the account to receive the new shares
       * @param amount the amount to issue, in nominal units
       * @param symbol the ticker symbol of the asset to issue
       * @param memo a memo to include in the transaction, readable by the recipient
       * @param broadcast \c true to broadcast the transaction on the network
       * @return the signed transaction issuing the new shares
       * @ingroup WalletAPI_Asset
       */
         signed_transaction issue_asset(string to_account,
                                        string amount,
                                        string symbol,
                                        string memo,
                                        bool broadcast = false);

      /**
       * @brief Update the parameters specific to a user issued asset.
       * User issued assets have some options which are not relevant to other asset types. This operation is used to update those
       * options an an existing user issues asset.
       * @param symbol the name or id of the asset to update, which must be a user-issued asset
       * @param new_issuer if the asset is to be given a new issuer, specify his ID here
       * @param description asset description
       * @param max_supply the maximum supply of this asset which may exist at any given time
       * @param core_exchange_rate price used to convert non-core asset to core asset
       * @param is_exchangeable \c true to allow implicit conversion of this asset to/from core asset
       * @param broadcast \c true to broadcast the transaction on the network
       * @return the signed transaction updating the user-issued asset
       * @ingroup WalletAPI_Asset
       */
      signed_transaction update_user_issued_asset(string symbol,
                                                  string new_issuer,
                                                  string description,
                                                  uint64_t max_supply,
                                                  price core_exchange_rate,
                                                  bool is_exchangeable,
                                                  bool broadcast = false);

      /** 
       * @brief Pay into the pools for the given asset. Allows anyone to deposit core/asset into pools.
       * @note User-issued assets can optionally have two asset pools.
       * This pools are used when conversion between assets is needed (paying fees, paying for a content in different asset ).
       * @param from the name or id of the account sending the core asset
       * @param uia_amount the amount of "this" asset to deposit
       * @param uia_symbol the name or id of the asset whose pool you wish to fund
       * @param dct_amount the amount of the core asset to deposit
       * @param dct_symbol the name or id of the DCT asset
       * @param broadcast \c true to broadcast the transaction on the network
       * @return the signed transaction funding the asset pools
       * @ingroup WalletAPI_Asset
       */
      signed_transaction fund_asset_pools(string from,
                                          string uia_amount,
                                          string uia_symbol,
                                          string dct_amount,
                                          string dct_symbol,
                                          bool broadcast = false);

      /** 
       * @brief Burns the given user-issued asset.
       * This command burns the user-issued asset to reduce the amount in circulation.
       * @note you cannot burn monitored asset.
       * @param from the account containing the asset you wish to burn
       * @param amount the amount to burn, in nominal units
       * @param symbol the name or id of the asset to burn
       * @param broadcast \c true to broadcast the transaction on the network
       * @return the signed transaction burning the asset
       * @ingroup WalletAPI_Asset
       */
      signed_transaction reserve_asset(string from,
                                       string amount,
                                       string symbol,
                                       bool broadcast = false);

      /** 
       * @brief Transfers accumulated assets from pools back to the issuer's balance.
       * @note You cannot claim assets from pools of monitored asset.
       * @param uia_amount the amount of "this" asset to claim, in nominal units
       * @param uia_symbol the name or id of the asset to claim
       * @param dct_amount the amount of DCT asset to claim, in nominal units
       * @param dct_symbol the name or id of the DCT asset to claim
       * @param broadcast \c true to broadcast the transaction on the network
       * @return the signed transaction claiming the fees
       * @ingroup WalletAPI_Asset
       */
      signed_transaction claim_fees(string uia_amount,
                                    string uia_symbol,
                                    string dct_amount,
                                    string dct_symbol,
                                    bool broadcast = false);

         /**
          * @brief Converts asset into DCT, using actual price feed.
          * @param amount the amount to convert in nominal units
          * @param asset_symbol_or_id the symbol or id of the asset to convert
          * @return price in DCT
          * @ingroup WalletAPI_Asset
          */
         string price_to_dct(const string& amount, const string& asset_symbol_or_id);

         /**
          * @brief Publishes a price feed for the named asset.
          * Price feed providers use this command to publish their price feeds for monitored assets. A price feed is
          * used to tune the market for a particular monitored asset. For each value in the feed, the median across all
          * miner feeds for that asset is calculated and the market for the asset is configured with the median of that
          * value.
          * @param publishing_account the account publishing the price feed
          * @param symbol the name or id of the asset whose feed we're publishing
          * @param feed the price feed object for particular monitored asset
          * @param broadcast \c true to broadcast the transaction on the network
          * @return the signed transaction updating the price feed for the given asset
          * @ingroup WalletAPI_Asset
          */
         signed_transaction publish_asset_feed(string publishing_account,
                                               string symbol,
                                               price_feed feed,
                                               bool broadcast = false);

         /**
          * @brief Get a list of published price feeds by a miner.
          * @param account_name_or_id the name or id of the account
          * @param count maximum number of price feeds to fetch (must not exceed 100)
          * @return list of price feeds published by the miner
          * @ingroup WalletAPI_Asset
          */
         multimap<time_point_sec, price_feed> get_feeds_by_miner(const string& account_name_or_id,
                                                               const uint32_t count);

         /**
          * @brief Lists all miners registered in the blockchain.
          * This returns a list of all account names that own miners, and the associated miner id,
          * sorted by name. This lists miners whether they are currently voted in or not.
          * Use the \c lowerbound and \c limit parameters to page through the list.  To retrieve all miners,
          * start by setting \c lowerbound to the empty string \c "", and then each iteration, pass
          * the last miner name returned as the \c lowerbound for the next \c list_miners() call.
          * @param lowerbound the name of the first miner to return.  If the named miner does not exist,
          *                   the list will start at the miner that comes after \c lowerbound
          * @param limit the maximum number of miners to return (max: 1000)
          * @return a list of miners mapping miner names to miner ids
          * @ingroup WalletAPI_Mining
          */
         map<string,miner_id_type>       list_miners(const string& lowerbound, uint32_t limit);

         /**
          * @brief Returns information about the given miner.
          * @param owner_account the name or id of the miner account owner, or the id of the miner
          * @return the information about the miner stored in the block chain
          * @ingroup WalletAPI_Mining
          */
         miner_object get_miner(string owner_account);

         /**
           * @brief Creates a miner object owned by the given account.
           * @note an account can have at most one miner object.
           * @param owner_account the name or id of the account which is creating the miner
           * @param url a URL to include in the miner record in the blockchain.  Clients may
           *            display this when showing a list of miners.  May be blank.
           * @param broadcast \c true to broadcast the transaction on the network
           * @return the signed transaction registering a miner
           * @ingroup WalletAPI_Mining
           */
         signed_transaction create_miner(string owner_account,
                                           string url,
                                           bool broadcast = false);

         /**
          * @brief Update a miner object owned by the given account.
          * @param miner_name The name of the miner's owner account. Also accepts the ID of the owner account or the ID of the miner.
          * @param url Same as for create_miner.  The empty string makes it remain the same.
          * @param block_signing_key the new block signing public key.  The empty string makes it remain the same
          * @param broadcast \c true if you wish to broadcast the transaction.
          * @ingroup WalletAPI_Mining
          */
         signed_transaction update_miner(string miner_name,
                                           string url,
                                           string block_signing_key,
                                           bool broadcast = false);

         /**
          * @brief Get information about a vesting balance object.
          * @param account_name an account name, account ID, or vesting balance object ID.
          * @ingroup WalletAPI_Mining
          */
         vector< vesting_balance_object_with_info > get_vesting_balances( string account_name );

         /**
          * @brief Withdraw a vesting balance.
          * @param miner_name the account name of the miner, also accepts account ID or vesting balance ID type.
          * @param amount the amount to withdraw.
          * @param asset_symbol the symbol of the asset to withdraw
          * @param broadcast \c true if you wish to broadcast the transaction
          * @ingroup WalletAPI_Mining
          */
         signed_transaction withdraw_vesting(
            string miner_name,
            string amount,
            string asset_symbol,
            bool broadcast = false);

         /**
          * @brief Vote for a given miner.
          * An account can publish a list of all miners they approve of. This
          * command allows you to add or remove miners from this list.
          * Each account's vote is weighted according to the number of shares of the
          * core asset owned by that account at the time the votes are tallied.
          * @note You cannot vote against a miner, you can only vote for the miner
          *       or not vote for the miner.
          * @see \c list_miners()
          * @param voting_account the name or id of the account who is voting with their shares
          * @param miner the name or id of the miner' owner account
          * @param approve \c true if you wish to vote in favor of that miner, \c false to
          *                remove your vote in favor of that miner
          * @param broadcast \c true if you wish to broadcast the transaction
          * @return the signed transaction changing your vote for the given miner
          * @ingroup WalletAPI_Mining
          */
         signed_transaction vote_for_miner(string voting_account,
                                             string miner,
                                             bool approve,
                                             bool broadcast = false);

         /**
          * @brief Set the voting proxy for an account.
          * If a user does not wish to take an active part in voting, they can choose
          * to allow another account to vote their stake.
          * Setting a vote proxy does not remove your previous votes from the blockchain,
          * they remain there but are ignored. If you later null out your vote proxy,
          * your previous votes will take effect again.
          * This setting can be changed at any time.
          * @param account_to_modify the name or id of the account to update
          * @param voting_account the name or id of an account authorized to vote account_to_modify's shares,
          *                       or null to vote your own shares
          * @param broadcast \c true if you wish to broadcast the transaction
          * @return the signed transaction changing your vote proxy settings
          * @ingroup WalletAPI_Mining
          */
         signed_transaction set_voting_proxy(string account_to_modify,
                                             optional<string> voting_account,
                                             bool broadcast = false);

         /**
          * @brief Set your vote for the number of miners in the system.
          * Each account can voice their opinion on how many
          * miners there should be in the active miner list. These
          * are independent of each other. You must vote your approval of at least as many
          * miners as you claim there should be (you can't say that there should
          * be 20 miners but only vote for 10).
          * There are maximum values for each set in the blockchain parameters (currently
          * defaulting to 1001).
          * This setting can be changed at any time. If your account has a voting proxy
          * set, your preferences will be ignored.
          * @param account_to_modify the name or id of the account to update
          * @param desired_number_of_miners
          * @param broadcast \c true if you wish to broadcast the transaction
          * @return the signed transaction changing your vote proxy settings
          * @ingroup WalletAPI_Mining
          */
         signed_transaction set_desired_miner_count(string account_to_modify,
                                                      uint16_t desired_number_of_miners,
                                                      bool broadcast = false);

         /**
          * @brief Signs a transaction.
          * Given a fully-formed transaction that is only lacking signatures, this signs
          * the transaction with the necessary keys and optionally broadcasts the transaction
          * @param tx the unsigned transaction
          * @param broadcast \c true if you wish to broadcast the transaction
          * @return the signed version of the transaction
          * @ingroup WalletAPI_TransactionBuilder
          */
         signed_transaction sign_transaction(signed_transaction tx, bool broadcast = false);

         /**
          * @brief Returns an uninitialized object representing a given blockchain operation.
          * This returns a default-initialized object of the given type.
          * Any operation the blockchain supports can be created using the transaction builder's
          * \c add_operation_to_builder_transaction() , but to do that from the CLI you need to
          * know what the JSON form of the operation looks like. This will give you a template
          * you can fill in.
          * @param operation_type the type of operation to return, must be one of the
          *                       operations defined in `graphene/chain/operations.hpp`
          *                       (e.g., "global_parameters_update_operation")
          * @return a default-constructed operation of the given type
          * @ingroup WalletAPI_TransactionBuilder
          */
         operation get_prototype_operation(string operation_type);

         /**
          * @brief Creates a transaction to propose a parameter change.
          * Multiple parameters can be specified if an atomic change is
          * desired.
          * @param proposing_account the account paying the fee to propose the transaction
          * @param expiration_time timestamp specifying when the proposal will either take effect or expire
          * @param changed_values the values to change; all other chain parameters are filled in with default values
          * @param broadcast \c true if you wish to broadcast the transaction
          * @return the signed version of the transaction
          * @ingroup WalletAPI_Proposals
          */
         signed_transaction propose_parameter_change(
            const string& proposing_account,
            fc::time_point_sec expiration_time,
            const variant_object& changed_values,
            bool broadcast = false);

         /**
          * @brief Propose a fee change.
          * @param proposing_account the account paying the fee to propose the transaction
          * @param expiration_time timestamp specifying when the proposal will either take effect or expire
          * @param changed_values map of operation type to new fee.  Operations may be specified by name or ID
          *    The "scale" key changes the scale.  All other operations will maintain current values
          * @param broadcast \c true if you wish to broadcast the transaction
          * @return the signed version of the transaction
          * @ingroup WalletAPI_Proposals
          */
         signed_transaction propose_fee_change(
            const string& proposing_account,
            fc::time_point_sec expiration_time,
            const variant_object& changed_values,
            bool broadcast = false);

         /**
          * @brief Approve or disapprove a proposal.
          * @param fee_paying_account the account paying the fee for the operation
          * @param proposal_id the proposal to modify
          * @param delta members contain approvals to create or remove.  In JSON you can leave empty members undefined
          * @param broadcast \c true if you wish to broadcast the transaction
          * @return the signed version of the transaction
          * @ingroup WalletAPI_Proposals
          */
         signed_transaction approve_proposal(
            const string& fee_paying_account,
            const string& proposal_id,
            const approval_delta& delta,
            bool broadcast /* = false */
         );

         /**
          * @param creator
          * @param symbol
          * @ingroup WalletAPI_Debug
          */
         void dbg_make_mia(string creator, string symbol);

         /**
          * @param src_filename
          * @param count
          * @ingroup WalletAPI_Debug
          */
         void dbg_push_blocks( std::string src_filename, uint32_t count );

         /**
          * @param debug_wif_key
          * @param count
          * @ingroup WalletAPI_Debug
          */
         void dbg_generate_blocks( std::string debug_wif_key, uint32_t count );

         /**
          * @param filename
          * @ingroup WalletAPI_Debug
          */
         void dbg_stream_json_objects( const std::string& filename );

         /**
          * @param update
          * @ingroup WalletAPI_Debug
          */
         void dbg_update_object( fc::variant_object update );

         /**
          * @brief
          * @param prefix
          * @param number_of_transactions
          * @ingroup WalletAPI_Network
          */
         void flood_network(string prefix, uint32_t number_of_transactions);

         /**
          * @brief
          * @param nodes
          * @ingroup WalletAPI_Network
          */
         void network_add_nodes( const vector<string>& nodes );

         /**
          * @brief
          * @ingroup WalletAPI_Network
          */
         vector< variant > network_get_connected_peers();


         std::map<string,std::function<string(fc::variant,const fc::variants&)>> get_result_formatters() const;

         fc::signal<void(bool)> lock_changed;
         std::shared_ptr<detail::wallet_api_impl> my;
         void encrypt_keys();

         /**
          * @brief Get current supply of the core asset
          * @return the number of shares currently in existence in account and vesting balances, escrows and pools
          * @ingroup WalletAPI_Asset
          */
         real_supply get_real_supply()const;

         /**
          * @brief This method is used to promote account to publishing manager.
          * Such an account can grant or remove right to publish a content. Only DECENT account has permission to use this method.
          * @see set_publishing_right()
          * @param from account ( DECENT account ) giving/removing status of the publishing manager.
          * @param to list of accounts getting status of the publishing manager.
          * @param is_allowed \c true to give the status, \c false to remove it
          * @param broadcast \c true to broadcast the transaction on the network
          * @return the signed transaction updating account status
          * @ingroup WalletAPI_Content
          */
         signed_transaction set_publishing_manager(const string from,
                                                   const vector<string> to,
                                                   bool is_allowed,
                                                   bool broadcast);

         /**
          * @brief Allows account to publish a content. Only account with publishing manager status has permission to use this method.
          * @see set_publishing_manager()
          * @param from account giving/removing right to publish a content.
          * @param to list of accounts getting right to publish a content.
          * @param is_allowed \c true to give the right, \c false to remove it
          * @param broadcast \c true to broadcast the transaction on the network
          * @return the signed transaction updating account status
          * @ingroup WalletAPI_Content
          */
         signed_transaction set_publishing_right(const string from,
                                                 const vector<string> to,
                                                 bool is_allowed,
                                                 bool broadcast);

         /**
          * @brief Get a list of accounts holding publishing manager status.
          * @param lower_bound_name the name of the first account to return. If the named account does not exist,
          * the list will start at the account that comes after \c lowerbound
          * @param limit the maximum number of accounts to return (max: 100)
          * @return a list of publishing managers
          * @ingroup WalletAPI_Content
          */
         vector<account_id_type> list_publishing_managers( const string& lower_bound_name, uint32_t limit );

         /**
          * @brief Submits or resubmits a content to the blockchain. In a case of resubmit, co-authors, price and synopsis fields
          * can be modified.
          * @see \c generate_encryption_key()
          * @see \c submit_content_async()
          * @param author the author of the content
          * @param co_authors the co-authors' account name or ID mapped to corresponding payment split based on basis points. The maximum number of co-authors is 10
          * @param URI the URI of the content
          * @param price_amounts the price of the content per regions
          * @param size the size of the content
          * @param hash the Hash of the package
          * @param seeders list of the seeders, which will publish the content
          * @param quorum defines number of seeders needed to restore the encryption key
          * @param expiration the expiration time of the content. The content is available to buy till it's expiration time
          * @param publishing_fee_asset ticker symbol of the asset which will be used to publish content
          * @param publishing_fee_amount publishing price
          * @param synopsis the description of the content
          * @param secret the AES key used to encrypt and decrypt the content
          * @param cd custody data
          * @param broadcast \c true to broadcast the transaction on the network
          * @return the signed transaction submitting the content
          * @ingroup WalletAPI_Content
          */
         signed_transaction
         submit_content(string const& author,
                        vector< pair< string, uint32_t>> co_authors,
                        string const& URI,
                        vector <regional_price_info> const& price_amounts,
                        uint64_t size,
                        fc::ripemd160 const& hash,
                        vector<account_id_type> const& seeders,
                        uint32_t quorum,
                        fc::time_point_sec const& expiration,
                        string const& publishing_fee_asset,
                        string const& publishing_fee_amount,
                        string const& synopsis,
                        DInteger const& secret,
                        decent::encrypt::CustodyData const& cd,
                        bool broadcast);

         /**
          * @brief This function is used to create and upload a package and submit content in one step.
          * @see create_package()
          * @see upload_package()
          * @see submit_content()
          * @param author the author of the content
          * @param co_authors the co-authors' account name or ID mapped to corresponding payment split based on basis points. The maximum number of co-authors is 10
          * @param content_dir path to the directory containing all content that should be packed
          * @param samples_dir path to the directory containing samples of content
          * @param protocol protocol for uploading package( ipfs )
          * @param price_amounts the prices of the content per regions
          * @param seeders list of the seeders, which will publish the content
          * @param expiration the expiration time of the content. The content is available to buy till it's expiration time
          * @param synopsis the description of the content
          * @ingroup WalletAPI_Content
          */
         void submit_content_async( string const &author,
                                             vector< pair< string, uint32_t>> co_authors,
                                             string const &content_dir,
                                             string const &samples_dir,
                                             string const &protocol,
                                             vector<regional_price_info> const &price_amounts,
                                             vector<account_id_type> const &seeders,
                                             fc::time_point_sec const &expiration,
                                             string const &synopsis);


         /**
          * @brief This function can be used to cancel submitted content. This content is immediately not available to purchase.
          * Seeders keep seeding this content up to next 24 hours.
          * @param author the author of the content
          * @param URI the URI of the content
          * @param broadcast \c true to broadcast the transaction on the network
          * @return the signed transaction cancelling the content
          * @ingroup WalletAPI_Content
          */
         signed_transaction content_cancellation(string author,
                                                 string URI,
                                                 bool broadcast);

         /**
          * @brief Downloads encrypted content specified by provided URI.
          * @param consumer consumer of the content
          * @param URI the URI of the content
          * @param region_code_from two letter region code
          * @param broadcast \c true to broadcast the transaction on the network
          * @ingroup WalletAPI_Content
          */
         void download_content(string const& consumer, string const& URI, string const& region_code_from, bool broadcast = false);

         /**
          * @brief Get status about particular download process specified by provided URI.
          * @param consumer consumer of the content
          * @param URI the URI of the content
          * @return download status, or \c null if no matching download process was found
          * @ingroup WalletAPI_Content
          */
         optional<content_download_status> get_download_status(string consumer, string URI) const;

         /**
          * @brief This function is used to send a request to buy a content. This request is caught by seeders.
          * @param consumer consumer of the content
          * @param URI the URI of the content
          * @param price_asset_name ticker symbol of the asset which will be used to buy content
          * @param price_amount the price of the content
          * @param str_region_code_from two letter region code
          * @param broadcast \c true to broadcast the transaction on the network
          * @return the signed transaction requesting buying of the content
          * @ingroup WalletAPI_Content
          */
         signed_transaction request_to_buy(string consumer,
                                           string URI,
                                           string price_asset_name,
                                           string price_amount,
                                           string str_region_code_from,
                                           bool broadcast);

         /**
          * @brief This method allows user to start seeding plugin from running application
          * @param account_id_type_or_name name or ID of account controlling this seeder
          * @param content_private_key El Gamal content private key
          * @param seeder_private_key private key of the account controlling this seeder
          * @param free_space allocated disk space, in MegaBytes
          * @param seeding_price price per MegaByte
          * @param packages_path packages storage path
          * @param region_code optional ISO 3166-1 alpha-2 two-letter region code
          * @ingroup WalletAPI_Seeding
          */
         void seeding_startup( string account_id_type_or_name,
                               DInteger content_private_key,
                               string seeder_private_key,
                               uint64_t free_space,
                               uint32_t seeding_price,
                               string packages_path,
                               string region_code = "" );

         /**
          * @brief Rates and comments a content.
          * @param consumer consumer giving the rating
          * @param URI the URI of the content
          * @param rating the rating. The available options are 1-5
          * @param comment the maximum length of a comment is 100 characters
          * @param broadcast \c true to broadcast the transaction on the network
          * @ingroup WalletAPI_Content
          */
         void leave_rating_and_comment(string consumer,
                                       string URI,
                                       uint64_t rating,
                                       string comment,
                                       bool broadcast = false);

         /**
          * @brief Creates a subscription to author. This function is used by consumers.
          * @param from account who wants subscription to author
          * @param to the author you wish to subscribe to
          * @param price_amount price for the subscription
          * @param price_asset_symbol ticker symbol of the asset which will be used to buy subscription
          * @param broadcast \c true if you wish to broadcast the transaction
          * @return the signed transaction subscribing the consumer to the author
          * @ingroup WalletAPI_Subscription
          */
         signed_transaction subscribe_to_author( string from,
                                                 string to,
                                                 string price_amount,
                                                 string price_asset_symbol,
                                                 bool broadcast/* = false */);

         /**
          * @brief Creates a subscription to author. This function is used by author.
          * @param from the account obtaining subscription from the author
          * @param to the name or id of the author
          * @param broadcast \c true if you wish to broadcast the transaction
          * @return the signed transaction subscribing the consumer to the author
          * @ingroup WalletAPI_Subscription
          */
         signed_transaction subscribe_by_author( string from,
                                                 string to,
                                                 bool broadcast/* = false */);

         /**
          * @brief This function can be used to allow/disallow subscription.
          * @param account the name or id of the account to update
          * @param allow_subscription \c true if account (author) wants to allow subscription, \c false otherwise
          * @param subscription_period duration of subscription in days
          * @param price_amount price for subscription per one subscription period
          * @param price_asset_symbol ticker symbol of the asset which will be used to buy subscription
          * @param broadcast \c true if you wish to broadcast the transaction
          * @return the signed transaction updating the account
          * @ingroup WalletAPI_Subscription
          */
         signed_transaction set_subscription( string account,
                                              bool allow_subscription,
                                              uint32_t subscription_period,
                                              string price_amount,
                                              string price_asset_symbol,
                                              bool broadcast/* = false */);

         /**
          * @brief This function can be used to allow/disallow automatic renewal of expired subscription.
          * @param account_id_or_name the name or id of the account to update
          * @param subscription_id the ID of the subscription.
          * @param automatic_renewal \c true if account (consumer) wants to allow automatic renewal of subscription, \c false otherwise
          * @param broadcast \c true if you wish to broadcast the transaction
          * @return the signed transaction allowing/disallowing renewal of the subscription
          * @ingroup WalletAPI_Subscription
          */
         signed_transaction set_automatic_renewal_of_subscription( string account_id_or_name,
                                                                   subscription_id_type subscription_id,
                                                                   bool automatic_renewal,
                                                                   bool broadcast/* = false */);

         /**
          * @brief Get a list of consumer's active (not expired) subscriptions.
          * @param account_id_or_name the name or id of the consumer
          * @param count maximum number of subscriptions to fetch (must not exceed 100)
          * @return list of active subscription objects corresponding to the provided consumer
          * @ingroup WalletAPI_Subscription
          */
         vector< subscription_object > list_active_subscriptions_by_consumer( const string& account_id_or_name, const uint32_t count)const;

         /**
          * @brief Get a list of consumer's subscriptions.
          * @param account_id_or_name the name or id of the consumer
          * @param count maximum number of subscriptions to fetch (must not exceed 100)
          * @return list of subscription objects corresponding to the provided consumer
          * @ingroup WalletAPI_Subscription
          */
         vector< subscription_object > list_subscriptions_by_consumer( const string& account_id_or_name, const uint32_t count)const;

         /**
          * @brief Get a list of active (not expired) subscriptions to author.
          * @param account_id_or_name the name or id of the author
          * @param count maximum number of subscriptions to fetch (must not exceed 100)
          * @return list of active subscription objects corresponding to the provided author
          * @ingroup WalletAPI_Subscription
          */
         vector< subscription_object > list_active_subscriptions_by_author( const string& account_id_or_name, const uint32_t count)const;

         /**
          * @brief Get a list of subscriptions to author.
          * @param account_id_or_name the name or id of the author
          * @param count maximum number of subscriptions to fetch (must not exceed 100)
          * @return list of subscription objects corresponding to the provided author
          * @ingroup WalletAPI_Subscription
          */
         vector< subscription_object > list_subscriptions_by_author( const string& account_id_or_name, const uint32_t count)const;

         /**
          * @brief Restores AES key( used to encrypt and decrypt a content) from key particles stored in a buying object.
          * @param account consumers account id or name
          * @param buying the buying object containing key particles
          * @return restored AES key from key particles
          * @ingroup WalletAPI_Content
          */
         DInteger restore_encryption_key(std::string account, buying_id_type buying);

         /**
          * @brief Generates private El Gamal key and corresponding public key.
          * @return pair of El Gamal keys
          * @ingroup WalletAPI_Account
          */
         el_gamal_key_pair generate_el_gamal_keys() const;
         
         /**
          * @brief Gets unique El Gamal key pair for consumer.
          * @return pair of El Gamal keys
          * @ingroup WalletAPI_Account
          */
         el_gamal_key_pair_str get_el_gammal_key(string const& consumer) const;

         /**
          * @brief Generates AES encryption key.
          * @return random encryption key
          * @ingroup WalletAPI_Content
          */
         DInteger generate_encryption_key() const;

         /**
          * @brief Get a list of open buyings.
          * @return a list of open buying objects
          * @ingroup WalletAPI_Content
          */
         vector<buying_object> get_open_buyings()const;

         /**
          * @brief Get a list of open buyings by URI.
          * @param URI URI of the buyings to retrieve
          * @return a list of open buying objects corresponding to the provided URI
          * @ingroup WalletAPI_Content
          */
         vector<buying_object> get_open_buyings_by_URI( const string& URI )const;

         /**
          * @brief Get a list of open buyings by consumer.
          * @param account_id_or_name consumer of the buyings to retrieve
          * @return a list of open buying objects corresponding to the provided consumer
          * @ingroup WalletAPI_Content
          */
         vector<buying_object> get_open_buyings_by_consumer( const string& account_id_or_name )const;

         /**
          * @brief Get history buyings by consumer.
          * @param account_id_or_name consumer of the buyings to retrieve
          * @return a list of history buying objects corresponding to the provided consumer
          * @ingroup WalletAPI_Content
          */
         vector<buying_object> get_buying_history_objects_by_consumer( const string& account_id_or_name )const;

         /**
          * @brief Get history buying objects by consumer that match search term.
          * @param account_id_or_name consumer of the buyings to retrieve
          * @param term search term to look up in \c title and \c description
          * @param order sort data by field
          * @param id object id to start searching from
          * @param count maximum number of contents to fetch (must not exceed 100)
          * @return a list of history buying objects corresponding to the provided consumer and matching search term
          * @ingroup WalletAPI_Content
          */
         vector<buying_object_ex> search_my_purchases(const string& account_id_or_name,
                                                      const string& term,
                                                      const string& order,
                                                      const string& id,
                                                      uint32_t count) const;

         /**
         * @brief Get buying object (open or history) by consumer and URI.
         * @param account_id_or_name consumer of the buying to retrieve
         * @param URI the URI of the buying to retrieve
         * @return buying objects corresponding to the provided consumer, or null if no matching buying was found
         * @ingroup WalletAPI_Content
         */
         optional<buying_object> get_buying_by_consumer_URI( const string& account_id_or_name, const string & URI )const;

         /**
          * @brief Search for term in users' feedbacks.
          * @param user the author of the feedback
          * @param URI the content object URI
          * @param id the id of feedback object to start searching from
          * @param count maximum number of feedbacks to fetch
          * @return the feedback found
          * @ingroup WalletAPI_Content
          */
         vector<rating_object_ex> search_feedback(const string& user,
                                                  const string& URI,
                                                  const string& id,
                                                  uint32_t count) const;

         /**
          * @brief Get a content by URI.
          * @param URI the URI of the content to retrieve
          * @return the content corresponding to the provided URI, or \c null if no matching content was found
          * @ingroup WalletAPI_Content
          */
         optional<content_object> get_content( const string& URI )const;
         
         /**
          * @brief Get a list of contents ordered alphabetically by search term.
          * @param term search term
          * @param order order field
          * @param user content owner
          * @param region_code two letter region code
          * @param id the id of content object to start searching from
          * @param type the application and content type to be filtered
          * @param count maximum number of contents to fetch (must not exceed 100)
          * @return the contents found
          * @ingroup WalletAPI_Content
          */
         vector<content_summary> search_content(const string& term,
                                                const string& order,
                                                const string& user,
                                                const string& region_code,
                                                const string& id,
                                                const string& type,
                                                uint32_t count )const;
         /**
          * @brief Get a list of contents ordered alphabetically by search term.
          * @param user content owner
          * @param term search term
          * @param order order field
          * @param region_code two letter region code
          * @param id the id of content object to start searching from
          * @param type the application and content type to be filtered
          * @param count maximum number of contents to fetch (must not exceed 100)
          * @return the contents found
          * @ingroup WalletAPI_Content
          */
         vector<content_summary> search_user_content(const string& user,
                                                     const string& term,
                                                     const string& order,
                                                     const string& region_code,
                                                     const string& id,
                                                     const string& type,
                                                     uint32_t count )const;

         /**
          * @brief Get a list of seeders by price, in increasing order.
          * @param count maximum number of seeders to retrieve
          * @return a list of seeders
          * @ingroup WalletAPI_Seeding
          */
         vector<seeder_object> list_seeders_by_price( uint32_t count )const;

         /**
          * @brief Get a list of seeders ordered by total upload, in decreasing order.
          * @param count maximum number of seeders to retrieve
          * @return a list of seeders
          * @ingroup WalletAPI_Seeding
          */
         optional<vector<seeder_object>> list_seeders_by_upload( const uint32_t count )const;

         /**
          * @brief Get a list of seeders by region code.
          * @param region_code region code of seeders to retrieve
          * @return a list of seeders
          * @ingroup WalletAPI_Seeding
          */
         vector<seeder_object> list_seeders_by_region( const string region_code )const;

         /**
          * @brief Get a list of seeders ordered by rating, in decreasing order.
          * @param count the maximum number of seeders to retrieve
          * @return a list of seeders
          * @ingroup WalletAPI_Seeding
          */
         vector<seeder_object> list_seeders_by_rating( const uint32_t count )const;

         /**
          * @brief Get author and list of co-authors of a content corresponding to the provided URI.
          * @param URI the URI of the content
          * @return the autor of the content and the list of co-authors, if provided
          * @ingroup WalletAPI_Content
          */
         pair<account_id_type, vector<account_id_type>> get_author_and_co_authors_by_URI( const string& URI )const;

         /**
          * @brief Creates a package from selected files.
          * @see \c upload_package()
          * @param content_dir the directory containing all content that should be packed
          * @param samples_dir the directory containing samples of the content
          * @param aes_key the AES key for encryption
          * @return the package hash and content custody data
          * @ingroup WalletAPI_Content
          */
         std::pair<string, decent::encrypt::CustodyData> create_package(const std::string& content_dir, const std::string& samples_dir, const DInteger& aes_key) const;


         /**
          * @brief Extracts selected package.
          * @see \c download_package()
          * @param package_hash hash of the package that needs to be extracted
          * @param output_dir directory where extracted files will be created
          * @param aes_key the AES key for decryption
          * @ingroup WalletAPI_Content
          */
         void extract_package(const std::string& package_hash, const std::string& output_dir, const DInteger& aes_key) const;

         /**
          * @brief Downloads the package.
          * @param url the URL of the package
          * @ingroup WalletAPI_Content
          */
         void download_package(const std::string& url) const;

         /**
          * @brief Starts uploading of the package.
          * @see \c create_package()
          * @param package_hash hash of the package that needs to be extracted
          * @param protocol protocol for uploading package ( ipfs )
          * @return URL of package
          * @ingroup WalletAPI_Content
          */
         std::string upload_package(const std::string& package_hash, const std::string& protocol) const;

         /**
          * @brief Removes the package.
          * @param package_hash hash of the package that needs to be removed
          * @ingroup WalletAPI_Content
          */
         void remove_package(const std::string& package_hash) const;

         /**
          * @brief Print statuses of all active transfers.
          * @param enable \c true to enable transfer logging
          * @ingroup WalletAPI_Debug
          */
         void set_transfer_logs(bool enable) const;

         /**
          * @brief Sign a buffer.
          * @param str_buffer the buffer to be signed
          * @param str_brainkey derives the private key used for signature
          * @return the signed buffer
          * @ingroup WalletAPI_Debug
          */
         std::string sign_buffer(std::string const& str_buffer,
                                 std::string const& str_brainkey) const;

         /**
          * @brief Verify if the signature is valid.
          * @param str_buffer the original buffer
          * @param str_publickey the public key used for verification
          * @param str_signature the signed buffer
          * @return \c true if valid, otherwise \c false
          * @ingroup WalletAPI_Debug
          */
         bool verify_signature(std::string const& str_buffer,
                               std::string const& str_publickey,
                               std::string const& str_signature) const;

         /**
          * @brief Query the last local block.
          * @return the block time
          * @ingroup WalletAPI_General
          */
         fc::time_point_sec head_block_time() const;

        /**
         * @brief Sends a text message to one or many users.
         * @param from account sending the message
         * @param to account or multiple accounts receiving the message
         * @param text the body of the message
         * @ingroup WalletAPI_Messaging
         */
         void send_message(const std::string& from, std::vector<string> to, string text);

         /**
         * @brief Receives message objects by sender and/or receiver.
         * @param sender name of message sender. If you dont want to filter by sender then let it empty
         * @param receiver name of message receiver. If you dont want to filter by receiver then let it empty
         * @param max_count maximal number of last messages to be displayed
         * @return a vector of message objects
         * @ingroup WalletAPI_Messaging
         */
         vector<message_object> get_message_objects(const std::string& sender, const std::string& receiver, uint32_t max_count) const;

         /**
         * @brief Receives messages by receiver.
         * @param receiver name of message receiver which must be imported to caller's wallet
         * @param max_count maximal number of last messages to be displayed
         * @return a vector of message objects
         * @ingroup WalletAPI_Messaging
         */
         vector<text_message> get_messages(const std::string& receiver, uint32_t max_count) const;

         /**
         * @brief Receives sent messages by sender.
         * @param sender name of message sender which must be imported to caller's wallet
         * @param max_count maximal number of last messages to be displayed
         * @return a vector of message objects
         * @ingroup WalletAPI_Messaging
         */
         vector<text_message> get_sent_messages(const std::string& sender, uint32_t max_count) const;
      };

   } }


FC_REFLECT( graphene::wallet::plain_keys, (keys)(checksum) )
FC_REFLECT( graphene::wallet::el_gamal_key_pair, (private_key)(public_key) )
FC_REFLECT( graphene::wallet::el_gamal_key_pair_str, (private_key)(public_key) )
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

FC_REFLECT( graphene::wallet::brain_key_info,
            (brain_priv_key)
               (wif_priv_key)
               (pub_key)
)

FC_REFLECT( graphene::wallet::regional_price_info,
             (region)
             (amount)
             (asset_symbol)
)

FC_REFLECT (graphene::wallet::content_download_status, 
              (total_key_parts)
              (received_key_parts)
              (total_download_bytes)
              (received_download_bytes)
              (status_text)
            )

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

FC_REFLECT_DERIVED( graphene::wallet::signed_block_with_info, (graphene::chain::signed_block),
                    (block_id)(signing_key)(transaction_ids)(miner_reward) )

FC_REFLECT_DERIVED( graphene::wallet::vesting_balance_object_with_info, (graphene::chain::vesting_balance_object),
                    (allowed_withdraw)(allowed_withdraw_time) )


FC_REFLECT_DERIVED( graphene::wallet::buying_object_ex,
                   (graphene::chain::buying_object)
                   (graphene::wallet::content_download_status),
                   (id)
                   (author_account)
                   (AVG_rating)
                   (times_bought)
                   (hash)
                  )

FC_REFLECT_DERIVED( graphene::wallet::rating_object_ex,
                    (graphene::chain::buying_object),(author) )


FC_REFLECT( graphene::wallet::operation_detail,
            (memo)(description)(op) )

FC_API( graphene::wallet::wallet_api,
           (help)
           (gethelp)
           (info)
           (about)
           (begin_builder_transaction)
           (add_operation_to_builder_transaction)
           (replace_operation_in_builder_transaction)
           (set_fees_on_builder_transaction)
           (preview_builder_transaction)
           (sign_builder_transaction)
           (propose_builder_transaction)
           (propose_builder_transaction2)
           (remove_builder_transaction)
           (is_new)
           (is_locked)
           (lock)(unlock)(set_password)
           (dump_private_keys)
           (list_my_accounts)
           (list_accounts)
           (search_accounts)
           (list_account_balances)
           (list_assets)
           (import_key)
           (import_accounts)
           (import_account_keys)
           (suggest_brain_key)
           (generate_brain_key_el_gamal_key)
           (get_brain_key_info)
           (register_account)
           (create_account_with_brain_key)
           (create_account_with_brain_key_noimport)
           (transfer)
           (transfer2)
           (propose_transfer)
           (get_transaction_id)
           (create_monitored_asset)
           (update_monitored_asset)
           (publish_asset_feed)
           (get_feeds_by_miner)
           (create_user_issued_asset)
           (update_user_issued_asset)
           (issue_asset)
           (fund_asset_pools)
           (reserve_asset)
           (claim_fees)
           (price_to_dct)
           (get_asset)
           (get_monitored_asset_data)
           (get_miner)
           (list_miners)
           (create_miner)
           (update_miner)
           (get_vesting_balances)
           (withdraw_vesting)
           (vote_for_miner)
           (set_voting_proxy)
           (set_desired_miner_count)
           (get_account)
           (get_account_id)
           (get_block)
           (get_account_count)
           (get_account_history)
           (search_account_history)
           (get_global_properties)
           (get_dynamic_global_properties)
           (get_object)
           (get_private_key)
           (load_wallet_file)
           (normalize_brain_key)
           (save_wallet_file)
           (serialize_transaction)
           (sign_transaction)
           (get_prototype_operation)
           (propose_parameter_change)
           (propose_fee_change)
           (approve_proposal)
           (dbg_make_mia)
           (dbg_push_blocks)
           (dbg_generate_blocks)
           (dbg_stream_json_objects)
           (dbg_update_object)
           (flood_network)
           (network_add_nodes)
           (network_get_connected_peers)
           (download_content)
           (get_download_status)
           (set_publishing_manager)
           (set_publishing_right)
           (list_publishing_managers)
           (submit_content)
           (submit_content_async)
           (content_cancellation)
           (request_to_buy)
           (leave_rating_and_comment)
           (seeding_startup)
           (restore_encryption_key)
           (generate_encryption_key)
           (generate_el_gamal_keys)
           (get_el_gammal_key)
           (subscribe_to_author)
           (subscribe_by_author)
           (set_subscription)
           (set_automatic_renewal_of_subscription)
           (list_active_subscriptions_by_consumer)
           (list_subscriptions_by_consumer)
           (list_active_subscriptions_by_author)
           (list_subscriptions_by_author)
           (get_open_buyings)
           (get_open_buyings_by_URI)
           (get_open_buyings_by_consumer)
           (get_buying_history_objects_by_consumer)
           (search_my_purchases)
           (get_buying_by_consumer_URI)
           (search_feedback)
           (get_content)
           (get_real_supply)
           (search_content)
           (search_user_content)
           (list_seeders_by_price)
           (list_seeders_by_upload)
           (list_seeders_by_region)
           (list_seeders_by_rating)
           (get_author_and_co_authors_by_URI)
           (create_package)
           (extract_package)
           (download_package)
           (upload_package)
           (remove_package)
           (set_transfer_logs)
           (sign_buffer)
           (verify_signature)
           (head_block_time)
           (get_proposed_transactions)
           (send_message)
           (get_message_objects)
           (get_messages)
           (get_sent_messages)
)
