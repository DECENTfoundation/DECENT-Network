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
         /// Add acct to @ref my_accounts, or update it if it is already in @ref my_accounts
         /// @return true if the account was newly inserted; false if it was only updated
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

         /* private el gamal key */
         DInteger priv_el_gamal_key;

         /** encrypted keys */
         vector<char>              cipher_keys;

         /** map an account to a set of extra keys that have been imported for that account */
         map<account_id_type, set<public_key_type> >  extra_keys;

         // map of account_name -> base58_private_key for
         //    incomplete account regs
         map<string, vector<string> > pending_account_registrations;
         map<string, string> pending_witness_registrations;


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
            price = paid_price;
            this->id = std::string(obj.id);
         }
         
         std::string         id;
         std::string         author_account;
         uint32_t            times_bought;
         fc::ripemd160       hash;
         double              AVG_rating;
      };


      struct signed_block_with_info : public signed_block
      {
         signed_block_with_info( const signed_block& block );
         signed_block_with_info( const signed_block_with_info& block ) = default;

         block_id_type block_id;
         public_key_type signing_key;
         vector< transaction_id_type > transaction_ids;
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
 * @defgroup WalletCLI
 */
      class wallet_api
      {
      public:
         wallet_api( const wallet_data& initial_data, fc::api<login_api> rapi );
         virtual ~wallet_api();

         /**
          * @brief Copy wallet file to a new file
          * @param destination_filename
          * @return true if the wallet is copied, false otherwise
          * @ingroup WalletCLI
          */
         bool copy_wallet_file( string destination_filename );

         /**
          * @brief Derive private key from given prefix and sequence
          * @param prefix_string
          * @param sequence_number
          * @return private_key Derived private key
          * @ingroup WalletCLI
          */
         fc::ecc::private_key derive_private_key(const std::string& prefix_string, int sequence_number) const;

         /**
          * @brief Lists all available commands
          * @return List of all available commands
          * @ingroup WalletCLI
          */
         variant                           info();

         /**
          * @brief Returns info such as client version, git version of graphene/fc, version of boost, openssl.
          * @returns compile time info and client and dependencies versions
          * @ingroup WalletCLI
          */
         variant_object                    about() const;

         /**
          * @brief Retrieve a full, signed block with info
          * @param num ID of the block
          * @return the referenced block with info, or null if no matching block was found
          * @ingroup WalletCLI
          */
         optional<signed_block_with_info>    get_block( uint32_t num );

         /**
          * @brief Returns the number of accounts registered on the blockchain
          * @returns the number of registered accounts
          * @ingroup WalletCLI
          */
         uint64_t                          get_account_count()const;

         /**
          * @brief Lists all accounts controlled by this wallet.
          * This returns a list of the full account objects for all accounts whose private keys
          * we possess.
          * @returns a list of account objects
          * @ingroup WalletCLI
          */
         vector<account_object>            list_my_accounts();

         /**
          * @brief Lists all accounts registered in the blockchain.
          * This returns a list of all account names and their account ids, sorted by account name.
          *
          * Use the \c lowerbound and limit parameters to page through the list.  To retrieve all accounts,
          * start by setting \c lowerbound to the empty string \c "", and then each iteration, pass
          * the last account name returned as the \c lowerbound for the next \c list_accounts() call.
          *
          * @param lowerbound the name of the first account to return.  If the named account does not exist,
          *                   the list will start at the account that comes after \c lowerbound
          * @param limit the maximum number of accounts to return (max: 1000)
          * @returns a list of accounts mapping account names to account ids
          * @ingroup WalletCLI
          */
         map<string,account_id_type>       list_accounts(const string& lowerbound, uint32_t limit);

         /**
          * @brief Get names and IDs for registered accounts that match search term
          * @param term will try to partially match account name or id
          * @param limit Maximum number of results to return -- must not exceed 1000
          * @param order Sort data by field
          * @param id object_id to start searching from
          * @return Map of account names to corresponding IDs
          * @ingroup WalletCLI
          */
         vector<account_object>       search_accounts(const string& term, const string& order, const string& id, uint32_t limit);

         /**
          * @brief List the balances of an account.
          * Each account can have multiple balances, one for each type of asset owned by that
          * account.  The returned list will only contain assets for which the account has a
          * nonzero balance
          * @param id the name or id of the account whose balances you want
          * @returns a list of the given account's balances
          * @ingroup WalletCLI
          */
         vector<asset>                     list_account_balances(const string& id);

         /**
          * @brief Lists all assets registered on the blockchain.
          *
          * To list all assets, pass the empty string \c "" for the lowerbound to start
          * at the beginning of the list, and iterate as necessary.
          *
          * @param lowerbound  the symbol of the first asset to include in the list.
          * @param limit the maximum number of assets to return (max: 100)
          * @returns the list of asset objects, ordered by symbol
          * @ingroup WalletCLI
          */
         vector<asset_object>              list_assets(const string& lowerbound, uint32_t limit)const;

         /**
          * @brief Returns the operations on the named account.
          *
          * This returns a list of transaction detail object, which describe activity on the account.
          *
          * @param account_name the name or id of the account
          * @param order Sort data by field
          * @param id object_id to start searching from
          * @param limit the number of entries to return (starting from the most recent) (max 100)
          * @returns a list of \c transaction_detail_object
          * @ingroup WalletCLI
          */
         vector<class transaction_detail_object> search_account_history(string const& account_name,
                                                                        string const& order,
                                                                        string const& id,
                                                                        int limit) const;

         /**
          *
          * @param symbol
          * @param symbol2
          * @param bucket
          * @return
          * @ingroup WalletCLI
          */
         vector<bucket_object>             get_market_history(string symbol, string symbol2, uint32_t bucket)const;

         /**
          * @brief Get limit orders in a given market
          * @param base ID of asset being sold
          * @param quote ID of asset being purchased
          * @param limit Maximum number of orders to retrieve
          * @return The limit orders, ordered from least price to greatest
          * @ingroup WalletCLI
          */
         vector<limit_order_object>        get_limit_orders(string base, string quote, uint32_t limit)const;

         /**
          * @brief Returns the block chain's slowly-changing settings.
          * This object contains all of the properties of the blockchain that are fixed
          * or that change only once per maintenance interval (daily) such as the
          * current list of witnesses, block interval, etc.
          * @see \c get_dynamic_global_properties() for frequently changing properties
          * @returns the global properties
          * @ingroup WalletCLI
          */
         global_property_object            get_global_properties() const;

         /**
          * @brief Returns the block chain's rapidly-changing properties.
          * The returned object contains information that changes every block interval
          * such as the head block number, the next witness, etc.
          * @see \c get_global_properties() for less-frequently changing properties
          * @returns the dynamic global properties
          * @ingroup WalletCLI
          */
         dynamic_global_property_object    get_dynamic_global_properties() const;

         /**
          * @brief Returns information about the given account.
          *
          * @param account_name_or_id the name or id of the account to provide information about
          * @returns the public account data stored in the blockchain
          * @ingroup WalletCLI
          */
         account_object                    get_account(string account_name_or_id) const;

         /**
          * @brief Returns information about the given asset.
          * @param asset_name_or_id the symbol or id of the asset in question
          * @returns the information about the asset stored in the block chain
          * @ingroup WalletCLI
          */
         asset_object                      get_asset(string asset_name_or_id) const;

         /**
          * @brief Returns the BitAsset-specific data for a given asset.
          * Market-issued assets's behavior are determined both by their "BitAsset Data" and
          * their basic asset data, as returned by \c get_asset().
          * @param asset_name_or_id the symbol or id of the BitAsset in question
          * @returns the BitAsset-specific data for this asset
          * @ingroup WalletCLI
          */
         monitored_asset_options        get_monitored_asset_data(string asset_name_or_id)const;

         /**
          * @brief Lookup the id of a named account.
          * @param account_name_or_id the name of the account to look up
          * @returns the id of the named account
          * @ingroup WalletCLI
          */
         account_id_type                   get_account_id(string account_name_or_id) const;

         /**
          * @brief Lookup the id of a named asset.
          * @param asset_name_or_id the symbol of an asset to look up
          * @returns the id of the given asset
          * @ingroup WalletCLI
          */
         asset_id_type                     get_asset_id(string asset_name_or_id) const;

         /**
          * @brief Returns the blockchain object corresponding to the given id.
          *
          * This generic function can be used to retrieve any object from the blockchain
          * that is assigned an ID.  Certain types of objects have specialized convenience
          * functions to return their objects -- e.g., assets have \c get_asset(), accounts
          * have \c get_account(), but this function will work for any object.
          *
          * @param id the id of the object to return
          * @returns the requested object
          * @ingroup WalletCLI
          */
         variant                           get_object(object_id_type id) const;

         /** @brief Returns the current wallet filename.
          *
          * This is the filename that will be used when automatically saving the wallet.
          *
          * @see set_wallet_filename()
          * @return the wallet filename
          * @ingroup WalletCLI
          */
         string                            get_wallet_filename() const;

         /**
          * @brief Get the WIF private key corresponding to a public key.  The
          * private key must already be in the wallet.
          * @param pubkey Public key
          * @return WIF private key corresponding to a public key
          * @ingroup WalletCLI
          */
         string                            get_private_key( public_key_type pubkey )const;

         /**
          * @ingroup Transaction Builder API
          * @ingroup WalletCLI
          */
         transaction_handle_type begin_builder_transaction();

         /**
          *
          * @param transaction_handle
          * @param op
          * @ingroup Transaction Builder API
          * @ingroup WalletCLI
          */
         void add_operation_to_builder_transaction(transaction_handle_type transaction_handle, const operation& op);

         /**
          *
          * @param handle
          * @param operation_index
          * @param new_op
          * @ingroup Transaction Builder API
          * @ingroup WalletCLI
          */
         void replace_operation_in_builder_transaction(transaction_handle_type handle,
                                                       unsigned operation_index,
                                                       const operation& new_op);
         /**
          *
          * @param handle
          * @param fee_asset
          * @return
          * @ingroup Transaction Builder API
          * @ingroup WalletCLI
          */
         asset set_fees_on_builder_transaction(transaction_handle_type handle, string fee_asset = GRAPHENE_SYMBOL);

         /**
          * @param handle
          * @ingroup Transaction Builder API
          * @ingroup WalletCLI
          */
         transaction preview_builder_transaction(transaction_handle_type handle);

         /**
          *
          * @param transaction_handle
          * @param broadcast true to broadcast the transaction on the network
          * @return
          * @ingroup Transaction Builder API
          * @ingroup WalletCLI
          */
         signed_transaction sign_builder_transaction(transaction_handle_type transaction_handle, bool broadcast = true);

         /**
          *
          * @param handle
          * @param expiration
          * @param review_period_seconds
          * @param broadcast true to broadcast the transaction on the network
          * @return
          * @ingroup Transaction Builder API
          * @ingroup WalletCLI
          */
         signed_transaction propose_builder_transaction(
            transaction_handle_type handle,
            time_point_sec expiration = time_point::now() + fc::minutes(1),
            uint32_t review_period_seconds = 0,
            bool broadcast = true
         );

         /**
          *
          * @param handle
          * @param account_name_or_id
          * @param expiration
          * @param review_period_seconds
          * @param broadcast true to broadcast the transaction on the network
          * @return
          * @ingroup WalletCLI
          */
         signed_transaction propose_builder_transaction2(
            transaction_handle_type handle,
            string account_name_or_id,
            time_point_sec expiration = time_point::now() + fc::minutes(1),
            uint32_t review_period_seconds = 0,
            bool broadcast = true
         );

         /**
          *
          * @param handle
          * @ingroup Transaction Builder API
          * @ingroup WalletCLI
          */
         void remove_builder_transaction(transaction_handle_type handle);

         /**
          * @brief Checks whether the wallet has just been created and has not yet had a password set.
          *
          * Calling \c set_password will transition the wallet to the locked state.
          * @return true if the wallet is new
          * @ingroup Wallet Management
          * @ingroup WalletCLI
          */
         bool    is_new()const;

         /**
          * @brief Checks whether the wallet is locked (is unable to use its private keys).
          *
          * This state can be changed by calling \c lock() or \c unlock().
          * @return true if the wallet is locked
          * @ingroup Wallet Management
          * @ingroup WalletCLI
          */
         bool    is_locked()const;

         /**
          * @brief Locks the wallet immediately.
          * @ingroup Wallet Management
          * @ingroup WalletCLI
          */
         void    lock();

         /**
          * @brief Unlocks the wallet.
          *
          * The wallet remain unlocked until the \c lock is called
          * or the program exits.
          * @param password the password previously set with \c set_password()
          * @ingroup Wallet Management
          * @ingroup WalletCLI
          */
         void    unlock(string password);

         /**
          * @brief Sets a new password on the wallet.
          *
          * The wallet must be either 'new' or 'unlocked' to
          * execute this command.
          * @param password
          * @ingroup Wallet Management
          * @ingroup WalletCLI
          */
         void    set_password(string password);

         /**
          * @brief Dumps all private keys owned by the wallet.
          *
          * The keys are printed in WIF format.  You can import these keys into another wallet
          * using \c import_key()
          * @returns a map containing the private keys, indexed by their public key
          * @ingroup WalletCLI
          */
         map<public_key_type, string> dump_private_keys();

         /**
          * @brief Returns a list of all commands supported by the wallet API.
          *
          * This lists each command, along with its arguments and return types.
          * For more detailed help on a single command, use \c get_help()
          *
          * @returns a multi-line string suitable for displaying on a terminal
          * @ingroup WalletCLI
          */
         string  help()const;

         /**
          * @brief Returns detailed help on a single API command.
          * @param method the name of the API command you want help with
          * @returns a multi-line string suitable for displaying on a terminal
          * @ingroup WalletCLI
          */
         string  gethelp(const string& method)const;

         /**
          * @brief Loads a specified Graphene wallet.
          *
          * The current wallet is closed before the new wallet is loaded.
          *
          * @warning This does not change the filename that will be used for future
          * wallet writes, so this may cause you to overwrite your original
          * wallet unless you also call \c set_wallet_filename()
          *
          * @param wallet_filename the filename of the wallet JSON file to load.
          *                        If \c wallet_filename is empty, it reloads the
          *                        existing wallet file
          * @returns true if the specified wallet is loaded
          * @ingroup WalletCLI
          */
         bool    load_wallet_file(string wallet_filename = "");

         /**
          * @brief Saves the current wallet to the given filename.
          *
          * @warning This does not change the wallet filename that will be used for future
          * writes, so think of this function as 'Save a Copy As...' instead of
          * 'Save As...'.  Use \c set_wallet_filename() to make the filename
          * persist.
          * @param wallet_filename the filename of the new wallet JSON file to create
          *                        or overwrite.  If \c wallet_filename is empty,
          *                        save to the current filename.
          * @ingroup WalletCLI
          */
         void    save_wallet_file(string wallet_filename = "");

         /**
          * @brief Sets the wallet filename used for future writes.
          *
          * This does not trigger a save, it only changes the default filename
          * that will be used the next time a save is triggered.
          *
          * @param wallet_filename the new filename to use for future saves
          * @ingroup WalletCLI
          */
         void    set_wallet_filename(string wallet_filename);

         /**
          * @brief Suggests a safe brain key to use for creating your account.
          * \c create_account_with_brain_key() requires you to specify a 'brain key',
          * a long passphrase that provides enough entropy to generate cyrptographic
          * keys.  This function will suggest a suitably random string that should
          * be easy to write down (and, with effort, memorize).
          * @returns a suggested brain_key
          * @ingroup WalletCLI
          */
         brain_key_info suggest_brain_key()const;

         /**
          * @brief Converts a signed_transaction in JSON form to its binary representation.
          *
          * TODO: I don't see a broadcast_transaction() function, do we need one?
          *
          * @param tx the transaction to serialize
          * @returns the binary form of the transaction.  It will not be hex encoded,
          *          this returns a raw string that may have null characters embedded
          *          in it
          * @ingroup WalletCLI
          */
         string serialize_transaction(signed_transaction tx) const;

         /**
          * @brief Imports the private key for an existing account.
          *
          * The private key must match either an owner key or an active key for the
          * named account.
          *
          * @see dump_private_keys()
          *
          * @param account_name_or_id the account owning the key
          * @param wif_key the private key in WIF format
          * @returns true if the key was imported
          * @ingroup WalletCLI
          */
         bool import_key(string account_name_or_id, string wif_key);

         /**
          * @brief Imports the private el gamal key into cli_wallet configuration file (wallet.json).
          *
          * @see generate_el_gamal_keys()
          *
          * @param privKey Private el gamal key
          * @returns true if the key was imported
          * @ingroup WalletCLI
          */
         void import_el_gamal_key(DInteger privKey );

         /**
          * @brief Imports accounts from the other wallet file
          * @param filename The filename of the wallet JSON file
          * @param password User's password to the wallet
          * @return mapped account names to boolean values indicating whether the account was successfully imported
          * @ingroup WalletCLI
          */
         map<string, bool> import_accounts( string filename, string password );

         /**
          * @brief Imports account keys from particular account from another wallet file to desired account located in wallet file currently used
          * @param filename The filename of the wallet JSON file
          * @param password User's password to the wallet
          * @param src_account_name Name of the source account
          * @param dest_account_name Name of the destination account
          * @return true if the keys were imported
          * @ingroup WalletCLI
          */
         bool import_account_keys( string filename, string password, string src_account_name, string dest_account_name );

         /**
          * @brief Transforms a brain key to reduce the chance of errors when re-entering the key from memory.
          *
          * This takes a user-supplied brain key and normalizes it into the form used
          * for generating private keys.  In particular, this upper-cases all ASCII characters
          * and collapses multiple spaces into one.
          * @param s the brain key as supplied by the user
          * @returns the brain key in its normalized form
          * @ingroup WalletCLI
          */
         string normalize_brain_key(string s) const;

         /**
          * @brief Registers a third party's account on the blockckain.
          *
          * This function is used to register an account for which you do not own the private keys.
          * When acting as a registrar, an end user will generate their own private keys and send
          * you the public keys.  The registrar will use this function to register the account
          * on behalf of the end user.
          *
          * @see create_account_with_brain_key()
          *
          * @param name the name of the account, must be unique on the blockchain.  Shorter names
          *             are more expensive to register; the rules are still in flux, but in general
          *             names of more than 8 characters with at least one digit will be cheap.
          * @param owner the owner key for the new account
          * @param active the active key for the new account
          * @param registrar_account the account which will pay the fee to register the user
          * @param referrer_account the account who is acting as a referrer, and may receive a
          *                         portion of the user's transaction fees.  This can be the
          *                         same as the registrar_account if there is no referrer.
          * @param referrer_percent the percentage (0 - 100) of the new user's transaction fees
          *                         not claimed by the blockchain that will be distributed to the
          *                         referrer; the rest will be sent to the registrar.  Will be
          *                         multiplied by GRAPHENE_1_PERCENT when constructing the transaction.
          * @param broadcast true to broadcast the transaction on the network
          * @returns the signed transaction registering the account
          * @ingroup WalletCLI
          */
         signed_transaction register_account(string name,
                                             public_key_type owner,
                                             public_key_type active,
                                             string  registrar_account,
                                             string  referrer_account,
                                             uint32_t referrer_percent,
                                             bool broadcast = false);

         /**
          * @brief Creates a new account and registers it on the blockchain.
          *
          * @todo why no referrer_percent here?
          *
          * @see suggest_brain_key()
          * @see register_account()
          *
          * @param brain_key the brain key used for generating the account's private keys
          * @param account_name the name of the account, must be unique on the blockchain.  Shorter names
          *                     are more expensive to register; the rules are still in flux, but in general
          *                     names of more than 8 characters with at least one digit will be cheap.
          * @param registrar_account the account which will pay the fee to register the user
          * @param referrer_account the account who is acting as a referrer, and may receive a
          *                         portion of the user's transaction fees.  This can be the
          *                         same as the registrar_account if there is no referrer.
          * @param broadcast true to broadcast the transaction on the network
          * @returns the signed transaction registering the account
          * @ingroup WalletCLI
          */
         signed_transaction create_account_with_brain_key(string brain_key,
                                                          string account_name,
                                                          string registrar_account,
                                                          string referrer_account,
                                                          bool broadcast = false);

         /** @brief Transfer an amount from one account to another.
          * @param from the name or id of the account sending the funds
          * @param to the name or id of the account receiving the funds
          * @param amount the amount to send (in nominal units -- to send half of a BTS, specify 0.5)
          * @param asset_symbol the symbol or id of the asset to send
          * @param memo a memo to attach to the transaction.  The memo will be encrypted in the
          *             transaction and readable for the receiver.  There is no length limit
          *             other than the limit imposed by maximum transaction size, but transaction
          *             increase with transaction size
          * @param broadcast true to broadcast the transaction on the network
          * @returns the signed transaction transferring funds
          * @ingroup WalletCLI
          */
         signed_transaction transfer(string from,
                                     string to,
                                     string amount,
                                     string asset_symbol,
                                     string memo,
                                     bool broadcast = false);

         /**
          *  @brief This method works just like transfer, except it always broadcasts and
          *  returns the transaction ID along with the signed transaction.
          *  @param from the name or id of the account sending the funds
          *  @param to the name or id of the account receiving the funds
          *  @param amount the amount to send (in nominal units -- to send half of a BTS, specify 0.5)
          *  @param asset_symbol the symbol or id of the asset to send
          *  @param memo a memo to attach to the transaction.  The memo will be encrypted in the
          *             transaction and readable for the receiver.  There is no length limit
          *             other than the limit imposed by maximum transaction size, but transaction
          *             increase with transaction size
          *  @ingroup WalletCLI
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
          * @brief This method is used to convert a JSON transaction to its transaction ID.
          * @param trx Signed transaction
          * @return
          * @ingroup WalletCLI
          */
         transaction_id_type get_transaction_id( const signed_transaction& trx )const { return trx.id(); }


         /**
          * @brief Place a limit order attempting to sell one asset for another.
          *
          * Buying and selling are the same operation on Graphene; if you want to buy BTS
          * with USD, you should sell USD for BTS.
          *
          * The blockchain will attempt to sell the \c symbol_to_sell for as
          * much \c symbol_to_receive as possible, as long as the price is at
          * least \c min_to_receive / \c amount_to_sell.
          *
          * In addition to the transaction fees, market fees will apply as specified
          * by the issuer of both the selling asset and the receiving asset as
          * a percentage of the amount exchanged.
          *
          * If either the selling asset or the receiving asset is whitelist
          * restricted, the order will only be created if the seller is on
          * the whitelist of the restricted asset type.
          *
          * Market orders are matched in the order they are included
          * in the block chain.
          *
          * @todo Allow order expiration to be set here.  Document default/max expiration time
          *
          * @param seller_account the account providing the asset being sold, and which will
          *                       receive the proceeds of the sale.
          * @param amount_to_sell the amount of the asset being sold to sell (in nominal units)
          * @param symbol_to_sell the name or id of the asset to sell
          * @param min_to_receive the minimum amount you are willing to receive in return for
          *                       selling the entire amount_to_sell
          * @param symbol_to_receive the name or id of the asset you wish to receive
          * @param timeout_sec if the order does not fill immediately, this is the length of
          *                    time the order will remain on the order books before it is
          *                    cancelled and the un-spent funds are returned to the seller's
          *                    account
          * @param fill_or_kill if true, the order will only be included in the blockchain
          *                     if it is filled immediately; if false, an open order will be
          *                     left on the books to fill any amount that cannot be filled
          *                     immediately.
          * @param broadcast true to broadcast the transaction on the network
          * @returns the signed transaction selling the funds
          * @ingroup WalletCLI
          */
         signed_transaction sell_asset(string seller_account,
                                       string amount_to_sell,
                                       string   symbol_to_sell,
                                       string min_to_receive,
                                       string   symbol_to_receive,
                                       uint32_t timeout_sec = 0,
                                       bool     fill_or_kill = false,
                                       bool     broadcast = false);

         /**
          * @brief Place a limit order attempting to sell one asset for another.
          *
          * This API call abstracts away some of the details of the sell_asset call to be more
          * user friendly. All orders placed with sell never timeout and will not be killed if they
          * cannot be filled immediately. If you wish for one of these parameters to be different,
          * then sell_asset should be used instead.
          *
          * @param seller_account the account providing the asset being sold, and which will
          *                       receive the processed of the sale.
          * @param base The name or id of the asset to sell.
          * @param quote The name or id of the asset to recieve.
          * @param rate The rate in base:quote at which you want to sell.
          * @param amount The amount of base you want to sell.
          * @param broadcast true to broadcast the transaction on the network.
          * @returns The signed transaction selling the funds.
          * @ingroup WalletCLI
          */
         signed_transaction sell( string seller_account,
                                  string base,
                                  string quote,
                                  double rate,
                                  double amount,
                                  bool broadcast );

         /**
          * @brief Place a limit order attempting to buy one asset with another.
          *
          * This API call abstracts away some of the details of the sell_asset call to be more
          * user friendly. All orders placed with buy never timeout and will not be killed if they
          * cannot be filled immediately. If you wish for one of these parameters to be different,
          * then sell_asset should be used instead.
          *
          * @param buyer_account The account buying the asset for another asset.
          * @param base The name or id of the asset to buy.
          * @param quote The name or id of the assest being offered as payment.
          * @param rate The rate in base:quote at which you want to buy.
          * @param amount the amount of base you want to buy.
          * @param broadcast true to broadcast the transaction on the network.
          * @returns The signed transaction selling the funds.
          * @ingroup WalletCLI
          */
         signed_transaction buy( string buyer_account,
                                 string base,
                                 string quote,
                                 double rate,
                                 double amount,
                                 bool broadcast );

         /**
          * @brief Cancel an existing order
          *
          * @param order_id the id of order to be cancelled
          * @param broadcast true to broadcast the transaction on the network
          * @returns the signed transaction canceling the order
          * @ingroup WalletCLI
          */
         signed_transaction cancel_order(object_id_type order_id, bool broadcast = false);

         /**
          * @brief Creates a new user-issued or market-issued asset.
          *
          * Many options can be changed later using \c update_asset()
          *
          * Right now this function is difficult to use because you must provide raw JSON data
          * structures for the options objects, and those include prices and asset ids.
          *
          * @param issuer the name or id of the account who will pay the fee and become the
          *               issuer of the new asset.  This can be updated later
          * @param symbol the ticker symbol of the new asset
          * @param precision the number of digits of precision to the right of the decimal point,
          *                  must be less than or equal to 12
          * @param common asset options required for all new assets.
          *               Note that core_exchange_rate technically needs to store the asset ID of
          *               this new asset. Since this ID is not known at the time this operation is
          *               created, create this price as though the new asset has instance ID 1, and
          *               the chain will overwrite it with the new asset's ID.
          * @param broadcast true to broadcast the transaction on the network
          * @returns the signed transaction creating a new asset
          * @ingroup WalletCLI
          */
         signed_transaction create_asset(string issuer,
                                         string symbol,
                                         uint8_t precision,
                                         asset_options common,
                                         bool broadcast = false);

         /**
          * @brief Issue new shares of an asset.
          *
          * @param to_account the name or id of the account to receive the new shares
          * @param amount the amount to issue, in nominal units
          * @param symbol the ticker symbol of the asset to issue
          * @param memo a memo to include in the transaction, readable by the recipient
          * @param broadcast true to broadcast the transaction on the network
          * @returns the signed transaction issuing the new shares
          * @ingroup WalletCLI
          */
         signed_transaction issue_asset(string to_account,
                                        string amount,
                                        string symbol,
                                        string memo,
                                        bool broadcast = false);

         /**
          * @brief Update the core options on an asset.
          * There are a number of options which all assets in the network use. These options are
          * enumerated in the asset_object::asset_options struct. This command is used to update
          * these options for an existing asset.
          *
          * @note This operation cannot be used to update
          * et-specific options. For these options,
          * \c update_bitasset() instead.
          *
          * @param symbol the name or id of the asset to update
          * @param new_issuer if changing the asset's issuer, the name or id of the new issuer.
          *                   null if you wish to remain the issuer of the asset
          * @param new_options the new asset_options object, which will entirely replace the existing
          *                    options.
          * @param broadcast true to broadcast the transaction on the network
          * @returns the signed transaction updating the asset
          * @ingroup WalletCLI
          */
         signed_transaction update_asset(string symbol,
                                         optional<string> new_issuer,
                                         asset_options new_options,
                                         bool broadcast = false);

         /**
          * @brief Update the options specific to a BitAsset.
          *
          * BitAssets have some options which are not relevant to other asset types. This operation is used to update those
          * options an an existing BitAsset.
          *
          * @see update_asset()
          *
          * @param symbol the name or id of the asset to update, which must be a market-issued asset
          * @param new_options the new bitasset_options object, which will entirely replace the existing
          *                    options.
          * @param broadcast true to broadcast the transaction on the network
          * @returns the signed transaction updating the bitasset
          * @ingroup WalletCLI
          */
         signed_transaction update_monitored_asset(string symbol,
                                                   monitored_asset_options new_options,
                                                   bool broadcast = false);



         /**
          * @brief Publishes a price feed for the named asset.
          *
          * Price feed providers use this command to publish their price feeds for market-issued assets. A price feed is
          * used to tune the market for a particular market-issued asset. For each value in the feed, the median across all
          * witness feeds for that asset is calculated and the market for the asset is configured with the median of that
          * value.
          *
          * The feed object in this command contains three prices: a call price limit, a short price limit, and a settlement price.
          * The call limit price is structured as (collateral asset) / (debt asset) and the short limit price is structured
          * as (asset for sale) / (collateral asset). Note that the asset IDs are opposite to eachother, so if we're
          * publishing a feed for USD, the call limit price will be CORE/USD and the short limit price will be USD/CORE. The
          * settlement price may be flipped either direction, as long as it is a ratio between the market-issued asset and
          * its collateral.
          *
          * @param publishing_account the account publishing the price feed
          * @param symbol the name or id of the asset whose feed we're publishing
          * @param feed the price_feed object containing the three prices making up the feed
          * @param broadcast true to broadcast the transaction on the network
          * @returns the signed transaction updating the price feed for the given asset
          * @ingroup WalletCLI
          */
         signed_transaction publish_asset_feed(string publishing_account,
                                               string symbol,
                                               price_feed feed,
                                               bool broadcast = false);

         /**
          * @brief Lists all witnesses registered in the blockchain.
          * This returns a list of all account names that own witnesses, and the associated witness id,
          * sorted by name.  This lists witnesses whether they are currently voted in or not.
          *
          * Use the \c lowerbound and limit parameters to page through the list.  To retrieve all witnesss,
          * start by setting \c lowerbound to the empty string \c "", and then each iteration, pass
          * the last witness name returned as the \c lowerbound for the next \c list_witnesss() call.
          *
          * @param lowerbound the name of the first witness to return.  If the named witness does not exist,
          *                   the list will start at the witness that comes after \c lowerbound
          * @param limit the maximum number of witnesss to return (max: 1000)
          * @returns a list of witnesses mapping witness names to witness ids
          * @ingroup WalletCLI
          */
         map<string,witness_id_type>       list_witnesses(const string& lowerbound, uint32_t limit);

         /**
          * s@brief Returns information about the given witness.
          * @param owner_account the name or id of the witness account owner, or the id of the witness
          * @returns the information about the witness stored in the block chain
          * @ingroup WalletCLI
          */
         witness_object get_witness(string owner_account);

         /**
           * @brief Creates a witness object owned by the given account.
           *
           * An account can have at most one witness object.
           *
           * @param owner_account the name or id of the account which is creating the witness
           * @param url a URL to include in the witness record in the blockchain.  Clients may
           *            display this when showing a list of witnesses.  May be blank.
           * @param broadcast true to broadcast the transaction on the network
           * @returns the signed transaction registering a witness
           * @ingroup WalletCLI
           */
         signed_transaction create_witness(string owner_account,
                                           string url,
                                           bool broadcast = false);

         /**
          * @brief Update a witness object owned by the given account.
          *
          * @param witness_name The name of the witness's owner account.  Also accepts the ID of the owner account or the ID of the witness.
          * @param url Same as for create_witness.  The empty string makes it remain the same.
          * @param block_signing_key The new block signing public key.  The empty string makes it remain the same.
          * @param broadcast true if you wish to broadcast the transaction.
          * @ingroup WalletCLI
          */
         signed_transaction update_witness(string witness_name,
                                           string url,
                                           string block_signing_key,
                                           bool broadcast = false);

         /**
          * @brief Get information about a vesting balance object.
          *
          * @param account_name An account name, account ID, or vesting balance object ID.
          * @ingroup WalletCLI
          */
         vector< vesting_balance_object_with_info > get_vesting_balances( string account_name );

         /**
          * @brief Withdraw a vesting balance.
          *
          * @param witness_name The account name of the witness, also accepts account ID or vesting balance ID type.
          * @param amount The amount to withdraw.
          * @param asset_symbol The symbol of the asset to withdraw.
          * @param broadcast true if you wish to broadcast the transaction
          * @ingroup WalletCLI
          */
         signed_transaction withdraw_vesting(
            string witness_name,
            string amount,
            string asset_symbol,
            bool broadcast = false);

         /**
          * @brief Vote for a given witness.
          *
          * An account can publish a list of all witnesses they approve of.  This
          * command allows you to add or remove witnesses from this list.
          * Each account's vote is weighted according to the number of shares of the
          * core asset owned by that account at the time the votes are tallied.
          *
          * @note you cannot vote against a witness, you can only vote for the witness
          *       or not vote for the witness.
          *
          * @param voting_account the name or id of the account who is voting with their shares
          * @param witness the name or id of the witness' owner account
          * @param approve true if you wish to vote in favor of that witness, false to
          *                remove your vote in favor of that witness
          * @param broadcast true if you wish to broadcast the transaction
          * @return the signed transaction changing your vote for the given witness
          * @ingroup WalletCLI
          */
         signed_transaction vote_for_witness(string voting_account,
                                             string witness,
                                             bool approve,
                                             bool broadcast = false);

         /**
          * @brief Set the voting proxy for an account.
          *
          * If a user does not wish to take an active part in voting, they can choose
          * to allow another account to vote their stake.
          *
          * Setting a vote proxy does not remove your previous votes from the blockchain,
          * they remain there but are ignored.  If you later null out your vote proxy,
          * your previous votes will take effect again.
          *
          * This setting can be changed at any time.
          *
          * @param account_to_modify the name or id of the account to update
          * @param voting_account the name or id of an account authorized to vote account_to_modify's shares,
          *                       or null to vote your own shares
          *
          * @param broadcast true if you wish to broadcast the transaction
          * @return the signed transaction changing your vote proxy settings
          * @ingroup WalletCLI
          */
         signed_transaction set_voting_proxy(string account_to_modify,
                                             optional<string> voting_account,
                                             bool broadcast = false);

         /**
          * @brief Set your vote for the number of witnesses in the system.
          *
          * Each account can voice their opinion on how many
          * witnesses there should be in the active witness list.  These
          * are independent of each other.  You must vote your approval of at least as many
          * witnesses as you claim there should be (you can't say that there should
          * be 20 witnesses but only vote for 10).
          *
          * There are maximum values for each set in the blockchain parameters (currently
          * defaulting to 1001).
          *
          * This setting can be changed at any time.  If your account has a voting proxy
          * set, your preferences will be ignored.
          *
          * @param account_to_modify the name or id of the account to update
          * @param desired_number_of_witnesses
          * @param broadcast true if you wish to broadcast the transaction
          * @return the signed transaction changing your vote proxy settings
          * @ingroup WalletCLI
          */
         signed_transaction set_desired_witness_count(string account_to_modify,
                                                      uint16_t desired_number_of_witnesses,
                                                      bool broadcast = false);

         /**
          * @brief Signs a transaction.
          *
          * Given a fully-formed transaction that is only lacking signatures, this signs
          * the transaction with the necessary keys and optionally broadcasts the transaction
          * @param tx the unsigned transaction
          * @param broadcast true if you wish to broadcast the transaction
          * @return the signed version of the transaction
          * @ingroup WalletCLI
          */
         signed_transaction sign_transaction(signed_transaction tx, bool broadcast = false);

         /**
          * @brief Returns an uninitialized object representing a given blockchain operation.
          *
          * This returns a default-initialized object of the given type; it can be used
          * during early development of the wallet when we don't yet have custom commands for
          * creating all of the operations the blockchain supports.
          *
          * Any operation the blockchain supports can be created using the transaction builder's
          * \c add_operation_to_builder_transaction() , but to do that from the CLI you need to
          * know what the JSON form of the operation looks like.  This will give you a template
          * you can fill in.  It's better than nothing.
          *
          * @param operation_type the type of operation to return, must be one of the
          *                       operations defined in `graphene/chain/operations.hpp`
          *                       (e.g., "global_parameters_update_operation")
          * @return a default-constructed operation of the given type
          * @ingroup WalletCLI
          */
         operation get_prototype_operation(string operation_type);

         /**
          * @brief Creates a transaction to propose a parameter change.
          *
          * Multiple parameters can be specified if an atomic change is
          * desired.
          *
          * @param proposing_account The account paying the fee to propose the tx
          * @param expiration_time Timestamp specifying when the proposal will either take effect or expire.
          * @param changed_values The values to change; all other chain parameters are filled in with default values
          * @param broadcast true if you wish to broadcast the transaction
          * @return the signed version of the transaction
          * @ingroup WalletCLI
          */
         signed_transaction propose_parameter_change(
            const string& proposing_account,
            fc::time_point_sec expiration_time,
            const variant_object& changed_values,
            bool broadcast = false);

         /**
          * @brief Propose a fee change.
          *
          * @param proposing_account The account paying the fee to propose the tx
          * @param expiration_time Timestamp specifying when the proposal will either take effect or expire.
          * @param changed_values Map of operation type to new fee.  Operations may be specified by name or ID.
          *    The "scale" key changes the scale.  All other operations will maintain current values.
          * @param broadcast true if you wish to broadcast the transaction
          * @return the signed version of the transaction
          * @ingroup WalletCLI
          */
         signed_transaction propose_fee_change(
            const string& proposing_account,
            fc::time_point_sec expiration_time,
            const variant_object& changed_values,
            bool broadcast = false);

         /**
          * @brief Approve or disapprove a proposal.
          *
          * @param fee_paying_account The account paying the fee for the op.
          * @param proposal_id The proposal to modify.
          * @param delta Members contain approvals to create or remove.  In JSON you can leave empty members undefined.
          * @param broadcast true if you wish to broadcast the transaction
          * @return the signed version of the transaction
          * @ingroup WalletCLI
          */
         signed_transaction approve_proposal(
            const string& fee_paying_account,
            const string& proposal_id,
            const approval_delta& delta,
            bool broadcast /* = false */
         );

         /**
          * @brief Returns the order book for the market base:quote
          * @param base String name of the first asset
          * @param quote String name of the second asset
          * @param limit Depth of the order book. Up to depth of each asks and bids, capped at 50. Prioritizes most moderate of each
          * @return Order book of the market
          * @ingroup DatabaseAPI
          */
         order_book get_order_book( const string& base, const string& quote, unsigned limit = 50);

         /**
          *
          * @param creator
          * @param symbol
          * @ingroup WalletCLI
          */
         void dbg_make_uia(string creator, string symbol);

         /**
          *
          * @param creator
          * @param symbol
          * @ingroup WalletCLI
          */
         void dbg_make_mia(string creator, string symbol);

         /**
          *
          * @param src_filename
          * @param count
          * @ingroup WalletCLI
          */
         void dbg_push_blocks( std::string src_filename, uint32_t count );

         /**
          *
          * @param debug_wif_key
          * @param count
          * @ingroup WalletCLI
          */
         void dbg_generate_blocks( std::string debug_wif_key, uint32_t count );

         /**
          *
          * @param filename
          * @ingroup WalletCLI
          */
         void dbg_stream_json_objects( const std::string& filename );

         /**
          *
          * @param update
          * @ingroup WalletCLI
          */
         void dbg_update_object( fc::variant_object update );

         /**
          *
          * @param prefix
          * @param number_of_transactions
          * @ingroup WalletCLI
          */
         void flood_network(string prefix, uint32_t number_of_transactions);

         /**
          *
          * @param nodes
          * @ingroup WalletCLI
          */
         void network_add_nodes( const vector<string>& nodes );

         /**
          *
          * @ingroup WalletCLI
          */
         vector< variant > network_get_connected_peers();


         std::map<string,std::function<string(fc::variant,const fc::variants&)>> get_result_formatters() const;

         fc::signal<void(bool)> lock_changed;
         std::shared_ptr<detail::wallet_api_impl> my;
         void encrypt_keys();

         /**
          * Get current supply of the core asset
          * @ingroup WalletCLI
          */
         real_supply get_real_supply()const;

         /**
          * @brief Submits or resubmits content to the blockchain. In a case of resubmit, price, seeders, quorum,
          * expiration, publishing fee and synopsis fields can be modified, but expiration time can't be shortened.
          * @see submit_content_new()
          * @param author The author of the content
          * @param URI The URI of the content
          * @param price_asset_name Ticker symbol of the asset which will be used to buy content
          * @param price_amounts The price of the content per regions
          * @param size The size of the content
          * @param hash The Hash of the package
          * @param seeders List of the seeders, which will publish the content
          * @param quorum Defines number of seeders needed to restore the encryption key
          * @param expiration The expiration time of the content. The content is available to buy till it's expiration time
          * @param publishing_fee_asset Ticker symbol of the asset which will be used to publish content
          * @param publishing_fee_amount Publishing price
          * @param synopsis The description of the content
          * @param secret The AES key used to encrypt and decrypt the content
          * @param cd Custody data
          * @param broadcast true to broadcast the transaction on the network
          * @return The signed transaction submitting the content
          * @ingroup WalletCLI
          */
         signed_transaction
         submit_content(string const& author,
                        string const& URI,
                        string const& price_asset_name,
                        vector <pair<string, string>> const& price_amounts,
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
          * @brief This function is used to create package, upload package and submit content in one step.
          * @see create_package()
          * @see upload_package()
          * @see submit_content()
          * @param author The author of the content
          * @param content_dir Path to the directory containing all content that should be packed
          * @param samples_dir Path to the directory containing samples of content
          * @param protocol Protocol for uploading package( magnet or IPFS)
          * @param price_asset_symbol Ticker symbol of the asset which will be used to buy content
          * @param price_amounts The prices of the content per regions
          * @param seeders List of the seeders, which will publish the content
          * @param expiration The expiration time of the content. The content is available to buy till it's expiration time
          * @param synopsis The description of the content
          * @param broadcast true to broadcast the transaction on the network
          * @return The signed transaction submitting the content
          * @ingroup WalletCLI
          */
         fc::ripemd160  submit_content_new(string const& author,
                                               string const& content_dir,
                                               string const& samples_dir,
                                               string const& protocol,
                                               string const& price_asset_symbol,
                                               vector <pair<string, string>> const& price_amounts,
                                               vector<account_id_type> const& seeders,
                                               fc::time_point_sec const& expiration,
                                               string const& synopsis,
                                               bool broadcast = false);

         /**
          * @brief Downloads encrypted content specified by provided URI.
          * @param consumer Consumer of the content
          * @param URI The URI of the content
          * @param region_code_from Two letter region code
          * @param broadcast true to broadcast the transaction on the network
          * @ingroup WalletCLI
          */
         void download_content(string const& consumer, string const& URI, string const& region_code_from, bool broadcast = false);

         /**
          * @brief Get status about particular download process specified by provided URI.
          * @param consumer Consumer of the content
          * @param URI The URI of the content
          * @return Download status, or null if no matching download process was found
          * @ingroup WalletCLI
          */
         optional<content_download_status> get_download_status(string consumer, string URI) const;

         /**
          * @brief This function is used to send a request to buy a content. This request is caught by seeders.
          * @param consumer Consumer of the content
          * @param URI The URI of the content
          * @param price_asset_name Ticker symbol of the asset which will be used to buy content
          * @param price_amount The price of the content
          * @param broadcast true to broadcast the transaction on the network
          * @return The signed transaction requesting buying of the content
          * @ingroup WalletCLI
          */
         signed_transaction request_to_buy(string consumer, string URI, string price_asset_name, string price_amount, bool broadcast);

         /**
          * @brief Rates and/or comments a content.
          * @param consumer Consumer giving the rating
          * @param URI The URI of the content
          * @param rating Rating
          * @param comment Comment
          * @param broadcast true to broadcast the transaction on the network
          * @return The signed transaction adding the rate to the content
          * @ingroup WalletCLI
          */
         signed_transaction leave_rating_and_comment(string consumer,
                                                     string URI,
                                                     uint64_t rating,
                                                     string comment,
                                                     bool broadcast = false);

         /**
          * @brief This function is used to register a new seeder, modify the existing seeder or to extend seeder's lifetime.
          * @param seeder The account becoming a seeder
          * @param space Available space on seeder's disc dedicated to contents
          * @param price_per_MByte The price charged to consumer for downloading 1 MB from seeder
          * @param broadcast true to broadcast the transaction on the network
          * @return The signed transaction registering or modifying the seeder
          * @ingroup WalletCLI
          */
         signed_transaction ready_to_publish(string seeder,
                                             uint64_t space,
                                             uint32_t price_per_MByte,
                                             vector<string> ipfs_IDs,
                                             bool broadcast = false);

         /**
          * @brief
          * @param seeder
          * @param URI The URI of the content
          * @param package
          * @param broadcast true to broadcast the transaction on the network
          * @return
          * @ingroup WalletCLI
          */
         signed_transaction proof_of_custody(string seeder,
                                             string URI,
                                             string package,
                                             bool broadcast = false);

         /**
          * @brief This function is used to send encrypted share of a content and proof of delivery to consumer.
          * @param seeder The seeder of the content
          * @param privKey Seeder's private key used to decrypt encrypted share of the content
          * @param buying The buying object
          * @param broadcast true to broadcast the transaction on the network
          * @return The signed transaction delivering encrypted share from seeder to consumer
          * @ingroup WalletCLI
          */
         signed_transaction deliver_keys(string seeder,
                                         DInteger privKey,
                                         buying_id_type buying,
                                         bool broadcast = false);

         /**
          * @brief Creates a subscription to author. This function is used by consumers.
          * @param from Account who wants subscription to author
          * @param to The author you wish to subscribe to
          * @param duration Duration of subscription in days
          * @param price_amount Price for the subscription
          * @param price_asset_symbol Ticker symbol of the asset which will be used to buy subscription
          * @param broadcast True if you wish to broadcast the transaction
          * @return The signed transaction subscribing the consumer to the author
          * @ingroup WalletCLI
          */
         signed_transaction subscribe_to_author( string from,
                                                 string to,
                                                 uint32_t duration,
                                                 string price_amount,
                                                 string price_asset_symbol,
                                                 bool broadcast/* = false */);

         /**
          * @brief Creates a subscription to author. This function is used by author.
          * @param from The account obtaining subscription from the author
          * @param to The name or id of the author
          * @param duration Duration of subscription in days
          * @param broadcast True if you wish to broadcast the transaction
          * @return The signed transaction subscribing the consumer to the author
          * @ingroup WalletCLI
          */
         signed_transaction subscribe_by_author( string from,
                                                 string to,
                                                 uint32_t duration,
                                                 bool broadcast/* = false */);

         /**
          * @brief This function can be used to allow/disallow subscription.
          * @param account The name or id of the account to update
          * @param allow_subscription True if account (author) wants to allow subscription, false otherwise
          * @param subscription_period Minimal duration of subscription in days
          * @param price_amount Price for subscription per one subscription period
          * @param price_asset_symbol Ticker symbol of the asset which will be used to buy subscription
          * @param broadcast True if you wish to broadcast the transaction
          * @return The signed transaction updating the account
          * @ingroup WalletCLI
          */
         signed_transaction set_subscription( string account,
                                              bool allow_subscription,
                                              uint32_t subscription_period,
                                              string price_amount,
                                              string price_asset_symbol,
                                              bool broadcast/* = false */);

         /**
          * @brief This function can be used to allow/disallow automatic renewal of expired subscription.
          * @param account The name or id of the account to update
          * @param subscription The ID of the subscription.
          * @param automatic_renewal True if account (consumer) wants to allow automatic renewal of subscription, false otherwise
          * @param broadcast True if you wish to broadcast the transaction
          * @return The signed transaction allowing/disallowing renewal of the subscription
          * @ingroup WalletCLI
          */
         signed_transaction set_automatic_renewal_of_subscription( string account_id_or_name,
                                                                   subscription_id_type subscription_id,
                                                                   bool automatic_renewal,
                                                                   bool broadcast/* = false */);

         /**
          * @brief Get a list of consumer's active (not expired) subscriptions.
          * @param account The name or id of the consumer
          * @param count Maximum number of subscriptions to fetch (must not exceed 100)
          * @return List of active subscription objects corresponding to the provided consumer
          * @ingroup WalletCLI
          */
         vector< subscription_object > list_active_subscriptions_by_consumer( const string& account_id_or_name, const uint32_t count)const;

         /**
          * @brief Get a list of consumer's subscriptions.
          * @param account The name or id of the consumer
          * @param count Maximum number of subscriptions to fetch (must not exceed 100)
          * @return List of subscription objects corresponding to the provided consumer
          * @ingroup WalletCLI
          */
         vector< subscription_object > list_subscriptions_by_consumer( const string& account_id_or_name, const uint32_t count)const;

         /**
          * @brief Get a list of active (not expired) subscriptions to author.
          * @param account The name or id of the author
          * @param count Maximum number of subscriptions to fetch (must not exceed 100)
          * @return List of active subscription objects corresponding to the provided author
          * @ingroup WalletCLI
          */
         vector< subscription_object > list_active_subscriptions_by_author( const string& account_id_or_name, const uint32_t count)const;

         /**
          * @brief Get a list of subscriptions to author.
          * @param account The name or id of the author
          * @param count Maximum number of subscriptions to fetch (must not exceed 100)
          * @return List of subscription objects corresponding to the provided author
          * @ingroup WalletCLI
          */
         vector< subscription_object > list_subscriptions_by_author( const string& account_id_or_name, const uint32_t count)const;

          /**
           * @brief This function is used to report stats. These stats are later used to rate seeders.
           * @param consumer The consumer of the content
           * @param stats The final stats about single download process. Map of seeders to amount they uploaded
           * @param broadcast true to broadcast the transaction on the network
           * @return The signed transaction reporting the stats
           * @ingroup WalletCLI
           */
         signed_transaction report_stats(string consumer,
                                         map<account_id_type,uint64_t> stats,
                                         bool broadcast = false);

         /**
          * @brief Restores AES key( used to encrypt and decrypt a content) from key particles stored in a buying object
          * @param account consumers account id or name
          * @param buying The buying object containing key particles
          * @return restored AES key from particles
          * @ingroup WalletCLI
          */
         DInteger restore_encryption_key(std::string account, buying_id_type buying);

         /**
          * @brief Generates private ElGamal key and corresponding public key.
          * @return Pair of ElGamal keys
          * @ingroup WalletCLI
          */
         el_gamal_key_pair generate_el_gamal_keys();

         /**
          * @brief Get a list of open buyings
          * @return Open buying objects
          * @ingroup WalletCLI
          */
         vector<buying_object> get_open_buyings()const;

         /**
          * @brief Get a list of open buyings by URI
          * @param URI URI of the buyings to retrieve
          * @return Open buyings corresponding to the provided URI
          * @ingroup WalletCLI
          */
         vector<buying_object> get_open_buyings_by_URI( const string& URI )const;

         /**
          * @brief Get a list of open buyings by consumer
          * @param consumer Consumer of the buyings to retrieve
          * @return Open buyings corresponding to the provided consumer
          * @ingroup WalletCLI
          */
         vector<buying_object> get_open_buyings_by_consumer( const string& account_id_or_name )const;

         /**
          * @brief Get history buyings by consumer
          * @param account_id_or_name Consumer of the buyings to retrieve
          * @return History buying objects corresponding to the provided consumer
          * @ingroup WalletCLI
          */
         vector<buying_object> get_buying_history_objects_by_consumer( const string& account_id_or_name )const;

         /**
          * @brief Get history buying objects by consumer that match search term
          * @param account_id_or_name Consumer of the buyings to retrieve
          * @param term Search term to look up in Title and Description
          * @param order Sort data by field
          * @param id object_id to start searching from
          * @param count Maximum number of contents to fetch (must not exceed 100)
          * @return History buying objects corresponding to the provided consumer and matching search term
          * @ingroup WalletCLI
          */
         vector<buying_object_ex> search_my_purchases(const string& account_id_or_name,
                                                      const string& term,
                                                      const string& order,
                                                      const string& id,
                                                      uint32_t count) const;

         /**
         * @brief Get buying (open or history) by consumer and URI
         * @param consumer Consumer of the buying to retrieve
         * @param URI URI of the buying to retrieve
         * @return buying_objects corresponding to the provided consumer, or null if no matching buying was found
         * @ingroup WalletCLI
         */
         optional<buying_object> get_buying_by_consumer_URI( const string& account_id_or_name, const string & URI )const;

         /**
          * @brief Get rating given by a consumer to a content
          * @param consumer Consumer giving rating
          * @param URI URI specifying the content
          * @return Rating, if given, empty otherwise
          * @ingroup WalletCLI
          */
         optional<uint64_t> get_rating( const string& consumer, const string & URI )const;

         /**
          * @brief Get comment given by a consumer to a content
          * @param consumer Consumer giving comment
          * @param URI URI specifying the content
          * @return Comment, if given, empty otherwise
          * @ingroup WalletCLI
          */
         optional<string> get_comment( const string& consumer, const string & URI )const;

         /**
          * @brief Get rating and/or comment given by a consumer to a content
          * @param consumer Consumer giving rating and/or comment
          * @param URI URI specifying the content
          * @return Rating and/or comment, if given, empty otherwise
          * @ingroup WalletCLI
          */
         std::pair<optional<uint64_t>, string> get_rating_and_comment( const string& consumer, const string& URI )const;

         /**
          * @brief Get a content by URI
          * @param URI URI of the content to retrieve
          * @return The content corresponding to the provided URI, or null if no matching content was found
          * @ingroup WalletCLI
          */
         optional<content_object> get_content( const string& URI )const;

         /**
          * @brief Get a list of contents by author
          * @param author Author of the contents to retrieve
          * @return The contents corresponding to the provided author
          * @ingroup WalletCLI
          */
         vector<content_object> list_content_by_author( const string& account_id_or_name )const;

         /**
          * @brief Get a list of contents ordered alphabetically by URI string
          * @param URI_begin Lower bound of URI strings to retrieve
          * @param count Maximum number of contents to fetch (must not exceed 100)
          * @return The contents found
          * @ingroup WalletCLI
          */
         vector<content_summary> list_content( const string& URI_begin, uint32_t count )const;
         
         /**
          * @brief Get a list of contents ordered alphabetically by search term
          * @param term seach term
          * @param order Order field
          * @param user Content owner
          * @param region_code Two letter region code
          * @param id The id of content object to start searching from
          * @param count Maximum number of contents to fetch (must not exceed 100)
          * @return The contents found
          * @ingroup WalletCLI
          */
         vector<content_summary> search_content(const string& term,
                                                const string& order,
                                                const string& user,
                                                const string& region_code,
                                                const string& id,
                                                uint32_t count )const;
         /**
          * @brief Get a list of contents ordered alphabetically by search term
          * @param user Content owner
          * @param term seach term
          * @param order Order field
          * @param region_code Two letter region code
          * @param id The id of content object to start searching from
          * @param count Maximum number of contents to fetch (must not exceed 100)
          * @return The contents found
          * @ingroup WalletCLI
          */
         vector<content_summary> search_user_content(const string& user,
                                                     const string& term,
                                                     const string& order,
                                                     const string& region_code,
                                                     const string& id,
                                                     uint32_t count )const;

         /**
          * @brief Get a list of contents by times bought, in decreasing order
          * @param count Maximum number of contents to retrieve
          * @return The contents found
          * @ingroup WalletCLI
          */
         vector<content_object> list_content_by_bought( uint32_t count )const;

         /**
          * @brief Get a list of seeders by price, in increasing order
          * @param count Maximum number of seeders to retrieve
          * @return The seeders found
          * @ingroup WalletCLI
          */
         vector<seeder_object> list_publishers_by_price( uint32_t count )const;

         /**
          * @brief Get a list of content ratings corresponding to the provided URI
          * @param URI URI of the content
          * @return The ratings of the content
          * @ingroup WalletCLI
          */
         vector<uint64_t> get_content_ratings( const string& URI )const;

         /**
          * @brief Get a list of content comments corresponding to the provided URI
          * @param URI URI of the content
          * @return Map of accounts to corresponding comments
          * @ingroup WalletCLI
          */
         map<string, string> get_content_comments( const string& URI )const;

         /**
          * @brief Get a list of IPFS IDs imported by a seeder
          * @param seeder Seeder, whose IPFS IDs should be retrieved
          * @return List of IPFS IDs
          * @ingroup WalletCLI
          */
         vector<string> list_imported_ipfs_IDs( const string& seeder )const;

         /**
          * @brief Get seeders and corresponding lists of IPFS IDs that are used by seeders to publish content provided by URI
          * @param URI URI of the content
          * @return Map of seeders to corresponding lists of IPFS IDs
          * @ingroup WalletCLI
          */
         map<string, vector<string>> list_seeders_ipfs_IDs( const string& URI )const;

         /**
          * @brief Get a list of seeders ordered by total upload, in decreasing order
          * @param count Maximum number of seeders to retrieve
          * @return The seeders found
          * @ingroup WalletCLI
          */
         optional<vector<seeder_object>> list_seeders_by_upload( const uint32_t count )const;

         

         /**
          * @brief Create package from selected files
          * @param content_dir Directory containing all content that should be packed
          * @param samples_dir Directory containing samples of content
          * @param aes_key AES enryption key
          * @return package hash (ripemd160 hash of package content) and content custody data
          * @ingroup WalletCLI
          */
         std::pair<string, decent::encrypt::CustodyData> create_package(const std::string& content_dir, const std::string& samples_dir, const DInteger& aes_key) const;


         /**
          * @brief Extract selected package
          * @param package_hash Hash of package that needs to be extracted
          * @param output_dir Directory where extracted files will be created
          * @param aes_key AES decryption key
          * @return Nothing
          * @ingroup WalletCLI
          */
         void extract_package(const std::string& package_hash, const std::string& output_dir, const DInteger& aes_key) const;

         /**
          * @brief Download package
          * @param url Magnet or IPFS URL of package
          * @return nothing
          * @ingroup WalletCLI
          */
         void download_package(const std::string& url) const;

         /**
          * @brief Start uploading package
          * @param package_hash Hash of package that needs to be extracted
          * @param protocol protocol for uploading package magnet or ipfs
          * @return nothing
          * @ingroup WalletCLI
          */
         std::string upload_package(const std::string& package_hash, const std::string& protocol) const;

         /**
          * @brief Remove package
          * @param package_hash Hash of package that needs to be removed
          * @return nothing
          * @ingroup WalletCLI
          */
         void remove_package(const std::string& package_hash) const;

         /**
          * @brief Print statuses of all active transfers
          * @return nothing
          * @ingroup WalletCLI
          */

         void set_transfer_logs(bool enable) const;

         /**
          * @brief Query the last local block
          * @return the block time
          */
         fc::time_point_sec head_block_time() const;
      };

   } }


