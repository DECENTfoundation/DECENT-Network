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

#include <iostream>
#include <regex>
#include <boost/filesystem.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/range/algorithm/reverse.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <fc/io/fstream.hpp>
#include <fc/rpc/websocket_api.hpp>
#include <graphene/utilities/time.hpp>
#include <graphene/utilities/dirhelper.hpp>
#include <graphene/egenesis/egenesis.hpp>
#include <graphene/net/exceptions.hpp>
#include <graphene/app/exceptions.hpp>
#include <graphene/app/application.hpp>
#include <graphene/app/api_access.hpp>
#include <graphene/app/plugin.hpp>
#include <graphene/chain/hardfork.hpp>

namespace graphene { namespace app {
using net::item_hash_t;
using net::item_id;
using net::message;
using net::block_message;
using net::trx_message;

using chain::block_header;
using chain::signed_block_header;
using chain::signed_block;
using chain::block_id_type;

namespace bpo = boost::program_options;

namespace detail {

   chain::genesis_state_type create_example_genesis()
   {
      //TODO_DECENT - replace with super trooper private key
      //auto decent_key = fc::ecc::private_key::regenerate(fc::sha256::hash(string("decent")));
      //dlog("Allocating all stake to ${key}", ("key", utilities::key_to_wif(decent_key)));
      chain::public_key_type decent_pub_key (std::string("DCT82MTCQVa9TDFmz3ZwaLzsFAmCLoJzrtFugpF72vsbuE1CpCwKy"));

      chain::genesis_state_type initial_state;
      initial_state.initial_parameters.current_fees = chain::fee_schedule::get_non_virtual_default();//->set_all_fees(GRAPHENE_BLOCKCHAIN_PRECISION);
      initial_state.initial_active_miners = GRAPHENE_DEFAULT_MIN_MINER_COUNT;
      initial_state.initial_timestamp = fc::time_point_sec(fc::time_point::now().sec_since_epoch() /
            initial_state.initial_parameters.block_interval *
            initial_state.initial_parameters.block_interval);
      for( uint64_t i = 0; i < initial_state.initial_active_miners; ++i )
      {
         auto name = "init"+fc::to_string(i);
         initial_state.initial_accounts.emplace_back(name,
                                                     decent_pub_key,
                                                     decent_pub_key);
         initial_state.initial_miner_candidates.push_back({name, decent_pub_key});
      }

      initial_state.initial_accounts.emplace_back("decent", decent_pub_key);
      initial_state.initial_balances.push_back({"decent",
                                                GRAPHENE_SYMBOL,
                                                GRAPHENE_INITIAL_SHARE_SUPPLY});
      initial_state.initial_chain_id = fc::sha256::hash( "DECENT" );

      return initial_state;
   }
   MONITORING_COUNTERS_BEGIN(application_impl)
   MONITORING_DEFINE_COUNTER(blocks_unhandled)
   MONITORING_COUNTERS_DEPENDENCIES
   MONITORING_COUNTERS_END

   class application_impl : public net::node_delegate PUBLIC_DERIVATION_FROM_MONITORING_CLASS(application_impl)
   {
   public:
      bool _is_block_producer = false;
      bool _force_validate = false;
      uint64_t _processed_transactions = 0;

      void reset_p2p_node(const boost::filesystem::path& data_dir)
      { try {
         _p2p_network = std::make_shared<net::node>("Graphene Reference Implementation",
            _options->count("p2p-cert-authority-file") ? _options->at("p2p-cert-authority-file").as<std::string>() : std::string(),
            _options->count("p2p-cert-file") ? _options->at("p2p-cert-file").as<std::string>() : std::string(),
            _options->count("p2p-cert-key-file") ? _options->at("p2p-cert-key-file").as<std::string>() : std::string(),
            _options->count("p2p-cert-key-password") ? _options->at("p2p-cert-key-password").as<std::string>() : std::string());

         _p2p_network->load_configuration(data_dir / "p2p");
         _p2p_network->set_node_delegate(this, _chain_db->get_global_properties().parameters.maximum_block_size);

         std::vector<std::string> seeds;
         if( _options->count("seed-node") )
         {
             std::vector<std::string> seeds_list = _options->at("seed-node").as<std::vector<std::string>>();
             for(const std::string& seed_string : seeds_list) {
                 if (std::find(seeds.begin(), seeds.end(), seed_string) != seeds.end())
                     continue;

                 seeds.push_back(seed_string);
             }
         }
         else if( _chain_db->get_chain_id() == graphene::egenesis::get_egenesis_chain_id() )
         {
             seeds = {
               "mainnet-eu1.dcore.io:40000",
               "mainnet-cn1.dcore.io:40000",
               "mainnet-sg1.dcore.io:40000",
               "mainnet-us1.dcore.io:40000",
               "45.76.205.241:40000",              // liberosist (JP)
               "dctapi.spacemx.tech:40000",        // decentspace
               "seed.decent.dgazek.tk:40000"       // dgazek
            };
         }

         for( const std::string& seed : seeds ) {
            try {
               ilog("Adding seed node ${seed}", ("seed", seed));
               for (const fc::ip::endpoint& endpoint : fc::ip::endpoint::resolve_string(seed)) {
                  dlog("Resolved seed node endpoint ${endpoint}", ("endpoint", endpoint));
                  _p2p_network->add_node(endpoint);
                  _p2p_network->connect_to_endpoint(endpoint);
               }
            }
            catch(const fc::exception& e) {
               elog(e.to_detail_string());
            }
         }

         if( _options->count("p2p-endpoint") )
            _p2p_network->listen_on_endpoint(fc::ip::endpoint::resolve_string(_options->at("p2p-endpoint").as<std::string>()).back(), true);
         else
            _p2p_network->listen_on_port(0, false);
         _p2p_network->listen_to_p2p_network();
         ilog("Configured p2p node to listen on ${ip}", ("ip", _p2p_network->get_actual_listening_endpoint()));

         _p2p_network->connect_to_p2p_network();
         _p2p_network->sync_from(net::item_id(net::core_message_type_enum::block_message_type,
                                              _chain_db->head_block_id()),
                                 std::vector<uint32_t>());
      } FC_RETHROW() }

