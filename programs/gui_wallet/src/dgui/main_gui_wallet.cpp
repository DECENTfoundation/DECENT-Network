/*
 *	File: main_gui_wallet.cpp
 *
 *	Created on: Nov 11, 2016
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
#include "stdafx.h"

#ifndef _MSC_VER
#include <QApplication>
#include <QProcess>
#include <QMessageBox>
#include <QDir>
#include <fc/interprocess/signals.hpp>
#include <fc/thread/thread.hpp>
#endif

#include "gui_wallet_mainwindow.hpp"

#ifndef _MSC_VER

#if NDEBUG
//#define SET_LIBRARY_PATHS
#endif


// used both by main() and runDecentD()
#include <string>
using string = std::string;

// used only by runDecentD()

#include <graphene/witness/witness.hpp>
#include <graphene/seeding/seeding.hpp>
#include <graphene/account_history/account_history_plugin.hpp>
#include <graphene/market_history/market_history_plugin.hpp>
#include <graphene/utilities/dirhelper.hpp>

#include <fc/exception/exception.hpp>
#include <fc/log/console_appender.hpp>
#include <fc/log/file_appender.hpp>
#include <fc/log/logger.hpp>
#include <fc/log/logger_config.hpp>
#include <fc/filesystem.hpp>

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#endif

#ifdef _MSC_VER
#include <Windows.h>
#endif

#include <signal.h>
#include <QTranslator>

#include "gui_design.hpp"


int runDecentD(int argc, char** argv, fc::promise<void>::ptr& exit_promise);
QProcess* run_ipfs_daemon(QObject* parent, QString app_dir);


int main(int argc, char* argv[])
{
   QApplication app(argc, argv);

   //app.setStyleSheet(d_global_white_style);


   QTranslator* translator = new QTranslator();
   if (translator->load("decent_en", ":/translations/languages")) {
      app.installTranslator(translator);
   }
   
   
   gui_wallet::Mainwindow_gui_wallet aMainWindow;
   QProcess* daemon_process = nullptr;
   try {
      daemon_process = run_ipfs_daemon(&aMainWindow, app.applicationDirPath());
   } catch (const std::exception& ex) {
      QMessageBox* msgBox = new QMessageBox();
      msgBox->setWindowTitle("Error");
      msgBox->setText(QString::fromStdString(ex.what()));
      msgBox->exec();
      std::cout << ex.what();
      exit(1);
   }
   
   fc::thread thread_decentd("decentd service");

   fc::promise<void>::ptr exit_promise = new fc::promise<void>("Decent Daemon Exit Promise");

   fc::set_signal_handler([&exit_promise, daemon_process](int /*signal*/) {
      elog( "Caught SIGINT attempting to exit cleanly" );
      daemon_process->terminate();
      exit_promise->set_value();
   }, SIGINT);

   fc::set_signal_handler([&exit_promise, daemon_process](int /*signal*/) {
     elog( "Caught SIGTERM attempting to exit cleanly" );
      daemon_process->terminate();
     exit_promise->set_value();
   }, SIGTERM);
   
   
#if !defined( _MSC_VER )
   
   fc::set_signal_handler([&exit_promise, daemon_process](int /*signal*/) {
     elog( "Caught SIGHUP attempting to exit cleanly" );
      daemon_process->terminate();
     exit_promise->set_value();
  }, SIGHUP);
   
#endif
   
   
   fc::future<int> future_decentd = thread_decentd.async([argc, argv, &exit_promise]() -> int {
      return runDecentD(argc, argv, exit_promise);
   });
   
   
#define SET_LIBRARY_PATHS
#ifdef SET_LIBRARY_PATHS
   auto pluginsDir = QDir(QCoreApplication::applicationDirPath());
   if (pluginsDir.dirName() == "MacOS") {
      pluginsDir.cdUp();
   }
   pluginsDir.cd("plugins");

   QCoreApplication::setLibraryPaths(QStringList(pluginsDir.absolutePath()));
