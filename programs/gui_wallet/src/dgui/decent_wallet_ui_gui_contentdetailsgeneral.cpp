/*
 *	File: decent_wallet_ui_gui_contentdetailsgeneral.cpp
 *
 *	Created on: 21 Feb 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */


#include "decent_wallet_ui_gui_contentdetailsgeneral.hpp"


decent::wallet::ui::gui::ContentDetailsGeneral::ContentDetailsGeneral()
{
    setFixedSize(397,381);
}


decent::wallet::ui::gui::ContentDetailsGeneral::~ContentDetailsGeneral()
{
    //
}


void decent::wallet::ui::gui::ContentDetailsGeneral::execCDD(
        const decent::wallet::ui::gui::SDigitalContent& a_cnt_details)
{
    execCDB(a_cnt_details);
}
