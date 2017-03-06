/*
 *	File: decent_wallet_ui_gui_contentdetailsbase.hpp
 *
 *	Created on: 22 Feb 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
#ifndef DECENT_WALLET_UI_GUI_CONTENTDETAILSBASE_HPP
#define DECENT_WALLET_UI_GUI_CONTENTDETAILSBASE_HPP

#define NUMBER_OF_SUB_LAYOUTS2   8

#include <QDialog>
#include "qt_commonheader.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

namespace gui_wallet {

class ContentDetailsBase : public QDialog
{
protected:
    ContentDetailsBase();
    virtual ~ContentDetailsBase();

    void execCDB(const SDigitalContent& a_cnt_details);

    //virtual void execCDD(const decent::wallet::ui::gui::SDigitalContent& a_cnt_details)=0;

protected:
    const SDigitalContent* m_pContentInfo;
    QVBoxLayout     m_main_layout;
    QHBoxLayout     m_free_for_child;
    QWidget         m_vSub_Widgets[NUMBER_OF_SUB_LAYOUTS2];
    QVBoxLayout     m_vSub_layouts[NUMBER_OF_SUB_LAYOUTS2];
    QLabel          m_vLabels[NUMBER_OF_SUB_LAYOUTS2*2];
    QLabel          m_stars[5];
};

}

#endif // DECENT_WALLET_UI_GUI_CONTENTDETAILSBASE_HPP
