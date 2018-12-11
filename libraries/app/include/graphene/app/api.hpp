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

#include <boost/container/flat_set.hpp>

#include <fc/crypto/elliptic.hpp>
#include <decent/monitoring/monitoring_fc.hpp>

#include <graphene/chain/protocol/asset.hpp>
#include <graphene/chain/message_object.hpp>
#include <graphene/chain/transaction_object.hpp>
#include <graphene/chain/operation_history_object.hpp>
#include <graphene/net/node.hpp>
#include <graphene/app/database_api.hpp>

/**
 * @defgroup HistoryAPI History API
 * @defgroup Network_broadcastAPI Network broadcastAPI
 * @defgroup Network_NodeAPI Network NodeAPI
 * @defgroup CryptoAPI Crypto API
 * @defgroup MessagingAPI Messaging API
 * @defgroup LoginAPI LoginAPI
 */
namespace graphene { namespace app {
   using namespace graphene::chain;
   using namespace fc::ecc;
   using namespace std;
   

   class application;

   struct asset_array
   {
      asset asset0;
      asset asset1;
   };

    struct balance_change_result
    {
       operation_history_object hist_object;
       asset_array balance;
       asset fee;
    };

   /**
    * @brief The history_api class implements the RPC API for account history
    *
    * This API contains methods to access account histories
    */
   class history_api
   {
      public:
         history_api(application& app):_app(app){}

         /**
          * @brief Get the name of the API.
          * @return the name of the API
          * @ingroup HistoryAPI
          */
         string info() { return "history_api";}

         /**
          * @brief Get operations relevant to the specificed account.
          * @param account the account whose history should be queried
          * @param stop ID of the earliest operation to retrieve
          * @param limit maximum number of operations to retrieve (must not exceed 100)
          * @param start ID of the most recent operation to retrieve
          * @return a list of operations performed by account, ordered from most recent to oldest
          * @ingroup HistoryAPI
          */
         vector<operation_history_object> get_account_history(account_id_type account,
                                                              operation_history_id_type stop = operation_history_id_type(),
                                                              unsigned limit = 100,
                                                              operation_history_id_type start = operation_history_id_type())const;
         /**
          * @brief Get operations relevant to the specified account referenced.
          * by an event numbering specific to the account. The current number of operations
          * for the account can be found in the account statistics (or use 0 for start).
          * @param account the account whose history should be queried
          * @param stop sequence number of earliest operation. 0 is default and will
          * query 'limit' number of operations
          * @param limit maximum number of operations to retrieve (must not exceed 100)
          * @param start sequence number of the most recent operation to retrieve.
          * 0 is default, which will start querying from the most recent operation
          * @return a list of operations performed by account, ordered from most recent to oldest
          * @ingroup HistoryAPI
          */
         vector<operation_history_object> get_relative_account_history( account_id_type account,
                                                                        uint32_t stop = 0,
                                                                        unsigned limit = 100,
                                                                        uint32_t start = 0) const;

         /**
          * @brief Returns the most recent balance operations on the named account.
          * This returns a list of operation history objects, which describe activity on the account.
          * @param account_id the account whose history should be queried
          * @param assets_list list of asset_ids to filter assets or empty for all assets
          * @param partner_account_id partner account_id to filter transfers to speccific account or empty
          * @param from_block filtering parameter, starting block number (can be determined by from time) or zero when not used
          * @param to_block filtering parameter, ending block number or zero when not used
          * @param order ordering parameter, not working yet
          * @param start_offset starting offset from zero
          * @param limit the number of entries to return (starting from the most recent)
          * @return a list of balance operation history objects
          * @ingroup HistoryAPI
          */
         vector<balance_change_result>  search_account_balance_history(account_id_type account_id,
                                                                       const flat_set<asset_id_type>& assets_list,
                                                                       fc::optional<account_id_type> partner_account_id,
                                                                       uint32_t from_block, uint32_t to_block,
                                                                       uint32_t start_offset,
                                                                       int limit) const;

