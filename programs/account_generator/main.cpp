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
#include <graphene/package/package.hpp>
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

static size_t WriteMemoryCallback(void* contents, size_t size, size_t nmemb, void* pcollect)
{
    size_t i_new_chunk_size = size * nmemb;
    vector<char>& arr_collect_response = *static_cast< vector<char>* >(pcollect);
    char* sz_new_chunk = static_cast<char*>(contents);
    
    for (size_t iIndex = 0; iIndex < i_new_chunk_size; ++iIndex)
        arr_collect_response.push_back(sz_new_chunk[iIndex]);
    
    return i_new_chunk_size;
}

string curl_escape(string const& str)
{
    CURL* curl_handle = curl_easy_init();
    char* sz_str = curl_easy_escape(curl_handle, str.c_str(), str.length());
    
    string str_res(sz_str);
    
    curl_free(static_cast<void*>(sz_str));
    curl_easy_cleanup(curl_handle);
    
    return str_res;
}

void curl_test_func(string const& str_url,
                    string const& str_post,
                    string& str_response)
{
    CURL* curl_handle = nullptr;
    CURLcode res;
    
    vector<char> arr_response;
    
    curl_handle = curl_easy_init();
    
    curl_easy_setopt(curl_handle, CURLOPT_URL, str_url.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, static_cast<void*>(&arr_response));
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    if (false == str_post.empty())
        curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, str_post.c_str());
    
    res = curl_easy_perform(curl_handle);
    
    if (res != CURLE_OK)
    {
        throw main_exception("curl_easy_perform() failed: " +
                             string(curl_easy_strerror(res)) +
                             "\n");
    }
    
    str_response = string(arr_response.begin(), arr_response.end());
    
    curl_easy_cleanup(curl_handle);
}

string account_id(account_object const& account)
{
    //   did not find a better way to get the user id
    auto id = dynamic_cast<object const&>(account).id;
    
    return std::string(id);
}

