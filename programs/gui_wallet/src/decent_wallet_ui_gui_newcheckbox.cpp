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

#include "decent_wallet_ui_gui_newcheckbox.hpp"
#include "gui_design.hpp"
#include <string>


#ifdef _MSC_VER
#include <string>
#endif

using namespace gui_wallet;

NewCheckBox::NewCheckBox(int a_index)
    :
      m_index(a_index)
{
    setStyleSheet(d_StyleSheet);
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
