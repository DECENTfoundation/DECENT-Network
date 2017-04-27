/*
 *	File: decent_wallet_ui_gui_newcheckbox.cpp
 *
 *	Created on: 22 Feb 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
#include "stdafx.h"

#include "gui_design.hpp"
#include "decent_button.hpp"

#ifndef _MSC_VER
#include <QGraphicsDropShadowEffect>
#include <string>
#endif

using namespace gui_wallet;

DecentButton::DecentButton()
{
   _isEnabled = true;
   setAlignment(Qt::AlignCenter);
   setStyleSheet(decent_button_style);
   setScaledContents(true);
}


void DecentButton::setEnabled(bool isEnabled) {
   _isEnabled = isEnabled;
   if (_isEnabled)
      setStyleSheet(d_upload_button_true);
   else
      setStyleSheet(d_upload_button_false);
}

DecentButton1::DecentButton1(QWidget *parent) : QPushButton(parent)
{
   this->setStyleSheet("QPushButton{border: 0px ; background-color :rgb(27,176,104); color : white;}"
                       "QPushButton:!enabled{background-color :rgb(180,180,180); color : rgb(30, 30, 30);}");
}
