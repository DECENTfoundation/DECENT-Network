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
#include <string>

std::string FindImagePath(bool& a_bRet,const char* a_image_name);

using namespace gui_wallet;

NewCheckBox::NewCheckBox(int a_index)
    :
      m_index(a_index)
{
    
    std::string csCheckedPath = ":/icon/images/green_asterix.png";
    std::string csUnCheckedPath = ":/icon/images/white_asterix.png";
    

    std::string qsStyleSheet =
            std::string("QCheckBox::indicator:checked { image: url(") + csCheckedPath + "); }" +
                        "QCheckBox::indicator:unchecked { image: url(" + csUnCheckedPath + "); }" +
                        "QCheckBox::indicator { width: 13px; height: 13px; }";
    setStyleSheet(qsStyleSheet.c_str());
    //setEnabled(false);
    
    //connect(this,SIGNAL(stateChanged(int)),this,SLOT(StateChangedSlot(int)));
}



NewCheckBox::~NewCheckBox()
{
    //
}


const int& NewCheckBox::GetIndex()const
{
    return m_index;
}


void NewCheckBox::SetIndex(int a_index)
{
    m_index = a_index;
}


void NewCheckBox::StateChangedSlot(int a_nState)
{
    emit StateChangedNewSignal(a_nState,m_index);
}
