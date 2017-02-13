/*
 *	File      : walletcontentdlg.cpp
 *
 *	Created on: 04 Jan 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */

#include "walletcontentdlg.hpp"
#include <thread>
#include <iostream>

extern int g_nDebugApplication;

gui_wallet::WalletContentDlg::WalletContentDlg()
{
    m_main_layout.addWidget(&m_num_acc_or_error_label);
    setLayout(&m_main_layout);
}

gui_wallet::WalletContentDlg::~WalletContentDlg()
{
}


int gui_wallet::WalletContentDlg::exec(vector<account_object>& a_pAcc, vector<vector<asset>>& a_pBl, int& a_nError, std::string& a_error_string)
{
    if(_LIKELY_(!a_nError))
    {
        m_pvAccounts = &a_pAcc;
        m_pvAccountsBalances = &a_pBl;
        const int cnNumOfAccounts(m_pvAccounts->size());
        QString aNumOfAccStr = tr("Number of accounts: ") + QString::number(cnNumOfAccounts,10);
        m_num_acc_or_error_label.setText(aNumOfAccStr);
    }
    else
    {
        QString aNumOfAccStr = tr("Error accured: \n") + tr(a_error_string.c_str());
        m_num_acc_or_error_label.setText(aNumOfAccStr);
    }

    return QDialog::exec();
}
