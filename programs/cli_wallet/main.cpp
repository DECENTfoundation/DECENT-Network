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

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <iterator>

#include <fc/io/json.hpp>
#include <fc/io/stdio.hpp>
#include <fc/network/http/server.hpp>
#include <fc/network/http/websocket.hpp>
#include <fc/rpc/cli.hpp>
#include <fc/rpc/http_api.hpp>
#include <fc/rpc/websocket_api.hpp>
#include <fc/smart_ref_impl.hpp>


#include <graphene/app/api.hpp>



#include <graphene/chain/protocol/protocol.hpp>
#include <graphene/egenesis/egenesis.hpp>
#include <graphene/utilities/key_conversion.hpp>
#include <graphene/utilities/keys_generator.hpp>
#include <graphene/utilities/git_revision.hpp>
#include <graphene/wallet/wallet.hpp>
#include <decent/package/package.hpp>
#include <graphene/utilities/dirhelper.hpp>

#include <fc/interprocess/signals.hpp>
#include <boost/program_options.hpp>
#include <boost/version.hpp>

#include <openssl/opensslv.h>

#include <fc/log/console_appender.hpp>
#include <fc/log/file_appender.hpp>
#include <fc/log/logger.hpp>
#include <fc/log/logger_config.hpp>

#ifdef WIN32
# include <signal.h>
#else
# include <csignal>
#endif

using namespace graphene::app;
using namespace graphene::chain;
using namespace graphene::utilities;
using namespace graphene::wallet;
namespace bpo = boost::program_options;

