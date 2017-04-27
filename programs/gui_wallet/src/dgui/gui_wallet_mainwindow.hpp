#pragma once

#include <QMainWindow>
#include <QAction>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "gui_wallet_centralwidget.hpp"
#include "gui_wallet_connectdlg.hpp"
#include "text_display_dialog.hpp"
#include "richdialog.hpp"

#include "json.hpp"

#include "decent_wallet_ui_gui_contentdetailsgeneral.hpp"

#include <stdarg.h>
#include <string>
#include <map>
#include <set>

namespace gui_wallet
{   

class Mainwindow_gui_wallet : public QMainWindow
{
   Q_OBJECT
public:
   
   Mainwindow_gui_wallet();
   virtual ~Mainwindow_gui_wallet(); 
   
   void GoToThisTab(int index, std::string info);
   void UpdateAccountBalances(const std::string& username);
   
   CentralWigdet* getCentralWidget();
   
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
   
   void AboutSlot();
   void HelpSlot();
   void InfoSlot();
   void ViewAction();

   void ImportKeySlot();
   void LockSlot();
   void UnlockSlot();
   void SendDCTSlot();

   void slot_showPurchasedTab();
   void slot_showTransactionsTab(std::string const&);
   void slot_updateAccountBalance(Asset const&);
   
   void slot_connected();
   void slot_query_blockchain();
   void slot_connecting_progress(std::string const&);
   
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
   QAction             m_ActionAbout;
   QAction             m_ActionInfo;
   QAction             m_ActionHelp;
   QAction             m_ActionLock;
   QAction             m_ActionUnlock;
   QAction             m_ActionImportKey;
   TextDisplayDialog   m_info_dialog;
   
   QVBoxLayout                         m_main_layout;
   bool                                m_locked;
   RichDialog*                         m_import_key_dlg;
   SendDialog*                         m_sendDCT_dialog;
   PasswordDialog                      m_SetPasswordDialog;
   PasswordDialog                      m_UnlockDialog;
   
   QTimer                              _downloadChecker;
   QTimer                              _balanceUpdater;
   std::set<std::string>               _activeDownloads;
};

   
}

inline void RunTask(std::string const& str_command, std::string& str_result)
{
   gui_wallet::Mainwindow_gui_wallet::RunTaskImpl(str_command, str_result);
}
inline bool RunTaskParse(std::string const& str_command, nlohmann::json& json_result)
{
   return gui_wallet::Mainwindow_gui_wallet::RunTaskParseImpl(str_command, json_result);
}


