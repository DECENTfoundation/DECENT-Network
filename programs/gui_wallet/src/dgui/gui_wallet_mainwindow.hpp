#pragma once

#include <QMainWindow>
#include <QAction>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QThread>

#include "gui_wallet_centralwidget.hpp"
#include "gui_wallet_connectdlg.hpp"
#include "text_display_dialog.hpp"
#include "richdialog.hpp"

#include "json.hpp"

#include <decent/wallet_utility/wallet_utility.hpp>
#include "decent_wallet_ui_gui_contentdetailsgeneral.hpp"

#include <stdarg.h>
#include <string>
#include <map>
#include <set>

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
   void slot_connect();
   //void slot_content_upload(std::string str_command);
signals:
   void signal_connected(std::string str_error);
   //void signal_content_uploaded(std::string str_error);
public:
   WalletAPI m_wallet_api;
};

   
   
   
   

class Mainwindow_gui_wallet : public QMainWindow
{
   Q_OBJECT
public:
   
   Mainwindow_gui_wallet();
   virtual ~Mainwindow_gui_wallet(); 
   
   void GoToThisTab(int index, std::string info);
   void UpdateAccountBalances(const std::string& username);
   
public:
   
   static void RunTaskImpl(std::string const& str_command, std::string& str_result);
   static bool RunTaskParseImpl(std::string const& str_command, nlohmann::json& json_result);

   
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
   void currentUserBalanceUpdate();
   
protected slots:
   
   void AboutSlot();
   void HelpSlot();
   void InfoSlot();
   void ViewAction();
   
   void ConnectSlot();
   void ImportKeySlot();
   void LockSlot();
   void UnlockSlot();
   void SendDCTSlot();

   void slot_connected(std::string str_error);

signals:
   void signal_connect();
   
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
   QAction             m_ActionSendDCT;
   TextDisplayDialog   m_info_dialog;
   
   QVBoxLayout                         m_main_layout;
   bool                                m_locked;
   RichDialog*                         m_import_key_dlg;
   SendDialog*                         m_sendDCT_dialog;
   int                                 m_nConnected;
   //SConnectionStruct                   m_wdata2;
   PasswordDialog                      m_SetPasswordDialog;
   PasswordDialog                      m_UnlockDialog;
   
   QTimer                              _downloadChecker;
   QTimer                              _balanceUpdater;
   std::set<std::string>               _activeDownloads;

public:
   WalletOperator*   m_p_wallet_operator;
protected:
   QThread           m_wallet_operator_thread;
};

   
}




#define RunTask gui_wallet::Mainwindow_gui_wallet::RunTaskImpl
#define RunTaskParse gui_wallet::Mainwindow_gui_wallet::RunTaskParseImpl