         /**
          * @brief Returns balance operation on the named account and transaction_id.
          * @param account_id the account whose history should be queried
          * @param operation_history_id the operation_history_id whose history should be queried
          * @return balance operation history object or empty when not found
          * @ingroup HistoryAPI
          */
         fc::optional<balance_change_result> get_account_balance_for_transaction(account_id_type account_id,
                                                                                 operation_history_id_type operation_history_id);

      private:
           application& _app;
   };

   /**
    * @brief The network_broadcast_api class allows broadcasting of transactions.
    */
   class network_broadcast_api : public std::enable_shared_from_this<network_broadcast_api>
   {
      public:
         network_broadcast_api(application& a);

         struct transaction_confirmation
         {
            transaction_id_type   id;
            uint32_t              block_num;
            uint32_t              trx_num;
            processed_transaction trx;
         };

         typedef std::function<void(variant/*transaction_confirmation*/)> confirmation_callback;

         /**
          * @brief Get the name of the API.
          * @return the name of the API
          * @ingroup Network_broadcastAPI
          */
         string info() { return "network_broadcast_api";}

         /**
          * @brief Broadcast a transaction to the network.
          * @param trx the transaction to broadcast
          * @note the transaction will be checked for validity in the local database prior to broadcasting. If it fails to
          * apply locally, an error will be thrown and the transaction will not be broadcast
          * @ingroup Network_broadcastAPI
          */
         void broadcast_transaction(const signed_transaction& trx);

         /**
          *
          * @brief This call will not return until the transaction is included in a block.
          * @param trx the transaction to broadcast
          * @ingroup Network_broadcastAPI
          */
         fc::variant broadcast_transaction_synchronous( const signed_transaction& trx);

         /**
          * @brief This version of broadcast transaction registers a callback method that will be called when the transaction is
          * included into a block.  The callback method includes the transaction id, block number, and transaction number in the
          * block.
          * @param cb callback function
          * @param trx the transaction to broadcast
          * @ingroup Network_broadcastAPI
          */
         void broadcast_transaction_with_callback( confirmation_callback cb, const signed_transaction& trx);

         /**
          * @brief Broadcast a block to the network.
          * @param block the signed block to broadcast
          * @ingroup Network_broadcastAPI
          */
         void broadcast_block( const signed_block& block );

         /**
          * @brief Not reflected, thus not accessible to API clients.
          * This function is registered to receive the applied_block
          * signal from the chain database when a block is received.
          * It then dispatches callbacks to clients who have requested
          * to be notified when a particular txid is included in a block.
          * @param b the signed block
          * @ingroup Network_broadcastAPI
          */
         void on_applied_block( const signed_block& b );
      private:
         boost::signals2::scoped_connection             _applied_block_connection;
         map<transaction_id_type,confirmation_callback> _callbacks;
         application&                                   _app;
   };

   /**
    * @brief The network_node_api class allows maintenance of p2p connections.
    */
   class network_node_api
   {
      public:
         network_node_api(application& a);

         /**
          * @brief Get the name of the API.
          * @return the name of the API
          * @ingroup Network_NodeAPI
          */
         string info() { return "network_node_api";}

         /**
          * @brief Returns general network information, such as p2p port.
          * @return general network information
          * @ingroup Network_NodeAPI
          */
         fc::variant_object get_info() const;

         /**
          * @brief Connects to a new peer.
          * @param ep the IP/Port of the peer to connect to
          * @ingroup Network_NodeAPI
          */
         void add_node(const fc::ip::endpoint& ep);

         /**
          * @brief Get status of all current connections to peers.
          * @return status of all connected peers
          * @ingroup Network_NodeAPI
          */
         std::vector<net::peer_status> get_connected_peers() const;

         /**
          * @brief Get advanced node parameters, such as desired and max number of connections.
          * @return advanced node parameters
          * @ingroup Network_NodeAPI
          */
         fc::variant_object get_advanced_node_parameters() const;

         /**
          * @brief Set advanced node parameters, such as desired and max number of connections.
          * @param params a JSON object containing the name/value pairs for the parameters to set
          * @ingroup Network_NodeAPI
          */
         void set_advanced_node_parameters(const fc::variant_object& params);

