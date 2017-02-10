/*
 *	File      : walletcontentdlg.hpp
 *
 *	Created on: 04 Jan 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
#ifndef WALLETCONTENTDLG_HPP
#define WALLETCONTENTDLG_HPP

#define WALLETCONTENTDLG_UNDERSTAND

#include <QDialog>
//#include <graphene/wallet/wallet.hpp>
//#include "connected_api_instance.hpp"
#include <QVBoxLayout>
#include <QLabel>
#include <vector>

#ifndef _OVERRIDE_
#if __cplusplus >= 199711L
#define _OVERRIDE_ override
#else
#define _OVERRIDE_
#endif
#endif // #ifndef _OVERRIDE_

#ifndef _LIKELY_
#define _LIKELY_(_expr_)    (_expr_)
#endif

#ifndef WALLETCONTENTDLG_UNDERSTAND

namespace gui_wallet
{

struct account_object_str
{};

struct asset_str
{};

class WalletContentDlg : public QDialog
{
    Q_OBJECT

public:
    WalletContentDlg();
    virtual ~WalletContentDlg();

    virtual int execWCt(std::vector<account_object_str>& a_pAcc, std::vector<std::vector<asset_str>>& a_pBl, int& err, std::string& err_str ) /*_OVERRIDE_*/ ;

protected:
    //void ListAccountThreadFunc();
    //void CallShowWalletContentFunction(struct StructApi* a_pApi);

protected slots:
    //void WalletContentReadySlot();

protected:
signals:
    //void WalletContentReadySig();


private:
    std::vector<account_object_str>* m_pvAccounts;
    std::vector<std::vector<asset_str>>*  m_pvAccountsBalances;
    QVBoxLayout             m_main_layout;
    QLabel                  m_num_acc_or_error_label;
};

}


#endif  // #ifndef WALLETCONTENTDLG_UNDERSTAND

#endif // WALLETCONTENTDLG_HPP
