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
#include <graphene/app/application.hpp>
#include <graphene/chain/protocol/fee_schedule.hpp>
#include <graphene/miner/miner.hpp>
#include <graphene/seeding/seeding.hpp>
#include <graphene/elasticsearch/elasticsearch_plugin.hpp>
#include <graphene/account_history/account_history_plugin.hpp>
#include <graphene/transaction_history/transaction_history_plugin.hpp>
#include <graphene/utilities/dirhelper.hpp>
#include <graphene/utilities/git_revision.hpp>

#include <fc/thread/thread.hpp>
#include <fc/interprocess/signals.hpp>
#include <fc/log/logger.hpp>
#include <fc/log/logger_config.hpp>
#include <fc/monitoring.hpp>

#include <boost/filesystem.hpp>

#include <decent/decent_config.hpp>
#include <decent/about.hpp>

#include <iostream>

#ifdef _MSC_VER
#include <signal.h>
#include <windows.h>
#include "winsvc.hpp"
#else
#include <csignal>
#include <sys/stat.h>
#endif

namespace bpo = boost::program_options;

#if defined(_MSC_VER)

// Stopping from GUI and stopping of win service
static fc::promise<int>::ptr s_exit_promise;

BOOL s_bStop = FALSE;
HANDLE s_hStopProcess = NULL;

void StopWinService()
{
   s_exit_promise->set_value(SIGTERM);
}

DWORD WINAPI StopFromGUIThreadProc(void* params)
{
   while (s_bStop == FALSE)
   {
      if (WaitForSingleObject(s_hStopProcess, 0) == 0)
         s_bStop = TRUE;
      Sleep(50);
   }
   CloseHandle(s_hStopProcess);
   elog("Caught stop from GUI attempting to exit cleanly");
   s_exit_promise->set_value(SIGTERM);
   return 0;
}
BOOL WINAPI HandlerRoutine(_In_ DWORD dwCtrlType) {
   switch (dwCtrlType)
   {
   case CTRL_C_EVENT:
      elog("Caught stop by Ctrl+C to exit cleanly");
      s_exit_promise->set_value(SIGTERM);
      return TRUE;
   case CTRL_BREAK_EVENT:
      elog("Caught stop by Ctrl+break to exit cleanly");
      s_exit_promise->set_value(SIGTERM);
      return TRUE;
   case CTRL_CLOSE_EVENT:
      elog("Caught stop by closing console window to exit cleanly");
      s_exit_promise->set_value(SIGTERM);
      return TRUE;
   case CTRL_LOGOFF_EVENT:
      elog("Caught stop by logoff event to exit cleanly");
      s_exit_promise->set_value(SIGTERM);
      return TRUE;
   case CTRL_SHUTDOWN_EVENT:
      elog("Caught stop by shutdown event to exit cleanly");
      s_exit_promise->set_value(SIGTERM);
      return TRUE;
   default:
      return FALSE;
   }
}

#elif defined(__linux__) || defined(__APPLE__)

int start_as_daemon()
{
   pid_t pid, sid;

   /* Fork off the parent process */
   pid = fork();
   if (pid < 0) {
       return -1;
   }
   /* If we got a good PID, then
     we can exit the parent process. */
   if (pid > 0) {
       return 1;
   }

   /* Change the file mode mask */
   umask(0);

   /* Open any logs here */

   /* Create a new SID for the child process */
   sid = setsid();
   if (sid < 0) {
       return -1;
   }

   /* Change the current working directory */
   if ((chdir("/")) < 0) {
      return -1;
   }

   /* Close out the standard file descriptors */
   close(STDIN_FILENO);
   close(STDOUT_FILENO);
   close(STDERR_FILENO);

   return 0;
}
#endif