FC_REFLECT( graphene::wallet::plain_keys, (keys)(checksum) )
FC_REFLECT( graphene::wallet::el_gamal_key_pair, (private_key)(public_key) )
FC_REFLECT( graphene::wallet::wallet_data,
            (chain_id)
               (my_accounts)
               (priv_el_gamal_key)
               (cipher_keys)
               (extra_keys)
               (pending_account_registrations)(pending_witness_registrations)
               (ws_server)
               (ws_user)
               (ws_password)
)

FC_REFLECT( graphene::wallet::brain_key_info,
            (brain_priv_key)
               (wif_priv_key)
               (pub_key)
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
                    (block_id)(signing_key)(transaction_ids) )

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
           (import_el_gamal_key)
           (import_accounts)
           (import_account_keys)
           (suggest_brain_key)
           (register_account)
           (create_account_with_brain_key)
           (sell_asset)
           (sell)
           (buy)
           (cancel_order)
           (transfer)
           (transfer2)
           (get_transaction_id)
           //(create_asset)
           //(update_asset)
           (update_monitored_asset)
           (publish_asset_feed)
           //(issue_asset)
           (get_asset)
           (get_monitored_asset_data)
           (get_witness)
           (list_witnesses)
           (create_witness)
           (update_witness)
           (get_vesting_balances)
           (withdraw_vesting)
           (vote_for_witness)
           (set_voting_proxy)
           (set_desired_witness_count)
           (get_account)
           (get_account_id)
           (get_block)
           (get_account_count)
           (search_account_history)
           (get_market_history)
           (get_global_properties)
           (get_dynamic_global_properties)
           (get_object)
           (get_private_key)
           (load_wallet_file)
           (normalize_brain_key)
           (get_limit_orders)
           (save_wallet_file)
           (serialize_transaction)
           (sign_transaction)
           (get_prototype_operation)
           (propose_parameter_change)
           (propose_fee_change)
           (approve_proposal)
           (dbg_make_uia)
           (dbg_make_mia)
           (dbg_push_blocks)
           (dbg_generate_blocks)
           (dbg_stream_json_objects)
           (dbg_update_object)
           (flood_network)
           (network_add_nodes)
           (network_get_connected_peers)
           (get_order_book)
           (download_content)
           (get_download_status)
           (submit_content)
           (submit_content_new)
           (request_to_buy)
           (leave_rating_and_comment)
           (ready_to_publish)
           (proof_of_custody)
           (deliver_keys)
           (restore_encryption_key)
           (generate_el_gamal_keys)
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
           (get_rating)
           (get_comment)
           (get_rating_and_comment)
           (get_content)
           (get_real_supply)
           (list_content_by_author)
           (list_content)
           (search_content)
           (search_user_content)
           (list_content_by_bought)
           (list_publishers_by_price)
           (get_content_ratings)
           (get_content_comments)
           (list_imported_ipfs_IDs)
           (create_package)
           (extract_package)
           (download_package)
           (upload_package)
           (remove_package)
           (set_transfer_logs)
           (head_block_time)
)
