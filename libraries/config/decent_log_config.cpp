
#include <fc/reflect/variant.hpp>
#include <fc/variant.hpp>
#include <fc/variant_object.hpp>
#include <fc/log/console_appender.hpp>
#include <fc/log/file_appender.hpp>
#include <fc/log/logger.hpp>
#include <fc/log/logger_config.hpp>
#include <fc/exception/exception.hpp>

#include <decent/config/decent_log_config.hpp>


#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include <iostream>
#include <fstream>
#include <string>

namespace decent {

    // logging config is too complicated to be parsed by boost::program_options,
    // so we do it by hand
    //
    // Currently, you can only specify the filenames and logging levels, which
    // are all most users would want to change.  At a later time, options can
    // be added to control rotation intervals, compression, and other seldom-
    // used features
    void write_default_logging_config_to_stream(std::ostream &out) {
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
              "level=info\n"
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
                      Ptree &pt) {
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
       while (stream.good()) {

          // Get line from stream
          ++line_no;
          std::getline(stream, line);
          if (!stream.good() && !stream.eof())
             BOOST_PROPERTY_TREE_THROW(boost::property_tree::ini_parser_error(
                                             "read error", "", line_no));

          // If line is non-empty
          line = boost::property_tree::detail::trim(line, stream.getloc());
          if (!line.empty()) {
             // Comment, section or key?
             if (line[0] == semicolon || line[0] == hash) {
                // Ignore comments
             } else if (line[0] == lbracket) {
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
             } else {
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
                      const std::locale &loc = std::locale()) {
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

    fc::optional<fc::logging_config> load_logging_config_from_ini_file(const fc::path &config_ini_filename)
    {
       try {
          fc::logging_config logging_config;
          bool found_logging_config = false;

          boost::property_tree::ptree config_ini_tree;
          read_log_ini(config_ini_filename.preferred_string().c_str(), config_ini_tree);
          for (const auto &section : config_ini_tree) {
             const std::string &section_name = section.first;
             const boost::property_tree::ptree &section_tree = section.second;

             const std::string console_appender_section_prefix = "log.console_appender.";
             const std::string file_appender_section_prefix = "log.file_appender.";
             const std::string logger_section_prefix = "logger.";

             if (boost::starts_with(section_name, console_appender_section_prefix)) {
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

//                if (stream_name == )

                fc::variant lala("std_error");
                auto type = lala.get_type();


//                fc::console_appender::stream::type aa = lala.as<fc::console_appender::stream::type>();


//                console_appender_config.stream = fc::variant(stream_name).as<fc::console_appender::stream::type>();
//                logging_config.appenders.push_back(fc::appender_config(console_appender_name, "console", fc::variant(console_appender_config)));
                found_logging_config = true;
             } else if (boost::starts_with(section_name, file_appender_section_prefix)) {
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
                logging_config.appenders.push_back(
                      fc::appender_config(file_appender_name, "file", fc::variant(file_appender_config)));
                found_logging_config = true;
             } else if (boost::starts_with(section_name, logger_section_prefix)) {
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

} //namespace decent

