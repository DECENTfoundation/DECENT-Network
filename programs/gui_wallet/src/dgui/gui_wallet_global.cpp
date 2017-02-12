/*
 *	File: gui_wallet_global.cpp
 *
 *	Created on: 30 Nov, 2016
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements the global functions,
 *  that can be used by different classes
 *
 */

//#include "gui_wallet_global.hpp"

#include <QMessageBox>

namespace gui_wallet
{

void makeWarningImediatly(const char* a_WaringTitle, const char* a_WaringText, const char* a_detailed, void* a_pParent )
{
    QMessageBox aMessageBox(QMessageBox::Warning,QObject::tr(a_WaringTitle),QObject::tr(a_WaringText),
                            QMessageBox::Ok,(QWidget*)a_pParent);
    aMessageBox.setDetailedText(QObject::tr(a_detailed));
    aMessageBox.exec();
}

}
