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

#include <graphene/miner/miner.hpp>
#include <graphene/seeding/seeding.hpp>
#include <graphene/account_history/account_history_plugin.hpp>
#include <graphene/transaction_history/transaction_history_plugin.hpp>
#include <graphene/messaging/messaging.hpp>
#include <graphene/utilities/dirhelper.hpp>
#include <graphene/utilities/git_revision.hpp>

#include <fc/exception/exception.hpp>
#include <fc/thread/thread.hpp>
#include <fc/interprocess/signals.hpp>
#include <fc/log/console_appender.hpp>
#include <fc/log/file_appender.hpp>
#include <fc/log/logger.hpp>
#include <fc/log/logger_config.hpp>
#include <fc/monitoring.hpp>

#include <boost/filesystem.hpp>
#include <boost/version.hpp>
#include <openssl/opensslv.h>
#include <cryptopp/config.h>

#include <decent/decent_config.hpp>

#include <iostream>
#include <fstream>

#ifdef _MSC_VER
#include <signal.h> 
#include <windows.h>
#include "winsvc.hpp"
#else
#include <csignal>
#include <sys/stat.h>
#endif

using namespace graphene;
namespace bpo = boost::program_options;

         
int main_internal(int argc, char** argv);

#ifdef _MSC_VER
// Stopping from GUI and stopping of win service
static fc::promise<int>::ptr s_exit_promise;

BOOL s_bStop = FALSE;
HANDLE s_hStopProcess = NULL;

SERVICE_TABLE_ENTRY DispatchTable[] = {
		{ (LPSTR)SVCNAME, (LPSERVICE_MAIN_FUNCTION)main_internal },
		{ NULL, NULL }
};

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
#endif

#if defined(__linux__) || defined(__APPLE__)

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

