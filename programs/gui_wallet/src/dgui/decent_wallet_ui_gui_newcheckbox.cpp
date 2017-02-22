/*
 *	File: decent_wallet_ui_gui_newcheckbox.cpp
 *
 *	Created on: 22 Feb 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */

#include "decent_wallet_ui_gui_newcheckbox.hpp"
#include "debug_decent_application.h"

decent::wallet::ui::gui::NewCheckBox::NewCheckBox()
{
    //setStyleSheet("QMainWindow{color:black;""background-color:white;}");
    __DEBUG_APP2__(0,"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    setStyleSheet("QCheckBox::indicator:unchecked {"
                  "image: url(/Users/davitkalantaryan/dev/decent/DECENT-Network/programs/gui_wallet/images/white_asterix.png);}"
                  "QCheckBox::indicator:checked {"
                  "image: url(/Users/davitkalantaryan/dev/decent/DECENT-Network/programs/gui_wallet/images/green_asterix.png);}");
}


decent::wallet::ui::gui::NewCheckBox::~NewCheckBox()
{
    //
}