      void register_apis( std::shared_ptr<fc::rpc::websocket_api_connection>& wsc )
      {
         const auto& itr = _apiaccess.permission_map.find("*");
         if( itr != _apiaccess.permission_map.end() )
         {
            const api_access_info& apis = itr->second;
            for( const std::string& api_name : apis.allowed_apis )
            {
               if( api_name == graphene::app::database_api::get_api_name() )
               {
                  continue; // this API is already enabled, TODO
                  auto database_api = std::make_shared<graphene::app::database_api>( std::ref(*_self->chain_database() ) );
                  wsc->register_api(fc::api<graphene::app::database_api>(database_api));
               }
               else if( api_name == graphene::app::network_broadcast_api::get_api_name() )
               {
                  auto broadcast_api = std::make_shared<graphene::app::network_broadcast_api>( std::ref(*_self) );
                  wsc->register_api(fc::api<graphene::app::network_broadcast_api>(broadcast_api));
               }
               else if( api_name == graphene::app::history_api::get_api_name() )
               {
                  auto history_api = std::make_shared<graphene::app::history_api>(*_self);
                  wsc->register_api(fc::api<graphene::app::history_api>(history_api));
               }
               else if( api_name == graphene::app::network_node_api::get_api_name() )
               {
                  auto network_node_api = std::make_shared<graphene::app::network_node_api>( std::ref(*_self) );
                  wsc->register_api(fc::api<graphene::app::network_node_api>(network_node_api));
               }
               else if( api_name == graphene::app::crypto_api::get_api_name() )
               {
                  auto crypto_api = std::make_shared<graphene::app::crypto_api>( std::ref(*_self) );
                  wsc->register_api(fc::api<graphene::app::crypto_api>(crypto_api));
               }
               else if( api_name == graphene::app::messaging_api::get_api_name() )
               {
                  auto messaging_api = std::make_shared<graphene::app::messaging_api>( std::ref(*_self) );
                  wsc->register_api(fc::api<graphene::app::messaging_api>(messaging_api));
               }
               else if( api_name == graphene::app::monitoring_api::get_api_name() ) {
                  auto monitoring_api = std::make_shared<graphene::app::monitoring_api>();
                  wsc->register_api(fc::api<graphene::app::monitoring_api>(monitoring_api));
               }
            }
         }
      }

      void reset_websocket_server()
      { try {
         if( !_options->count("rpc-endpoint") )
            return;

         bool enable_deflate_compression = _options->count("enable-permessage-deflate") != 0;

         _websocket_server = std::make_shared<fc::http::websocket_server>(enable_deflate_compression);

         if (_options->count("server-allowed-domains") != 0) {
            _websocket_server->add_headers("Access-Control-Allow-Origin", _options->at("server-allowed-domains").as<std::string>() );
         }

         _websocket_server->on_connection([&]( const fc::http::websocket_connection_ptr& c, bool& is_tls){
            is_tls = false;
            auto wsc = std::make_shared<fc::rpc::websocket_api_connection>(*c);

            auto db_api = std::make_shared<graphene::app::database_api>( std::ref(*_self->chain_database()) );
            auto login = std::make_shared<graphene::app::login_api>( std::ref(*_self) );
            wsc->register_api(fc::api<graphene::app::database_api>(db_api));
            wsc->register_api(fc::api<graphene::app::login_api>(login));
            register_apis( wsc );
            c->set_session_data( wsc );
         });
         ilog("Configured websocket rpc to listen on ${ip}", ("ip",_options->at("rpc-endpoint").as<std::string>()));
         _websocket_server->listen( fc::ip::endpoint::resolve_string(_options->at("rpc-endpoint").as<std::string>()).back() );
         _websocket_server->start_accept();
      } FC_RETHROW() }


      void reset_websocket_tls_server()
      { try {
         if( !_options->count("rpc-tls-endpoint") )
            return;
         if( !_options->count("server-cert-file") || !_options->count("server-cert-key-file") )
         {
            wlog( "Please specify a server-cert-file or server-cert-key-file to use rpc-tls-endpoint" );
            return;
         }

         std::string password = _options->count("server-cert-password") ? _options->at("server-cert-password").as<std::string>() : std::string();
         std::string cert_chain_file = _options->count("server-cert-chain-file") ? _options->at("server-cert-chain-file").as<std::string>() : std::string();
         bool enable_deflate_compression = _options->count("enable-permessage-deflate") != 0;
         _websocket_tls_server = std::make_shared<fc::http::websocket_tls_server>( _options->at("server-cert-file").as<std::string>(),
                                                                                   _options->at("server-cert-key-file").as<std::string>(),
                                                                                   cert_chain_file,
                                                                                   password,
                                                                                   enable_deflate_compression );

         if (_options->count("server-allowed-domains") != 0) {
            _websocket_tls_server->add_headers("Access-Control-Allow-Origin", _options->at("server-allowed-domains").as<std::string>() );
         }

         _websocket_tls_server->on_connection([&]( const fc::http::websocket_connection_ptr& c, bool& is_tls){
            is_tls = true;
            auto wsc = std::make_shared<fc::rpc::websocket_api_connection>(*c);

            auto db_api = std::make_shared<graphene::app::database_api>( std::ref(*_self->chain_database()) );
            auto login = std::make_shared<graphene::app::login_api>( std::ref(*_self) );
            wsc->register_api(fc::api<graphene::app::database_api>(db_api));
            wsc->register_api(fc::api<graphene::app::login_api>(login));
            register_apis( wsc );
            c->set_session_data( wsc );
         });
         ilog("Configured websocket TLS rpc to listen on ${ip}", ("ip",_options->at("rpc-tls-endpoint").as<std::string>()));
         _websocket_tls_server->listen( fc::ip::endpoint::resolve_string(_options->at("rpc-tls-endpoint").as<std::string>()).back() );
         _websocket_tls_server->start_accept();

      } FC_RETHROW() }

      application_impl(application* self)
         : _self(self),
         _chain_db(std::make_shared<chain::database>(std::vector<uint8_t>{ chain::local_object_type_count, chain::protocol_object_type_count, chain::impl_object_type_count }))
      {
      }

      ~application_impl()
      {
      }

      void write_db_version()
      {
         std::ofstream db_version( (_data_dir / "db_version").generic_string().c_str(), std::ios::out | std::ios::binary | std::ios::trunc );
         std::string version_string = GRAPHENE_CURRENT_DB_VERSION;
         db_version.write( version_string.c_str(), version_string.size() );
         db_version.close();
      }