int main_internal(int argc, char** argv) {
#ifdef _MSC_VER
   DWORD err = 0;
	bool is_win_service = IsRunningAsSystemService();
	if(is_win_service) {
      err = InitializeService();
      if(err)
			return err;
	}
#endif

   bpo::options_description app_options("DECENT Daemon");
   bpo::options_description cfg_options("DECENT Daemon");
   bpo::variables_map options;

   using decent_plugins = graphene::app::plugin_set<
      miner_plugin::miner_plugin,
      account_history::account_history_plugin,
      decent::seeding::seeding_plugin,
      decent::messaging::messaging_plugin,
      transaction_history::transaction_history_plugin
   >;

   try
   {
      bpo::options_description cli, cfg;
      app::application::set_program_options(cli, cfg);
      decent_plugins::set_program_options(cli, cfg);
      cli.add_options()
#ifndef _MSC_VER 
         ("daemon", "Run DECENT as daemon.")
#else
         ("install-win-service", "It registers application like system service and sets other command line parameters as service startup parameters. It can be used also for re-registering and changing service startup parameters.")
         ("remove-win-service", "It unregisters application if previously registered as Windows service.")
#endif
      ;

      app_options.add(cli);
      cfg_options.add(cfg);
      bpo::store(bpo::parse_command_line(argc, argv, app_options), options);
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
      unsigned int boost_major_version = BOOST_VERSION / 100000;
      unsigned int boost_minor_version = BOOST_VERSION / 100 - boost_major_version * 1000;
      std::string boost_version_text = std::to_string(boost_major_version) + "." + std::to_string(boost_minor_version) + "." + std::to_string(BOOST_VERSION % 100);
      std::string openssl_version_text = std::string(OPENSSL_VERSION_TEXT);
      openssl_version_text = openssl_version_text.substr(0, openssl_version_text.length() - 11);
      unsigned int cryptopp_major_version = CRYPTOPP_VERSION / 100;
      unsigned int cryptopp_minor_version = CRYPTOPP_VERSION / 10 - cryptopp_major_version * 10;
      std::string cryptopp_version_text = std::to_string(cryptopp_major_version) + "." + std::to_string(cryptopp_minor_version) + "." + std::to_string(CRYPTOPP_VERSION % 10);

      std::cout << "DECENT Daemon " << graphene::utilities::git_version();
#ifndef NDEBUG
      std::cout << " (debug)";
#endif /* NDEBUG */
      std::cout << "\nBoost " << boost_version_text << "\n" << openssl_version_text << "\nCryptopp " << cryptopp_version_text << std::endl;
      return EXIT_SUCCESS;
   }

   bool run_as_daemon = false;
#ifndef _MSC_VER
   run_as_daemon = options.count("daemon");
#else
   bool install_winsvc = options.count("install-win-service");

   if(is_win_service) {
      run_as_daemon = true;
   } else if(install_winsvc) { // install like service and start it
	   std::string cmd_line_str;
	   for(int i = 1; i < argc; i++) {
		   cmd_line_str += " ";
		   cmd_line_str += argv[i];
	   }
	   err = install_win_service(cmd_line_str.c_str());
	   return err;
	} else if(options.count("remove-win-service")) {
         err = remove_win_service();
         return err;
	}
#endif
   app::application* node = new app::application();
   fc::oexception unhandled_exception;
   try {
      decent_plugins::types plugins = decent_plugins::create(*node);

      fc::path logs_dir, data_dir, config_filename;
      auto& path_finder = utilities::decent_path_finder::instance();
	  
      if( run_as_daemon ) {
#ifdef _MSC_VER
		  char service_data_dir[256] = "";
		  GetAppDataDir(service_data_dir, sizeof(service_data_dir) - 1);
		  data_dir = service_data_dir;
		  data_dir = data_dir / "decentd";
		  logs_dir = data_dir / "logs";
		  config_filename = data_dir / "config.ini";
        path_finder.set_decent_temp_path(data_dir / "tmp");
#else
         int ret = start_as_daemon();

         if (ret < 0) {
            std::cerr << "Error running as daemon.\n";
            return 1;
         }
         else if (ret == 1) {
            return 0;
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
               data_dir = fc::current_path() / data_dir;
         }
         else
         {
            data_dir = path_finder.get_decent_data();
         }

         config_filename = data_dir / "config.ini";
         logs_dir = data_dir;
      }

      if( fc::exists(config_filename) )
      {
         // get the basic options
         try {
            bpo::store(bpo::parse_config_file<char>(config_filename.preferred_string().c_str(), cfg_options, true), options);
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
		 if (!fc::exists(data_dir)) {
			 std::string data_dir_str = data_dir.string();
			 fc::create_directories(data_dir);
		 }

         if( !run_as_daemon )
            logs_dir /= "logs";

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
         elog("Error parsing logging options from config file ${cfg}. str: ${str}", ("cfg", config_filename.preferred_string())("str", e.to_string()));
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
#ifdef _MSC_VER
      s_exit_promise = exit_promise;
      s_hStopProcess = OpenEventA(SYNCHRONIZE, FALSE, "A883EF36-8168-4E45-A08F-97EFFC5B6694");
      if (s_hStopProcess)
      {
         DWORD tid = 0;
         CreateThread(NULL, 0, StopFromGUIThreadProc, NULL, 0, &tid);
      }
	  if(is_win_service)
		  ReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);
#else
      fc::set_signal_handler([&exit_promise](int signal) {
         elog( "Caught SIGINT attempting to exit cleanly" );
         exit_promise->set_value(signal);
      }, SIGINT);

      fc::set_signal_handler([&exit_promise](int signal) {
         elog( "Caught SIGTERM attempting to exit cleanly" );
         exit_promise->set_value(signal);
      }, SIGTERM);

      fc::set_signal_handler([&exit_promise](int signal) {
           elog( "Caught SIGHUP attempting to exit cleanly" );
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
#ifdef _MSC_VER
	  if(is_win_service)
		ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
      s_bStop = TRUE;
#endif
      return 0;
   } catch( const fc::exception& e ) {
      // deleting the node can yield, so do this outside the exception handler
      unhandled_exception = e;
   }
#ifdef _MSC_VER
   s_bStop = TRUE;
#endif
   if (unhandled_exception)
   {
      elog("Exiting with error:\n${e}", ("e", unhandled_exception->to_detail_string()));
      monitoring::monitoring_counters_base::stop_monitoring_thread();
      node->shutdown();
      delete node;
      return 1;
   }
   return 0;
}

int main(int argc, char** argv)
{
	int result = 0;
#ifdef _MSC_VER
	DWORD err = 0;
	bool is_win_service = IsRunningAsSystemService();
	if(is_win_service == false) {
		SetConsoleCtrlHandler(HandlerRoutine, TRUE);
		result = main_internal(argc, argv);
	} else {
		
		if (!StartServiceCtrlDispatcher(DispatchTable)) {
         err = GetLastError();
			SvcReportEvent((LPTSTR)"StartServiceCtrlDispatcher");
         return err;
		} 
      return 0;
	}
#else
	result = main_internal(argc, argv);
#endif
	return result;
}
