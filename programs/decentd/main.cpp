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
#include <graphene/messaging/messaging.hpp>
#include <graphene/utilities/dirhelper.hpp>

#include <fc/exception/exception.hpp>
#include <fc/thread/thread.hpp>
#include <fc/interprocess/signals.hpp>
#include <fc/log/console_appender.hpp>
#include <fc/log/file_appender.hpp>
#include <fc/log/logger.hpp>
#include <fc/log/logger_config.hpp>

#include <boost/filesystem.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include <iostream>
#include <fstream>

#ifdef WIN32
#include <signal.h> 
#include <windows.h>
#else
# include <csignal>
#endif

using namespace graphene;
namespace bpo = boost::program_options;
         
void write_default_logging_config_to_stream(std::ostream& out);
fc::optional<fc::logging_config> load_logging_config_from_ini_file(const fc::path& config_ini_filename);

#ifdef _MSC_VER
// Stopping from GUI
static fc::promise<int>::ptr s_exit_promise;
BOOL s_bStop = FALSE;
HANDLE s_hStopProcess = NULL;

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
int main(int argc, char** argv) {
#ifdef _MSC_VER
   SetConsoleCtrlHandler(HandlerRoutine, TRUE);
#endif
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

      auto miner_plug = node->register_plugin<miner_plugin::miner_plugin>();
      auto history_plug = node->register_plugin<account_history::account_history_plugin>();
      auto seeding_plug = node->register_plugin<decent::seeding::seeding_plugin>();
      auto messaging_plug = node->register_plugin<decent::messaging::messaging_plugin>();

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

      fc::promise<int>::ptr exit_promise = new fc::promise<int>("UNIX Signal Handler");
#ifdef _MSC_VER
      s_exit_promise = exit_promise;
      s_hStopProcess = OpenEventA(SYNCHRONIZE, FALSE, "A883EF36-8168-4E45-A08F-97EFFC5B6694");
      if (s_hStopProcess)
      {
         DWORD tid = 0;
         CreateThread(NULL, 0, StopFromGUIThreadProc, NULL, 0, &tid);
      }
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

      if( !seeding_plug->my )
      {
         decent::seeding::seeding_promise = new fc::promise<decent::seeding::seeding_plugin_startup_options>("Seeding Promise");

         while( !decent::seeding::seeding_promise->ready() && !exit_promise->ready() ){
            fc::usleep(fc::microseconds(1000000));
         }

         if( decent::seeding::seeding_promise->ready() && !exit_promise->ready() ){
            seeding_plug->plugin_pre_startup(decent::seeding::seeding_promise->wait(fc::microseconds(1)));
            seeding_plug->plugin_startup();
         }
      }

      int signal = exit_promise->wait();
      ilog("Exiting from signal ${n}", ("n", signal));
      node->shutdown_plugins();
      node->shutdown();
      delete node;
#ifdef _MSC_VER
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
      node->shutdown();
      delete node;
      return 1;
   }
}

