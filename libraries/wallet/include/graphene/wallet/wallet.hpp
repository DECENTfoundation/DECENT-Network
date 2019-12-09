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
#include <boost/signals2/signal.hpp>

namespace fc
{
   void to_variant(const graphene::chain::account_multi_index_type& accts, variant& vo);
   void from_variant(const variant &var, graphene::chain::account_multi_index_type &vo);
}

namespace graphene { namespace wallet {

namespace fs = boost::filesystem;
typedef uint16_t transaction_handle_type;

/**
 * This class takes a variant and turns it into an object
 * of the given type, with the new operator.
 */

db::object* create_object( const fc::variant& v );

struct server_data
{
   std::string server;
   std::string user;
   std::string password;
};

struct wallet_data
{
   int version = 0;
   /** Chain ID this wallet is used with */
   chain::chain_id_type chain_id;
   chain::account_multi_index_type my_accounts;
   /// @return IDs of all accounts in @ref my_accounts
   std::vector<db::object_id_type> my_account_ids()const
   {
      std::vector<db::object_id_type> ids;
      ids.reserve(my_accounts.size());
      std::transform(my_accounts.begin(), my_accounts.end(), std::back_inserter(ids),
                     [](const chain::account_object& ao) { return ao.id; });
      return ids;
   }
   /// @brief Add acct to @ref my_accounts, or update it if it is already in @ref my_accounts
   /// @return \c true if the account was newly inserted; \c false if it was only updated
   bool update_account(const chain::account_object& acct)
   {
      auto& idx = my_accounts.get<db::by_id>();
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
   std::vector<char> cipher_keys;

   /** map an account to a set of extra keys that have been imported for that account */
   std::map<chain::account_id_type, std::set<chain::public_key_type>> extra_keys;

   // map of account_name -> base58_private_key for
   //    incomplete account regs
   std::map<std::string, std::vector<std::string>> pending_account_registrations;
   std::map<std::string, std::string> pending_miner_registrations;

   std::string ws_server = "ws://localhost:8090";
   std::string ws_user;
   std::string ws_password;
   std::string update_time;
};

struct wallet_about
{
   decent::about_info daemon_info;
   decent::about_info wallet_info;
};

struct wallet_info
{
   uint32_t head_block_num;
   chain::block_id_type head_block_id;
   std::string head_block_age;
   std::string next_maintenance_time;
   chain::chain_id_type chain_id;
   double participation;
   std::vector<chain::miner_id_type> active_miners;
};

struct el_gamal_key_pair
{
   decent::encrypt::DInteger private_key;
   decent::encrypt::DInteger public_key;
};

struct el_gamal_key_pair_str
{
   decent::encrypt::DIntegerString private_key;
   decent::encrypt::DIntegerString public_key;
};

/**
 * Needed for backward compatibility. Old wallet json files use this struct to store encrypted ec keys.
 */
struct plain_keys
{
   std::map<chain::public_key_type, std::string> ec_keys;
   fc::sha512 checksum;
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
   std::vector<el_gamal_key_pair_str> el_gamal_keys;
};

struct brain_key_info
{
   std::string brain_priv_key;
   std::string wif_priv_key;
   chain::public_key_type pub_key;
};

struct exported_account_keys
{
   std::string account_name;
   std::vector<std::vector<char>> encrypted_private_keys;
   std::vector<chain::public_key_type> public_keys;
};

struct exported_keys
{
   fc::sha512 password_checksum;
   std::vector<exported_account_keys> account_keys;
};

struct approval_delta
{
   std::vector<std::string> active_approvals_to_add;
   std::vector<std::string> active_approvals_to_remove;
   std::vector<std::string> owner_approvals_to_add;
   std::vector<std::string> owner_approvals_to_remove;
   std::vector<std::string> key_approvals_to_add;
   std::vector<std::string> key_approvals_to_remove;
};

struct regional_price_info
{
   std::string region;
   std::string amount;
   std::string asset_symbol;
};

struct content_download_status
{
   int total_key_parts;
   int received_key_parts;
   int total_download_bytes;
   int received_download_bytes;
   std::string status_text;
};

struct operation_detail
{
   std::string memo;
   std::string description;
   chain::operation_history_object op;
};

struct buying_object_ex : public chain::buying_object, public content_download_status {
   buying_object_ex(const chain::buying_object& obj, const content_download_status& status)
    : chain::buying_object(obj), content_download_status(status)
   {
      // buying_object price is used for other purposes
      // but the GUI relies on buying_object_ex::price
      // so overwrite as a quick fix
      price = paid_price_before_exchange;
      this->id = std::string(obj.id);
   }

   std::string id;
   std::string author_account;
   uint32_t times_bought;
   fc::ripemd160 hash;
   uint32_t AVG_rating;
};

struct rating_object_ex : public chain::buying_object
{
   rating_object_ex( const chain::buying_object& buying, const std::string& author )
   : chain::buying_object( buying ), author(author) {}
   std::string author;
};

struct vesting_balance_object_with_info : public chain::vesting_balance_object
{
   vesting_balance_object_with_info( const chain::vesting_balance_object& vbo, fc::time_point_sec now );
   vesting_balance_object_with_info( const vesting_balance_object_with_info& vbo ) = default;

   /**
    * How much is allowed to be withdrawn.
    */
   chain::asset allowed_withdraw;

   /**
    * The time at which allowed_withdrawal was calculated.
    */
   fc::time_point_sec allowed_withdraw_time;
};

struct balance_change_result_detail : public app::balance_change_result
{
   std::string memo;
   std::string description;
};

struct extended_asset : public chain::asset
{
   extended_asset( const chain::asset& a, const std::string& pretty_amount )
      : chain::asset( a ), pretty_amount( pretty_amount ) {}
   std::string pretty_amount;
};

struct signed_transaction_info : public chain::signed_transaction
{
   signed_transaction_info(const chain::signed_transaction& tx)
      : chain::signed_transaction( tx ), transaction_id( tx.id() ) {}
   chain::transaction_id_type transaction_id;
};

struct message_data : public chain::message_object
{
   message_data() = default;
   message_data(const chain::message_object& obj) : chain::message_object(obj) {}
   std::string text;
};

struct text_message
{
   fc::time_point_sec created;
   std::string from;
   std::vector<std::string> to;
   std::string text;
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
   wallet_api( const fc::api<app::login_api> &rapi, const chain::chain_id_type &chain_id, const server_data &ws );
   virtual ~wallet_api();

   /**
    * @brief Returns info such as client version, git version of graphene/fc, version of boost, openssl.
    * @return compile time info and client and dependencies versions
    * @ingroup WalletAPI_General
    */
   wallet_about about() const;

   /**
    * @brief Retrieve a full, signed block with info.
    * @param num ID/height of the block
    * @return the referenced block with info, or \c null if no matching block was found
    * @ingroup WalletAPI_General
    */
   fc::optional<chain::signed_block_with_info> get_block(uint32_t num) const;

   /**
    * @brief Returns the blockchain's slowly-changing properties.
    * This object contains all of the properties of the blockchain that are fixed
    * or that change only once per maintenance interval such as the
    * current list of miners, block interval, etc.
    * @see \c get_dynamic_global_properties() for frequently changing properties
    * @return the global properties
    * @ingroup WalletAPI_General
    */
   chain::global_property_object get_global_properties() const;

   /**
    * @brief Returns the blockchain's rapidly-changing properties.
    * The returned object contains information that changes every block interval
    * such as the head block number, the next miner, etc.
    * @see \c get_global_properties() for less-frequently changing properties
    * @return the dynamic global properties
    * @ingroup WalletAPI_General
    */
   chain::dynamic_global_property_object get_dynamic_global_properties() const;

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
   fc::variant get_object(db::object_id_type id) const;

   /**
    * @brief Query the last local block.
    * @return the block time
    * @ingroup WalletAPI_General
    */
   fc::time_point_sec head_block_time() const;

   /**
    * @brief Get information about current state of the blockchain,
    * such as head block number, chain_id, list of active miners,...
    * @return information about current state of the blockchain
    * @ingroup WalletAPI_General
    */
   wallet_info info() const;

   /**
    * @brief Returns a list of all commands supported by the wallet API.
    * This lists each command, along with its arguments and return types.
    * For more detailed help on a single command, use \c get_help()
    * @return a multi-line string suitable for displaying on a terminal
    * @ingroup WalletAPI_General
    */
   std::string help() const;

   /**
    * @brief Returns detailed help on a single API command.
    * @param method the name of the API command you want help with
    * @return a multi-line string suitable for displaying on a terminal
    * @ingroup WalletAPI_General
    */
   std::string get_help(const std::string& method) const;

   /**
    * @brief Sign a buffer.
    * @param str_buffer the buffer to be signed
    * @param str_brainkey derives the private key used for signature
    * @return the signed buffer
    * @ingroup WalletAPI_General
    */
   std::string sign_buffer(const std::string& str_buffer,
                           const std::string& str_brainkey) const;

   /**
    * @brief Verify if the signature is valid.
    * @param str_buffer the original buffer
    * @param str_publickey the public key used for verification
    * @param str_signature the signed buffer
    * @return \c true if valid, otherwise \c false
    * @ingroup WalletAPI_General
    */
   bool verify_signature(const std::string& str_buffer,
                         const std::string& str_publickey,
                         const std::string& str_signature) const;

   /**
    * @brief
    * @param nodes
    * @ingroup WalletAPI_General
    */
   void network_add_nodes(const std::vector<std::string>& nodes) const;

   /**
    * @brief
    * @ingroup WalletAPI_General
    */
   fc::variants network_get_connected_peers() const;

