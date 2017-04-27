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
#include "decent_button.hpp"
#include <QLabel>

namespace gui_wallet {

class ContentDetailsGeneral : public ContentDetailsBase
{
    Q_OBJECT
public:
    ContentDetailsGeneral(QWidget* pParent);

    virtual void execCDD(const SDigitalContent& a_cnt_details);

public:
signals:
    void ContentWasBought();

protected slots:
    void LabelPushCallbackGUI();
   

protected:
    DecentButton1   m_label;
    DecentButton1   m_close;
};
}



#endif // DECENT_WALLET_UI_GUI_CONTENTDETAILSGENERAL_HPP
