/*
 *	File: decent_wallet_ui_gui_contentdetailsbougth.hpp
 *
 *	Created on: 22 Feb 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
#ifndef DECENT_WALLET_UI_GUI_CONTENTDETAILSBOUGTH_HPP
#define DECENT_WALLET_UI_GUI_CONTENTDETAILSBOUGTH_HPP

#define MAX_RATE_VALUE  5

#include "decent_wallet_ui_gui_contentdetailsbase.hpp"
#include "decent_wallet_ui_gui_newcheckbox.hpp"

namespace gui_wallet {

class ContentDetailsBougth : protected ContentDetailsBase
{
    Q_OBJECT
public:
    ContentDetailsBougth();
    virtual ~ContentDetailsBougth();

    virtual void execCDD(const QString& user_name,
                         const SDigitalContent& a_cnt_details);

protected slots:
    void RateContentSlot(int selected, int index);

protected:
    QHBoxLayout     m_rate_layout_all;
    QHBoxLayout     m_rate_layout_left;
    QWidget         m_asterix_widget;
    QHBoxLayout     m_rate_layout_right;
    QLabel          m_RateText;
    //TableWidgetItemW<QLabel>*   m_pRateText;
    NewCheckBox     m_vRate_check_boxes[MAX_RATE_VALUE];
    QString         m_user_name;
    //uint64_t        m_ullnOldRate;

};

}

#endif // DECENT_WALLET_UI_GUI_CONTENTDETAILSBOUGTH_HPP
