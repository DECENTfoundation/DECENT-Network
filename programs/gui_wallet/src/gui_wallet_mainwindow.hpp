#pragma once

#include <QMainWindow>
#include <QAction>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTimer>

#include "gui_wallet_centralwidget.hpp"
#include "text_display_dialog.hpp"
#include "richdialog.hpp"

#include "json.hpp"

#include <stdarg.h>
#include <string>
#include <map>
#include <set>

class QCloseEvent;

namespace gui_wallet
{
class Asset;

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

   void SetSplash();

   virtual void closeEvent(QCloseEvent *event) override;
protected slots:
   void CloseSplash();

signals:
   void signal_setSplashMainText(QString const&);
   
protected slots:
   void CurrentUserChangedSlot(const QString&);
   void CheckDownloads();
   void DisplayWalletContentGUI();
   void DisplayConnectionError(std::string errorMessage);
   void currentUserBalanceUpdate();
   
   void AboutSlot();
   void HelpSlot();
   void InfoSlot();
   void ViewAction();

   void ImportKeySlot();
   void SendDCTSlot();

   void slot_showPurchasedTab();
   void slot_showTransactionsTab(std::string const&);
   void slot_updateAccountBalance(Asset const&);
   
   void slot_connection_status_changed(Globals::ConnectionState from, Globals::ConnectionState to);
   
   void slot_enableSendButton();
      
protected:
   class QVBoxLayout*   m_pCentralAllLayout;
   class QHBoxLayout*   m_pMenuLayout;
   CentralWigdet*       m_pCentralWidget;
   uint32_t             m_iSyncUpCount;
   
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
   QAction             m_ActionImportKey;
   TextDisplayDialog   m_info_dialog;
   
   QVBoxLayout                         m_main_layout;
   
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


