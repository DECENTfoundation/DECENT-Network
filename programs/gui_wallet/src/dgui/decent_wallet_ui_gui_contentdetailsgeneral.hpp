/*
 *	File: decent_wallet_ui_gui_contentdetailsbougth.hpp
 *
 *	Created on: 22 Feb 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
#ifndef DECENT_WALLET_UI_GUI_CONTENTDETAILSGENERAL_HPP
#define DECENT_WALLET_UI_GUI_CONTENTDETAILSGENERAL_HPP

#include "decent_wallet_ui_gui_contentdetailsbase.hpp"

namespace decent{ namespace wallet{ namespace ui{ namespace gui{

class ContentDetailsGeneral : public ContentDetailsBase
{
public:
    ContentDetailsGeneral();
    virtual ~ContentDetailsGeneral();

    virtual void execCDD(const decent::wallet::ui::gui::SDigitalContent& a_cnt_details);


};

}}}}

#endif // DECENT_WALLET_UI_GUI_CONTENTDETAILSGENERAL_HPP
