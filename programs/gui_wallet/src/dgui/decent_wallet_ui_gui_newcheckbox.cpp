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

decent::wallet::ui::gui::NewCheckBox::NewCheckBox(const char* a_checkedImg2, const char* a_uncheckedImg2,
                                                  int a_index)
    :
      m_index(a_index)
{
    //FindImagePath(bRet,"green_asterix.png").c_str(),
    //FindImagePath(bRet,"white_asterix.png").c_str())
    //setScaledContents();
    bool bRet;
    std::string csCheckedPath, csUnCheckedPath;
    csCheckedPath = a_checkedImg2 ? a_checkedImg2 : FindImagePath(bRet,"green_asterix.png");
    csUnCheckedPath = a_uncheckedImg2 ? a_uncheckedImg2 : FindImagePath(bRet,"white_asterix.png");

#if 0
    QCheckBox::indicator {
        width: 13px;
        height: 13px;
    }
#endif

    //setStyleSheet("QMainWindow{color:black;""background-color:white;}");
    std::string qsStyleSheet =
            std::string("QCheckBox::indicator:checked {image: url(") +
            csCheckedPath +
            ");}QCheckBox::indicator:unchecked {image: url(" +
            csUnCheckedPath +
            ");}"
            "QCheckBox::indicator{width: 13px; height: 13px;}";
    setStyleSheet(qsStyleSheet.c_str());

    //StateChangedSlot(int state)
    connect(this,SIGNAL(stateChanged(int)),this,SLOT(StateChangedSlot(int)));
    __DEBUG_APP2__(1,"%s  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!",qsStyleSheet.c_str());
}


decent::wallet::ui::gui::NewCheckBox::~NewCheckBox()
{
    //
}


const int& decent::wallet::ui::gui::NewCheckBox::GetIndex()const
{
    return m_index;
}


void decent::wallet::ui::gui::NewCheckBox::SetIndex(int a_index)
{
    m_index = a_index;
}


void decent::wallet::ui::gui::NewCheckBox::StateChangedSlot(int a_nState)
{
    emit StateChangedNewSignal(a_nState,m_index);
}
