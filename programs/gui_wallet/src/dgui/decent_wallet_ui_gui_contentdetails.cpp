// decent_wallet_ui_gui_contentdetails
/*
 *	File: decent_wallet_ui_gui_contentdetails.cpp
 *
 *	Created on: 21 Feb 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */


#include "decent_wallet_ui_gui_contentdetails.hpp"

static const char* s_vcpcFieldsGeneral[NUMBER_OF_SUB_LAYOUTS] = {
    "Author", "Expiration","Created","Price", "Amount","Asset ID",
    "Averege Rating","Size","Times Bought"
};


static const char* s_vcpcFieldsBougth[NUMBER_OF_SUB_LAYOUTS] = {
    "Author", "Expiration","Created","Price", "Amount","Asset ID",
    "Averege Rating","Size","Times Bought"
};

typedef const char* NewType[];

//static NewType  s_vFields[]={s_vcpcFieldsGeneral,s_vcpcFieldsBougth};


void decent::wallet::ui::gui::ContentDetails::execCD(const decent::wallet::ui::gui::SDigitalContent& a_cnt_details)
{
#if 0
    int i;
    NewType vNames = s_vFields[a_cnt_details.type];
    m_pContentInfo = &a_cnt_details;

    for(i=0;i<NUMBER_OF_SUB_LAYOUTS;++i)
    {
        //m_v
    }
    setLayout(&m_main_layout);

    QDialog::exec();
#endif
}