         /**
          * @brief Get a list of potential peers we can connect to.
          * @return a list of potential peers
          * @ingroup Network_NodeAPI
          */
         std::vector<net::potential_peer_record> get_potential_peers() const;

        /**
         * @brief This method allows user to start seeding plugin from running application.
         * @param account_id ID of the account controlling this seeder
         * @param content_private_key El Gamal content private key
         * @param seeder_private_key private key of the account controlling this seeder
         * @param free_space allocated disk space, in MegaBytes
         * @param seeding_price price per MegaBytes
         * @param seeding_symbol seeding price asset, e.g. DCT
         * @param packages_path packages storage path
         * @param region_code optional ISO 3166-1 alpha-2 two-letter region code
         * @ingroup Network_NodeAPI
         */
         void seeding_startup(const account_id_type& account_id,
                              const DInteger& content_private_key,
                              const fc::ecc::private_key& seeder_private_key,
                              const uint64_t free_space,
                              const uint32_t seeding_price,
                              const string seeding_symbol,
                              const string packages_path,
                              const string region_code = "" );

      private:
         application& _app;
   };

   /**
    * @brief The crypto_api class implements cryptograhic operations
    */
   class crypto_api
   {
      public:
         crypto_api(application& a);

         /**
          * @brief Get the name of the API.
          * @return the name of the API
          * @ingroup CryptoAPI
          */
         string info() { return "crypto_api";}

         /**
          * @brief Get public key from private key.
          * @param wif_priv_key the wif private key
          * @return corresponding public key
          * @ingroup CryptoAPI
          */
         public_key_type get_public_key(const string& wif_priv_key );

         /**
          * @brief Convert wif key to private key.
          * @param wif the wif key to convert
          * @return private key
          * @ingroup CryptoAPI
          */
         private_key_type wif_to_private_key(const string &wif);

         /**
          * @brief Sign transaction with given private key.
          * @param trx the transaction to sign
          * @param key the private key to sign the given transaction
          * @return signed transaction
          * @ingroup CryptoAPI
          */
         signed_transaction sign_transaction(signed_transaction trx, const private_key_type &key);

         /**
          * @brief Encrypt message.
          * @param message the message to encrypt
          * @param key the private key of sender
          * @param pub the public key of receiver
          * @param nonce the salt number to use for message encryption (will be generated if zero)
          * @return encrypted memo data
          * @ingroup CryptoAPI
          */
         memo_data encrypt_message(const std::string &message,
                                   const private_key_type &key,
                                   const public_key_type &pub,
                                   uint64_t nonce = 0) const;

         /**
          * @brief Decrypt message.
          * @param message the message to decrypt
          * @param key the private key of sender/receiver
          * @param pub the public key of receiver/sender
          * @param nonce the salt number used for message encryption
          * @return decrypted message
          * @ingroup CryptoAPI
          */
         std::string decrypt_message(const memo_data::message_type &message,
                                     const private_key_type &key,
                                     const public_key_type &pub,
                                     uint64_t nonce) const;

      private:
         application& _app;
   };

   /**
   * @brief The messaging_api class implements instant messaging
   */
   class messaging_api
   {
   public:
      messaging_api(application& a);

      /**
       * @brief Get the name of the API.
       * @return the name of the API
       * @ingroup MessagingAPI
       */
      string info() { return "messaging_api";}

      /**
       * @brief Receives message objects by sender and/or receiver.
       * @param sender name of message sender. If you dont want to filter by sender then let it empty
       * @param receiver name of message receiver. If you dont want to filter by receiver then let it empty
       * @param max_count maximal number of last messages to be displayed
       * @return a vector of message objects
       * @ingroup MessagingAPI
       */
      vector<message_object> get_message_objects(optional<account_id_type> sender, optional<account_id_type> receiver, uint32_t max_count) const;
   private:
      application& _app;
   };

   class monitoring_api
   {
   public:
      monitoring_api();
      /**
      * @brieg Get the name of the API.
      * @return the name of the API
      * @ingroup MessagingAPI
      */
      std::string info() const;
      void reset_counters(const std::vector<std::string>& names);
      std::vector<monitoring::counter_item> get_counters(const std::vector<std::string>& names) const;
   };