   /**
    * @brief This method is used to convert a JSON transaction to its transaction ID.
    * @param trx Signed transaction
    * @return The transaction ID
    * @ingroup WalletAPI_General
    */
   chain::transaction_id_type get_transaction_id(const chain::signed_transaction& trx) const;

   /**
    * @brief This method will return the transaction for the given ID (transaction hash) or
    * it will return \c null if it is not known. Just because it is not known does not mean it wasn't
    * included in the blockchain.
    * @note By default these objects are not tracked, the transaction_history_plugin must
    * be loaded for these objects to be maintained.
    * @param id ID (transaction hash) of the transaction to retrieve
    * @return The transaction
    * @ingroup WalletAPI_General
    */
   fc::optional<chain::processed_transaction> get_transaction_by_id(chain::transaction_id_type id) const;

   /**
    * @brief Listing the operations available.
    * @return a vector of operations with ids, names and fees
    * @ingroup WalletAPI_General
    */
   std::vector<app::operation_info> list_operations() const;

   /**
    * @brief This method encapsulates the functionality of running a sequence of calls
    * loaded from a text file.
    * @param command_file_name The name of the command file to load
    * @return a string with an aggregated result for all operations
    * @ingroup WalletAPI_General
    */
   std::string from_command_file( const std::string& command_file_name ) const;

   /**
    * @brief Lists all accounts controlled by this wallet.
    * This returns a list of the full account objects for all accounts whose private keys
    * we possess.
    * @return a list of accounts imported in the wallet
    * @ingroup WalletAPI_Wallet
    */
   std::vector<chain::account_object> list_my_accounts() const;

   /**
    * @brief Returns the current wallet filename.
    * @note This is the filename that will be used when automatically saving the wallet.
    * @return the wallet filename
    * @ingroup WalletAPI_Wallet
    */
   fs::path get_wallet_filename() const;

   /**
    * @brief Sets the wallet filename used for future writes.
    * @param wallet_filename the wallet filename that will be used when automatically saving the wallet
    * @ingroup WalletAPI_Wallet
    */
   void set_wallet_filename( const fs::path& wallet_filename );

   /**
    * @brief Get the WIF private key corresponding to a public key.  The
    * private key must already be imported in the wallet.
    * @note The wallet needs to be unlocked and a required key needs to be imported.
    * @param pubkey public key
    * @return WIF private key corresponding to a public key
    * @ingroup WalletAPI_Wallet
    */
   std::string get_private_key(const chain::public_key_type& pubkey) const;

   /**
    * @brief Checks whether the wallet has just been created and has not yet had a password set.
    * Calling \c set_password() will transition the wallet to the locked state.
    * @return \c true if the wallet is new
    * @ingroup WalletAPI_Wallet
    */
   bool is_new() const;

   /**
    * @brief Checks whether the wallet is locked (is unable to use its private keys).
    * This state can be changed by calling \c lock() or \c unlock().
    * @see \c unlock()
    * @return \c true if the wallet is locked
    * @ingroup WalletAPI_Wallet
    */
   bool is_locked() const;

   /**
    * @brief Locks the wallet immediately.
    * @return \c true if the wallet is successfuly locked and \c false if wallet is already locked
    * @see \c unlock()
    * @ingroup WalletAPI_Wallet
    */
   bool lock();

   /**
    * @brief Unlocks the wallet.
    * The wallet remain unlocked until the \c lock() is called
    * or the program exits.
    * @param password the password previously set with \c set_password()
    * @return \c true if the wallet is successfuly unlocked and \c false if wallet is already unlocked
    * @ingroup WalletAPI_Wallet
    */
   bool unlock(const std::string& password);

   /**
    * @brief Sets a new password on the wallet.
    * The wallet must be either \c new or \c unlocked to execute this command.
    * @param password
    * @ingroup WalletAPI_Wallet
    */
   void set_password(const std::string& password);

   /**
    * @brief Loads a specified wallet file.
    * The current wallet is closed before the new wallet is loaded.
    * @warning This changes the filename that will be used for future wallet writes.
    * @param wallet_filename the filename of the wallet JSON file to load.
    *                        If \c wallet_filename is empty, it reloads the
    *                        existing wallet file
    * @return \c true if the specified wallet is loaded
    * @ingroup WalletAPI_Wallet
    */
   bool load_wallet_file(const fs::path& wallet_filename = fs::path());

   /**
    * @brief Saves the current wallet to the given filename.
    * @note The wallet needs to be unlocked.
    * @warning This does not change the wallet filename that will be used for future
    * writes, so think of this function as 'Save a Copy As...' instead of 'Save As...'.
    * @param wallet_filename the filename of the new wallet JSON file to create
    *                        or overwrite.  If \c wallet_filename is empty,
    *                        save to the current filename.
    * @ingroup WalletAPI_Wallet
    */
   void save_wallet_file(const fs::path& wallet_filename = fs::path());

   /**
    * @brief Imports the private key for an existing account.
    * The private key should match an owner key, an active key or a memo key for the
    * named account.
    * @note This command also automatically derives and imports active and memo key
    * if the private key match the owner key.
    * @note The wallet needs to be unlocked.
    * @see dump_private_keys()
    * @see list_my_accounts()
    * @param account_name_or_id the account owning the key
    * @param wif_key the private key in WIF format
    * @return \c true if the key was imported
    * @ingroup WalletAPI_Wallet
    */
   bool import_key(const std::string& account_name_or_id, const std::string& wif_key);

   /**
    * @brief Imports a private key for an existing account.
    * The private key should match an owner key, an active key or a memo key for the
    * named account.
    * @note The wallet needs to be unlocked.
    * @see dump_private_keys()
    * @see list_my_accounts()
    * @param account_name_or_id the account owning the key
    * @param wif_key the private key in WIF format
    * @return \c true if the key was imported
    * @ingroup WalletAPI_Wallet
    */
   bool import_single_key(const std::string& account_name_or_id, const std::string& wif_key);

   /**
    * @brief Dumps all private keys successfully imported in the wallet.
    * @note The keys are printed in WIF format.  You can import these keys into another wallet
    * using \c import_key()
    * @note The wallet needs to be unlocked.
    * @return a map containing the private keys and corresponding public keys
    * @ingroup WalletAPI_Wallet
    */
   fc::variant dump_private_keys() const;

   /**
    * @brief Returns the number of accounts registered on the blockchain.
    * @return the number of registered accounts
    * @ingroup WalletAPI_Account
    */
   uint64_t get_account_count() const;

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
   std::map<std::string, chain::account_id_type> list_accounts(const std::string& lowerbound, uint32_t limit) const;

   /**
    * @brief Get registered accounts that match search term
    * @param term will try to partially match account name or id
    * @param limit maximum number of results to return ( must not exceed 1000 )
    * @param order sort data by field
    * @param id object_id to start searching from
    * @return map of account names to corresponding IDs
    * @ingroup WalletAPI_Account
    */
   std::vector<chain::account_object> search_accounts(const std::string& term, const std::string& order, const std::string& id, uint32_t limit) const;