      void startup()
      { try {
         bool clean = !exists(_data_dir / "blockchain/dblock");

         create_directories(_data_dir / "blockchain/dblock");
         FC_ASSERT(!_db_lock, "Database is already opened");
         _db_lock.reset(new fc::simple_lock_file(_data_dir / "blockchain/dblock/decentd"));
         if( !_db_lock->try_lock() )
         {
            _db_lock.reset();
            FC_THROW_EXCEPTION(database_already_used_exception, "");
         }

         auto initial_state = [&] {
            ilog("Initializing database...");
            if( _options->count("genesis-json") )
            {
               std::string genesis_str;
               fc::read_file_contents( _options->at("genesis-json").as<boost::filesystem::path>(), genesis_str );
               chain::genesis_state_type genesis = fc::json::from_string( genesis_str ).as<chain::genesis_state_type>();
               if( _options->count("genesis-timestamp") )
               {
                  genesis.initial_timestamp = fc::time_point_sec(_options->at("genesis-timestamp").as<uint32_t>());
                  genesis.initial_timestamp -= genesis.initial_timestamp.sec_since_epoch() % genesis.initial_parameters.block_interval;
                  ilog("Effective genesis timestamp: ${t}", ("t", genesis.initial_timestamp.to_iso_string()));
               }
               if( _options->count("dbg-init-key") )
               {
                  std::string init_key = _options->at( "dbg-init-key" ).as<std::string>();
                  FC_ASSERT( genesis.initial_miner_candidates.size() >= genesis.initial_active_miners );
                  chain::public_key_type init_pubkey( init_key );
                  for( uint64_t i=0; i<genesis.initial_active_miners; i++ )
                     genesis.initial_miner_candidates[i].block_signing_key = init_pubkey;
                  std::cerr << "Set init miner key to " << init_key << "\n";
                  std::cerr << "WARNING:  GENESIS WAS MODIFIED, YOUR CHAIN ID MAY BE DIFFERENT\n";
                  genesis_str += "BOGUS";
                  genesis.initial_chain_id = fc::sha256::hash( genesis_str );
               }
               else
                  genesis.initial_chain_id = fc::sha256::hash( genesis_str );
               return genesis;
            }
            else
            {
               std::string egenesis_json;
               graphene::egenesis::compute_egenesis_json( egenesis_json );
               FC_ASSERT( egenesis_json != "" );
               FC_ASSERT( graphene::egenesis::get_egenesis_json_hash() == fc::sha256::hash( egenesis_json ) );
               auto genesis = fc::json::from_string( egenesis_json ).as<chain::genesis_state_type>();
               genesis.initial_chain_id = fc::sha256::hash( egenesis_json );

               return genesis;
            }
         };

         if( _options->count("resync-blockchain") )
            _chain_db->wipe(_data_dir / "blockchain", true);

         boost::container::flat_map<uint32_t,block_id_type> loaded_checkpoints;
         if( _options->count("checkpoint") )
         {
            auto cps = _options->at("checkpoint").as<std::vector<std::string>>();
            loaded_checkpoints.reserve( cps.size() );
            for( auto cp : cps )
            {
               auto item = fc::json::from_string(cp).as<std::pair<uint32_t,block_id_type> >();
               loaded_checkpoints[item.first] = item.second;
               ilog ( "loaded checkpoint ${s} at ${n}", ("s",loaded_checkpoints[item.first])("n", item.first));
            }
         }

         _chain_db->add_checkpoints( loaded_checkpoints );

         bool objdb = exists(_data_dir / "blockchain/object_database");
         if( !objdb || _options->count("replay-blockchain") )
         {
            if( objdb )
               ilog("Replaying blockchain on user request.");
            else
               ilog("Replaying blockchain due missing object database.");
            _chain_db->reindex(_data_dir/"blockchain", initial_state());
            write_db_version();
         } else if( clean ) {

            auto is_new = [&]() -> bool
            {
               // directory doesn't exist
               if( !exists( _data_dir ) )
                  return true;
               // if directory exists but is empty, return true; else false.
               return ( boost::filesystem::directory_iterator( _data_dir ) == boost::filesystem::directory_iterator() );
            };

            auto is_outdated = [&]() -> bool
            {
               if( !exists( _data_dir / "db_version" ) )
                  return true;
               std::string version_str;
               fc::read_file_contents( _data_dir / "db_version", version_str );
               return (version_str != GRAPHENE_CURRENT_DB_VERSION);
            };

            bool need_reindex = (!is_new() && is_outdated());
            std::string reindex_reason = "version upgrade";

            if( !need_reindex )
            {
               try
               {
                  _chain_db->open(_data_dir / "blockchain", initial_state);
               }
               catch( const fc::exception& e )
               {
                  ilog( "caught exception ${e} in open()", ("e", e.to_detail_string()) );
                  need_reindex = true;
                  reindex_reason = "exception in open()";
               }
            }

            if( need_reindex )
            {
               ilog("Replaying blockchain due to ${reason}", ("reason", reindex_reason) );

               _chain_db->reindex(_data_dir / "blockchain", initial_state());
               write_db_version();
            }
         } else {
            wlog("Detected unclean shutdown. Replaying blockchain...");
            _chain_db->reindex(_data_dir / "blockchain", initial_state());
            write_db_version();
         }

         if (!_options->count("genesis-json") &&
             _chain_db->get_chain_id() != graphene::egenesis::get_egenesis_chain_id()) {
            elog("Detected old database. Nuking and starting over.");
            _chain_db->wipe(_data_dir / "blockchain", true);
            _chain_db.reset();
            _chain_db = std::make_shared<chain::database>(std::vector<uint8_t>{ chain::local_object_type_count, chain::protocol_object_type_count, chain::impl_object_type_count });
            _chain_db->add_checkpoints(loaded_checkpoints);
            _chain_db->open(_data_dir / "blockchain", initial_state);
         }

         if( _options->count("force-validate") )
         {
            ilog( "All transaction signatures will be validated" );
            _force_validate = true;
         }

         graphene::utilities::now();

         if( _options->count("api-access") )
            _apiaccess = fc::json::from_file( _options->at("api-access").as<boost::filesystem::path>() )
               .as<api_access>();
         else
         {
            // TODO:  Remove this generous default access policy
            // when the UI logs in properly
            _apiaccess = api_access();
         }

         reset_p2p_node(_data_dir);
         reset_websocket_server();
         reset_websocket_tls_server();
      } FC_LOG_AND_RETHROW() }