   /**
    * @brief The login_api class implements the bottom layer of the RPC API
    *
    * All other APIs must be requested from this API.
    */
   class login_api
   {
      public:
         login_api(application& a);
         ~login_api();

         /**
          * @brief Get the name of the API.
          * @return the name of the API
          * @ingroup LoginAPI
          */
         string info() { return "login_api";}

         /**
          * @brief Authenticate to the RPC server.
          * @note This must be called prior to requesting other APIs. Other APIs may not be accessible until the client
          * has sucessfully authenticated.
          * @param user username to login with
          * @param password password to login with
          * @return \c true if logged in successfully, \c false otherwise
          * @ingroup LoginAPI
          */
         bool login(const string& user, const string& password);

         /**
          * @brief Retrieve the network broadcast API.
          * @ingroup LoginAPI
          */
         fc::api<network_broadcast_api> network_broadcast()const;
         /**
          * @brief Retrieve the database API.
          * @ingroup LoginAPI
          */
         fc::api<database_api> database()const;
         /**
          * @brief Retrieve the history API.
          * @ingroup LoginAPI
          */
         fc::api<history_api> history()const;
         /**
          * @brief Retrieve the network node API.
          * @ingroup LoginAPI
          */
         fc::api<network_node_api> network_node()const;
         /**
          * @brief Retrieve the cryptography API.
          * @ingroup LoginAPI
          */
         fc::api<crypto_api> crypto()const;
         /**
         * @brief Retrieve the messaging API.
         * @ingroup LoginAPI
         */
         fc::api<messaging_api> messaging()const;
         /**
         * @brief Retrieve the monitoring API.
         * @ingroup LoginAPI
         */
         fc::api<monitoring_api> monitoring()const;

      private:
         /**
          * @brief Called to enable an API, not reflected.
          * @param api_name name of the API we are trying to enable
          * @ingroup LoginAPI
          */
         void enable_api( const string& api_name );

         application& _app;
         optional< fc::api<database_api> > _database_api;
         optional< fc::api<network_broadcast_api> > _network_broadcast_api;
         optional< fc::api<network_node_api> > _network_node_api;
         optional< fc::api<history_api> >  _history_api;
         optional< fc::api<crypto_api> > _crypto_api;
         optional< fc::api<messaging_api> > _messaging_api;
         optional< fc::api<monitoring_api> > _monitoring_api;
   };

}}  // graphene::app

FC_REFLECT( graphene::app::network_broadcast_api::transaction_confirmation,
        (id)(block_num)(trx_num)(trx) )
//FC_REFLECT_TYPENAME( fc::ecc::compact_signature );
//FC_REFLECT_TYPENAME( fc::ecc::commitment_type );
FC_REFLECT( graphene::app::asset_array, (asset0)(asset1) )
FC_REFLECT( graphene::app::balance_change_result, (hist_object)(balance)(fee) )


FC_API(graphene::app::history_api,
       (info)
       (get_account_history)
       (get_relative_account_history)
       (search_account_balance_history)
       (get_account_balance_for_transaction)
     )
FC_API(graphene::app::network_broadcast_api,
       (info)
       (broadcast_transaction)
       (broadcast_transaction_with_callback)
       (broadcast_block)
     )
FC_API(graphene::app::network_node_api,
       (info)
       (get_info)
       (add_node)
       (get_connected_peers)
       (get_potential_peers)
       (get_advanced_node_parameters)
       (set_advanced_node_parameters)
       (seeding_startup)
     )
FC_API(graphene::app::crypto_api,
       (info)
       (get_public_key)
       (wif_to_private_key)
       (sign_transaction)
       (encrypt_message)
       (decrypt_message)
     )
FC_API(graphene::app::messaging_api,
       (info)
       (get_message_objects)
     )
FC_API(graphene::app::monitoring_api,
   (info)
      (reset_counters)
      (get_counters)
   )
FC_API(graphene::app::login_api,
       (info)
       (login)
       (network_broadcast)
       (database)
       (history)
       (network_node)
       (crypto)
       (messaging)
       (monitoring)
     )
