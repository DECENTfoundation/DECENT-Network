/*
 *	File: decent_wallet_ui_gui_newcheckbox.cpp
 *
 *	Created on: 22 Feb 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */

#include "decent_button.hpp"
#include <QGraphicsDropShadowEffect>
#include <string>


using namespace gui_wallet;

DecentButton::DecentButton()
{
   _isEnabled = true;
   setAlignment(Qt::AlignCenter);
   setStyleSheet("QLabel { background-color :rgb(27,176,104); color : white;}");
   setScaledContents(true);
}


void DecentButton::setEnabled(bool isEnabled) {
   _isEnabled = isEnabled;
   if (_isEnabled)
      setStyleSheet("QLabel { background-color :rgb(27,176,104); color : white;}");
   else
      setStyleSheet("QLabel { background-color :rgb(180,180,180); color : rgb(30, 30, 30); }");
}

 
