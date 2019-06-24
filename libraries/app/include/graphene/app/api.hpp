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

#include <fc/monitoring.hpp>

#include <graphene/chain/protocol/asset.hpp>
#include <graphene/chain/message_object.hpp>
#include <graphene/chain/transaction_object.hpp>
#include <graphene/chain/operation_history_object.hpp>
#include <graphene/net/node.hpp>
#include <graphene/net/core_messages.hpp>
#include <graphene/app/database_api.hpp>

/**
 * @defgroup HistoryAPI History API
 * @defgroup Network_broadcastAPI Network broadcast API
 * @defgroup Network_NodeAPI Network Node API
 * @defgroup CryptoAPI Crypto API
 * @defgroup MessagingAPI Messaging API
 * @defgroup MonitoringAPI Monitoring API
 * @defgroup LoginAPI Login API
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
       fc::time_point_sec timestamp;
       transaction_id_type transaction_id;
    };

    struct network_node_info
    {
       fc::ip::endpoint listening_on;
       graphene::net::node_id_t node_public_key;
       graphene::net::node_id_t node_id;
       graphene::net::firewalled_state firewalled;
       uint32_t connection_count;
    };

    struct advanced_node_parameters
    {
       uint32_t peer_connection_retry_timeout;
       uint32_t desired_number_of_connections;
       uint32_t maximum_number_of_connections;
       unsigned maximum_number_of_blocks_to_handle_at_one_time;
       unsigned maximum_number_of_sync_blocks_to_prefetch;
       unsigned maximum_blocks_per_peer_during_syncing;
    };

   /**
    * @brief The history_api class implements the RPC API for account history
    *
    * This API contains methods to access account histories
    */
   class history_api : public fc::api_base<history_api>
   {
      public:
         history_api(application& app):_app(app){}

         /**
          * @brief Get the name of the API.
          * @return the name of the API
          * @ingroup HistoryAPI
          */
         string info() { return get_api_name();}

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
          * @brief Get operations relevant to the specified account referenced
          * by an event numbering specific to the account. The current number of operations
          * for the account can be found in the account statistics (or use 0 for start).
          * @note The sequence number of the oldest operation is 1 and the operations are in increasing order, 
          * from the oldest operation to the most recent.
          * @param account The account whose history should be queried
          * @param stop Sequence number of earliest operation. 0 is default and will
          * query 'limit' number of operations
          * @param limit Maximum number of operations to retrieve (must not exceed 100)
          * @param start Sequence number of the most recent operation to retrieve
          * 0 is default, which will start querying from the most recent operation
          * @return A list of operations performed by account, ordered from most recent to oldest
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
                                                                       unsigned limit) const;

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
   class network_broadcast_api : public std::enable_shared_from_this<network_broadcast_api>, public fc::api_base<network_broadcast_api>
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
         string info() { return get_api_name();}

         /**
          * @brief Broadcast a transaction to the network.
          * @param trx the transaction to broadcast
          * @note the transaction will be checked for validity in the local database prior to broadcasting. If it fails to
          * apply locally, an error will be thrown and the transaction will not be broadcast
          * @ingroup Network_broadcastAPI
          */
         void broadcast_transaction(const signed_transaction& trx);

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

      private:
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

         boost::signals2::scoped_connection             _applied_block_connection;
         map<transaction_id_type,confirmation_callback> _callbacks;
         application&                                   _app;
   };

   /**
    * @brief The network_node_api class allows maintenance of p2p connections.
    */
   class network_node_api : public fc::api_base<network_node_api>
   {
      public:
         network_node_api(application& a);

         /**
          * @brief Get the name of the API.
          * @return the name of the API
          * @ingroup Network_NodeAPI
          */
         string info() { return get_api_name();}

         /**
          * @brief Returns general network information, such as p2p port.
          * @return general network information
          * @ingroup Network_NodeAPI
          */
         network_node_info get_info() const;

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
         advanced_node_parameters get_advanced_node_parameters() const;

         /**
          * @brief Set advanced node parameters, such as desired and max number of connections.
          * @param params a JSON object containing the name/value pairs for the parameters to set
          * @ingroup Network_NodeAPI
          */
         void set_advanced_node_parameters(const advanced_node_parameters& params);

         /**
          * @brief Get a list of potential peers we can connect to.
          * @return a list of potential peers
          * @ingroup Network_NodeAPI
          */
         std::vector<net::potential_peer_record> get_potential_peers() const;

      private:
         application& _app;
   };

   /**
    * @brief The crypto_api class implements cryptograhic operations
    */
   class crypto_api : public fc::api_base<crypto_api>
   {
      public:
         crypto_api(application& a);

         /**
          * @brief Get the name of the API.
          * @return the name of the API
          * @ingroup CryptoAPI
          */
         string info() { return get_api_name();}

         /**
          * @brief Get public key from private key.
          * @deprecated use wif_to_public_key instead
          * @param wif_priv_key the wif private key
          * @return corresponding public key
          * @ingroup CryptoAPI
          */
         public_key_type get_public_key(const string& wif_priv_key );

         /**
          * @brief Convert wif key to public key.
          * @param wif the wif key to convert
          * @return corresponding public key
          * @ingroup CryptoAPI
          */
         public_key_type wif_to_public_key(const string &wif);

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
   class messaging_api : public fc::api_base<messaging_api>
   {
   public:
      messaging_api(application& a);

      /**
       * @brief Get the name of the API.
       * @return the name of the API
       * @ingroup MessagingAPI
       */
      string info() { return get_api_name();}

      /**
       * @brief Receives message objects by sender and/or receiver.
       * @note You need to specify at least one account.
       * @param sender message sender account - pass null if you dont want to filter by sender
       * @param receiver message receiver account - pass null if you dont want to filter by receiver
       * @param max_count maximal number of messages to be returned
       * @return a vector of message objects
       * @ingroup MessagingAPI
       */
      vector<message_object> get_message_objects(optional<account_id_type> sender, optional<account_id_type> receiver, uint32_t max_count) const;
   private:
      application& _app;
   };

   /**
   * @brief The monitoring_api class provides access to monitoring counters
   */
   class monitoring_api : public fc::api_base<monitoring_api>
   {
   public:
      monitoring_api();
      /**
      * @brief Get the name of the API.
      * @return the name of the API
      * @ingroup MonitoringAPI
      */
      std::string info() const;
      /**
      * @brief Reset persistent monitoring counters by names. It has not impact on non-persistent counters.
      * @param names Counter names. Pass empty vector to reset all counters.
      * @ingroup MonitoringAPI
      */
      void reset_counters(const std::vector<std::string>& names);
      /**
      * @brief Retrieves monitoring counters by names.
      * @param names Counter names. Pass epmty vector to retrieve all counters.
      * @return Vector of monitoring counters. Persistent counters which was not reset yet or non-persisten counters shows datetime of reset equal to begin of epoch. 
      * @ingroup MonitoringAPI
      */
      std::vector<monitoring::counter_item> get_counters(const std::vector<std::string>& names) const;
   };

   /**
    * @brief The login_api class implements the bottom layer of the RPC API
    *
    * All other APIs must be requested from this API.
    */
   class login_api : public fc::api_base<login_api>
   {
      public:
         login_api(application& a);
         ~login_api();

         /**
          * @brief Get the name of the API.
          * @return the name of the API
          * @ingroup LoginAPI
          */
         string info() { return get_api_name();}

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
FC_REFLECT( graphene::app::balance_change_result, (hist_object)(balance)(fee)(timestamp)(transaction_id) )
FC_REFLECT( graphene::app::network_node_info, (listening_on)(node_public_key)(node_id)(firewalled)(connection_count) )
FC_REFLECT( graphene::app::advanced_node_parameters, (peer_connection_retry_timeout)(desired_number_of_connections)(maximum_number_of_connections)(maximum_number_of_blocks_to_handle_at_one_time)(maximum_number_of_sync_blocks_to_prefetch)(maximum_blocks_per_peer_during_syncing) )


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
     )
FC_API(graphene::app::crypto_api,
       (info)
       (get_public_key)
       (wif_to_public_key)
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
