/*
 *	File: gui_wallet_centralwigdet.h
 *
 *	Created on: Nov 11, 2016
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
#ifndef CENTRALWIGDET_GUI_WALLET_H
#define CENTRALWIGDET_GUI_WALLET_H

//#define USE_TABLE_FOR_FIRST_LINE
#define API_SHOULD_BE_DEFINED


#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QTabWidget>
#include "browse_content_tab.hpp"
#include "transactions_tab.hpp"
#include "upload_tab.hpp"
#include "overview_tab.hpp"
#include "purchased_tab.hpp"
#include <stdio.h>
#include <QLabel>
#include <QComboBox>
#include <vector>
#include <QTableWidget>
#include "purchased_tab.hpp"
#include <QString>


extern int g_nDebugApplication;

namespace gui_wallet {

enum FRST_LINE_ELEMS{DECENT_LOGO,USERNAME,BALANCE,SEND_,NUMBER_OF_FRST_LINE_ELEMS};
enum MAIN_TABS_ENM{BROWSE_CONTENT,TRANSACTIONS,UPLOAD,OVERVIEW,PURCHASED};



class AccountBalanceWidget : public TableWidgetItemW_base<QWidget,int>
{
public:
    AccountBalanceWidget();
    void addItem(const std::string& a_balance);
    void clear();
    void setCurrentIndex(int index);
private:
    void ClbFunction(_NEEDED_ARGS1_(int));
    void SetAccountBalanceFromStringGUIprivate(const std::string& a_balance);
private:
    QHBoxLayout m_main_layout;
    QLabel      m_amount_label;
    QLabel      m_asset_type_label;
    std::vector<std::string> m_vBalances;
    int                 m_nCurrentIndex;
};


    
class CentralWigdet : public QWidget
{
    friend class Mainwindow_gui_wallet;
    Q_OBJECT
public:
    CentralWigdet(class QBoxLayout* pAllLayout, class Mainwindow_gui_wallet* a_pPar);
    virtual ~CentralWigdet(); /* virtual because may be this class will be */
                              /* used by inheritance */

    void SetAccountBalancesFromStrGUI(const std::vector<std::string>& a_balances_and_names);
    void AddNewUserGUI(const std::string& user_name);
    
    Browse_content_tab* GetBrowseContentTab() { return &m_browse_cont_tab; }
    PurchasedTab* GetPurchasedTab() { return &m_Purchased_tab; }
    
    
    void SetDigitalContentsGUI(const std::vector<SDigitalContent>& vContents);
    
    
    QString getFilterText()const;
    QComboBox* usersCombo();
    
    
    int GetMyCurrentTabIndex()const {
        return m_main_tabs.currentIndex();
    }
    
    QString FilterStr();
    
public slots:
    void tabChanged(int index);
    
protected:
    virtual void showEvent ( QShowEvent * event ) ;
    virtual void resizeEvent ( QResizeEvent * event );

private:
    void PrepareGUIprivate(class QBoxLayout* pAllLayout);
    QWidget* GetWidgetFromTable5(int column, int widget);

private slots:
    void make_deleyed_warning();

private:
    QVBoxLayout         m_main_layout;
    QHBoxLayout         m_first_line_lbl;

    
    
    Mainwindow_gui_wallet* m_parent_main_window;
    
    QTabWidget          m_main_tabs;
    
    Browse_content_tab  m_browse_cont_tab;
    Transactions_tab    m_trans_tab;
    Upload_tab          m_Upload_tab;
    Overview_tab        m_Overview_tab;
    PurchasedTab        m_Purchased_tab;
    
    

    QString             m_DelayedWaringTitle;
    QString             m_DelayedWaringText;
    QString             m_DelayedWaringDetails;

    QWidget*            m_pDcLogoWgt;
    QWidget*            m_pUsernameWgt;
    QWidget*            m_pBalanceWgt1;

    
};

}


#endif // CENTRALWIGDET_GUI_WALLET_H