#endif



   qRegisterMetaType<string>( "std::string" );
   qRegisterMetaType<int64_t>( "int64_t" );
   app.setApplicationDisplayName("Decent");

   aMainWindow.show();
   
   
   app.exec();
   // qt event loop does not support exceptions anyway
   // there is no need for all try-catches

   exit_promise->set_value();
   daemon_process->terminate();
   future_decentd.wait();

   return 0;
}
//
// below is decent daemon staff, that's executed in a parallel thread
//
using namespace graphene;
namespace bpo = boost::program_options;


void write_default_logging_config_to_stream(std::ostream& out);
fc::optional<fc::logging_config> load_logging_config_from_ini_file(const fc::path& config_ini_filename);


bool check_for_ipfs(QObject* parent, QString program) {
   
   QProcess *checkProcess = new QProcess(parent);
   checkProcess->start(program, QStringList("version"));
   
   if (!checkProcess->waitForStarted(2000)) {
      return false;
   }
   
   
   if (!checkProcess->waitForFinished(2000)) {
      return false;
   }
   
   return true;
}

QString get_ipfs_path(QObject* parent, QString app_dir) {
   
   QString ipfs_bin = QString::fromStdString(utilities::decent_path_finder::instance().get_ipfs_bin().string());
   QString ipfs_path_bin = QString::fromStdString((utilities::decent_path_finder::instance().get_ipfs_path() / "ipfs").string());

   QString ipfs_path_next_to_exe = app_dir + QDir::separator() + "ipfs";
   QString ipfs_path_next_to_bin_exe = app_dir + QDir::separator() + ".."  + QDir::separator() + "bin"  + QDir::separator() + "ipfs";

   
   if (utilities::decent_path_finder::instance().get_ipfs_bin() != fc::path()) {
      if (check_for_ipfs(parent, ipfs_bin)) {
         return ipfs_bin;
      }
   }
   
   if (utilities::decent_path_finder::instance().get_ipfs_path() != fc::path()) {
      if (check_for_ipfs(parent, ipfs_path_bin)) {
         return ipfs_path_bin;
      }
   }
   
   if (check_for_ipfs(parent, ipfs_path_next_to_exe)) {
      return ipfs_path_next_to_exe;
   }
   
   if (check_for_ipfs(parent, ipfs_path_next_to_bin_exe)) {
      return ipfs_path_next_to_bin_exe;
   }
   
   
   if (check_for_ipfs(parent, "ipfs")) {
      return "ipfs";
   }
   
   
   return "";
}


QProcess* run_ipfs_daemon(QObject* parent, QString app_dir) {
   
   QString program = get_ipfs_path(parent, app_dir);
   
   if (program.isEmpty()) {
      throw std::runtime_error("Can not find IPFS executable. Please export IPFS_BIN or IPFS_PATH environment variables");
   }
   
   

   QProcess *initProcess = new QProcess(parent);
   initProcess->start(program, QStringList("init"));
   
   // If init timeout throw something
   if (!initProcess->waitForFinished(2000)) {
      throw std::runtime_error("Timeout while initializing ipfs");
   }
   
   // Run daemon
   QProcess *daemonProcess = new QProcess(parent);
   daemonProcess->start(program, QStringList("daemon"));
   return daemonProcess;
}