int main_internal(int argc, char** argv, bool run_as_daemon = false)
{
   bpo::options_description app_options("DECENT Daemon");
   bpo::options_description cfg_options("Configuration options");
   bpo::variables_map options;

   using decent_plugins = graphene::app::plugin_set<
      graphene::miner_plugin::miner_plugin,
      graphene::account_history::account_history_plugin,
      decent::seeding::seeding_plugin,
      decent::elasticsearch::elasticsearch_plugin,
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
         ("daemon", "Run DECENT as daemon.")
#endif
      ;

      bpo::parsed_options optparsed = bpo::command_line_parser(argc, argv).options(app_options).allow_unregistered().run();
      bpo::store(optparsed, options);
      if (decent::check_unrecognized(optparsed))
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
      std::string boost_version_text = decent::get_boost_version();
      std::string openssl_version_text = decent::get_openssl_version();
      std::string cryptopp_version_text = decent::get_cryptopp_version();

      std::cout << "DECENT Daemon " << graphene::utilities::git_version();
#ifndef NDEBUG
      std::cout << " (debug)";
#endif /* NDEBUG */
      std::cout << "\nBoost " << boost_version_text << "\n" << openssl_version_text << "\nCryptopp " << cryptopp_version_text << std::endl;
      return EXIT_SUCCESS;
   }

#if defined(_MSC_VER)
   if( options.count("install-win-service") )
   {
	   return install_win_service();
	}
   else if( options.count("remove-win-service") )
   {
      return remove_win_service();
	}
#else
   run_as_daemon = options.count("daemon");