   /**
    * @brief List the balances of an account.
    * Each account can have multiple balances, one for each type of asset owned by that
    * account.
    * @param id the name or id of the account whose balances you want
    * @return a list of the given account's balances
    * @ingroup WalletAPI_Account
    */
   std::vector<extended_asset> list_account_balances(const std::string& id) const;

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
   std::vector<chain::transaction_detail_object> search_account_history(const std::string& account_name,
                                                                 const std::string& order,
                                                                 const std::string& id,
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
   std::vector<operation_detail> get_account_history(const std::string& name, int limit) const;

   /**
    * @brief Get operations relevant to the specified account referenced
    * by an event numbering specific to the account. The current number of operations
    * for the account can be found in the account statistics (or use 0 for start).
    * @note The sequence number of the oldest operation is 1 and the operations are in increasing order,
    * from the oldest operation to the most recent.
    * @param name The account whose history should be queried
    * @param stop Sequence number of earliest operation. 0 is default and will
    * query 'limit' number of operations
    * @param limit Maximum number of operations to retrieve (must not exceed 100)
    * @param start Sequence number of the most recent operation to retrieve
    * 0 is default, which will start querying from the most recent operation
    * @return A list of operations performed by account, ordered from most recent to oldest
    * @ingroup WalletAPI_Account
    */
   std::vector<operation_detail> get_relative_account_history(const std::string& name, uint32_t stop, int limit, uint32_t start) const;

   /**
    * @brief Returns the most recent balance operations on the named account.
    * This returns a list of operation history objects, which describe activity on the account.
    * @param account_name the name or id of the account
    * @param assets_list list of asset names to filter or empty for all assets
    * @param partner_account partner account_id to filter transfers to speccific account or empty
    * @param from_block filtering parameter, starting block number (can be determined by from time) or zero when not used
    * @param to_block filtering parameter, ending block number or zero when not used
    * @param start_offset starting offset from zero
    * @param limit the number of entries to return (starting from the most recent)
    * @return a list of balance operation history objects
    * @ingroup WalletAPI_Account
    */
   std::vector<balance_change_result_detail> search_account_balance_history(const std::string& account_name,
                                                                    const boost::container::flat_set<std::string>& assets_list,
                                                                    const std::string& partner_account,
                                                                    uint32_t from_block, uint32_t to_block,
                                                                    uint32_t start_offset,
                                                                    int limit) const;

   /**
    * @brief Returns the most recent balance operations on the named account.
    * @param account_name the name or id of the account
    * @param operation_history_id the operation_history_id to search for
    * @return returns balance_change_result_detail or empty when not found
    * @ingroup WalletAPI_Account
    */
   fc::optional<balance_change_result_detail> get_account_balance_for_transaction(const std::string& account_name,
                                                                                  chain::operation_history_id_type operation_history_id) const;

   /**
    * @brief Returns information about the given account.
    * @param account_name_or_id the name or id of the account to provide information about
    * @return the public account data stored in the blockchain
    * @ingroup WalletAPI_Account
    */
   chain::account_object get_account(const std::string& account_name_or_id) const;

   /**
    * @brief Derive private key from given prefix and sequence.
    * @param prefix_string
    * @param sequence_number
    * @return derived private key
    * @ingroup WalletAPI_Account
    */
   std::string derive_private_key(const std::string& prefix_string, int sequence_number) const;

   /**
    * @brief Get public key from given private key.
    * @param wif_private_key the private key in wallet import format
    * @return corresponding public key
    * @ingroup WalletAPI_Account
    */
   chain::public_key_type get_public_key( const std::string& wif_private_key ) const;

   /**
    * @brief Suggests a safe brain key to use for creating your account.
    * \c create_account_with_brain_key() requires you to specify a brain key,
    * a long passphrase that provides enough entropy to generate cyrptographic
    * keys.  This function will suggest a suitably random string that should
    * be easy to write down (and, with effort, memorize).
    * @return a suggested brain key
    * @ingroup WalletAPI_Account
    */
   brain_key_info suggest_brain_key() const;

   /**
    * @brief Calculates the private key and public key corresponding to any brain key
    * @param brain_key the brain key to be used for calculation
    * @return the corresponding \c brain_key_info
    * @ingroup WalletAPI_Account
    */
   brain_key_info get_brain_key_info(const std::string& brain_key) const;

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
   std::pair<brain_key_info, el_gamal_key_pair> generate_brain_key_el_gamal_key() const;

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
    * @note The memo key is the key this account will typically use to encrypt/sign transaction memos and other non-
      validated account activities.
    * @see suggest_brain_key()
    * @param name the name of the account, must be unique on the blockchain and contains at least 5 characters
    * @param owner the owner key for the new account
    * @param active the active key for the new account
    * @param memo the memo key for the new account
    * @param registrar_account the account which will pay the fee to register the user
    * @param broadcast \c true to broadcast the transaction on the network
    * @return the signed transaction registering the account
    * @ingroup WalletAPI_Account
    */
   signed_transaction_info register_account_with_keys(const std::string& name,
                                                      const chain::public_key_type& owner,
                                                      const chain::public_key_type& active,
                                                      const chain::public_key_type& memo,
                                                      const std::string& registrar_account,
                                                      bool broadcast = false);

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
   signed_transaction_info register_account(const std::string& name,
                                            const chain::public_key_type& owner,
                                            const chain::public_key_type& active,
                                            const std::string& registrar_account,
                                            bool broadcast = false);

   /**
    * @brief Registers a third party's multisignature account on the blockckain.
    * This function is used to register an account for which you do not own the private keys.
    * When acting as a registrar, an end user will generate their own private keys and send
    * you the public keys or account ID/IDs.  The registrar will use this function to register the account
    * on behalf of the end user.
    * @note The owner authority represents absolute control over the account. Generally, the only time the owner authority
    * is required is to update the active authority.
    * @note The active authority represents the hot key/keys of the account. This authority has control over nearly all
    * operations the account may perform.
    * @note The memo key is the key this account will typically use to encrypt/sign transaction memos and other non-
      validated account activities.
    * @see suggest_brain_key()
    * @param name the name of the account, must be unique on the blockchain and contains at least 5 characters
    * @param owner the owner authority for the new account
    * @param active the active authority for the new account
    * @param memo the memo public_key_type for the new account
    * @param registrar_account the account which will pay the fee to register the user
    * @param broadcast \c true to broadcast the transaction on the network
    * @return the signed transaction registering the account
    * @ingroup WalletAPI_Account
    */
   signed_transaction_info register_multisig_account(const std::string& name,
                                                     const chain::authority& owner,
                                                     const chain::authority& active,
                                                     const chain::public_key_type& memo,
                                                     const std::string& registrar_account,
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
   signed_transaction_info create_account_with_brain_key(const std::string& brain_key,
                                                         const std::string& account_name,
                                                         const std::string& registrar_account,
                                                         bool broadcast = false);

   /**
    * @brief Updates an account keys.
    * This function is used to update an account keys.
    * At least one account key needs to be specified. Use empty string to specify keys you do not want to update.
    * @note The owner key represents absolute control over the account. Generally, the only time the owner key is required
    * is to update the active key.
    * @note The active key represents the hot key of the account. This key has control over nearly all
    * operations the account may perform.
    * @note The memo key is the key this account will typically use to encrypt/sign transaction memos and other non-
      validated account activities.
    * @see suggest_brain_key()
    * @param name the name of the account to update
    * @param owner the new owner key for the account
    * @param active the new active key for the account
    * @param memo the new memo key for the account
    * @param broadcast \c true to broadcast the transaction on the network
    * @return the signed transaction registering the account
    * @ingroup WalletAPI_Account
    */
   signed_transaction_info update_account_keys(const std::string& name,
                                               const std::string& owner,
                                               const std::string& active,
                                               const std::string& memo,
                                               bool broadcast = false);

   /**
    * @brief Updates an account keys.
    * This function is used to update an account authorities.
    * At least one account key needs to be specified. Use empty string to specify keys you do not want to update.
    * @note The owner authority represents absolute control over the account. Generally, the only time the owner authority
    * is required is to update the active authority.
    * @note The active authority represents the hot key/keys of the account. This authority has control over nearly all
    * operations the account may perform.
    * @note The memo key is the key this account will typically use to encrypt/sign transaction memos and other non-
      validated account activities.
    * @see suggest_brain_key()
    * @param name the name of the account to update
    * @param owner the new owner authority for the account
    * @param active the new active authority for the account
    * @param memo the new memo key for the account
    * @param broadcast \c true to broadcast the transaction on the network
    * @return the signed transaction registering the account
    * @ingroup WalletAPI_Account
    */
   signed_transaction_info update_account_keys_to_multisig(const std::string& name,
                                                           const chain::authority& owner,
                                                           const chain::authority& active,
                                                           const chain::public_key_type& memo,
                                                           bool broadcast = false);

   /**
    * @brief Transfer an amount from one account to another account or to content.
    * In the case of transferring to a content, amount is transferred to author and co-authors of the content,
    * if they are specified.
    * @param from the name or id of the account sending the funds
    * @param to the name or id of the account or id of the content receiving the funds
    * @param amount the amount to send (in nominal units -- to send half of a DCT, specify 0.5)
    * @param asset_symbol the symbol or id of the asset to send
    * @param memo a memo to attach to the transaction.  The memo will be encrypted in the
    *             transaction and readable for the receiver. There is no length limit
    *             other than the limit imposed by maximum transaction size.
    * @note transaction fee is fixed and does not depend on the length of the memo
    * @param broadcast \c true to broadcast the transaction on the network
    * @return the signed transaction transferring funds
    * @ingroup WalletAPI_Account
    */
   signed_transaction_info transfer(const std::string& from,
                                    const std::string& to,
                                    const std::string& amount,
                                    const std::string& asset_symbol,
                                    const std::string& memo,
                                    bool broadcast = false);

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
   el_gamal_key_pair_str get_el_gammal_key(const std::string& consumer) const;

   /**
    * @brief Lists all assets registered on the blockchain.
    * To list all assets, pass the empty string \c "" for the \c lowerbound to start
    * at the beginning of the list, and iterate as necessary.
    * @param lowerbound  the symbol of the first asset to include in the list
    * @param limit the maximum number of assets to return (max: 100)
    * @return the list of asset objects, ordered by symbol
    * @ingroup WalletAPI_Asset
    */
   std::vector<chain::asset_object> list_assets(const std::string& lowerbound, uint32_t limit) const;

   /**
    * @brief Returns information about the given asset.
    * @param asset_name_or_id the symbol or id of the asset in question
    * @return the information about the asset stored in the block chain
    * @ingroup WalletAPI_Asset
    */
   chain::asset_object get_asset(const std::string& asset_name_or_id) const;

   /**
    * @brief Returns the specific data for a given monitored asset.
    * @see \c get_asset()
    * @param asset_name_or_id the symbol or id of the monitored asset in question
    * @return the specific data for this monitored asset
    * @ingroup WalletAPI_Asset
    */
   chain::monitored_asset_options get_monitored_asset_data(const std::string& asset_name_or_id) const;

   /**
    * @brief Creates a new monitored asset.
    * @note some parameters can be changed later using \c update_monitored_asset()
    * @note The wallet needs to be unlocked and a required key/s needs to be imported.
    * @param issuer the name or id of the account who will pay the fee and become the
    *               issuer of the new asset.  This can be updated later
    * @param symbol the ticker symbol of the new asset
    * @param precision the number of digits of precision to the right of the decimal point,
    *                  must be less than or equal to 12
    * @param description asset description. Maximal length is 100 chars
    * @param feed_lifetime_sec time before a price feed expires
    * @param minimum_feeds minimum number of unexpired feeds required to extract a median feed from
    * @param broadcast \c true to broadcast the transaction on the network
    * @return the signed transaction creating a new asset
    * @ingroup WalletAPI_Asset
    */
   signed_transaction_info create_monitored_asset(const std::string& issuer,
                                                  const std::string& symbol,
                                                  uint8_t precision,
                                                  const std::string& description,
                                                  uint32_t feed_lifetime_sec,
                                                  uint8_t minimum_feeds,
                                                  bool broadcast = false);

   /**
    * @brief Update the parameters specific to a monitored asset.
    * @note The wallet needs to be unlocked and a required key/s needs to be imported.
    * @param symbol the name or id of the asset to update, which must be a monitored asset
    * @param description asset description. Maximal length is 100 chars
    * @param feed_lifetime_sec time before a price feed expires
    * @param minimum_feeds minimum number of unexpired feeds required to extract a median feed from
    * @param broadcast \c true to broadcast the transaction on the network
    * @return the signed transaction updating the monitored asset
    * @ingroup WalletAPI_Asset
    */
   signed_transaction_info update_monitored_asset(const std::string& symbol,
                                                  const std::string& description,
                                                  uint32_t feed_lifetime_sec,
                                                  uint8_t minimum_feeds,
                                                  bool broadcast = false);

   /**
    * @brief Creates a new user-issued asset.
    * @note Some parameters can be changed later using \c update_user_issued_asset()
    * @note The wallet needs to be unlocked and a required key/s needs to be imported.
    * @see \c issue_asset()
    * @param issuer the name or id of the account who will pay the fee and become the
    *               issuer of the new asset.  This can be updated later
    * @param symbol the ticker symbol of the new asset
    * @param precision the number of digits of precision to the right of the decimal point,
    *               must be less than or equal to 12
    * @param description asset description. Maximal length is 100 chars
    * @param max_supply the maximum supply of this asset which may exist at any given time
    * @param core_exchange_rate core_exchange_rate is a price struct which consist from base asset
    *               and quote asset (see price). One of the asset has to be core asset.
    *               Technically core_exchange_rate needs to store the asset id of
    *               this new asset. Since this id is not known at the time this operation is
    *               created, create this price as though the new asset id has instance 1, and
    *               the chain will overwrite it with the new asset's id
    * @param is_exchangeable \c true to allow implicit conversion when buing content of this asset to/from core asset
    * @param is_fixed_max_supply true to deny future modifications of 'max_supply' otherwise false
    * @param broadcast \c true to broadcast the transaction on the network
    * @return the signed transaction creating a new asset
    * @ingroup WalletAPI_Asset
    */
   signed_transaction_info create_user_issued_asset(const std::string& issuer,
                                                    const std::string& symbol,
                                                    uint8_t precision,
                                                    const std::string& description,
                                                    uint64_t max_supply,
                                                    chain::price core_exchange_rate,
                                                    bool is_exchangeable,
                                                    bool is_fixed_max_supply,
                                                    bool broadcast = false);

   /**
    * @brief Issue new shares of an asset.
    * @note The wallet needs to be unlocked and a required key/s needs to be imported.
    * @param to_account the name or id of the account to receive the new shares
    * @param amount the amount to issue, in nominal units
    * @param symbol the ticker symbol of the asset to issue
    * @param memo a memo to include in the transaction, readable by the recipient
    * @param broadcast \c true to broadcast the transaction on the network
    * @return the signed transaction issuing the new shares
    * @ingroup WalletAPI_Asset
    */
   signed_transaction_info issue_asset(const std::string& to_account,
                                       const std::string& amount,
                                       const std::string& symbol,
                                       const std::string& memo,
                                       bool broadcast = false);

   /**
    * @brief Update the parameters specific to a user issued asset.
    * User issued assets have some options which are not relevant to other asset types. This operation is used to update those
    * options an an existing user issues asset.
    * @note The wallet needs to be unlocked and a required key/s needs to be imported.
    * @param symbol the name or id of the asset to update, which must be a user-issued asset
    * @param new_issuer if the asset is to be given a new issuer, specify his ID here
    * @param description asset description. Maximal length is 100 chars
    * @param max_supply the maximum supply of this asset which may exist at any given time
    * @param core_exchange_rate price used to convert non-core asset to core asset
    * @param is_exchangeable \c true to allow implicit conversion of this asset to/from core asset
    * @param broadcast \c true to broadcast the transaction on the network
    * @return the signed transaction updating the user-issued asset
    * @ingroup WalletAPI_Asset
    */
   signed_transaction_info update_user_issued_asset(const std::string& symbol,
                                                    const std::string& new_issuer,
                                                    const std::string& description,
                                                    uint64_t max_supply,
                                                    chain::price core_exchange_rate,
                                                    bool is_exchangeable,
                                                    bool broadcast = false);

   /**
    * @brief Pay into the pools for the given asset. Allows anyone to deposit core/asset into pools.
    * @note User-issued assets can optionally have two asset pools.
    * This pools are used when conversion between assets is needed (paying fees, paying for a content in different asset ).
    * @note The wallet needs to be unlocked and a required key/s needs to be imported.
    * @param from the name or id of the account sending the core asset
    * @param uia_amount the amount of "this" asset to deposit
    * @param uia_symbol the name or id of the asset whose pool you wish to fund
    * @param dct_amount the amount of the core asset to deposit
    * @param dct_symbol the name or id of the DCT asset
    * @param broadcast \c true to broadcast the transaction on the network
    * @return the signed transaction funding the asset pools
    * @ingroup WalletAPI_Asset
    */
   signed_transaction_info fund_asset_pools(const std::string& from,
                                            const std::string& uia_amount,
                                            const std::string& uia_symbol,
                                            const std::string& dct_amount,
                                            const std::string& dct_symbol,
                                            bool broadcast = false);

   /**
    * @brief Burns the given user-issued asset.
    * This command burns the user-issued asset to reduce the amount in circulation.
    * @note you cannot burn monitored asset.
    * @note The wallet needs to be unlocked and a required key/s needs to be imported.
    * @param from the account containing the asset you wish to burn
    * @param amount the amount to burn, in nominal units
    * @param symbol the name or id of the asset to burn
    * @param broadcast \c true to broadcast the transaction on the network
    * @return the signed transaction burning the asset
    * @ingroup WalletAPI_Asset
    */
   signed_transaction_info reserve_asset(const std::string& from,
                                         const std::string& amount,
                                         const std::string& symbol,
                                         bool broadcast = false);

   /**
    * @brief Transfers accumulated assets from pools back to the issuer's balance.
    * @note You cannot claim assets from pools of monitored asset.
    * @note The wallet needs to be unlocked and a required key/s needs to be imported.
    * @param uia_amount the amount of "this" asset to claim, in nominal units
    * @param uia_symbol the name or id of the asset to claim
    * @param dct_amount the amount of DCT asset to claim, in nominal units
    * @param dct_symbol the name or id of the DCT asset to claim
    * @param broadcast \c true to broadcast the transaction on the network
    * @return the signed transaction claiming the fees
    * @ingroup WalletAPI_Asset
    */
   signed_transaction_info claim_fees(const std::string& uia_amount,
                                      const std::string& uia_symbol,
                                      const std::string& dct_amount,
                                      const std::string& dct_symbol,
                                      bool broadcast = false);

   /**
    * @brief Converts asset into DCT, using actual price feed.
    * @param amount the amount to convert in nominal units
    * @param asset_symbol_or_id the symbol or id of the asset to convert
    * @return price in DCT
    * @ingroup WalletAPI_Asset
    */
   std::string price_to_dct(const std::string& amount, const std::string& asset_symbol_or_id) const;

   /**
    * @brief Publishes a price feed for the named asset.
    * Price feed providers use this command to publish their price feeds for monitored assets. A price feed is
    * used to tune the market for a particular monitored asset. For each value in the feed, the median across all
    * miner feeds for that asset is calculated and the market for the asset is configured with the median of that
    * value.
    * @note The wallet needs to be unlocked and a required key/s needs to be imported.
    * @param publishing_account the account publishing the price feed
    * @param symbol the name or id of the asset whose feed we're publishing
    * @param feed the price feed object for particular monitored asset
    * @param broadcast \c true to broadcast the transaction on the network
    * @return the signed transaction updating the price feed for the given asset
    * @ingroup WalletAPI_Asset
    */
   signed_transaction_info publish_asset_feed(const std::string& publishing_account,
                                              const std::string& symbol,
                                              const chain::price_feed& feed,
                                              bool broadcast = false);

   /**
    * @brief Get a list of published price feeds by a miner.
    * @param account_name_or_id the name or id of the account
    * @param count maximum number of price feeds to fetch (must not exceed 100)
    * @return list of price feeds published by the miner
    * @ingroup WalletAPI_Asset
    */
   std::multimap<fc::time_point_sec, chain::price_feed> get_feeds_by_miner(const std::string& account_name_or_id, uint32_t count) const;

   /**
    * @brief Get current supply of the core asset
    * @return the number of shares currently in existence in account and vesting balances, escrows and pools
    * @ingroup WalletAPI_Asset
    */
   chain::real_supply get_real_supply() const;

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
   void add_operation_to_builder_transaction(transaction_handle_type transaction_handle, const chain::operation& op);

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
                                                 const chain::operation& new_op);

   /**
    * @brief Set fees on all operations in a transaction
    * @see \c begin_builder_transaction()
    * @param handle the number identifying transaction under contruction process
    * @param fee_asset the asset in which fees are calculated
    * @return total fee in specified asset
    * @ingroup WalletAPI_TransactionBuilder
    */
   chain::asset set_fees_on_builder_transaction(transaction_handle_type handle, const std::string& fee_asset);

   /**
    * @brief Previews a transaction from transaction builder.
    * @see \c begin_builder_transaction()
    * @param handle the number identifying transaction under contruction process
    * @return the transaction to preview
    * @ingroup WalletAPI_TransactionBuilder
    */
   chain::transaction preview_builder_transaction(transaction_handle_type handle);

   /**
    * @brief Signs a transaction from transaction builder
    * @note The wallet needs to be unlocked and a required key/s needs to be imported.
    * @see \c prewiev_builder_transaction()
    * @param transaction_handle the number identifying transaction under contruction process
    * @param broadcast \c true to broadcast the transaction on the network
    * @return the signed transaction
    * @ingroup WalletAPI_TransactionBuilder
    */
   signed_transaction_info sign_builder_transaction(transaction_handle_type transaction_handle, bool broadcast = true);

   /**
    * @brief Allows creation of a proposed transaction suitable for miner-account.
    * Proposed transaction requires approval of multiple accounts in order to execute.
    * @note The wallet needs to be unlocked and a required key/s needs to be imported.
    * @param handle the number identifying transaction under contruction process
    * @param expiration the expiration time of the transaction
    * @param review_period_seconds the time reserved for reviewing the proposal transaction.
    * It's not allowed to vote for the proposal when the transaction is under review
    * @param broadcast \c true to broadcast the transaction on the network
    * @return the signed transaction
    * @ingroup WalletAPI_TransactionBuilder
    */
   signed_transaction_info propose_builder_transaction(transaction_handle_type handle,
                                                       fc::time_point_sec expiration,
                                                       uint32_t review_period_seconds,
                                                       bool broadcast = true);

   /**
    * @brief Allows creation of a proposed transaction. Proposed transaction requires approval of multiple accounts
    * in order to execute.
    * @note The wallet needs to be unlocked and a required key/s needs to be imported.
    * @see \c propose_builder_transaction()
    * @param handle the number identifying transaction under contruction process
    * @param account_name_or_id the account which will pay the fee to propose the transaction
    * @param expiration the expiration time of the transaction
    * @param review_period_seconds the time reserved for reviewing the proposal transaction.
    * It's not allowed to vote for the proposal when the transaction is under review
    * @param broadcast \c true to broadcast the transaction on the network
    * @return the signed transaction
    * @ingroup WalletAPI_TransactionBuilder
    */
   signed_transaction_info propose_builder_transaction2(transaction_handle_type handle,
                                                        const std::string& account_name_or_id,
                                                        fc::time_point_sec expiration,
                                                        uint32_t review_period_seconds,
                                                        bool broadcast = true);

   /**
    * @brief Removes a transaction from transaction builder
    * @param handle the number identifying transaction under contruction process
    * @ingroup WalletAPI_TransactionBuilder
    */
   void remove_builder_transaction(transaction_handle_type handle);

   /**
    * @brief Converts a signed_transaction in JSON form to its binary representation.
    * @param tx the transaction to serialize
    * @return the binary form of the transaction.  It will not be hex encoded,
    *         this returns a raw string that may have null characters embedded in it
    * @ingroup WalletAPI_TransactionBuilder
    */
   std::string serialize_transaction(const chain::signed_transaction& tx) const;

   /**
    * @brief Signs a transaction.
    * Given a fully-formed transaction that is only lacking signatures, this signs
    * the transaction with the necessary keys and optionally broadcasts the transaction.
    * @note The wallet needs to be unlocked and a required key/s needs to be imported.
    * @param tx the unsigned transaction
    * @param broadcast \c true if you wish to broadcast the transaction
    * @return the signed version of the transaction
    * @ingroup WalletAPI_TransactionBuilder
    */
   signed_transaction_info sign_transaction(const chain::transaction& tx, bool broadcast = false);

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
   chain::operation get_prototype_operation(const std::string& operation_type) const;

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
   std::map<std::string, chain::miner_id_type> list_miners(const std::string& lowerbound, uint32_t limit) const;

   /**
    * @brief Returns information about the given miner.
    * @param owner_account the name or id of the miner account owner, or the id of the miner
    * @return the information about the miner stored in the block chain
    * @ingroup WalletAPI_Mining
    */
   chain::miner_object get_miner(const std::string& owner_account) const;

   /**
    * @brief Creates a miner object owned by the given account.
    * @note an account can have at most one miner object.
    * @note The wallet needs to be unlocked and a required key/s needs to be imported.
    * @param owner_account the name or id of the account which is creating the miner
    * @param url a URL to include in the miner record in the blockchain.  Clients may
    *            display this when showing a list of miners.  May be blank.
    * @param broadcast \c true to broadcast the transaction on the network
    * @return the signed transaction registering a miner
    * @ingroup WalletAPI_Mining
    */
   signed_transaction_info create_miner(const std::string& owner_account,
                                        const std::string& url,
                                        bool broadcast = false);

   /**
    * @brief Update a miner object owned by the given account.
    * @note The wallet needs to be unlocked and a required key/s needs to be imported.
    * @param miner_name The name of the miner's owner account. Also accepts the ID of the owner account or the ID of the miner.
    * @param url Same as for create_miner.  The empty string makes it remain the same.
    * @param block_signing_key the new block signing public key.  The empty string makes it remain the same
    * @param broadcast \c true if you wish to broadcast the transaction.
    * @ingroup WalletAPI_Mining
    */
   signed_transaction_info update_miner(const std::string& miner_name,
                                        const std::string& url,
                                        const std::string& block_signing_key,
                                        bool broadcast = false);

   /**
    * @brief Get information about a vesting balance object.
    * @param account_name an account name, account ID, or vesting balance object ID.
    * @ingroup WalletAPI_Mining
    */
   std::vector<vesting_balance_object_with_info> get_vesting_balances(const std::string& account_name) const;

   /**
    * @brief Withdraw a vesting balance.
    * @note The wallet needs to be unlocked and a required key/s needs to be imported.
    * @param miner_name the account name of the miner, also accepts account ID or vesting balance ID type.
    * @param amount the amount to withdraw.
    * @param asset_symbol the symbol of the asset to withdraw
    * @param broadcast \c true if you wish to broadcast the transaction
    * @ingroup WalletAPI_Mining
    */
   signed_transaction_info withdraw_vesting(const std::string& miner_name,
                                            const std::string& amount,
                                            const std::string& asset_symbol,
                                            bool broadcast = false);

   /**
    * @brief Vote for a given miner.
    * An account can publish a list of all miners they approve of. This
    * command allows you to add or remove miners from this list.
    * Each account's vote is weighted according to the number of shares of the
    * core asset owned by that account at the time the votes are tallied.
    * @note You cannot vote against a miner, you can only vote for the miner
    *       or not vote for the miner.
    * @note The wallet needs to be unlocked and a required key/s needs to be imported.
    * @see \c list_miners()
    * @param voting_account the name or id of the account who is voting with their shares
    * @param miner the name or id of the miner' owner account
    * @param approve \c true if you wish to vote in favor of that miner, \c false to
    *                remove your vote in favor of that miner
    * @param broadcast \c true if you wish to broadcast the transaction
    * @return the signed transaction changing your vote for the given miner
    * @ingroup WalletAPI_Mining
    */
   signed_transaction_info vote_for_miner(const std::string& voting_account,
                                          const std::string& miner,
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
    * @note The wallet needs to be unlocked and a required key/s needs to be imported.
    * @param account_to_modify the name or id of the account to update
    * @param voting_account the name or id of an account authorized to vote account_to_modify's shares,
    *                       or null to vote your own shares
    * @param broadcast \c true if you wish to broadcast the transaction
    * @return the signed transaction changing your vote proxy settings
    * @ingroup WalletAPI_Mining
    */
   signed_transaction_info set_voting_proxy(const std::string& account_to_modify,
                                            fc::optional<std::string> voting_account,
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
    * @note The wallet needs to be unlocked and a required key/s needs to be imported.
    * @param account_to_modify the name or id of the account to update
    * @param desired_number_of_miners
    * @param broadcast \c true if you wish to broadcast the transaction
    * @return the signed transaction changing your vote proxy settings
    * @ingroup WalletAPI_Mining
    */
   signed_transaction_info set_desired_miner_count(const std::string& account_to_modify,
                                                   uint16_t desired_number_of_miners,
                                                   bool broadcast = false);

   /**
    * @brief Get miner voting info list by account that match search term.
    * @param account_id account name or empty when search without account
    * @param term search term - miner name
    * @param only_my_votes when \c true it selects only votes given by account
    * @param order order field. Available options are 'name,link,votes'
    * @param id the id of the miner to start searching from, or empty when start from beginning
    * @param count maximum number of miners info to fetch (must not exceed 1000)
    * @return the list of miner voting info found
    * @ingroup WalletAPI_Mining
    */
   std::vector<app::miner_voting_info> search_miner_voting(const std::string& account_id,
                                                 const std::string& term,
                                                 bool only_my_votes,
                                                 const std::string& order,
                                                 const std::string& id,
                                                 uint32_t count ) const;

   /**
    * @brief Get a list of seeders by price, in increasing order.
    * @param count maximum number of seeders to retrieve
    * @return a list of seeders
    * @ingroup WalletAPI_Seeding
    */
   std::vector<chain::seeder_object> list_seeders_by_price(uint32_t count) const;

   /**
    * @brief Get a list of seeders ordered by total upload, in decreasing order.
    * @param count maximum number of seeders to retrieve
    * @return a list of seeders
    * @ingroup WalletAPI_Seeding
    */
   fc::optional<std::vector<chain::seeder_object>> list_seeders_by_upload(uint32_t count) const;

   /**
    * @brief Get a list of seeders by region code.
    * @param region_code region code of seeders to retrieve
    * @return a list of seeders
    * @ingroup WalletAPI_Seeding
    */
   std::vector<chain::seeder_object> list_seeders_by_region(const std::string& region_code) const;

   /**
    * @brief Get a list of seeders ordered by rating, in decreasing order.
    * @param count the maximum number of seeders to retrieve
    * @return a list of seeders
    * @ingroup WalletAPI_Seeding
    */
   std::vector<chain::seeder_object> list_seeders_by_rating(uint32_t count) const;

   /**
    * @brief Lists proposed transactions relevant to a user
    * @param account_or_id the name or id of the account
    * @return a list of proposed transactions
    * @ingroup WalletAPI_Proposals
    */
   std::vector<chain::proposal_object> get_proposed_transactions(const std::string& account_or_id) const;

   /**
    * @brief Encapsulates begin_builder_transaction(), add_operation_to_builder_transaction(),
    * propose_builder_transaction2(), set_fees_on_builder_transaction() functions for transfer operation.
    * @note The wallet needs to be unlocked and a required key/s needs to be imported.
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
   signed_transaction_info propose_transfer(const std::string& proposer,
                                            const std::string& from,
                                            const std::string& to,
                                            const std::string& amount,
                                            const std::string& asset_symbol,
                                            const std::string& memo,
                                            fc::time_point_sec expiration);

   /**
    * @brief Creates a transaction to propose a parameter change.
    * Multiple parameters can be specified if an atomic change is desired.
    * @note The wallet needs to be unlocked and a required key/s needs to be imported.
    * @param proposing_account the account paying the fee to propose the transaction
    * @param expiration_time timestamp specifying when the proposal will either take effect or expire
    * @param changed_values the values to change; all other chain parameters are filled in with default values
    * @param broadcast \c true if you wish to broadcast the transaction
    * @return the signed version of the transaction
    * @ingroup WalletAPI_Proposals
    */
   signed_transaction_info propose_parameter_change(const std::string& proposing_account,
                                                    fc::time_point_sec expiration_time,
                                                    const fc::variant_object& changed_values,
                                                    bool broadcast = false);

   /**
    * @brief Propose a fee change.
    * @note The wallet needs to be unlocked and a required key/s needs to be imported.
    * @param proposing_account the account paying the fee to propose the transaction
    * @param expiration_time timestamp specifying when the proposal will either take effect or expire
    * @param changed_values map of operation type to new fee.  Operations may be specified by name or ID
    *    The "scale" key changes the scale.  All other operations will maintain current values
    * @param broadcast \c true if you wish to broadcast the transaction
    * @return the signed version of the transaction
    * @ingroup WalletAPI_Proposals
    */
   signed_transaction_info propose_fee_change(const std::string& proposing_account,
                                              fc::time_point_sec expiration_time,
                                              const fc::variant_object& changed_values,
                                              bool broadcast = false);

   /**
    * @brief Approve or disapprove a proposal.
    * @note The wallet needs to be unlocked and a required key/s needs to be imported.
    * @param fee_paying_account the account paying the fee for the operation
    * @param proposal_id the proposal to modify
    * @param delta members contain approvals to create or remove.  In JSON you can leave empty members undefined
    * @param broadcast \c true if you wish to broadcast the transaction
    * @return the signed version of the transaction
    * @ingroup WalletAPI_Proposals
    */
   signed_transaction_info approve_proposal(const std::string& fee_paying_account,
                                            const std::string& proposal_id,
                                            const approval_delta& delta,
                                            bool broadcast = false);

   /**
    * @brief Submits or resubmits a content to the blockchain. In a case of resubmit, co-authors, price and synopsis fields
    * can be modified (if the content is uploaded to "CDN", you can also change the expiration time).
    * @see \c generate_encryption_key()
    * @see \c submit_content_async()
    * @note The wallet needs to be unlocked and a required key/s needs to be imported.
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
   signed_transaction_info submit_content(const std::string& author,
                                          const std::vector<std::pair<std::string, uint32_t>>& co_authors,
                                          const std::string& URI,
                                          const std::vector<regional_price_info>& price_amounts,
                                          uint64_t size,
                                          const fc::ripemd160& hash,
                                          const std::vector<chain::account_id_type>& seeders,
                                          uint32_t quorum,
                                          const fc::time_point_sec& expiration,
                                          const std::string& publishing_fee_asset,
                                          const std::string& publishing_fee_amount,
                                          const std::string& synopsis,
                                          const decent::encrypt::DInteger& secret,
                                          const decent::encrypt::CustodyData& cd,
                                          bool broadcast = false);

   /**
    * @brief This function is used to create and upload a package and submit content in one step.
    * @see create_package()
    * @see upload_package()
    * @see submit_content()
    * @note The wallet needs to be unlocked and a required key/s needs to be imported.
    * @param author the author of the content
    * @param co_authors the co-authors' account name or ID mapped to corresponding payment split based on basis points. The maximum number of co-authors is 10
    * @param content_dir path to the directory containing all content that should be packed
    * @param samples_dir path to the directory containing samples of content
    * @param protocol protocol for uploading package( ipfs )
    * @param price_amounts the prices of the content per regions
    * @param seeders list of the seeders, which will publish the content
    * @param expiration the expiration time of the content. The content is available to buy till it's expiration time
    * @param synopsis the description of the content
    * @return generated key, key parts and quorum
    * @ingroup WalletAPI_Content
    */
   app::content_keys submit_content_async(const std::string& author,
                                          const std::vector<std::pair<std::string, uint32_t>>& co_authors,
                                          const std::string& content_dir,
                                          const std::string& samples_dir,
                                          const std::string& protocol,
                                          const std::vector<regional_price_info>& price_amounts,
                                          const std::vector<chain::account_id_type>& seeders,
                                          const fc::time_point_sec& expiration,
                                          const std::string& synopsis);

   /**
    * @brief This function can be used to cancel submitted content. This content is immediately not available to purchase.
    * Seeders keep seeding this content up to next 24 hours.
    * @note The wallet needs to be unlocked and a required key/s needs to be imported.
    * @param author the author of the content
    * @param URI the URI of the content
    * @param broadcast \c true to broadcast the transaction on the network
    * @return the signed transaction cancelling the content
    * @ingroup WalletAPI_Content
    */
   signed_transaction_info content_cancellation(const std::string& author,
                                                const std::string& URI,
                                                bool broadcast = false);

   /**
    * @brief Downloads encrypted content specified by provided URI.
    * @note The wallet needs to be unlocked and a required key/s needs to be imported.
    * @param consumer consumer of the content
    * @param URI the URI of the content
    * @param region_code_from two letter region code
    * @param broadcast \c true to broadcast the transaction on the network
    * @ingroup WalletAPI_Content
    */
   void download_content(const std::string& consumer, const std::string& URI, const std::string& region_code_from, bool broadcast = false);

   /**
    * @brief Get status about particular download process specified by provided URI.
    * @param consumer consumer of the content
    * @param URI the URI of the content
    * @return download status, or \c null if no matching download process was found
    * @ingroup WalletAPI_Content
    */
   content_download_status get_download_status(const std::string& consumer, const std::string& URI) const;

   /**
    * @brief This function is used to send a request to buy a content. This request is caught by seeders.
    * @note The wallet needs to be unlocked and a required key/s needs to be imported.
    * @param consumer consumer of the content
    * @param URI the URI of the content
    * @param price_asset_name ticker symbol of the asset which will be used to buy content
    * @param price_amount the price of the content
    * @param str_region_code_from two letter region code
    * @param broadcast \c true to broadcast the transaction on the network
    * @return the signed transaction requesting buying of the content
    * @ingroup WalletAPI_Content
    */
   signed_transaction_info request_to_buy(const std::string& consumer,
                                          const std::string& URI,
                                          const std::string& price_asset_name,
                                          const std::string& price_amount,
                                          const std::string& str_region_code_from,
                                          bool broadcast = false);

   /**
    * @brief Rates and comments a content.
    * @note The wallet needs to be unlocked and a required key/s needs to be imported.
    * @param consumer consumer giving the rating
    * @param URI the URI of the content
    * @param rating the rating. The available options are 1-5
    * @param comment the maximum length of a comment is 100 characters
    * @param broadcast \c true to broadcast the transaction on the network
    * @ingroup WalletAPI_Content
    */
   signed_transaction_info leave_rating_and_comment(const std::string& consumer,
                                                    const std::string& URI,
                                                    uint64_t rating,
                                                    const std::string& comment,
                                                    bool broadcast = false);

   /**
    * @brief Get a list of open buyings.
    * @return a list of open buying objects
    * @ingroup WalletAPI_Content
    */
   std::vector<chain::buying_object> get_open_buyings() const;

   /**
    * @brief Get a list of open buyings by URI.
    * @param URI URI of the buyings to retrieve
    * @return a list of open buying objects corresponding to the provided URI
    * @ingroup WalletAPI_Content
    */
   std::vector<chain::buying_object> get_open_buyings_by_URI(const std::string& URI) const;

   /**
    * @brief Get a list of open buyings by consumer.
    * @param account_id_or_name consumer of the buyings to retrieve
    * @return a list of open buying objects corresponding to the provided consumer
    * @ingroup WalletAPI_Content
    */
   std::vector<chain::buying_object> get_open_buyings_by_consumer(const std::string& account_id_or_name) const;

   /**
    * @brief Get history buyings by consumer.
    * @param account_id_or_name consumer of the buyings to retrieve
    * @return a list of history buying objects corresponding to the provided consumer
    * @ingroup WalletAPI_Content
    */
   std::vector<chain::buying_object> get_buying_history_objects_by_consumer(const std::string& account_id_or_name) const;

   /**
    * @brief Get history buying objects by consumer that match search term.
    * @param account_id_or_name consumer of the buyings to retrieve
    * @param term search term to look up in \c title and \c description
    * @param order sort data by field. Available options are defined in 'database_api.cpp'
    * @param id the id of buying object to start searching from
    * @param count maximum number of contents to fetch (must not exceed 100)
    * @return a list of history buying objects corresponding to the provided consumer and matching search term
    * @ingroup WalletAPI_Content
    */
   std::vector<buying_object_ex> search_my_purchases(const std::string& account_id_or_name,
                                                const std::string& term,
                                                const std::string& order,
                                                const std::string& id,
                                                uint32_t count) const;

   /**
    * @brief Get buying object (open or history) by consumer and URI.
    * @param account_id_or_name consumer of the buying to retrieve
    * @param URI the URI of the buying to retrieve
    * @return buying objects corresponding to the provided consumer, or null if no matching buying was found
    * @ingroup WalletAPI_Content
    */
   fc::optional<chain::buying_object> get_buying_by_consumer_URI(const std::string& account_id_or_name, const std::string& URI) const;

   /**
    * @brief Search for term in users' feedbacks.
    * @param user the author of the feedback
    * @param URI the content object URI
    * @param id the id of feedback object to start searching from
    * @param count maximum number of feedbacks to fetch
    * @return the feedback found
    * @ingroup WalletAPI_Content
    */
   std::vector<rating_object_ex> search_feedback(const std::string& user,
                                            const std::string& URI,
                                            const std::string& id,
                                            uint32_t count) const;

   /**
    * @brief Get a content by URI.
    * @param URI the URI of the content to retrieve
    * @return the content corresponding to the provided URI, or \c null if no matching content was found
    * @ingroup WalletAPI_Content
    */
   fc::optional<chain::content_object> get_content(const std::string& URI) const;

   /**
    * @brief Get a list of contents ordered alphabetically by search term.
    * @param term search term
    * @param order order field. Available options are defined in 'database_api.cpp'
    * @param user content owner
    * @param region_code two letter region code
    * @param id the id of content object to start searching from
    * @param type the application and content type to be filtered, separated by comma.
    * Available options are defined in 'content_object.hpp'
    * @param count maximum number of contents to fetch (must not exceed 100)
    * @return the contents found
    * @ingroup WalletAPI_Content
    */
   std::vector<app::content_summary> search_content(const std::string& term,
                                          const std::string& order,
                                          const std::string& user,
                                          const std::string& region_code,
                                          const std::string& id,
                                          const std::string& type,
                                          uint32_t count) const;

   /**
    * @brief Get a list of contents ordered alphabetically by search term.
    * @param user content owner
    * @param term search term. Available options are defined in 'database_api.cpp'
    * @param order order field
    * @param region_code two letter region code
    * @param id the id of content object to start searching from
    * @param type the application and content type to be filtered, separated by comma.
    * Available options are defined in 'content_object.hpp'
    * @param count maximum number of contents to fetch (must not exceed 100)
    * @return the contents found
    * @ingroup WalletAPI_Content
    */
   std::vector<app::content_summary> search_user_content(const std::string& user,
                                               const std::string& term,
                                               const std::string& order,
                                               const std::string& region_code,
                                               const std::string& id,
                                               const std::string& type,
                                               uint32_t count) const;

   /**
    * @brief Get author and list of co-authors of a content corresponding to the provided URI.
    * @param URI the URI of the content
    * @return the autor of the content and the list of co-authors, if provided
    * @ingroup WalletAPI_Content
    */
   std::pair<chain::account_id_type, std::vector<chain::account_id_type>> get_author_and_co_authors_by_URI(const std::string& URI) const;

   /**
    * @brief Creates a package from selected files.
    * @see \c upload_package()
    * @note The wallet needs to be unlocked and a required key/s needs to be imported.
    * @param content_dir the directory containing all content that should be packed
    * @param samples_dir the directory containing samples of the content
    * @param aes_key the AES key for encryption
    * @return the package hash and content custody data
    * @ingroup WalletAPI_Content
    */
   std::pair<std::string, decent::encrypt::CustodyData> create_package(const std::string& content_dir,
                                                             const std::string& samples_dir,
                                                             const decent::encrypt::DInteger& aes_key) const;

   /**
    * @brief Extracts selected package.
    * @see \c download_package()
    * @note The wallet needs to be unlocked and a required key/s needs to be imported.
    * @param package_hash hash of the package that needs to be extracted
    * @param output_dir directory where extracted files will be created
    * @param aes_key the AES key for decryption
    * @ingroup WalletAPI_Content
    */
   void extract_package(const std::string& package_hash, const std::string& output_dir, const decent::encrypt::DInteger& aes_key) const;

   /**
    * @brief Downloads the package.
    * @note The wallet needs to be unlocked and a required key/s needs to be imported.
    * @param url the URL of the package
    * @ingroup WalletAPI_Content
    */
   void download_package(const std::string& url) const;

   /**
    * @brief Starts uploading of the package.
    * @see \c create_package()
    * @note The wallet needs to be unlocked and a required key/s needs to be imported.
    * @param package_hash hash of the package that needs to be extracted
    * @param protocol protocol for uploading package ( ipfs )
    * @return URL of package
    * @ingroup WalletAPI_Content
    */
   std::string upload_package(const std::string& package_hash, const std::string& protocol) const;

   /**
    * @brief Removes the package.
    * @note The wallet needs to be unlocked and a required key/s needs to be imported.
    * @param package_hash hash of the package that needs to be removed
    * @ingroup WalletAPI_Content
    */
   void remove_package(const std::string& package_hash) const;

   /**
    * @brief Restores AES key( used to encrypt and decrypt a content) from key particles stored in a buying object.
    * @note The wallet needs to be unlocked and a required key/s needs to be imported.
    * @param account consumers account id or name
    * @param buying the buying object containing key particles
    * @return restored AES key from key particles
    * @ingroup WalletAPI_Content
    */
   decent::encrypt::DInteger restore_encryption_key(const std::string& account, chain::buying_id_type buying);

   /**
    * @brief Generates AES encryption key.
    * @return random encryption key
    * @ingroup WalletAPI_Content
    */
   decent::encrypt::DInteger generate_encryption_key() const;

   /**
    * @brief Generate keys for new content submission
    * @param seeders list of seeder account IDs
    * @return generated key and key parts
    * @ingroup WalletCLI
    */
   app::content_keys generate_content_keys(const std::vector<chain::account_id_type>& seeders) const;

   /**
    * @brief Returns true if any package manager task is waiting (e.g. if content submission is still being processed in the background).
    * @return true if any package manager task is waiting, otherwise false
    * @ingroup WalletCLI
    */
   bool is_package_manager_task_waiting() const;

   /**
    * @brief Creates a subscription to author. This function is used by consumers.
    * @note The wallet needs to be unlocked and a required key/s needs to be imported.
    * @param from account who wants subscription to author
    * @param to the author you wish to subscribe to
    * @param price_amount price for the subscription
    * @param price_asset_symbol ticker symbol of the asset which will be used to buy subscription
    * @param broadcast \c true if you wish to broadcast the transaction
    * @return the signed transaction subscribing the consumer to the author
    * @ingroup WalletAPI_Subscription
    */
   signed_transaction_info subscribe_to_author(const std::string& from,
                                               const std::string& to,
                                               const std::string& price_amount,
                                               const std::string& price_asset_symbol,
                                               bool broadcast = false);

   /**
    * @brief Creates a subscription to author. This function is used by author.
    * @note The wallet needs to be unlocked and a required key/s needs to be imported.
    * @param from the account obtaining subscription from the author
    * @param to the name or id of the author
    * @param broadcast \c true if you wish to broadcast the transaction
    * @return the signed transaction subscribing the consumer to the author
    * @ingroup WalletAPI_Subscription
    */
   signed_transaction_info subscribe_by_author(const std::string& from,
                                               const std::string& to,
                                               bool broadcast = false);

   /**
    * @brief This function can be used to allow/disallow subscription.
    * @note The wallet needs to be unlocked and a required key/s needs to be imported.
    * @param account the name or id of the account to update
    * @param allow_subscription \c true if account (author) wants to allow subscription, \c false otherwise
    * @param subscription_period duration of subscription in days
    * @param price_amount price for subscription per one subscription period
    * @param price_asset_symbol ticker symbol of the asset which will be used to buy subscription
    * @param broadcast \c true if you wish to broadcast the transaction
    * @return the signed transaction updating the account
    * @ingroup WalletAPI_Subscription
    */
   signed_transaction_info set_subscription(const std::string& account,
                                            bool allow_subscription,
                                            uint32_t subscription_period,
                                            const std::string& price_amount,
                                            const std::string& price_asset_symbol,
                                            bool broadcast = false);

   /**
    * @brief This function can be used to allow/disallow automatic renewal of expired subscription.
    * @note The wallet needs to be unlocked and a required key/s needs to be imported.
    * @param account_id_or_name the name or id of the account to update
    * @param subscription_id the ID of the subscription.
    * @param automatic_renewal \c true if account (consumer) wants to allow automatic renewal of subscription, \c false otherwise
    * @param broadcast \c true if you wish to broadcast the transaction
    * @return the signed transaction allowing/disallowing renewal of the subscription
    * @ingroup WalletAPI_Subscription
    */
   signed_transaction_info set_automatic_renewal_of_subscription(const std::string& account_id_or_name,
                                                                 chain::subscription_id_type subscription_id,
                                                                 bool automatic_renewal,
                                                                 bool broadcast = false);

   /**
    * @brief Get a list of consumer's active (not expired) subscriptions.
    * @param account_id_or_name the name or id of the consumer
    * @param count maximum number of subscriptions to fetch (must not exceed 100)
    * @return list of active subscription objects corresponding to the provided consumer
    * @ingroup WalletAPI_Subscription
    */
   std::vector<chain::subscription_object> list_active_subscriptions_by_consumer(const std::string& account_id_or_name, uint32_t count) const;

   /**
    * @brief Get a list of consumer's subscriptions.
    * @param account_id_or_name the name or id of the consumer
    * @param count maximum number of subscriptions to fetch (must not exceed 100)
    * @return list of subscription objects corresponding to the provided consumer
    * @ingroup WalletAPI_Subscription
    */
   std::vector<chain::subscription_object> list_subscriptions_by_consumer(const std::string& account_id_or_name, uint32_t count) const;

   /**
    * @brief Get a list of active (not expired) subscriptions to author.
    * @param account_id_or_name the name or id of the author
    * @param count maximum number of subscriptions to fetch (must not exceed 100)
    * @return list of active subscription objects corresponding to the provided author
    * @ingroup WalletAPI_Subscription
    */
   std::vector<chain::subscription_object> list_active_subscriptions_by_author(const std::string& account_id_or_name, uint32_t count) const;

   /**
    * @brief Get a list of subscriptions to author.
    * @param account_id_or_name the name or id of the author
    * @param count maximum number of subscriptions to fetch (must not exceed 100)
    * @return list of subscription objects corresponding to the provided author
    * @ingroup WalletAPI_Subscription
    */
   std::vector<chain::subscription_object> list_subscriptions_by_author(const std::string& account_id_or_name, uint32_t count) const;

   /**
    * @brief Sends an encrypted text message to one or many users.
    * @note The wallet needs to be unlocked and a required key/s needs to be imported.
    * @param from account sending the message
    * @param to account or multiple accounts receiving the message
    * @param text the body of the message
    * @param broadcast \c true to broadcast the transaction on the network
    * @ingroup WalletAPI_Messaging
    */
   signed_transaction_info send_message(const std::string& from,
                                        const std::vector<std::string>& to,
                                        const std::string& text,
                                        bool broadcast = false);

   /**
    * @brief Sends an unencrypted text message to one or many users.
    * @note The wallet needs to be unlocked and a required key/s needs to be imported.
    * @param from account sending the message
    * @param to account or multiple accounts receiving the message
    * @param text the body of the message
    * @param broadcast \c true to broadcast the transaction on the network
    * @ingroup WalletAPI_Messaging
    */
   signed_transaction_info send_unencrypted_message(const std::string& from,
                                                    const std::vector<std::string>& to,
                                                    const std::string& text,
                                                    bool broadcast = false);

   /**
    * @brief Receives message objects by sender and/or receiver.
    * @note You need to specify at least one account.
    * @note The wallet needs to be unlocked and a required key/s needs to be imported.
    * @param sender name of message sender. If you dont want to filter by sender then let it empty
    * @param receiver name of message receiver. If you dont want to filter by receiver then let it empty
    * @param max_count maximal number of last messages to be displayed
    * @return a vector of message objects
    * @ingroup WalletAPI_Messaging
    */
   std::vector<message_data> get_message_objects(const std::string& sender, const std::string& receiver, uint32_t max_count) const;

   /**
    * @brief Receives messages by receiver.
    * @note The wallet needs to be unlocked and a required key/s needs to be imported.
    * @param receiver name of message receiver which must be imported to caller's wallet
    * @param max_count maximal number of last messages to be displayed
    * @return a vector of message objects
    * @ingroup WalletAPI_Messaging
    */
   std::vector<text_message> get_messages(const std::string& receiver, uint32_t max_count) const;

   /**
    * @brief Receives sent messages by sender.
    * @note The wallet needs to be unlocked and a required key/s needs to be imported.
    * @param sender name of message sender which must be imported to caller's wallet
    * @param max_count maximal number of last messages to be displayed
    * @return a vector of message objects
    * @ingroup WalletAPI_Messaging
    */
   std::vector<text_message> get_sent_messages(const std::string& sender, uint32_t max_count) const;

   /**
    * @brief Reset values of monitoring counters to zero.
    * @param names vector of names of counters to reset. Use empty vector to reset all counters
    * @ingroup WalletAPI_Monitoring
    */
   void reset_counters(const std::vector<std::string>& names) const;

   /**
   * @brief Retrieves monitoring counters.
   * @param names vector of names of counters to return. Use empty vector to return all counters
   * @return a vector of counter objects
   * @ingroup WalletAPI_Monitoring
   */
   std::vector<monitoring::counter_item_cli> get_counters(const std::vector<std::string>& names) const;

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
   std::vector<chain::non_fungible_token_object> list_non_fungible_tokens(const std::string& lowerbound, uint32_t limit) const;

   /**
    * @brief Returns information about the given non fungible token.
    * @param nft_symbol_or_id the name or id of the non fungible token symbol in question
    * @return the information about the non fungible token stored in the block chain
    * @ingroup WalletAPI_NonFungibleToken
    */
   chain::non_fungible_token_object get_non_fungible_token(const std::string& nft_symbol_or_id) const;

   /**
    * @brief Creates a new non fungible token definition.
    * @param issuer the name or id of the account who will pay the fee and become the issuer of the new non fungible token
    * @param symbol the ticker symbol of the new non fungible token
    * @param description non fungible token description (max: 100)
    * @param definitions non fungible token data definitions
    * @param max_supply the maximum supply of this non fungible token which may exist at any given time
    * @param fixed_max_supply true to deny future modifications of 'max_supply' otherwise false
    * @param transferable true to allow token transfer to other account otherwise false
    * @param broadcast \c true to broadcast the transaction on the network
    * @return the signed transaction creating the new non fungible token
    * @ingroup WalletAPI_NonFungibleToken
    */
   signed_transaction_info create_non_fungible_token(const std::string& issuer,
                                                     const std::string& symbol,
                                                     const std::string& description,
                                                     const chain::non_fungible_token_data_definitions& definitions,
                                                     uint32_t max_supply,
                                                     bool fixed_max_supply,
                                                     bool transferable,
                                                     bool broadcast = false);

   /**
    * @brief Updates the non fungible token definition.
    * @note Maximum supply will be changed only if fixed_max_supply is not set.
    * @param issuer the name or id of the account who will become the new issuer (or pass empty string)
    * @param symbol the ticker symbol of the non fungible token to update
    * @param description non fungible token description (max: 100)
    * @param max_supply the maximum supply of this non fungible token which may exist at any given time
    * @param fixed_max_supply true to deny future modifications of 'max_supply'
    * @param broadcast \c true to broadcast the transaction on the network
    * @return the signed transaction creating the new non fungible token
    * @ingroup WalletAPI_NonFungibleToken
    */
   signed_transaction_info update_non_fungible_token(const std::string& issuer,
                                                     const std::string& symbol,
                                                     const std::string& description,
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
   signed_transaction_info issue_non_fungible_token(const std::string& to_account,
                                                    const std::string& symbol,
                                                    const fc::variants& data,
                                                    const std::string& memo,
                                                    bool broadcast = false);

   /**
    * @brief Gets non fungible token instances by registered token symbol.
    * @param nft_symbol_or_id the name or id of the non fungible token symbol in question
    * @return the non fungible token data objects found
    * @ingroup WalletAPI_NonFungibleToken
    */
   std::vector<chain::non_fungible_token_data_object> list_non_fungible_token_data(const std::string& nft_symbol_or_id) const;

   /**
    * @brief Get account's summary of various non fungible tokens.
    * @param account the name or id of the account
    * @return a summary of non fungible token ids
    * @ingroup DatabaseAPI_Balance
    */
   std::map<chain::non_fungible_token_id_type,uint32_t> get_non_fungible_token_summary(const std::string& account) const;

   /**
    * @brief Gets account's balances in various non fungible tokens.
    * @param account the name or id of the account
    * @param symbols_or_ids set of symbol names or non fungible token ids to filter retrieved tokens (to disable filtering pass empty set)
    * @return the list of non fungible token data objects
    * @ingroup WalletAPI_NonFungibleToken
    */
   std::vector<chain::non_fungible_token_data_object> get_non_fungible_token_balances(const std::string& account,
                                                                          const std::set<std::string>& symbols_or_ids) const;

   /**
    * @brief Gets non fungible token data object transfer history.
    * @param nft_data_id the non fungible token data object id to search history for
    * @return a list of transaction detail objects
    * @ingroup WalletAPI_NonFungibleToken
    */
   std::vector<chain::transaction_detail_object> search_non_fungible_token_history(chain::non_fungible_token_data_id_type nft_data_id) const;

   /**
    * @brief Transfers ownership of token instance.
    * @param to_account the name or id of the account to receive the token instance
    * @param nft_data_id the token instance id to transfer
    * @param memo a memo to include in the transaction, readable by the recipient
    * @param broadcast \c true to broadcast the transaction on the network
    * @return the signed transaction transfering the token instance
    * @ingroup WalletAPI_NonFungibleToken
    */
   signed_transaction_info transfer_non_fungible_token_data(const std::string& to_account,
                                                            chain::non_fungible_token_data_id_type nft_data_id,
                                                            const std::string& memo,
                                                            bool broadcast = false);

   /**
    * @brief Burns (destroys) the token instance.
    * @param nft_data_id the token instance id to destroy
    * @param broadcast \c true to broadcast the transaction on the network
    * @ingroup WalletAPI_NonFungibleToken
    */
   signed_transaction_info burn_non_fungible_token_data(chain::non_fungible_token_data_id_type nft_data_id, bool broadcast = false);

   /**
    * @brief Updates data of token instance.
    * @param modifier the name or id of the modifier account
    * @param nft_data_id the token instance id to update
    * @param data name to value pairs to be updated
    * @param broadcast \c true to broadcast the transaction on the network
    * @return the signed transaction updating the token instance
    * @ingroup WalletAPI_NonFungibleToken
    */
   signed_transaction_info update_non_fungible_token_data(const std::string& modifier,
                                                          chain::non_fungible_token_data_id_type nft_data_id,
                                                          const std::vector<std::pair<std::string,fc::variant>>& data,
                                                          bool broadcast = false);

   std::map<std::string,std::function<std::string(fc::variant,const fc::variants&)>> get_result_formatters() const;

   boost::signals2::signal<void(bool)> lock_changed;
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

FC_REFLECT_DERIVED( graphene::wallet::extended_asset, (graphene::chain::asset),(pretty_amount))

FC_REFLECT_DERIVED( graphene::wallet::signed_transaction_info,(graphene::chain::signed_transaction),(transaction_id))

FC_REFLECT_DERIVED( graphene::wallet::message_data,(graphene::chain::message_object),(text))

FC_REFLECT( graphene::wallet::text_message, (created)(from)(to)(text) )

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
