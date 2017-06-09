#include "stdafx.h"

#ifndef _MSC_VER
#include <QApplication>

#include <QMessageBox>
#include <QDir>
#include <QFile>
#include <QTextStream>

#if NDEBUG
//#define SET_LIBRARY_PATHS
#endif

#include <string>
#endif

#include "gui_wallet_global.hpp"
#include "gui_wallet_mainwindow.hpp"

#ifdef _MSC_VER
#include <Windows.h>
#endif

#include <signal.h>
#include <QTranslator>

#include "gui_design.hpp"
using string = std::string;

int main(int argc, char* argv[])
{
   QApplication app(argc, argv);

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

   app.setFont(MainFont());
   
   gui_wallet::Mainwindow_gui_wallet aMainWindow;

   try {
      gui_wallet::Globals::instance().startDaemons(false);
   } catch (const std::exception& ex) {
      QMessageBox* msgBox = new QMessageBox();
      msgBox->setWindowTitle("Error");
      msgBox->setText(QString::fromStdString(ex.what()));
      msgBox->exec();
      std::cout << ex.what();
      exit(1);
   }
   
   
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
   app.setApplicationDisplayName("DECENT");

   aMainWindow.show();
   app.exec();

   gui_wallet::Globals::instance().stopDaemons();
   gui_wallet::Globals::instance().clear();

   return 0;
}




