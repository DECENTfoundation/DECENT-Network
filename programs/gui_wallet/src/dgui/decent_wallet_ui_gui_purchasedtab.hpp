//decent_wallet_ui_gui_purchasedtab
/*
 *	File: decent_wallet_ui_gui_purchasedtab.hpp
 *
 *	Created on: 11 Feb 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */

#ifndef DECENT_WALLET_UI_GUI_PURCHASEDTAB_HPP
#define DECENT_WALLET_UI_GUI_PURCHASEDTAB_HPP

#include <QWidget>
#include <string>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QLabel>
#include <QLineEdit>
#include "qt_commonheader.hpp"

namespace gui_wallet {

namespace DCF_PURCHASE {
    enum DIG_CONT_FIELDS { TIME, SYNOPSIS, RATING,SIZE, PRICE, PURCHASED, NUM_OF_DIG_CONT_FIELDS };
}

class PurchasedTab : public QWidget
{

    friend class CentralWigdet;
    friend class Mainwindow_gui_wallet;
    Q_OBJECT
public:
    PurchasedTab();
    virtual ~PurchasedTab();

    void SetDigitalContentsGUI(const std::vector<SDigitalContent>& contents);
    //QString getFilterText()const;

public:
signals:
    void ShowDetailsOnDigContentSig(SDigitalContent dig_cont);

protected:
    void PrepareTableWidgetHeaderGUI();
    void DigContCallback(_NEEDED_ARGS2_);
    virtual void resizeEvent ( QResizeEvent * a_event );
    void ArrangeSize();

protected:
    QVBoxLayout     m_main_layout;
    //QHBoxLayout     m_search_layout;
    //QTableWidget    m_TableWidget; // Should be investigated
    QTableWidget*    m_pTableWidget;
    //int              m_nNumberOfContentsPlus1;
    QLineEdit       m_filterLineEditer;
};

}


#include "decent_wallet_ui_gui_common.tos"

#endif // DECENT_WALLET_UI_GUI_PURCHASEDTAB_HPP
