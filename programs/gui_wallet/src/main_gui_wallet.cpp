/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */

#ifndef STDAFX_H
#include "../stdafx.h"
#endif

#include "gui_wallet_global.hpp"
#include "mainwindow.hpp"

#if NDEBUG
//#define SET_LIBRARY_PATHS
#endif

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
      bpo::store(bpo::parse_command_line(argc, argv, app_options), options);
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
      unsigned int boost_major_version = BOOST_VERSION / 100000;
      unsigned int boost_minor_version = BOOST_VERSION / 100 - boost_major_version * 1000;
      std::string boost_version_text = std::to_string(boost_major_version) + "." + std::to_string(boost_minor_version) + "." + std::to_string(BOOST_VERSION % 100);
      std::string openssl_version_text = std::string(OPENSSL_VERSION_TEXT);
      openssl_version_text = openssl_version_text.substr(0, openssl_version_text.length() - 11);

      std::cout << "DECENT Wallet " << graphene::utilities::git_version() << "\nBoost " << boost_version_text << "\n" << openssl_version_text << std::endl;
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
   gui_wallet::MainWindow aMainWindow(options["wallet-file"].as<std::string>());
   QObject::connect(&gui_wallet::Globals::instance(), &gui_wallet::Globals::signal_daemonFinished,
                    &aMainWindow, &gui_wallet::MainWindow::slot_daemonFinished);

   try {
      gui_wallet::Globals::instance().startDaemons(gui_wallet::BlockChainStartType::Simple, aMainWindow.walletFile());
   } catch (const std::exception& ex) {
      QMessageBox* msgBox = new QMessageBox();
      msgBox->setWindowTitle("Error");
      msgBox->setText(QString::fromStdString(ex.what()));
      msgBox->exec();
      std::cout << ex.what();
      exit(1);
   }
   
   
#define SET_LIBRARY_PATHS 1
#ifdef SET_LIBRARY_PATHS
   auto pluginsDir = QDir(QCoreApplication::applicationDirPath());
   if (pluginsDir.dirName() == "MacOS") {
      pluginsDir.cdUp();
   }
   pluginsDir.cd("plugins");
   

   QCoreApplication::setLibraryPaths(QStringList(pluginsDir.absolutePath()));
   QStringList paths = QCoreApplication::libraryPaths();
#endif

   try {
      qRegisterMetaType<std::string>( "std::string" );
      qRegisterMetaType<int64_t>( "int64_t" );
      app.setApplicationDisplayName("DECENT");

      aMainWindow.show();
      //aMainWindow.StartUpdateThread();
      app.exec();
   }
   catch(const std::exception& ex) {
      std::cout << "exception:" << ex.what() << std::endl;
   }
   catch(const fc::exception& ex) {
      std::cout << "exception:" << ex.what() << std::endl;
   }
   catch(...) {
      std::cout << "yay! exception..." << std::endl;
   }

   gui_wallet::Globals::instance().stopDaemons();
   gui_wallet::Globals::instance().clear();

   return 0;
}