      void shutdown()
      { try {
         if( _p2p_network )
         {
            ilog("Closing p2p node");
            _p2p_network->close();
            _p2p_network.reset();
         }

         if( _websocket_tls_server )
         {
            ilog("Closing websocket TLS rpc");
            _websocket_tls_server.reset();
         }

         if( _websocket_server )
         {
            ilog("Closing websocket rpc");
            _websocket_server.reset();
         }

         if( _chain_db )
         {
            ilog("Closing database");
            _chain_db->close();
         }

         if( _db_lock )
         {
            ilog("Release database lock");
            _db_lock->unlock();
            _db_lock.reset();
            remove_all(_data_dir / "blockchain/dblock");
         }
      } FC_LOG_AND_RETHROW() }

      fc::optional<api_access_info> get_api_access_info(const std::string& username)const
      {
         fc::optional<api_access_info> result;
         auto it = _apiaccess.permission_map.find(username);
         if( it == _apiaccess.permission_map.end() )
         {
            it = _apiaccess.permission_map.find("*");
            if( it == _apiaccess.permission_map.end() )
               return result;
         }
         return it->second;
      }

      void set_api_access_info(const std::string& username, api_access_info&& permissions)
      {
         _apiaccess.permission_map.insert(std::make_pair(username, std::move(permissions)));
      }

      /**
       * If delegate has the item, the network has no need to fetch it.
       */
      virtual bool has_item(const net::item_id& id) override
      {
         try
         {
            return id.item_type == graphene::net::block_message_type && _chain_db->is_known_block(id.item_hash);
         }
         FC_CAPTURE_AND_RETHROW( (id) )
      }

      /**
       * @brief allows the application to validate an item prior to broadcasting to peers.
       *
       * @param sync_mode true if the message was fetched through the sync process, false during normal operation
       * @returns maximum block size (as set in global properties)
       *
       * @throws exception if error validating the item, otherwise the item is safe to broadcast on.
       */
      virtual uint32_t handle_block(const graphene::net::block_message& blk_msg, bool sync_mode,
                                    std::vector<fc::uint160_t>& contained_transaction_message_ids) override
      { try {

         auto latency = graphene::utilities::now() - blk_msg.block.timestamp;
         if (!sync_mode || blk_msg.block.block_num() % 10000 == 0)
         {
            const auto& miner = blk_msg.block.miner(*_chain_db);
            const auto& miner_account = miner.miner_account(*_chain_db);
            auto last_irr = _chain_db->get_dynamic_global_properties().last_irreversible_block_num;
            ilog("Got block: #${n} time: ${t} latency: ${l} ms from: ${w}  irreversible: ${i} (-${d})",
                 ("t",blk_msg.block.timestamp)
                 ("n", blk_msg.block.block_num())
                 ("l", (latency.count()/1000))
                 ("w",miner_account.name)
                 ("i",last_irr)("d",blk_msg.block.block_num()-last_irr) );
         }

         try {
            // TODO: in the case where this block is valid but on a fork that's too old for us to switch to,
            // you can help the network code out by throwing a block_older_than_undo_history exception.
            // when the net code sees that, it will stop trying to push blocks from that chain, but
            // leave that peer connected so that they can get sync blocks from us
            _chain_db->push_block(blk_msg.block,
                                                (_is_block_producer | _force_validate) ? chain::database::skip_nothing
                                                                                       : chain::database::skip_transaction_signatures,
                                                sync_mode);

            // the block was accepted, so we now know all of the transactions contained in the block
            if (!sync_mode)
            {
               // if we're not in sync mode, there's a chance we will be seeing some transactions
               // included in blocks before we see the free-floating transaction itself.  If that
               // happens, there's no reason to fetch the transactions, so  construct a list of the
               // transaction message ids we no longer need.
               // during sync, it is unlikely that we'll see any old
               for (const chain::processed_transaction& transaction : blk_msg.block.transactions)
               {
                  graphene::net::trx_message transaction_message(transaction);
                  contained_transaction_message_ids.push_back(graphene::net::message(transaction_message).id());
               }
            }

            return _chain_db->get_global_properties().parameters.maximum_block_size;
         } catch ( const graphene::chain::unlinkable_block_exception& e ) {
            // translate to a graphene::net exception
            MONITORING_COUNTER_VALUE(blocks_unhandled)++;
            elog("Error when pushing block:\n${e}", ("e", e.to_detail_string()));
            FC_THROW_EXCEPTION(graphene::net::unlinkable_block_resync_peer_exception, "Error when pushing block:\n${e}", ("e", e.to_detail_string()));
         } catch( const fc::exception& e ) {
            elog("Error when pushing block:\n${e}", ("e", e.to_detail_string()));
            throw;
         }
      } FC_CAPTURE_AND_RETHROW( (blk_msg)(sync_mode) ) }

      virtual void handle_transaction(const graphene::net::trx_message& transaction_message) override
      { try {
         static fc::time_point last_call;
         static int trx_count = 0;
         ++trx_count;
         auto now = fc::time_point::now();
         if( now - last_call > fc::seconds(1) ) {
            ilog("Got ${c} transactions from network", ("c",trx_count) );
            last_call = now;
            trx_count = 0;
         }

         _chain_db->push_transaction( transaction_message.trx );
      } FC_CAPTURE_AND_RETHROW( (transaction_message) ) }

      virtual void handle_message(const message& message_to_process) override
      {
         // not a transaction, not a block
         FC_THROW( "Invalid Message Type" );
      }

      bool is_included_block(const block_id_type& block_id)
      {
        uint32_t block_num = block_header::num_from_id(block_id);
        block_id_type block_id_in_preferred_chain = _chain_db->get_block_id_for_num(block_num);
        return block_id == block_id_in_preferred_chain;
      }