int main( int argc, char** argv )
{
    fc::path decent_home;
    try {
        decent_home = decent_path_finder::instance().get_decent_home();
    } catch (const std::exception& ex) {
        std::cout << "Failed to initialize home directory." << std::endl;
        std::cout << "Error: " << ex.what() << std::endl;
        return 1;
    } catch (const fc::exception& ex) {
        std::cout << "Failed to initialize home directory." << std::endl;
        std::cout << "Error: " << ex.what() << std::endl;
        return 1;
    }

   try {

      bpo::options_description opts;
         opts.add_options()
         ("help,h", "Print this help message and exit.")
         ("version,v", "Print version information and exit.")
         ("generate-keys,g", "Generate brain, wif private and public keys.")
         ("wallet-file,w", bpo::value<std::string>()->implicit_value("wallet.json"), "Wallet to load.")
         ("daemon", "Run the wallet in daemon mode.")
         ("chain-id", bpo::value<std::string>(), "Chain ID to connect to.")
         ("server-rpc-endpoint,s", bpo::value<std::string>()->default_value("ws://127.0.0.1:8090"), "Server websocket RPC endpoint")
         ("server-rpc-user,u", bpo::value<std::string>(), "Server Username")
         ("server-rpc-password,p", bpo::value<std::string>(), "Server Password")
         ("rpc-endpoint,r", bpo::value<std::string>()->implicit_value("127.0.0.1:8091"), "Endpoint for wallet websocket RPC to listen on")
         ("rpc-tls-endpoint,t", bpo::value<std::string>()->implicit_value("127.0.0.1:8092"), "Endpoint for wallet websocket TLS RPC to listen on")
         ("rpc-tls-certificate,c", bpo::value<std::string>()->default_value("server.pem"), "PEM certificate for wallet websocket TLS RPC")
         ("rpc-http-endpoint,H", bpo::value<std::string>()->implicit_value("127.0.0.1:8093"), "Endpoint for wallet HTTP RPC to listen on")
         ("from-command-file,f", bpo::value<std::string>(), "Load commands from a command file")
      ;

      bpo::variables_map options;
      bpo::store( bpo::parse_command_line(argc, argv, opts), options );

      if( options.count("help") )
      {
         std::cout << opts << "\n";
         return EXIT_SUCCESS;
      }
      else if( options.count("version") )
      {
         unsigned int boost_major_version = BOOST_VERSION / 100000;
         unsigned int boost_minor_version = BOOST_VERSION / 100 - boost_major_version * 1000;
         string boost_version_text = to_string(boost_major_version) + "." + to_string(boost_minor_version) + "." + to_string(BOOST_VERSION % 100);
         string openssl_version_text = string(OPENSSL_VERSION_TEXT);
         openssl_version_text = openssl_version_text.substr(0, openssl_version_text.length() - 11);

         std::cout << "CLI Wallet " << graphene::utilities::git_version() << "\nBoost " << boost_version_text << "\n" << openssl_version_text << std::endl;
         return EXIT_SUCCESS;
      }
      else if( options.count("generate-keys") )
      {
         try
         {
            std::string brain_key = graphene::utilities::generate_brain_key();
            fc::ecc::private_key priv_key = graphene::utilities::derive_private_key(brain_key);
            graphene::chain::public_key_type pub_key(priv_key.get_public_key());

            std::cout << "Brain key:    " << brain_key << std::endl;
            std::cout << "Private key:  " << graphene::utilities::key_to_wif(priv_key) << std::endl;
            std::cout << "Public key:   " << std::string(pub_key) << std::endl;
            return EXIT_SUCCESS;
         }
         catch (const fc::exception& e)
         {
            std::cerr << e.to_detail_string() << std::endl;
         }
         catch (const std::exception& e)
         {
            std::cerr << e.what() << std::endl;
         }

         return EXIT_FAILURE;
      }

      const fc::path log_dir = decent_path_finder::instance().get_decent_logs();

      fc::file_appender::config ac_default;
      ac_default.filename             = log_dir / "cli_wallet.log";
      ac_default.flush                = true;
      ac_default.rotate               = true;
      ac_default.rotation_interval    = fc::hours( 1 );
      ac_default.rotation_limit       = fc::days( 1 );

      fc::logger_config lc_default("default");
      lc_default.level          = fc::log_level::info;
      lc_default.appenders      = {"default"};

      fc::logging_config cfg;
      cfg.appenders.push_back(fc::appender_config( "default", "file", fc::variant(ac_default)));
      cfg.loggers.push_back(lc_default);
      std::clog << "Logging to file: " << ac_default.filename.preferred_string() << std::endl;

      fc::configure_logging( cfg );

      //
      // TODO:  We read wallet_data twice, once in main() to grab the
      //    socket info, again in wallet_api when we do
      //    load_wallet_file().  Seems like this could be better
      //    designed.
      //
      wallet_data wdata;
      fc::path wallet_file( options.count("wallet-file") ? options.at("wallet-file").as<std::string>() : decent_path_finder::instance().get_decent_home() / "wallet.json");
      bool has_wallet_file = fc::exists( wallet_file );
      if( has_wallet_file )
      {
         wdata = fc::json::from_file( wallet_file ).as<wallet_data>();
      }

      // but allow CLI to override
      if( options.count("server-rpc-endpoint") )
         wdata.ws_server = options.at("server-rpc-endpoint").as<std::string>();
      if( options.count("server-rpc-user") )
         wdata.ws_user = options.at("server-rpc-user").as<std::string>();
      if( options.count("server-rpc-password") )
         wdata.ws_password = options.at("server-rpc-password").as<std::string>();

      fc::http::websocket_client client;
      ilog( "Connecting to server at ${s}", ("s", wdata.ws_server) );
      auto con  = client.connect( wdata.ws_server );
      auto apic = std::make_shared<fc::rpc::websocket_api_connection>(*con);

      auto remote_api = apic->get_remote_api< login_api >(1);
      // TODO:  Error message here
      FC_ASSERT( remote_api->login( wdata.ws_user, wdata.ws_password ) );

      if( !has_wallet_file )
      {
         if( options.count("chain-id") )
         {
            wdata.chain_id = chain_id_type(options.at("chain-id").as<std::string>());
            // the --chain-id on the CLI must match the chain ID of database we connect to
            if( remote_api->database()->get_chain_id() != wdata.chain_id )
            {
               std::cerr << "Chain ID from CLI does not match database chain ID\n";
               return 1;
            }

            std::cout << "Starting a new wallet with chain ID " << wdata.chain_id.str() << " (from CLI)\n";
         }
         else
         {
            wdata.chain_id = remote_api->database()->get_chain_id();
            std::cout << "Starting a new wallet with chain ID " << wdata.chain_id.str() << " (empty one)\n";
         }
      }
      else if( options.count("chain-id") )
      {
         // the --chain-id on the CLI must match the chain ID embedded in the wallet file
         if( chain_id_type(options.at("chain-id").as<std::string>()) != wdata.chain_id )
         {
            std::cerr << "Chain ID in wallet file " << wallet_file.generic_string() << " does not match specified chain ID\n";
            return 1;
         }
      }
      else if( remote_api->database()->get_chain_id() != wdata.chain_id )
      {
         std::cerr << "Chain ID in wallet file " << wallet_file.generic_string() << " does not match database chain ID\n";
         return 1;
      }

      server_data ws{ wdata.ws_server, wdata.ws_user, wdata.ws_password };
      auto wapiptr = std::make_shared<wallet_api>( remote_api, wdata.chain_id, ws );
      if( has_wallet_file && !wapiptr->load_wallet_file(wallet_file.generic_string()) )
      {
         std::cerr << "Failed to load wallet file " << wallet_file.generic_string() << std::endl;
         return 1;
      }

      fc::api<wallet_api> wapi(wapiptr);

      auto wallet_cli = std::make_shared<fc::rpc::cli>();
      for( auto& name_formatter : wapiptr->get_result_formatters() )
         wallet_cli->format_result( name_formatter.first, name_formatter.second );

      boost::signals2::scoped_connection closed_connection(con->closed.connect([=]{
         std::cerr << "Server has disconnected us.\n";
         wallet_cli->stop();
      }));
      (void)(closed_connection);

      if( wapiptr->is_new() )
      {
         std::cout << "Please use the set_password method to initialize a new wallet before continuing\n";
         wallet_cli->set_prompt( "new >>> " );
      } else
         wallet_cli->set_prompt( "locked >>> " );

      boost::signals2::scoped_connection locked_connection(wapiptr->lock_changed.connect([&](bool locked) {
         wallet_cli->set_prompt(  locked ? "locked >>> " : "unlocked >>> " );
      }));

      auto _websocket_server = options.count("rpc-endpoint") ?
               std::make_shared<fc::http::websocket_server>() :
               std::shared_ptr<fc::http::websocket_server>();

      if( _websocket_server )
      {
         _websocket_server->on_connection([&]( const fc::http::websocket_connection_ptr& c, bool& is_tls ){
            is_tls = false;
            wlog("." );
            auto wsc = std::make_shared<fc::rpc::websocket_api_connection>(*c);
            wsc->register_api(wapi);
            c->set_session_data( wsc );
         });
         ilog( "Listening for incoming RPC requests on ${p}", ("p", options.at("rpc-endpoint").as<std::string>() ));
         _websocket_server->listen( fc::ip::endpoint::from_string(options.at("rpc-endpoint").as<std::string>()) );
         _websocket_server->start_accept();
      }

      auto _websocket_tls_server = options.count("rpc-tls-endpoint") ?
               std::make_shared<fc::http::websocket_tls_server>(options.at("rpc-tls-certificate").as<std::string>()) :
               std::shared_ptr<fc::http::websocket_tls_server>();

      if( _websocket_tls_server )
      {
         _websocket_tls_server->on_connection([&]( const fc::http::websocket_connection_ptr& c, bool& is_tls ){
            is_tls = true;
            auto wsc = std::make_shared<fc::rpc::websocket_api_connection>(*c);
            wsc->register_api(wapi);
            c->set_session_data( wsc );
         });
         ilog( "Listening for incoming TLS RPC requests on ${p}, certificate file ${c}",
               ("p", options.at("rpc-tls-endpoint").as<std::string>() )("c", options.at("rpc-tls-certificate").as<std::string>() ));
         _websocket_tls_server->listen( fc::ip::endpoint::from_string(options.at("rpc-tls-endpoint").as<std::string>()) );
         _websocket_tls_server->start_accept();
      }

      auto _http_server = options.count("rpc-http-endpoint" ) ?
               std::make_shared<fc::http::server>() :
               std::shared_ptr<fc::http::server>();

      if( _http_server )
      {
         ilog( "Listening for incoming HTTP RPC requests on ${p}", ("p", options.at("rpc-http-endpoint").as<std::string>() ) );
         _http_server->listen( fc::ip::endpoint::from_string( options.at( "rpc-http-endpoint" ).as<std::string>() ) );
         //
         // due to implementation, on_request() must come AFTER listen()
         //
         _http_server->on_request(
            [&]( const fc::http::request& req, const fc::http::server::response& resp )
            {
               std::shared_ptr< fc::rpc::http_api_connection > conn =
                  std::make_shared< fc::rpc::http_api_connection>();
               conn->register_api( wapi );
               conn->on_request( req, resp );
            } );
      }

      if( !options.count( "daemon" ) )
      {
         wallet_cli->register_api( wapi );
         if( options.count("from-command-file") )
         {
             wallet_cli->set_command_file(options.at( "from-command-file" ).as<std::string>());
         }
         wallet_cli->start();
         wallet_cli->wait();
      }
      else
      {
        fc::promise<int>::ptr exit_promise = new fc::promise<int>("UNIX Signal Handler");
        fc::set_signal_handler([&exit_promise](int signal) {
           exit_promise->set_value(signal);
        }, SIGINT);

        ilog( "Entering Daemon Mode, ^C to exit" );
        exit_promise->wait();
      }

      wapi->save_wallet_file(wallet_file.generic_string());
      locked_connection.disconnect();
      closed_connection.disconnect();
   }
   catch ( const fc::exception& e )
   {
      std::cout << e.to_detail_string() << "\n";
      return -1;
   }
   return 0;
}