int runDecentD(int argc, char** argv, fc::promise<void>::ptr& exit_promise) {
   app::application* node = new app::application();
   fc::oexception unhandled_exception;
   try {
      bpo::options_description app_options("DECENT Daemon");
      bpo::options_description cfg_options("DECENT Daemon");
      app_options.add_options()
      ("help,h", "Print this help message and exit.")
      ("data-dir,d", bpo::value<boost::filesystem::path>()->default_value( utilities::decent_path_finder::instance().get_decent_data() / "decentd"), "Directory containing databases, configuration file, etc.")
      ;


      bpo::variables_map options;

      auto witness_plug = node->register_plugin<witness_plugin::witness_plugin>();
      auto history_plug = node->register_plugin<account_history::account_history_plugin>();
      auto seeding_plug = node->register_plugin<decent::seeding::seeding_plugin>();
      auto market_history_plug = node->register_plugin<market_history::market_history_plugin>();

      try
      {
         bpo::options_description cli, cfg;
         node->set_program_options(cli, cfg);
         app_options.add(cli);
         cfg_options.add(cfg);

         bpo::store(bpo::parse_command_line(argc, argv, app_options), options);
      }
      catch (const boost::program_options::error& e)
      {
         std::cerr << "Error parsing command line: " << e.what() << "\n";
         return 1;
      }

      if( options.count("help") )
      {
         std::cout << app_options << "\n";
         return 0;
      }

      fc::path data_dir;
      if( options.count("data-dir") )
      {
         data_dir = options["data-dir"].as<boost::filesystem::path>();
         if( data_dir.is_relative() )
            data_dir = fc::current_path() / data_dir;
      }

      fc::path config_ini_path = data_dir / "config.ini";
      if( fc::exists(config_ini_path) )
      {
         // get the basic options
         bpo::store(bpo::parse_config_file<char>(config_ini_path.preferred_string().c_str(), cfg_options, true), options);

         // try to get logging options from the config file.
         try
         {
            fc::optional<fc::logging_config> logging_config = load_logging_config_from_ini_file(config_ini_path);
            if (logging_config)
               fc::configure_logging(*logging_config);
         }
         catch (const fc::exception&)
         {
            wlog("Error parsing logging config from config file ${config}, using default config", ("config", config_ini_path.preferred_string()));
         }
      }
      else
      {
         ilog("Writing new config file at ${path}", ("path", config_ini_path));
         if( !fc::exists(data_dir) )
            fc::create_directories(data_dir);

         std::ofstream out_cfg(config_ini_path.preferred_string());
         for( const boost::shared_ptr<bpo::option_description> od : cfg_options.options() )
         {
            if( !od->description().empty() )
               out_cfg << "# " << od->description() << "\n";
            boost::any store;
            if( !od->semantic()->apply_default(store) )
               out_cfg << "# " << od->long_name() << " = \n";
            else {
               auto example = od->format_parameter();
               if( example.empty() )
                  // This is a boolean switch
                  out_cfg << od->long_name() << " = " << "false\n";
               else {
                  // The string is formatted "arg (=<interesting part>)"
                  example.erase(0, 6);
                  example.erase(example.length()-1);
                  out_cfg << od->long_name() << " = " << example << "\n";
               }
            }
            out_cfg << "\n";
         }
         write_default_logging_config_to_stream(out_cfg);
         out_cfg.close();
         // read the default logging config we just wrote out to the file and start using it
         fc::optional<fc::logging_config> logging_config = load_logging_config_from_ini_file(config_ini_path);
         if (logging_config)
            fc::configure_logging(*logging_config);
      }
      bpo::notify(options);
      node->initialize(data_dir, options);
      node->initialize_plugins( options );

      node->startup();
      node->startup_plugins();

      ilog("Started witness node on a chain with ${h} blocks.", ("h", node->chain_database()->head_block_num()));
      ilog("Chain ID is ${id}", ("id", node->chain_database()->get_chain_id()) );


      exit_promise->wait();
      node->shutdown_plugins();
      node->shutdown();
      delete node;
      return 0;
   } catch( const fc::exception& e ) {
      // deleting the node can yield, so do this outside the exception handler
      unhandled_exception = e;
   }

   if (unhandled_exception)
   {
      elog("Exiting with error:\n${e}", ("e", unhandled_exception->to_detail_string()));
      node->shutdown();
      delete node;
      return 1;
   }
}