// logging config is too complicated to be parsed by boost::program_options, 
// so we do it by hand
//
// Currently, you can only specify the filenames and logging levels, which
// are all most users would want to change.  At a later time, options can
// be added to control rotation intervals, compression, and other seldom-
// used features
void write_default_logging_config_to_stream(std::ostream& out)
{
   out << "# declare an appender named \"stderr\" that writes messages to the console\n"
          "[log.console_appender.stderr]\n"
          "stream=std_error\n\n"
          "# declare an appender named \"p2p\" that writes messages to p2p.log\n"
          "[log.file_appender.p2p]\n"
          "filename=logs/p2p/p2p.log\n"
          "# filename can be absolute or relative to this config file\n\n"
          "# declare an appender named \"transfer\" that writes messages to transfer.log\n"
          "[log.file_appender.transfer]\n"
          "filename=logs/transfer.log\n"
          "# filename can be absolute or relative to this config file\n\n"
          "# route any messages logged to the default logger to the \"stderr\" logger we\n"
          "# declared above, if they are info level are higher\n"
          "[logger.default]\n"
          "level=error\n"
          "appenders=stderr\n\n"
          "# route messages sent to the \"p2p\" logger to the p2p appender declared above\n"
          "[logger.p2p]\n"
          "level=error\n"
          "appenders=p2p\n\n"
          "# route messages sent to the \"transfer\" logger to the transfer appender declared above\n"
          "[logger.transfer]\n"
          "level=error\n"
          "appenders=transfer\n\n";
}
// Log ini parser. It is needed to use small adjustment because original boost ini parser throw exception when reads the same key again.
template<class Ptree>
void read_log_ini(std::basic_istream<
   typename Ptree::key_type::value_type> &stream,
   Ptree &pt)
{
   typedef typename Ptree::key_type::value_type Ch;
   typedef std::basic_string<Ch> Str;
   const Ch semicolon = stream.widen(';');
   const Ch hash = stream.widen('#');
   const Ch lbracket = stream.widen('[');
   const Ch rbracket = stream.widen(']');

   Ptree local;
   unsigned long line_no = 0;
   Ptree *section = 0;
   Str line;

   // For all lines
   while (stream.good())
   {

      // Get line from stream
      ++line_no;
      std::getline(stream, line);
      if (!stream.good() && !stream.eof())
         BOOST_PROPERTY_TREE_THROW(boost::property_tree::ini_parser_error(
            "read error", "", line_no));

      // If line is non-empty
      line = boost::property_tree::detail::trim(line, stream.getloc());
      if (!line.empty())
      {
         // Comment, section or key?
         if (line[0] == semicolon || line[0] == hash)
         {
            // Ignore comments
         }
         else if (line[0] == lbracket)
         {
            // If the previous section was empty, drop it again.
            if (section && section->empty())
               local.pop_back();
            typename Str::size_type end = line.find(rbracket);
            if (end == Str::npos)
               BOOST_PROPERTY_TREE_THROW(boost::property_tree::ini_parser_error(
                  "unmatched '['", "", line_no));
            Str key = boost::property_tree::detail::trim(
               line.substr(1, end - 1), stream.getloc());
            if (local.find(key) != local.not_found())
               BOOST_PROPERTY_TREE_THROW(boost::property_tree::ini_parser_error(
                  "duplicate section name", "", line_no));
            section = &local.push_back(
               std::make_pair(key, Ptree()))->second;
         }
         else
         {
            Ptree &container = section ? *section : local;
            typename Str::size_type eqpos = line.find(Ch('='));
            if (eqpos == Str::npos)
               BOOST_PROPERTY_TREE_THROW(boost::property_tree::ini_parser_error(
                  "'=' character not found in line", "", line_no));
            if (eqpos == 0)
               BOOST_PROPERTY_TREE_THROW(boost::property_tree::ini_parser_error(
                  "key expected", "", line_no));
            Str key = boost::property_tree::detail::trim(
               line.substr(0, eqpos), stream.getloc());
            Str data = boost::property_tree::detail::trim(
               line.substr(eqpos + 1, Str::npos), stream.getloc());
            if (container.find(key) != container.not_found()) { // jump over if found already used key
               //BOOST_PROPERTY_TREE_THROW(boost::property_tree::ini_parser_error(
                 // "duplicate key name", "", line_no));
            } else
               container.push_back(std::make_pair(key, Ptree(data)));
         }
      }
   }
   // If the last section was empty, drop it again.
   if (section && section->empty())
      local.pop_back();

   // Swap local ptree with result ptree
   pt.swap(local);

}

template<class Ptree>
void read_log_ini(const std::string &filename,
   Ptree &pt,
   const std::locale &loc = std::locale())
{
   std::basic_ifstream<typename Ptree::key_type::value_type>
      stream(filename.c_str());
   if (!stream)
      BOOST_PROPERTY_TREE_THROW(boost::property_tree::ini_parser_error(
         "cannot open file", filename, 0));
   stream.imbue(loc);
   try {
      read_log_ini(stream, pt);
   }
   catch (boost::property_tree::ini_parser_error &e) {
      BOOST_PROPERTY_TREE_THROW(boost::property_tree::ini_parser_error(
         e.message(), filename, e.line()));
   }
}

fc::optional<fc::logging_config> load_logging_config_from_ini_file(const fc::path& config_ini_filename)
{
   try
   {
      fc::logging_config logging_config;
      bool found_logging_config = false;

      boost::property_tree::ptree config_ini_tree;
      read_log_ini(config_ini_filename.preferred_string().c_str(), config_ini_tree);
      for (const auto& section : config_ini_tree)
      {
         const std::string& section_name = section.first;
         const boost::property_tree::ptree& section_tree = section.second;

         const std::string console_appender_section_prefix = "log.console_appender.";
         const std::string file_appender_section_prefix = "log.file_appender.";
         const std::string logger_section_prefix = "logger.";

         if (boost::starts_with(section_name, console_appender_section_prefix))
         {
            std::string console_appender_name = section_name.substr(console_appender_section_prefix.length());
            std::string stream_name = section_tree.get<std::string>("stream");

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
            std::string file_appender_name = section_name.substr(file_appender_section_prefix.length());
            fc::path file_name = section_tree.get<std::string>("filename");
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
            std::string logger_name = section_name.substr(logger_section_prefix.length());
            std::string level_string = section_tree.get<std::string>("level");
            std::string appenders_string = section_tree.get<std::string>("appenders");
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
