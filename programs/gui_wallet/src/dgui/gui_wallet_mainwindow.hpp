/*
 *	File: gui_wallet_mainwindow.cpp
 *
 *	Created on: Nov 11, 2016
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
#ifndef MAINWINDOW_GUI_WALLET_H
#define MAINWINDOW_GUI_WALLET_H

#include <QMainWindow>
#include "gui_wallet_centralwidget.hpp"
#include <QAction>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QThread>
#include "gui_wallet_connectdlg.hpp"
#include "text_display_dialog.hpp"
#include "richdialog.hpp"
#include <unnamedsemaphorelite.hpp>
#include <stdarg.h>
#include <string>
#include <map>
#include <set>
#include <decent/wallet_utility/wallet_utility.hpp>
#include "decent_wallet_ui_gui_contentdetailsgeneral.hpp"

namespace gui_wallet
{
using WalletAPI = decent::wallet_utility::WalletAPI;

class WalletOperator : public QObject
{
    Q_OBJECT
public:
    WalletOperator();
    ~WalletOperator();

public slots:
   void slot_connect(WalletAPI* pwallet_api);
signals:
   void signal_connected();
};


class Mainwindow_gui_wallet : public QMainWindow
{
   Q_OBJECT
public:
   Mainwindow_gui_wallet();
   virtual ~Mainwindow_gui_wallet(); 
   
   void GoToThisTab(int index, std::string info);
   void UpdateAccountBalances(const std::string& username);
   
protected:
   void CreateActions();
   void CreateMenues();
   
   
private:
   
   void UpdateLockedStatus();
   void SetPassword();
   
   
protected slots:
   void CurrentUserChangedSlot(const QString&);
   void CheckDownloads();
   void DisplayWalletContentGUI(bool isNewWallet);
   void DisplayConnectionError(std::string errorMessage);
   
protected slots:
   
   void AboutSlot();
   void HelpSlot();
   void InfoSlot();
   void ViewAction();
   
   void ConnectSlot();
   void ImportKeySlot();
   void LockSlot();
   void UnlockSlot();

   void slot_connected();

public:
   void RunTask(std::string str_command, std::string str_result);

signals:
   void signal_connect(WalletAPI* pwallet_api);
   
protected:
   class QVBoxLayout*   m_pCentralAllLayout;
   class QHBoxLayout*   m_pMenuLayout;
   CentralWigdet*       m_pCentralWidget;
   
   QMenuBar *          m_barLeft;
   QMenuBar *          m_barRight;
   QMenu*              m_pMenuFile;
   QMenu*              m_pMenuSetting;
   QMenu*              m_pMenuHelpL;
   QMenu*              m_pMenuContent;
   QMenu*              m_pMenuHelpR;
   QMenu*              m_pMenuView;
   
   QAction             m_ActionExit;
   QAction             m_ActionConnect;
   QAction             m_ActionAbout;
   QAction             m_ActionInfo;
   QAction             m_ActionHelp;
   QAction             m_ActionLock;
   QAction             m_ActionUnlock;
   QAction             m_ActionImportKey;
   TextDisplayDialog   m_info_dialog;
   
   
   QVBoxLayout                         m_main_layout;
   bool                                m_locked;
   RichDialog                          m_import_key_dlg;
   int                                 m_nConnected;
   //SConnectionStruct                   m_wdata2;
   PasswordDialog                      m_SetPasswordDialog;
   PasswordDialog                      m_UnlockDialog;
   
   QTimer                              _downloadChecker;
   std::set<std::string>               _activeDownloads;

   WalletOperator*   m_p_wallet_operator;
   QThread           m_wallet_operator_thread;
   WalletAPI         m_wallet_api;
};

   
   
   
}

#endif // MAINWINDOW_GUI_WALLET_H
