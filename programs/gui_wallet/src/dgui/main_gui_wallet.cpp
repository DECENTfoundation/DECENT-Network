/*
 *	File: main_gui_wallet.cpp
 *
 *	Created on: Nov 11, 2016
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */

#include <stdio.h>
#include <wchar.h>
#include <string.h>
#include <string>
#include <fc/exception/exception.hpp>

#include "gui_wallet_application.hpp"
#include "gui_wallet_mainwindow.hpp"


extern InGuiThreatCaller* s_pWarner;


int main(int argc, char* argv[])
{
  


//    freopen( "/dev/null", "w", stderr);

    try{

        gui_wallet::application aApp(argc,argv);
        gui_wallet::Mainwindow_gui_wallet aMainWindow;
        aMainWindow.show();
        aApp.exec();
    } catch(const char* a_ext_str) {
        std::cout << "Exception: " << a_ext_str << std::endl;
    } catch(const std::exception& ex) {
        std::cout << "Exception: " << ex.what() << std::endl;
    } catch(const fc::exception& ex) {
        std::cout << "Exception: " << ex.what() << std::endl;
    } catch (...) {
        std::cout << "Unknown Exception " << std::endl;
    }

    return 0;
}