void write_default_logging_config_to_stream(std::ostream& out)
{
   out << "# declare an appender named \"stderr\" that writes messages to the console\n"
          "[log.console_appender.stderr]\n"
          "stream=std_error\n\n"
          "# declare an appender named \"p2p\" that writes messages to p2p.log\n"
          "[log.file_appender.p2p]\n"
          "filename=logs/p2p/p2p.log\n"
          "# filename can be absolute or relative to this config file\n\n"
          "# route any messages logged to the default logger to the \"stderr\" logger we\n"
          "# declared above, if they are info level are higher\n"
          "[logger.default]\n"
          "level=info\n"
          "appenders=stderr\n\n"
          "# route messages sent to the \"p2p\" logger to the p2p appender declared above\n"
          "[logger.p2p]\n"
          "level=debug\n"
          "appenders=p2p\n\n";
}

fc::optional<fc::logging_config> load_logging_config_from_ini_file(const fc::path& config_ini_filename)
{
   try
   {
      fc::logging_config logging_config;
      bool found_logging_config = false;

      boost::property_tree::ptree config_ini_tree;
      boost::property_tree::ini_parser::read_ini(config_ini_filename.preferred_string().c_str(), config_ini_tree);
      for (const auto& section : config_ini_tree)
      {
         const string& section_name = section.first;
         const boost::property_tree::ptree& section_tree = section.second;

         const string console_appender_section_prefix = "log.console_appender.";
         const string file_appender_section_prefix = "log.file_appender.";
         const string logger_section_prefix = "logger.";

         if (boost::starts_with(section_name, console_appender_section_prefix))
         {
            string console_appender_name = section_name.substr(console_appender_section_prefix.length());
            string stream_name = section_tree.get<string>("stream");

            // construct a default console appender config here
            // stdout/stderr will be taken from ini file, everything else hard-coded here
            fc::console_appender::config console_appender_config;
            console_appender_config.level_colors.emplace_back(
               fc::console_appender::level_color(fc::log_level::debug, 
                                                 fc::console_appender::color::green));
            console_appender_config.level_colors.emplace_back(
               fc::console_appender::level_color(fc::log_level::warn, 
                                                 fc::console_appender::color::brown));
            console_appender_config.level_colors.emplace_back(
               fc::console_appender::level_color(fc::log_level::error, 
                                                 fc::console_appender::color::cyan));
            console_appender_config.stream = fc::variant(stream_name).as<fc::console_appender::stream::type>();
            logging_config.appenders.push_back(fc::appender_config(console_appender_name, "console", fc::variant(console_appender_config)));
            found_logging_config = true;
         }
         else if (boost::starts_with(section_name, file_appender_section_prefix))
         {
            string file_appender_name = section_name.substr(file_appender_section_prefix.length());
            fc::path file_name = section_tree.get<string>("filename");
            if (file_name.is_relative())
               file_name = fc::absolute(config_ini_filename).parent_path() / file_name;
            

            // construct a default file appender config here
            // filename will be taken from ini file, everything else hard-coded here
            fc::file_appender::config file_appender_config;
            file_appender_config.filename = file_name;
            file_appender_config.flush = true;
            file_appender_config.rotate = true;
            file_appender_config.rotation_interval = fc::hours(1);
            file_appender_config.rotation_limit = fc::days(1);
            logging_config.appenders.push_back(fc::appender_config(file_appender_name, "file", fc::variant(file_appender_config)));
            found_logging_config = true;
         }
         else if (boost::starts_with(section_name, logger_section_prefix))
         {
            string logger_name = section_name.substr(logger_section_prefix.length());
            string level_string = section_tree.get<string>("level");
            string appenders_string = section_tree.get<string>("appenders");
            fc::logger_config logger_config(logger_name);
            logger_config.level = fc::variant(level_string).as<fc::log_level>();
            boost::split(logger_config.appenders, appenders_string, 
                         boost::is_any_of(" ,"), 
                         boost::token_compress_on);
            logging_config.loggers.push_back(logger_config);
            found_logging_config = true;
         }
      }
      if (found_logging_config)
         return logging_config;
      else
         return fc::optional<fc::logging_config>();
   }
   FC_RETHROW_EXCEPTIONS(warn, "")
}



