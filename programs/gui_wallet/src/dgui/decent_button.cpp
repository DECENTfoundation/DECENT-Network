/*
 *	File: decent_wallet_ui_gui_newcheckbox.cpp
 *
 *	Created on: 22 Feb 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */

#include "gui_design.hpp"
#include "decent_button.hpp"
#include <QGraphicsDropShadowEffect>
#include <string>


using namespace gui_wallet;

DecentButton::DecentButton()
{
    setAlignment(Qt::AlignCenter);
    setStyleSheet(decent_button_style);
    setScaledContents(true);

}

DecentButton::~DecentButton()
{
    
}