int main( int argc, char** argv )
{
    curl_global_init(CURL_GLOBAL_ALL);
    
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

      boost::program_options::options_description opts;
         opts.add_options()
       ("help,h", "Print this help message and exit.")
       ("password", bpo::value<string>(), "The password to unlock the wallet.")
       ("registrar", bpo::value<string>(), "The registrar account.")
       ("referrer", bpo::value<string>(), "The referrer account.")
       ("transfer-amount", bpo::value<double>(), "The amount to transfer to accounts.")
       ("server-rpc-endpoint,s", bpo::value<string>()->implicit_value("ws://127.0.0.1:8090"), "Server websocket RPC endpoint")
       ("server-rpc-user,u", bpo::value<string>(), "Server Username")
       ("server-rpc-password,p", bpo::value<string>(), "Server Password")
       ("rpc-endpoint,r", bpo::value<string>()->implicit_value("127.0.0.1:8091"), "Endpoint for wallet websocket RPC to listen on")
       ("rpc-tls-endpoint,t", bpo::value<string>()->implicit_value("127.0.0.1:8092"), "Endpoint for wallet websocket TLS RPC to listen on")
       ("rpc-tls-certificate,c", bpo::value<string>()->implicit_value("server.pem"), "PEM certificate for wallet websocket TLS RPC")
       ("rpc-http-endpoint,H", bpo::value<string>()->implicit_value("127.0.0.1:8093"), "Endpoint for wallet HTTP RPC to listen on")
       ("daemon,d", "Run the wallet in daemon mode" )
       ("wallet-file,w", bpo::value<string>()->implicit_value("wallet.json"), "wallet to load")
       ("chain-id", bpo::value<string>(), "chain ID to connect to")
       ("skip", bpo::value<size_t>(), "skip accounts")
       ;

      bpo::variables_map options;

      bpo::store( bpo::parse_command_line(argc, argv, opts), options );
       
       string str_password;
       string str_registrar;
       string str_referrer;
       bool bool_override = false;
       double transfer_amount = 0;
       if (options.count("password") &&
           options.count("registrar") &&
           options.count("referrer") &&
           options.count("transfer-amount"))
       {
           str_password = options.at("password").as<string>();
           str_registrar = options.at("registrar").as<string>();
           str_referrer = options.at("referrer").as<string>();
           transfer_amount = options.at("transfer-amount").as<double>();
           bool_override = true;
       }
       
      if (options.count("help"))
      {
         std::cout << opts << "\n";
         return 0;
      }


       if (false == bool_override)
       {
           cout << "look up the usage in \"help\"\n";
           return 0;
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

      std::cout << "Logging RPC to file: " << (decent_path_finder::instance().get_decent_data() / ac.filename).preferred_string() << "\n";

      cfg.appenders.push_back(fc::appender_config( "default", "console", fc::variant(fc::console_appender::config())));
      cfg.appenders.push_back(fc::appender_config( "rpc", "file", fc::variant(ac)));

      cfg.loggers = { fc::logger_config("default"), fc::logger_config( "rpc") };
      cfg.loggers.front().level = fc::log_level::info;
      cfg.loggers.front().appenders = {"default"};
      cfg.loggers.back().level = fc::log_level::debug;
      cfg.loggers.back().appenders = {"rpc"};

      //fc::configure_logging( cfg );

      fc::ecc::private_key committee_private_key = fc::ecc::private_key::regenerate(fc::sha256::hash(string("null_key")));

      idump( (key_to_wif( committee_private_key ) ) );

      fc::ecc::private_key nathan_private_key = fc::ecc::private_key::regenerate(fc::sha256::hash(string("nathan")));
      public_key_type nathan_pub_key = nathan_private_key.get_public_key();
      idump( (nathan_pub_key) );
      idump( (key_to_wif( nathan_private_key ) ) );

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

//      package_manager::instance().set_packages_path(wdata.packages_path);
      //package_manager::instance().set_libtorrent_config(wdata.libtorrent_config_path);

      fc::http::websocket_client client;
      idump((wdata.ws_server));
      auto con  = client.connect( wdata.ws_server );
      auto apic = std::make_shared<fc::rpc::websocket_api_connection>(*con);

      auto remote_api = apic->get_remote_api< login_api >(1);
      edump((wdata.ws_user)(wdata.ws_password) );
      // TODO:  Error message here
      FC_ASSERT( remote_api->login( wdata.ws_user, wdata.ws_password ) );

      auto wapiptr = std::make_shared<wallet_api>( wdata, remote_api );
      wapiptr->set_wallet_filename( wallet_file.generic_string() );
      wapiptr->load_wallet_file();
       
       if (bool_override)
       {
           if (wapiptr->is_new())
               wapiptr->set_password(str_password);
           
           wapiptr->unlock(str_password);   //  throws if the password is wrong
           
           account_object account_referrer = wapiptr->get_account(str_referrer);
           account_object account_registrar = wapiptr->get_account(str_registrar);
           
           size_t i_users_get = 100;
           size_t i_users_got = 0;
           
           if( options.count("skip") )
           {
               i_users_got = options.at("skip").as<size_t>();
           }

           
           bool b_all_accounts_have_right_amount = true;
           
           size_t i_users_already_exist = 0, i_responses = 0;
           
           while (true)
           {
               string str_response;
               curl_test_func("https://api.decent.ch/v1.0/subscribers/" +
                              std::to_string(i_users_got) +
                              "/" +
                              std::to_string(i_users_get) +
                              "?_format=json", string(), str_response);
               
               auto json_arr_user = nlohmann::json::parse(str_response);
               size_t i_user_count = json_arr_user.size();
               
               for (size_t i_index = 0; i_index < i_user_count; ++i_index)
               {
                   auto json_user_info = json_arr_user[i_index];
                   auto& json_bki = json_user_info["brainPrivKey"];
                   auto& json_priv_key = json_user_info["wifPrivKey"];
                   auto& json_pub_key = json_user_info["pubKey"];
                   auto& json_id = json_user_info["id"];
                   string str_bki;
                   if (false == json_bki.empty())
                       str_bki = json_bki.get<string>();
                   string str_user_id = std::to_string(json_id.get<size_t>());
                   int i_length = str_user_id.length();
                   int i_zero_count = 4 - i_length;
                   for (int i = 0; i < i_zero_count; ++i)
                   {
                       str_user_id = "0" + str_user_id;
                   }
                   
                   string str_wif_priv_key, str_pub_key;
                   if (false == json_priv_key.empty())
                       str_wif_priv_key = json_priv_key.get<string>();
                   if (false == json_pub_key.empty())
                       str_pub_key = json_pub_key.get<string>();
                   
                   bool b_post_back = false;
                   
                   if (str_bki.empty())
                   {
                       brain_key_info bki = wapiptr->suggest_brain_key();
                       str_bki = bki.brain_priv_key;
                       str_wif_priv_key = bki.wif_priv_key;
                       str_pub_key = string(bki.pub_key);
                       
                       b_post_back = true;
                   }
                   
                   string str_new_account_name = "decentuser" + str_user_id;
                   account_object account_newly_created;
                   
                   bool b_user_already_exists = false;
                   
                   try
                   {
                       account_newly_created = wapiptr->get_account(str_new_account_name);
                       ++i_users_already_exist;
                       cout << std::to_string(i_users_already_exist) << " : " << str_new_account_name << " user already exists\n";
                       b_user_already_exists = true;
                   }
                   catch (...)
                   {
                   }
                   
                   int64_t decent_amount = 0;
                   if (false == b_user_already_exists)
                   {
                       wapiptr->create_account_with_brain_key(str_bki,
                                                              str_new_account_name,
                                                              account_id(account_referrer),
                                                              account_id(account_registrar),
                                                              true);
                   
                       account_newly_created = wapiptr->get_account(str_new_account_name);
                       wapiptr->import_key(str_new_account_name, str_wif_priv_key);
                   }
                   else
                   {
                       bool b_imported = wapiptr->import_key(str_new_account_name, str_wif_priv_key);
                       if (false == b_imported)
                           throw main_exception("already existing user " + str_new_account_name + " does not match the private key stored in db");
                       
                       vector<asset> arr_balances = wapiptr->list_account_balances(str_new_account_name);
                       for (asset const& it : arr_balances)
                       {
                           int64_t decents = it.amount.value;
                           decent_amount += decents;
                       }
                       
                       if (arr_balances.size() > 1)
                           throw main_exception("already existing user " + str_new_account_name + " has many assets");
                       
                       decent_amount /= 100000000;
                       
                       if (decent_amount != transfer_amount)
                           b_all_accounts_have_right_amount = false;
                   }
                   
                   if (b_post_back &&
                       false == b_user_already_exists)
                   {
                       curl_test_func("https://api.decent.ch/v1.0/subscribers/" +
                                      str_user_id,
                                      string("_format=json") +
                                      "&appbundle_subscriber[brainPrivKey]=" + curl_escape(str_bki) +
                                      "&appbundle_subscriber[wifPrivKey]=" + curl_escape(str_wif_priv_key) +
                                      "&appbundle_subscriber[pubKey]=" + curl_escape(str_pub_key),
                                      str_response);
                       
                       if (false == str_response.empty())
                       {
                           ++i_responses;
                           cout << std::to_string(i_responses) << " : " << str_response << "\n";
                       }
                   }
                   
                   if (false == b_user_already_exists ||
                       0 == decent_amount)
                   {
                       wapiptr->transfer(account_id(account_registrar),
                                         account_id(account_newly_created),
                                         std::to_string(transfer_amount),
                                         "DCT",
                                         "",
                                         true); //  throws if not enough funds
                   }
               }
               
               i_users_got += i_user_count;
               
               if (i_user_count < i_users_get)
                   break;
           }

           if (b_all_accounts_have_right_amount)
               cout << "all accounts have the right amount\n";
           cout << endl;
           cout << "DONE!!!\n";
       }
#if 0
       else
       {
       
           //   don't care for what was in cli_wallet
           //   what's below - does not run

      fc::api<wallet_api> wapi(wapiptr);

      auto wallet_cli = std::make_shared<fc::rpc::cli>();
      for( auto& name_formatter : wapiptr->get_result_formatters() )
         wallet_cli->format_result( name_formatter.first, name_formatter.second );

      boost::signals2::scoped_connection closed_connection(con->closed.connect([=]{
         cerr << "Server has disconnected us.\n";
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

      auto _websocket_server = std::make_shared<fc::http::websocket_server>();
      if( options.count("rpc-endpoint") )
      {
         _websocket_server->on_connection([&]( const fc::http::websocket_connection_ptr& c ){
            std::cout << "here... \n";
            wlog("." );
            auto wsc = std::make_shared<fc::rpc::websocket_api_connection>(*c);
            wsc->register_api(wapi);
            c->set_session_data( wsc );
         });
         ilog( "Listening for incoming RPC requests on ${p}", ("p", options.at("rpc-endpoint").as<string>() ));
         _websocket_server->listen( fc::ip::endpoint::from_string(options.at("rpc-endpoint").as<string>()) );
         _websocket_server->start_accept();
      }

      string cert_pem = "server.pem";
      if( options.count( "rpc-tls-certificate" ) )
         cert_pem = options.at("rpc-tls-certificate").as<string>();

      auto _websocket_tls_server = std::make_shared<fc::http::websocket_tls_server>(cert_pem);
      if( options.count("rpc-tls-endpoint") )
      {
         _websocket_tls_server->on_connection([&]( const fc::http::websocket_connection_ptr& c ){
            auto wsc = std::make_shared<fc::rpc::websocket_api_connection>(*c);
            wsc->register_api(wapi);
            c->set_session_data( wsc );
         });
         ilog( "Listening for incoming TLS RPC requests on ${p}", ("p", options.at("rpc-tls-endpoint").as<string>() ));
         _websocket_tls_server->listen( fc::ip::endpoint::from_string(options.at("rpc-tls-endpoint").as<string>()) );
         _websocket_tls_server->start_accept();
      }

      auto _http_server = std::make_shared<fc::http::server>();
      if( options.count("rpc-http-endpoint" ) )
      {
         ilog( "Listening for incoming HTTP RPC requests on ${p}", ("p", options.at("rpc-http-endpoint").as<string>() ) );
         _http_server->listen( fc::ip::endpoint::from_string( options.at( "rpc-http-endpoint" ).as<string>() ) );
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
#endif// 0
   }
   catch ( const fc::exception& e )
   {
      std::cout << e.to_detail_string() << "\n";
      return -1;
   }
    catch (std::exception const& e)
    {
        std::cout << e.what() << endl;
    }
    
    curl_global_cleanup();
    
    return 0;
}