      /**
       * Assuming all data elements are ordered in some way, this method should
       * return up to limit ids that occur *after* the last ID in synopsis that
       * we recognize.
       *
       * On return, remaining_item_count will be set to the number of items
       * in our blockchain after the last item returned in the result,
       * or 0 if the result contains the last item in the blockchain
       */
      virtual std::vector<item_hash_t> get_block_ids(const std::vector<item_hash_t>& blockchain_synopsis,
                                                     uint32_t& remaining_item_count,
                                                     uint32_t limit) override
      { try {
         std::vector<block_id_type> result;
         remaining_item_count = 0;
         if( _chain_db->head_block_num() == 0 )
            return result;

         result.reserve(limit);
         block_id_type last_known_block_id;

         if (blockchain_synopsis.empty() ||
             (blockchain_synopsis.size() == 1 && blockchain_synopsis[0] == block_id_type()))
         {
           // peer has sent us an empty synopsis meaning they have no blocks.
           // A bug in old versions would cause them to send a synopsis containing block 000000000
           // when they had an empty blockchain, so pretend they sent the right thing here.

           // do nothing, leave last_known_block_id set to zero
         }
         else
         {
           bool found_a_block_in_synopsis = false;
           for (const item_hash_t& block_id_in_synopsis : boost::adaptors::reverse(blockchain_synopsis))
             if (block_id_in_synopsis == block_id_type() ||
                 (_chain_db->is_known_block(block_id_in_synopsis) && is_included_block(block_id_in_synopsis)))
             {
               last_known_block_id = block_id_in_synopsis;
               found_a_block_in_synopsis = true;
               break;
             }
           FC_VERIFY_AND_THROW(found_a_block_in_synopsis, net::peer_is_on_an_unreachable_fork_exception, "Unable to provide a list of blocks starting at any of the blocks in peer's synopsis");
         }
         for( uint32_t num = block_header::num_from_id(last_known_block_id);
              num <= _chain_db->head_block_num() && result.size() < limit;
              ++num )
            if( num > 0 )
               result.push_back(_chain_db->get_block_id_for_num(num));

         if( !result.empty() && block_header::num_from_id(result.back()) < _chain_db->head_block_num() )
            remaining_item_count = _chain_db->head_block_num() - block_header::num_from_id(result.back());

         return result;
      } FC_CAPTURE_AND_RETHROW( (blockchain_synopsis)(remaining_item_count)(limit) ) }

      /**
       * Given the hash of the requested data, fetch the body.
       */
      virtual message get_item(const item_id& id) override
      { try {
        // ilog("Request for item ${id}", ("id", id));
         FC_VERIFY_AND_THROW(id.item_type == net::block_message_type, fc::key_not_found_exception);
         auto opt_block = _chain_db->fetch_block_by_id(id.item_hash);
         if( !opt_block )
            elog("Couldn't find block ${id} -- corresponding ID in our chain is ${id2}",
                  ("id", id.item_hash)("id2", _chain_db->get_block_id_for_num(block_header::num_from_id(id.item_hash))));
         FC_ASSERT( opt_block.valid() );
         // ilog("Serving up block #${num}", ("num", opt_block->block_num()));
         return block_message(std::move(*opt_block));
      } FC_CAPTURE_AND_RETHROW( (id) ) }

      virtual chain::chain_id_type get_chain_id() const override
      {
         return _chain_db->get_chain_id();
      }

      /**
       * Returns a synopsis of the blockchain used for syncing.  This consists of a list of
       * block hashes at intervals exponentially increasing towards the genesis block.
       * When syncing to a peer, the peer uses this data to determine if we're on the same
       * fork as they are, and if not, what blocks they need to send us to get us on their
       * fork.
       *
       * In the over-simplified case, this is a straighforward synopsis of our current
       * preferred blockchain; when we first connect up to a peer, this is what we will be sending.
       * It looks like this:
       *   If the blockchain is empty, it will return the empty list.
       *   If the blockchain has one block, it will return a list containing just that block.
       *   If it contains more than one block:
       *     the first element in the list will be the hash of the highest numbered block that
       *         we cannot undo
       *     the second element will be the hash of an item at the half way point in the undoable
       *         segment of the blockchain
       *     the third will be ~3/4 of the way through the undoable segment of the block chain
       *     the fourth will be at ~7/8...
       *       &c.
       *     the last item in the list will be the hash of the most recent block on our preferred chain
       * so if the blockchain had 26 blocks labeled a - z, the synopsis would be:
       *    a n u x z
       * the idea being that by sending a small (<30) number of block ids, we can summarize a huge
       * blockchain.  The block ids are more dense near the end of the chain where because we are
       * more likely to be almost in sync when we first connect, and forks are likely to be short.
       * If the peer we're syncing with in our example is on a fork that started at block 'v',
       * then they will reply to our synopsis with a list of all blocks starting from block 'u',
       * the last block they know that we had in common.
       *
       * In the real code, there are several complications.
       *
       * First, as an optimization, we don't usually send a synopsis of the entire blockchain, we
       * send a synopsis of only the segment of the blockchain that we have undo data for.  If their
       * fork doesn't build off of something in our undo history, we would be unable to switch, so there's
       * no reason to fetch the blocks.
       *
       * Second, when a peer replies to our initial synopsis and gives us a list of the blocks they think
       * we are missing, they only send a chunk of a few thousand blocks at once.  After we get those
       * block ids, we need to request more blocks by sending another synopsis (we can't just say "send me
       * the next 2000 ids" because they may have switched forks themselves and they don't track what
       * they've sent us).  For faster performance, we want to get a fairly long list of block ids first,
       * then start downloading the blocks.
       * The peer doesn't handle these follow-up block id requests any different from the initial request;
       * it treats the synopsis we send as our blockchain and bases its response entirely off that.  So to
       * get the response we want (the next chunk of block ids following the last one they sent us, or,
       * failing that, the shortest fork off of the last list of block ids they sent), we need to construct
       * a synopsis as if our blockchain was made up of:
       *    1. the blocks in our block chain up to the fork point (if there is a fork) or the head block (if no fork)
       *    2. the blocks we've already pushed from their fork (if there's a fork)
       *    3. the block ids they've previously sent us
       * Segment 3 is handled in the p2p code, it just tells us the number of blocks it has (in
       * number_of_blocks_after_reference_point) so we can leave space in the synopsis for them.
       * We're responsible for constructing the synopsis of Segments 1 and 2 from our active blockchain and
       * fork database.  The reference_point parameter is the last block from that peer that has been
       * successfully pushed to the blockchain, so that tells us whether the peer is on a fork or on
       * the main chain.
       */
      virtual std::vector<item_hash_t> get_blockchain_synopsis(const item_hash_t& reference_point,
                                                               uint32_t number_of_blocks_after_reference_point) override
      { try {
          std::vector<item_hash_t> synopsis;
          synopsis.reserve(30);
          uint32_t high_block_num;
          uint32_t non_fork_high_block_num;
          uint32_t low_block_num = _chain_db->last_non_undoable_block_num();
          std::vector<block_id_type> fork_history;

          if (reference_point != item_hash_t())
          {
            // the node is asking for a summary of the block chain up to a specified
            // block, which may or may not be on a fork
            // for now, assume it's not on a fork
            if (is_included_block(reference_point))
            {
              // reference_point is a block we know about and is on the main chain
              uint32_t reference_point_block_num = block_header::num_from_id(reference_point);
              assert(reference_point_block_num > 0);
              high_block_num = reference_point_block_num;
              non_fork_high_block_num = high_block_num;

              if (reference_point_block_num < low_block_num)
              {
                // we're on the same fork (at least as far as reference_point) but we've passed
                // reference point and could no longer undo that far if we diverged after that
                // block.  This should probably only happen due to a race condition where
                // the network thread calls this function, and then immediately pushes a bunch of blocks,
                // then the main thread finally processes this function.
                // with the current framework, there's not much we can do to tell the network
                // thread what our current head block is, so we'll just pretend that
                // our head is actually the reference point.
                // this *may* enable us to fetch blocks that we're unable to push, but that should
                // be a rare case (and correctly handled)
                low_block_num = reference_point_block_num;
              }
            }
            else
            {
              // block is a block we know about, but it is on a fork
              try
              {
                fork_history = _chain_db->get_block_ids_on_fork(reference_point);
                // returns a vector where the last element is the common ancestor with the preferred chain,
                // and the first element is the reference point you passed in
                assert(fork_history.size() >= 2);

                if( fork_history.front() != reference_point )
                {
                   edump( (fork_history)(reference_point) );
                   assert(fork_history.front() == reference_point);
                }
                block_id_type last_non_fork_block = fork_history.back();
                fork_history.pop_back();  // remove the common ancestor
                boost::reverse(fork_history);

                if (last_non_fork_block == block_id_type()) // if the fork goes all the way back to genesis (does graphene's fork db allow this?)
                  non_fork_high_block_num = 0;
                else
                  non_fork_high_block_num = block_header::num_from_id(last_non_fork_block);

                high_block_num = non_fork_high_block_num + static_cast<uint32_t>(fork_history.size());
                assert(high_block_num == block_header::num_from_id(fork_history.back()));
              }
              catch (const fc::exception& e)
              {
                // unable to get fork history for some reason.  maybe not linked?
                // we can't return a synopsis of its chain
                elog("Unable to construct a blockchain synopsis for reference hash ${hash}: ${exception}", ("hash", reference_point)("exception", e));
                throw;
              }
              if (non_fork_high_block_num < low_block_num)
              {
                wlog("Unable to generate a usable synopsis because the peer we're generating it for forked too long ago "
                     "(our chains diverge after block #${non_fork_high_block_num} but only undoable to block #${low_block_num})",
                     ("low_block_num", low_block_num)
                     ("non_fork_high_block_num", non_fork_high_block_num));
                FC_THROW_EXCEPTION(graphene::net::block_older_than_undo_history_exception, "Peer is are on a fork I'm unable to switch to");
              }
            }
          }
          else
          {
            // no reference point specified, summarize the whole block chain
            high_block_num = _chain_db->head_block_num();
            non_fork_high_block_num = high_block_num;
            if (high_block_num == 0)
              return synopsis; // we have no blocks
          }

          // at this point:
          // low_block_num is the block before the first block we can undo,
          // non_fork_high_block_num is the block before the fork (if the peer is on a fork, or otherwise it is the same as high_block_num)
          // high_block_num is the block number of the reference block, or the end of the chain if no reference provided

          // true_high_block_num is the ending block number after the network code appends any item ids it
          // knows about that we don't
          uint32_t orig_low_block_num =  low_block_num;
          uint32_t true_high_block_num = high_block_num + number_of_blocks_after_reference_point;
          do
          {
            // for each block in the synopsis, figure out where to pull the block id from.
            // if it's <= non_fork_high_block_num, we grab it from the main blockchain;
            // if it's not, we pull it from the fork history
            if( !low_block_num )
               ++low_block_num;
            if (low_block_num <= non_fork_high_block_num)
              synopsis.push_back(_chain_db->get_block_id_for_num(low_block_num));
            else
              synopsis.push_back(fork_history[low_block_num - non_fork_high_block_num - 1]);
            low_block_num += (true_high_block_num - low_block_num + 2) / 2;
          }
          while (low_block_num <= high_block_num);

          ddump((synopsis));
          dlog("synopsis for blocks ${l} - ${h}",("l", orig_low_block_num)("h", high_block_num));
          return synopsis;
      } FC_RETHROW() }

