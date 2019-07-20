/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */

#ifndef STDAFX_H
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QTextStream>
#include <QTranslator>

#include <iostream>

#include <boost/filesystem.hpp>

#include <fc/log/file_appender.hpp>

#include <graphene/utilities/git_revision.hpp>
#include <graphene/utilities/key_conversion.hpp>
#include <graphene/utilities/keys_generator.hpp>

#include <decent/decent_config.hpp>
#include <decent/about.hpp>
#endif

#include "gui_wallet_global.hpp"
#include "mainwindow.hpp"

int main(int argc, char* argv[])
{
   QApplication app(argc, argv);
   QCoreApplication::setApplicationName("DECENT Wallet");
   QCoreApplication::setApplicationVersion(QString::fromStdString(graphene::utilities::git_version()));

   bpo::options_description app_options("DECENT Wallet");
   bpo::options_description cfg_options("DECENT Wallet");
   bpo::variables_map options;

   try
   {
      gui_wallet::Globals::setCommandLine(app_options, cfg_options);
      bpo::parsed_options optparsed = bpo::command_line_parser(argc, argv).options(app_options).allow_unregistered().run();
      bpo::store(optparsed, options);
      if (decent::check_unrecognized(optparsed))
      {
         return EXIT_FAILURE;
      }
   }
   catch (const bpo::error& e)
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

      std::cout << "DECENT Wallet " << graphene::utilities::git_version();
#ifndef NDEBUG
      std::cout << " (debug)";
#endif /* NDEBUG */
      std::cout << "\nBoost " << boost_version_text << "\n" << openssl_version_text << "\nCryptopp " << cryptopp_version_text << "\nQt " << qVersion() << std::endl;

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
      catch ( const fc::exception& e )
      {
         std::cerr << e.to_detail_string() << std::endl;
      }
      catch (const std::exception& e)
      {
         std::cerr << e.what() << std::endl;
      }

      return EXIT_FAILURE;
   }

   QFile styleFile(":/css/styles/white_green.css");
   if(styleFile.open(QIODevice::ReadOnly))
   {
      QTextStream textStream(&styleFile);
      QString styleSheet = textStream.readAll();
      styleFile.close();
      app.setStyleSheet(styleSheet);
   }

   QTranslator* translator = new QTranslator();
   if (translator->load("decent_en", ":/translations/languages")) {
      app.installTranslator(translator);
   }

   app.setFont(gui_wallet::MainFont());

   boost::filesystem::path data_dir = options.count("data-dir") ?
      boost::filesystem::path(options["data-dir"].as<boost::filesystem::path>()) :
      graphene::utilities::decent_path_finder::instance().get_decent_data();

   if(data_dir.is_relative())
      data_dir = boost::filesystem::current_path() / data_dir;

   boost::filesystem::path config_filename = data_dir / "config.ini";
   if( exists(config_filename) )
   {
      // get the basic options
      boost::filesystem::ifstream cfg_stream(config_filename);
      bpo::store(bpo::parse_config_file<char>(cfg_stream, cfg_options, true), options);
   }
   else
   {
      ilog("Writing new config file at ${path}", ("path", config_filename));
      if( !exists(data_dir) )
         create_directories(data_dir);

      decent::write_default_config_file(config_filename, cfg_options, false);
   }

   // try to get logging options from the config file.
   try
   {
      fc::optional<fc::logging_config> logging_config = decent::load_logging_config_from_ini_file(config_filename, data_dir);
      if (logging_config) {
         if (!fc::configure_logging(*logging_config)) {
            std::cerr << "Error configure logging." << std::endl;
            return EXIT_FAILURE;
         }

         fc::file_appender::config ap_config(decent_path_finder::instance().get_decent_logs() / "gui_wallet.log");
         ap_config.flush                = true;
         ap_config.rotate               = true;
         ap_config.rotation_interval    = fc::hours( 1 );
         ap_config.rotation_limit       = fc::days( 1 );

         auto ap = fc::appender::create("gui", "file", fc::variant(ap_config));
         if(ap)
         {
            fc::log_level level = fc::log_level::off;
            switch(options["log-level"].as<char>())
            {
               case 'D':
                  level = fc::log_level::debug;
                  break;
               case 'I':
                  level = fc::log_level::info;
                  break;
               case 'W':
                  level = fc::log_level::warn;
                  break;
               case 'E':
                  level = fc::log_level::error;
                  break;
               case 'O':
                  level = fc::log_level::off;
                  break;
               default:
                  std::cerr << "Unknown log level: " << options["log-level"].as<char>() << std::endl;
                  break;
            }
            auto lgr = fc::logger::get("gui");
            lgr.set_name("gui");
            lgr.set_log_level(level);
            lgr.add_appender(ap);
         }
      }
   }
   catch (const fc::exception&)
   {
      wlog("Error parsing logging config from config file ${cfg}, using default config", ("cfg", config_filename.string()));
   }

   graphene::wallet::server_data ws{ "ws://" + options["rpc-endpoint"].as<std::string>() };
   gui_wallet::MainWindow aMainWindow(options["wallet-file"].as<boost::filesystem::path>(), ws);
   QObject::connect(&gui_wallet::Globals::instance(), &gui_wallet::Globals::signal_daemonFinished,
                    &aMainWindow, &gui_wallet::MainWindow::slot_daemonFinished);

   try {
      gui_wallet::Globals::instance().startDaemons(gui_wallet::BlockChainStartType::Simple, aMainWindow.walletFile(), ws);
   } catch (const std::exception& ex) {
      QMessageBox* msgBox = new QMessageBox();
      msgBox->setWindowTitle("Error");
      msgBox->setText(QString::fromStdString(ex.what()));
      msgBox->exec();
      GUI_ELOG("Exception: ${e}", ("e", ex.what()));
      exit(1);
   }

   try {
      qRegisterMetaType<std::string>( "std::string" );
      qRegisterMetaType<int64_t>( "int64_t" );
      app.setApplicationDisplayName("DECENT");

      aMainWindow.show();
      //aMainWindow.StartUpdateThread();
      app.exec();
   }
   catch(const std::exception& ex) {
      GUI_ELOG("Exception: ${e}", ("e", ex.what()));
   }
   catch(const fc::exception& ex) {
      GUI_ELOG("Exception: ${e}", ("e", ex.what()));
   }
   catch(...) {
      GUI_ELOG("Unknown exception");
   }

   gui_wallet::Globals::instance().stopDaemons();
   gui_wallet::Globals::instance().clear();

   return 0;
}
