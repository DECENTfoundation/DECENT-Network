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

#include "delayed_node_plugin.hpp"
#include <graphene/account_history/account_history_plugin.hpp>
#include <graphene/transaction_history/transaction_history_plugin.hpp>

#include <fc/thread/thread.hpp>
#include <fc/interprocess/signals.hpp>
#include <fc/log/console_appender.hpp>
#include <fc/log/file_appender.hpp>
#include <fc/log/logger.hpp>
#include <fc/log/logger_config.hpp>

#include <boost/filesystem.hpp>

#include <decent/decent_config.hpp>
#include <decent/about.hpp>

#include <iostream>
#include <fstream>

#ifdef WIN32
# include <signal.h>
#else
# include <csignal>
#endif

using namespace graphene;
namespace bpo = boost::program_options;

int main(int argc, char** argv)
{
   try {
      bpo::options_description app_options("DECENT Delayed Node");
      bpo::options_description cfg_options("Configuration options");
      bpo::variables_map options;

      bpo::variables_map options;

      using decent_plugins = graphene::app::plugin_set<
         graphene::delayed_node::delayed_node_plugin,
         graphene::account_history::account_history_plugin,
         graphene::transaction_history::transaction_history_plugin
      >;

      try
      {
         graphene::app::application::set_program_options(app_options, cfg_options);
         decent_plugins::set_program_options(app_options, cfg_options);
         app_options.add_options()
   #if defined(_MSC_VER)
            ("install-win-service", "Register itself as Windows service")
            ("remove-win-service", "Unregister itself as Windows service")
   #else
            ("daemon", "Run DECENT as daemon")
   #endif
         ;

         bpo::parsed_options optparsed = bpo::command_line_parser(argc, argv).options(app_options).allow_unregistered().run();
         bpo::store(optparsed, options);
         if( decent::check_unrecognized(optparsed) )
         {
            return EXIT_FAILURE;
         }
      }
      catch (const boost::program_options::error& e)
      {
      std::cerr << "Error parsing command line: " << e.what() << "\n";
      return EXIT_FAILURE;
      }

      if( options.count("help") )
      {
         std::cout << app_options << std::endl;
         return EXIT_SUCCESS;
      }
      else if( options.count("version") )
      {
         decent::dump_version_info("DECENT Daemon");
         return EXIT_SUCCESS;
      }

      boost::filesystem::path data_dir;
      if( options.count("data-dir") )
      {
         data_dir = options["data-dir"].as<boost::filesystem::path>();
         if( data_dir.is_relative() )
            data_dir = boost::filesystem::current_path() / data_dir;
      }

      boost::filesystem::path config_ini_path = data_dir / "config.ini";
      // Create config file if not already present
      if( !exists(config_ini_path) )
      {
         ilog("Writing new config file at ${path}", ("path", config_ini_path));
         if( !exists(data_dir) )
            create_directories(data_dir);

         decent::write_default_config_file(config_ini_path, cfg_options, false);
      }
      else
      {
         // get the basic options
         try {
            boost::filesystem::ifstream cfg_stream(config_ini_path);
            bpo::store(bpo::parse_config_file<char>(cfg_stream, cfg_options, true), options);
         }
         catch (std::exception& e) {
            elog(e.what());
            return EXIT_FAILURE;
         }
         catch (...) {
            elog("unknown exception");
            return EXIT_FAILURE;
         }
      }

      // try to get logging options from the config file.
      try
      {
         fc::optional<fc::logging_config> logging_config = decent::load_logging_config_from_ini_file(config_ini_path, data_dir);
         if (logging_config) {
            if (!fc::configure_logging(*logging_config)) {
               std::cerr << "Error configure logging!\n";
               return 1;
            }
         }
      }
      catch (const fc::exception& e)
      {
         elog("Error parsing logging options from config file ${cfg}. str: ${str}", ("cfg", config_ini_path)("str", e.to_string()));
         return EXIT_FAILURE;
      }

      app::application node;
      decent_plugins::types plugins = decent_plugins::create(node);

      node.initialize(data_dir, options);
      node.initialize_plugins( options );

      node.startup();
      node.startup_plugins();

      fc::promise<int>::ptr exit_promise = new fc::promise<int>("UNIX Signal Handler");
      fc::set_signal_handler([&exit_promise](int signal) {
         exit_promise->set_value(signal);
      }, SIGINT);

      ilog("Started delayed node on a chain with ${h} blocks.", ("h", node.chain_database()->head_block_num()));
      ilog("Chain ID is ${id}", ("id", node.chain_database()->get_chain_id()) );

      int signal = exit_promise->wait();
      ilog("Exiting from signal ${n}", ("n", signal));
      node.shutdown_plugins();
      return 0;
   } catch( const fc::exception& e ) {
      elog("Exiting with error:\n${e}", ("e", e.to_detail_string()));
      return 1;
   }
}