      /**
       * Call this after the call to handle_message succeeds.
       *
       * @param item_type the type of the item we're synchronizing, will be the same as item passed to the sync_from() call
       * @param item_count the number of items known to the node that haven't been sent to handle_item() yet.
       *                   After `item_count` more calls to handle_item(), the node will be in sync
       */
      virtual void sync_status(uint32_t item_type, uint32_t item_count) override
      {
         // any status reports to GUI go here
      }

      /**
       * Call any time the number of connected peers changes.
       */
      virtual void connection_count_changed(uint32_t c) override
      {
        // any status reports to GUI go here
      }

      virtual uint32_t get_block_number(const item_hash_t& block_id) override
      { try {
         return block_header::num_from_id(block_id);
      } FC_CAPTURE_AND_RETHROW( (block_id) ) }

      /**
       * Returns the time a block was produced (if block_id = 0, returns genesis time).
       * If we don't know about the block, returns time_point_sec::min()
       */
      virtual fc::time_point_sec get_block_time(const item_hash_t& block_id) override
      { try {
         auto opt_block = _chain_db->fetch_block_by_id( block_id );
         if( opt_block.valid() ) return opt_block->timestamp;
         return fc::time_point_sec::min();
      } FC_CAPTURE_AND_RETHROW( (block_id) ) }

      /** returns graphene::time::now() */
      virtual fc::time_point_sec get_blockchain_now() override
      {
         return graphene::utilities::now();
      }

      virtual item_hash_t get_head_block_id() const override
      {
         return _chain_db->head_block_id();
      }

      virtual void error_encountered(const std::string& message, const fc::optional<fc::exception>& error) override
      {
         // notify GUI or something cool
      }

      uint8_t get_current_block_interval_in_seconds() const override
      {
         return _chain_db->get_global_properties().parameters.block_interval;
      }

      application* _self;

      boost::filesystem::path _data_dir;
      const bpo::variables_map* _options = nullptr;
      api_access _apiaccess;

      std::unique_ptr<fc::simple_lock_file> _db_lock;
      std::shared_ptr<graphene::chain::database>            _chain_db;
      std::shared_ptr<graphene::net::node>                  _p2p_network;
      std::shared_ptr<fc::http::websocket_server>      _websocket_server;
      std::shared_ptr<fc::http::websocket_tls_server>  _websocket_tls_server;

