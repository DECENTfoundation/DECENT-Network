/*
 *	File: decent_wallet_ui_gui_contentdetails.hpp
 *
 *	Created on: 21 Feb 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
#ifndef DECENT_WALLET_UI_GUI_CONTENTDETAILS_HPP
#define DECENT_WALLET_UI_GUI_CONTENTDETAILS_HPP

#define NUMBER_OF_SUB_LAYOUTS   9

#include <QDialog>
#include "qt_commonheader.hpp"
#include <QVBoxLayout>
#include <QLabel>

namespace decent{ namespace wallet{ namespace ui{ namespace gui{

class ContentDetails : public QDialog
{
    Q_OBJECT
public:
    void execCD(const decent::wallet::ui::gui::SDigitalContent& a_cnt_details);

private:
    const decent::wallet::ui::gui::SDigitalContent* m_pContentInfo;
    QVBoxLayout     m_main_layout;
    QVBoxLayout     m_vSub_layouts[NUMBER_OF_SUB_LAYOUTS];
    QLabel          m_vLabels[NUMBER_OF_SUB_LAYOUTS*2];
};

}}}}

#endif // DECENT_WALLET_UI_GUI_CONTENTDETAILS_HPP