#endif

   graphene::app::application* node = new graphene::app::application();
   fc::optional<fc::exception> unhandled_exception;
   try {
      decent_plugins::types plugins = decent_plugins::create(*node);

      boost::filesystem::path logs_dir, data_dir, config_filename;
      auto& path_finder = graphene::utilities::decent_path_finder::instance();

      if( run_as_daemon ) {
#if defined(_MSC_VER)
         data_dir = GetAppDataDir();
         data_dir = data_dir / "decentd";
         logs_dir = data_dir / "logs";
         config_filename = data_dir / "config.ini";
         path_finder.set_decent_temp_path(data_dir / "tmp");
#else
         int ret = start_as_daemon();

         if (ret < 0) {
            std::cerr << "Error running as daemon.\n";
            return EXIT_FAILURE;
         }
         else if (ret == 1) {
            return EXIT_SUCCESS;
         }

         //default path settings for daemon
         config_filename = "/etc/decentd";
         logs_dir = "/var/log/decentd/";
         data_dir = "/var/lib/decentd/";
         path_finder.set_decent_temp_path("/var/tmp/decentd/");
#endif
         path_finder.set_decent_data_path(data_dir);
      }
      else {
         if( options.count("data-dir") )
         {
            data_dir = options["data-dir"].as<boost::filesystem::path>();
            if( data_dir.is_relative() )
               data_dir = boost::filesystem::current_path() / data_dir;
         }
         else
         {
            data_dir = path_finder.get_decent_data();
         }

         config_filename = data_dir / "config.ini";
         logs_dir = data_dir;
      }

      if( exists(config_filename) )
      {
         // get the basic options
         try {
            boost::filesystem::ifstream cfg_stream(config_filename);
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
      else //NOTE: We should not write a config when we run as daemon, but for now we leave it as is.
      {
         ilog("Writing new config file at ${path}", ("path", config_filename));
         if( !exists(data_dir) ) {
            create_directories(data_dir);
         }

         decent::write_default_config_file(config_filename, cfg_options, run_as_daemon);
      }

      // try to get logging options from the config file.
      try
      {
         fc::optional<fc::logging_config> logging_config = decent::load_logging_config_from_ini_file(config_filename, logs_dir);
         if (logging_config) {
            if (!fc::configure_logging(*logging_config)) {
               std::cerr << "Error configure logging!\n";
               return 1;
            }
         }
      }
      catch (const fc::exception& e)
      {
         elog("Error parsing logging options from config file ${cfg}. str: ${str}", ("cfg", config_filename)("str", e.to_string()));
         return EXIT_FAILURE;
      }

      monitoring::set_data_dir(data_dir);
      monitoring::monitoring_counters_base::start_monitoring_thread();

      bpo::notify(options);
      node->initialize(data_dir, options);
      node->initialize_plugins( options );

      node->startup();
      node->startup_plugins();

      fc::promise<int>::ptr exit_promise = new fc::promise<int>("UNIX Signal Handler");
#if defined(_MSC_VER)
      s_exit_promise = exit_promise;
      s_hStopProcess = OpenEventA(SYNCHRONIZE, FALSE, "A883EF36-8168-4E45-A08F-97EFFC5B6694");
      if (s_hStopProcess)
      {
         DWORD tid = 0;
         CreateThread(NULL, 0, StopFromGUIThreadProc, NULL, 0, &tid);
      }

      if (run_as_daemon)
      {
         ReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);
      }
#else
      fc::set_signal_handler([&exit_promise](int signal) {
         dlog( "Caught SIGINT attempting to exit cleanly" );
         exit_promise->set_value(signal);
      }, SIGINT);

      fc::set_signal_handler([&exit_promise](int signal) {
         dlog( "Caught SIGTERM attempting to exit cleanly" );
         exit_promise->set_value(signal);
      }, SIGTERM);

      fc::set_signal_handler([&exit_promise](int signal) {
         dlog( "Caught SIGHUP attempting to exit cleanly" );
         exit_promise->set_value(signal);
      }, SIGHUP);
#endif
      ilog("Started miner node on a chain with ${h} blocks.", ("h", node->chain_database()->head_block_num()));
      ilog("Chain ID is ${id}", ("id", node->chain_database()->get_chain_id()) );

      int signal = exit_promise->wait();
      ilog("Exiting from signal ${n}", ("n", signal));
      node->shutdown_plugins();
      monitoring::monitoring_counters_base::stop_monitoring_thread();
      node->shutdown();
      delete node;
#if defined(_MSC_VER)
      if (run_as_daemon)
      {
         ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
      }

      s_bStop = TRUE;
#endif
      return EXIT_SUCCESS;
   } catch( const fc::exception& e ) {
      // deleting the node can yield, so do this outside the exception handler
      unhandled_exception = e;
   } catch( const std::exception& e ) {
      unhandled_exception = fc::exception(fc::std_exception_code, typeid(e).name(), e.what());
   } catch( ... ) {
      unhandled_exception = fc::unhandled_exception(FC_LOG_MESSAGE(error, "unknown"));
   }
#if defined(_MSC_VER)
   s_bStop = TRUE;
#endif
   if (unhandled_exception)
   {
      elog("Exiting with error:\n${e}", ("e", unhandled_exception->to_detail_string()));
      monitoring::monitoring_counters_base::stop_monitoring_thread();
      node->shutdown();
      delete node;
      return EXIT_FAILURE;
   }
   return EXIT_SUCCESS;
}

#if defined(_MSC_VER)
void service_main(int argc, char** argv)
{
   bool is_win_service = IsRunningAsSystemService();
	if(!is_win_service || InitializeService() == 0)
      main_internal(argc, argv, is_win_service);
}

SERVICE_TABLE_ENTRY DispatchTable[] = {
		{ (LPSTR)SVCNAME, (LPSERVICE_MAIN_FUNCTION)service_main },
		{ NULL, NULL }
};
#endif

int main(int argc, char** argv)
{
#if defined(_MSC_VER)
	if(IsRunningAsSystemService()) {
		if(!StartServiceCtrlDispatcher(DispatchTable)) {
         int err = GetLastError();
			SvcReportEvent((LPTSTR)"StartServiceCtrlDispatcher");
         return err;
		}
      return EXIT_SUCCESS;
   }
   else {
		SetConsoleCtrlHandler(HandlerRoutine, TRUE);
	}
#endif

	return main_internal(argc, argv);
}