      std::map<std::string, std::shared_ptr<abstract_plugin>> _plugins;
   };

} // namespace detail

class param_validator_app
{
public:
   param_validator_app(const std::string param_name)
      : _name(param_name)
   {
   }
   // ipv4:port or dnsname:port
   const std::string REG_EXPR_IPV4_AND_PORT_OR_DNS =
      "^"
      "(((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\x2E){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?):([0-9]{1,4}|[1-5][0-9]{4}|6[0-4][0-9]{3}|65[0-4][0-9]{2}|655[0-2][0-9]|6553[0-5]))|" // ipv4::port or
      "(([^:]+):([0-9]{1,4}|[1-5][0-9]{4}|6[0-4][0-9]{3}|65[0-4][0-9]{2}|655[0-2][0-9]|6553[0-5]))" // dnsname:port. it is problem to test dnsname. in this case dnsname can be anything without character :
      "$";

   const std::string REG_EXPR_CHECKPOINT =
      "^"
      "\\x5B[0-9]{1,19},\"([0-9a-fA-F]){40}\"\\x5D"
      "$";

   void check_reg_expr(const std::regex& rx, const std::string& val)
   {
      FC_VERIFY_AND_THROW(std::regex_match(val, rx), fc::parse_error_exception, "Invalid argument: ${name} = ${value}", ("name", _name)("value", val));
   }

   void check_reg_expr(const std::regex& rx, const std::vector<std::string>& val)
   {
      for (size_t i = 0; i < val.size(); i++) {
         FC_VERIFY_AND_THROW(std::regex_match(val[i], rx), fc::parse_error_exception, "Invalid argument: ${name} = ${value}", ("name", _name)("value", val[i]));
      }
   }

   void operator()(const std::string& val)
   {
      if (_name == "p2p-endpoint" || _name == "rpc-endpoint" || _name == "rpc-tls-endpoint")
      {
         const std::regex rx(REG_EXPR_IPV4_AND_PORT_OR_DNS);
         check_reg_expr(rx, val);
         // additional checks
         try {
            fc::ip::endpoint::resolve_string(val);
         }
         catch (...) {
            FC_THROW_EXCEPTION(fc::parse_error_exception, "Invalid argument: ${name} = ${value}, Cannot convert string to IP endpoint", ("name", _name)("value", val));
         }
      }
      else if (_name == "p2p-cert-authority-file" || _name == "p2p-cert-file" || _name == "p2p-cert-key-file" || _name == "p2p-cert-key-file" ||
               _name == "server-cert-file" || _name == "server-cert-key-file" || _name == "server-cert-key-file" || _name == "server-cert-chain-file") {
         boost::filesystem::path p(val);
         FC_VERIFY_AND_THROW(boost::filesystem::exists(p), fc::parse_error_exception, "Invalid argument: ${name} = ${value}, The file does not exist", ("name", _name)("value", p.string()));
         FC_VERIFY_AND_THROW(boost::filesystem::is_regular(p), fc::parse_error_exception, "Invalid argument: ${name} = ${value}, The path does not point to a regular file", ("name", _name)("value", p.string()));
      }
   }
   void operator()(const std::vector<std::string>& val)
   {
      if (_name == "seed-node")
      {
         const std::regex rx(REG_EXPR_IPV4_AND_PORT_OR_DNS);

         // additional checks
         for (size_t i = 0; i < val.size(); i++) {
            check_reg_expr(rx, val);
            try
            {
               fc::ip::endpoint::resolve_string(val[i]);
            }
            catch (...) {
               FC_THROW_EXCEPTION(fc::parse_error_exception, "Invalid argument: ${name} = ${value}, Cannot convert string to IP endpoint", ("name", _name)("value", val[i]));
            }
         }
      } else
      if(_name == "checkpoint")
      {
         const std::regex rx(REG_EXPR_CHECKPOINT);
         for(size_t i = 0; i < val.size(); i++) {
            check_reg_expr(rx, val);
         }
      }
   }
   void operator()(const boost::filesystem::path& p)
   {
      FC_VERIFY_AND_THROW(boost::filesystem::exists(p), fc::parse_error_exception, "Invalid argument: ${name} = ${value}, The file does not exist", ("name", _name)("value", p.string()));
      FC_VERIFY_AND_THROW(boost::filesystem::is_regular(p), fc::parse_error_exception, "Invalid argument: ${name} = ${value}, The path does not point to a regular file", ("name", _name)("value", p.string()));
   }

