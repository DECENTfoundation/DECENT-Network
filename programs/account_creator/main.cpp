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
#include <graphene/wallet/wallet.hpp>

#include <graphene/utilities/dirhelper.hpp>

#include <fc/interprocess/signals.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

#include <fc/log/console_appender.hpp>
#include <fc/log/file_appender.hpp>
#include <fc/log/logger.hpp>
#include <fc/log/logger_config.hpp>
#include "json.hpp"

#include <curl/curl.h>
#include <cstdlib>

#ifdef WIN32
# include <signal.h>
#else
# include <csignal>
#endif

using namespace graphene::app;
using namespace graphene::chain;
using namespace graphene::utilities;
using namespace graphene::wallet;
using namespace nlohmann;

using namespace std;
namespace bpo = boost::program_options;

class main_exception : public std::exception
{
public:
   main_exception(std::string const& str_info) noexcept
   : m_str_info(str_info) {}
   virtual ~main_exception() {}
   
   char const* what() const noexcept override
   {
      return m_str_info.c_str();
   }
private:
   std::string m_str_info;
};

string account_id(account_object const& account)
{
   //   did not find a better way to get the user id
   auto id = dynamic_cast<object const&>(account).id;
   
   return std::string(id);
}

int main( int argc, char** argv )
{
   
   fc::path decent_home;
   try {
      decent_home = decent_path_finder::instance().get_decent_home();
   } catch (const std::exception& ex) {
      std::cerr << "Failed to initialize home directory." << std::endl;
      std::cerr << "Error: " << ex.what() << std::endl;
      return 1;
   } catch (const fc::exception& ex) {
      std::cerr << "Failed to initialize home directory." << std::endl;
      std::cerr << "Error: " << ex.what() << std::endl;
      return 1;
   }
   
   try {
      
      boost::program_options::options_description opts;
      opts.add_options()
      ("help,h", "Print this help message and exit.")
      ("username", bpo::value<string>(), "User name.")
      ("server-rpc-endpoint,s", bpo::value<string>()->implicit_value("ws://stage.decentgo.com:8090"), "Server websocket RPC endpoint")
      ("server-rpc-user,u", bpo::value<string>(), "Server Username")
      ("server-rpc-password,p", bpo::value<string>(), "Server Password")
      ("rpc-endpoint,r", bpo::value<string>()->implicit_value("127.0.0.1:8091"), "Endpoint for wallet websocket RPC to listen on")
      ("rpc-tls-endpoint,t", bpo::value<string>()->implicit_value("127.0.0.1:8092"), "Endpoint for wallet websocket TLS RPC to listen on")
      ("rpc-tls-certificate,c", bpo::value<string>()->implicit_value("server.pem"), "PEM certificate for wallet websocket TLS RPC")
      ("rpc-http-endpoint,H", bpo::value<string>()->implicit_value("127.0.0.1:8093"), "Endpoint for wallet HTTP RPC to listen on")
      ("daemon,d", "Run the wallet in daemon mode" )
      ("wallet-file,w", bpo::value<string>()->implicit_value("wallet.json"), "wallet to load")
      ("chain-id", bpo::value<string>(), "chain ID to connect to")
      ("testnet", bpo::value<size_t>(), "testnet version 1 or 2")
      ;
      
      bpo::variables_map options;
      
      bpo::store( bpo::parse_command_line(argc, argv, opts), options );
      
      string str_password = "somepasswordthatwillbeuserdforwallet";
      string str_registrar = "1.2.20";
      string str_referrer = "1.2.20";
      string str_username;
      bool bool_override = false;
      
      if (options.count("username")) {
         str_username = options.at("username").as<string>();
         bool_override = true;
      }
      
      if (options.count("help"))
      {
         std::cerr << opts << "\n";
         return 127;
      }
      
      
      if (false == bool_override)
      {
         cerr << "look up the usage in \"help\"\n";
         return 127;
      }
      
      fc::path data_dir;
      fc::logging_config cfg;
      fc::path log_dir = decent_path_finder::instance().get_decent_logs();
      
      fc::file_appender::config ac;
      ac.filename             = log_dir / "rpc" / "rpc.log";
      ac.flush                = true;
      ac.rotate               = true;
      ac.rotation_interval    = fc::hours( 1 );
      ac.rotation_limit       = fc::days( 1 );
      
      //std::cout << "Logging RPC to file: " << (decent_path_finder::instance().get_decent_data() / ac.filename).preferred_string() << "\n";
      
      cfg.appenders.push_back(fc::appender_config( "default", "console", fc::variant(fc::console_appender::config())));
      cfg.appenders.push_back(fc::appender_config( "rpc", "file", fc::variant(ac)));
      
      cfg.loggers = { fc::logger_config("default"), fc::logger_config( "rpc") };
      cfg.loggers.front().level = fc::log_level::off;
      cfg.loggers.front().appenders = {"default"};
      cfg.loggers.back().level = fc::log_level::off;
      cfg.loggers.back().appenders = {"rpc"};
      
      
      //
      // TODO:  We read wallet_data twice, once in main() to grab the
      //    socket info, again in wallet_api when we do
      //    load_wallet_file().  Seems like this could be better
      //    designed.
      //
      wallet_data wdata;
      
      fc::path wallet_file( options.count("wallet-file") ? options.at("wallet-file").as<string>() : decent_path_finder::instance().get_decent_home() / "wallet.json");
      
      if( fc::exists( wallet_file ) )
      {
         wdata = fc::json::from_file( wallet_file ).as<wallet_data>();
         if( options.count("chain-id") )
         {
            // the --chain-id on the CLI must match the chain ID embedded in the wallet file
            if( chain_id_type(options.at("chain-id").as<std::string>()) != wdata.chain_id )
            {
               std::cout << "Chain ID in wallet file does not match specified chain ID\n";
               return 1;
            }
         }
      }
      else
      {
         if( options.count("chain-id") )
         {
            wdata.chain_id = chain_id_type(options.at("chain-id").as<std::string>());
            std::cout << "Starting a new wallet with chain ID " << wdata.chain_id.str() << " (from CLI)\n";
         }
         else
         {
            wdata.chain_id = chain_id_type ("0000000000000000000000000000000000000000000000000000000000000000"); //graphene::egenesis::get_egenesis_chain_id();
            std::cout << "Starting a new wallet with chain ID " << wdata.chain_id.str() << " (empty one)\n";
         }
      }
      
      // but allow CLI to override
      if( options.count("server-rpc-endpoint") )
         wdata.ws_server = options.at("server-rpc-endpoint").as<std::string>();
      if( options.count("server-rpc-user") )
         wdata.ws_user = options.at("server-rpc-user").as<std::string>();
      if( options.count("server-rpc-password") )
         wdata.ws_password = options.at("server-rpc-password").as<std::string>();
      
      
      fc::http::websocket_client client;
      auto con  = client.connect( wdata.ws_server );
      auto apic = std::make_shared<fc::rpc::websocket_api_connection>(*con);
      
      auto remote_api = apic->get_remote_api< login_api >(1);
      // TODO:  Error message here
      FC_ASSERT( remote_api->login( wdata.ws_user, wdata.ws_password ) );
      
      auto wapiptr = std::make_shared<wallet_api>( wdata, remote_api );
      wapiptr->set_wallet_filename( wallet_file.generic_string() );
      wapiptr->load_wallet_file();
      
      try {
         if (bool_override)
         {
            if (wapiptr->is_new()) {
               wapiptr->set_password(str_password);
               wapiptr->unlock(str_password);   //  throws if the password is wrong
               
               wapiptr->import_key("decentdev-hayk", "5JRPoVy5ZUX4ZxDyec78Hb7GV1424XnoD3yDKEYTWMqS4JBMB9q");
               wapiptr->save_wallet_file();
            } else {
               wapiptr->unlock(str_password);   //  throws if the password is wrong
            }
            
            
            account_object account_referrer = wapiptr->get_account(str_referrer);
            account_object account_registrar = wapiptr->get_account(str_registrar);
            
            auto keys = wapiptr->suggest_brain_key();

            string brain_key = keys.brain_priv_key;
            
            wapiptr->create_account_with_brain_key(brain_key, str_username, str_registrar, str_referrer, true);
            
            account_object new_account = wapiptr->get_account( str_username );
            
            json result;
            

            result["brain_key"] = brain_key;
            result["username"] = str_username;
            result["public_key"] = (std::string)keys.pub_key;
            std::cout << result << std::endl;
            
         }
      }
      catch ( const fc::exception& e )
      {
         std::cerr << e.to_detail_string() << "\n";
         return 1;
      }
      catch (std::exception const& e)
      {
         std::cerr << e.what() << endl;
         return 1;
      }
      
      
   }
   catch ( const fc::exception& e )
   {
      std::cerr << e.to_detail_string() << "\n";
      return 1;
   }
   catch (std::exception const& e)
   {
      std::cerr << e.what() << endl;
      return 1;
   }
   
   
   return 0;
}
