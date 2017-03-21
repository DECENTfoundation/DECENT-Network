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
    setAlignment(Qt::AlignCenter);
    setStyleSheet("QLabel { background-color :rgb(27,176,104); color : white;}");
    setScaledContents(true);
    
    //QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect(this);
    //effect->setBlurRadius(20);
    //effect->setOffset(2,2);
    
    //setGraphicsEffect(effect);
}

DecentButton::~DecentButton()
{
    
}