   std::string _name;
};

application::application()
   : my(new detail::application_impl(this))
{}

application::~application()
{
}

void application::set_program_options(boost::program_options::options_description& command_line_options,
                                      boost::program_options::options_description& configuration_file_options)
{
   command_line_options.add_options()
      ("help,h", "Print this help message and exit")
      ("version,v", "Print version information and exit")
      ;

   configuration_file_options.add_options()
         ("p2p-endpoint", bpo::value<std::string>()->notifier(param_validator_app("p2p-endpoint")), "Endpoint for P2P node to listen on")
         ("p2p-cert-authority-file", bpo::value<std::string>()->notifier(param_validator_app("p2p-cert-authority-file")), "The TLS certificate authority file")
         ("p2p-cert-file", bpo::value<std::string>()->notifier(param_validator_app("p2p-cert-file")), "The TLS certificate file (public) for this P2P node")
         ("p2p-cert-key-file", bpo::value<std::string>()->notifier(param_validator_app("p2p-cert-key-file")), "The TLS certificate file (private key) for this P2P node")
         ("p2p-cert-key-password", bpo::value<std::string>(), "Password for TLS certificate file (private key) for this P2P node")
         ("seed-node,s", bpo::value<std::vector<std::string>>()->composing()->notifier(param_validator_app("seed-node")),"P2P nodes to connect to on startup (may specify multiple times)")
         ("checkpoint,c", bpo::value<std::vector<std::string>>()->composing()->notifier(param_validator_app("checkpoint")), "Pairs of [BLOCK_NUM,BLOCK_ID] that should be enforced as checkpoints.")
         ("rpc-endpoint", bpo::value<std::string>()->default_value("127.0.0.1:8090")->notifier(param_validator_app("rpc-endpoint")), "Endpoint for websocket RPC to listen on")
         ("rpc-tls-endpoint", bpo::value<std::string>()->implicit_value("127.0.0.1:8089")->notifier(param_validator_app("rpc-tls-endpoint")), "Endpoint for TLS websocket RPC to listen on")
         ("enable-permessage-deflate", "Enable support for per-message deflate compression in the websocket servers (--rpc-endpoint and --rpc-tls-endpoint), disabled by default")
         ("server-allowed-domains", "List of allowed domains to communicate with or asterix for all domains")
         ("server-cert-file", bpo::value<std::string>()->notifier(param_validator_app("server-cert-file")), "The TLS certificate file (public) for this websocket server")
         ("server-cert-key-file", bpo::value<std::string>()->notifier(param_validator_app("server-cert-key-file")), "The TLS certificate file (private key) for this websocket server")
         ("server-cert-chain-file", bpo::value<std::string>()->notifier(param_validator_app("server-cert-chain-file")), "The TLS certificate chain file for this websocket server")
         ("server-cert-password", bpo::value<std::string>(), "Password for TLS certificate file (private key) for this websocket server")
         ("genesis-json", bpo::value<boost::filesystem::path>()->notifier(param_validator_app("genesis-json")), "File to read Genesis State from")
         ("dbg-init-key", bpo::value<std::string>(), "Block signing key to use for init miners, overrides genesis file")
         ("api-access", bpo::value<boost::filesystem::path>()->notifier(param_validator_app("api-access")), "JSON file specifying API permissions")
         ("track-account", bpo::value<std::vector<std::string>>()->composing()->multitoken(), "Account ID to track history for (may specify multiple times)")
         ;

   bpo::options_description common_options("Common options");
   common_options.add_options()
         ("data-dir,d", bpo::value<boost::filesystem::path>()->default_value(utilities::decent_path_finder::instance().get_decent_data() / "decentd"),
          "Directory containing databases, configuration file, etc.")
         ("create-genesis-json", bpo::value<boost::filesystem::path>(),
          "Path to create a Genesis State at. If a well-formed JSON file exists at the path, it will be parsed and any "
          "missing fields in a Genesis State will be added, and any unknown fields will be removed. If no file or an "
          "invalid file is found, it will be replaced with an example Genesis State.")
         ("replay-blockchain", "Rebuild object graph by replaying all blocks")
         ("resync-blockchain", "Delete all blocks and re-sync with network from scratch")
         ;

   bpo::options_description advanced_options("Advanced options");
   advanced_options.add_options()
         ("force-validate", bpo::bool_switch(), "Force validation of all transactions")
         ("genesis-timestamp", bpo::value<uint32_t>(), "Replace timestamp from genesis.json")
         ;

   command_line_options.add(common_options);
   command_line_options.add(advanced_options);
   command_line_options.add(configuration_file_options);

   // hidden settings only in config file
   configuration_file_options.add_options()
         ("fork-times", bpo::value<std::string>()->notifier([](const std::string& args) {
               std::size_t fork = 0;
               for(std::size_t i = 0, j = 0; i != std::string::npos; i = j) {
                  FC_ASSERT(fork < graphene::chain::fork_times.size(), "Too many fork times, should be ${n}", ("n", graphene::chain::fork_times.size()));
                  j = args.find(',', i ? ++i : i);
                  graphene::chain::fork_times[fork++] = fc::time_point_sec::from_iso_string(boost::trim_copy(args.substr(i, j - i)));
               }
               FC_ASSERT(fork == graphene::chain::fork_times.size(), "Too few fork times, should be ${n}", ("n", graphene::chain::fork_times.size()));
            }), "INTERNAL: List of comma separated fork times in ISO format")
         ;
}

void application::initialize(const boost::filesystem::path& data_dir, const boost::program_options::variables_map& options)
{
   my->_data_dir = data_dir;
   my->_options = &options;

   if( options.count("create-genesis-json") )
   {
      boost::filesystem::path genesis_out = options.at("create-genesis-json").as<boost::filesystem::path>();
      chain::genesis_state_type genesis_state = detail::create_example_genesis();
      if( exists(genesis_out) )
      {
         try {
            genesis_state = fc::json::from_file(genesis_out).as<chain::genesis_state_type>();
         } catch(const fc::exception& e) {
            std::cerr << "Unable to parse existing genesis file:\n" << e.to_string()
                      << "\nWould you like to replace it? [y/N] ";
            char response = static_cast<char>(std::cin.get());
            if( toupper(response) != 'Y' )
               return;
         }

         std::cerr << "Updating genesis state in file " << genesis_out.generic_string() << "\n";
      } else {
         std::cerr << "Creating example genesis state in file " << genesis_out.generic_string() << "\n";
      }
      fc::json::save_to_file(genesis_state, genesis_out);

      std::exit(EXIT_SUCCESS);
   }

   if(options.count("track-account")) {
      const std::vector<std::string>& ops = options["track-account"].as<std::vector<std::string>>();
      std::for_each(ops.begin(), ops.end(), [](const std::string &acc) {
         try {
            graphene::db::object_id_type account(acc);
            FC_ASSERT( account.is<graphene::chain::account_id_type>(), "Invalid account ${a}", ("a", acc) );
            graphene::chain::generic_evaluator::track_account(account);
         } FC_RETHROW_EXCEPTIONS(error, "Invalid argument: track-account = ${a}", ("a", acc));
      });
   }
}

void application::startup()
{
   my->startup();
}

std::shared_ptr<abstract_plugin> application::get_plugin(const std::string& name) const
{
   return my->_plugins[name];
}

net::node_ptr application::p2p_node()
{
   return my->_p2p_network;
}

std::shared_ptr<chain::database> application::chain_database() const
{
   return my->_chain_db;
}

void application::set_block_production(bool producing_blocks)
{
   my->_is_block_producer = producing_blocks;
}

fc::optional<api_access_info> application::get_api_access_info( const std::string& username )const
{
   return my->get_api_access_info( username );
}

void application::set_api_access_info(const std::string& username, api_access_info&& permissions)
{
   my->set_api_access_info(username, std::move(permissions));
}

uint64_t application::get_processed_transactions()
{
   return my->_processed_transactions;
}

void graphene::app::application::add_plugin(const std::string& name, std::shared_ptr<graphene::app::abstract_plugin> p)
{
   my->_plugins[name] = p;
}

void application::shutdown_plugins()
{
   for( auto& entry : my->_plugins )
      entry.second->plugin_shutdown();
}

void application::shutdown()
{
   my->shutdown();
}

void application::initialize_plugins( const boost::program_options::variables_map& options )
{
   for( auto& entry : my->_plugins )
      entry.second->plugin_initialize( options );
}

void application::startup_plugins()
{
   for( auto& entry : my->_plugins )
      entry.second->plugin_startup();
}

} } // graphene::app
