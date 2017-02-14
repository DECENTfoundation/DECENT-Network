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
#include <stdio.h>
#include <QLabel>
#include <QComboBox>
#include <vector>

#define __TEMPORARY__

extern int g_nDebugApplication;

struct qtWidget_test : public QWidget{~qtWidget_test(){if(g_nDebugApplication){printf("qtWidget_test::~qtWidget_test();\n");}}};
#define tmpWidget  qtWidget_test

namespace gui_wallet
{

    class CentralWigdet : public QWidget
    {
        Q_OBJECT
    public:
        CentralWigdet(class QBoxLayout* pAllLayout);
        virtual ~CentralWigdet(); /* virtual because may be this class will be */
                                  /* used by inheritance */

        void SetAccountBalancesFromStrGUI(const std::vector<std::string>& a_balances_and_names);

        QComboBox&  GetUsersList(){return m_users_list;}
        void AddNewUserGUI(const std::string& user_name);

        __TEMPORARY__ QComboBox& usersCombo(){return m_users_list;}

        QString getFilterText()const;

        QWidget* GetBrowseContentTab(){return &m_browse_cont_tab;}

        //QTableWidget& getDigitalContentsTable();
        void SetDigitalContentsGUI(const std::vector<gui_wallet::SDigitalContent>& contents);

    protected:
        virtual void showEvent ( QShowEvent * event ) ;
        virtual void resizeEvent ( QResizeEvent * event );

    private:
        void PrepareGUIprivate(class QBoxLayout* pAllLayout);

    private slots:
        void make_deleyed_warning();

    private:
        QVBoxLayout         m_main_layout;
        tmpWidget           m_first_line_widget;
        QHBoxLayout         m_first_line_layout;
        QLineEdit           m_search_box;
        QTabWidget          m_main_tabs;
        Browse_content_tab  m_browse_cont_tab;
        Transactions_tab    m_trans_tab;
        Upload_tab          m_Upload_tab;
        Overview_tab        m_Overview_tab;
        QString             m_DelayedWaringTitle;
        QString             m_DelayedWaringText;
        QString             m_DelayedWaringDetails;
        class QLabel*       m_imageLabel;
        //QLabel              m_balanceLabel;
        QComboBox           m_balanceCombo;
        /* 'm_nBalance' to have this filed in order to skip parsing the text each time balance is needed*/
        //double              m_lfBalance;
        QComboBox           m_users_list;

    };
}


#endif // CENTRALWIGDET_GUI_WALLET_H
