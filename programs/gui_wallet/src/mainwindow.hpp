#pragma once

#include <QMainWindow>
#include <QAction>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTimer>

#include "gui_wallet_centralwidget.hpp"
#include "richdialog.hpp"

#include "json.hpp"

#include <stdarg.h>
#include <string>
#include <map>
#include <set>

class QCloseEvent;
class QStackedWidget;

namespace gui_wallet
{
class Asset;

class MainWindow : public QMainWindow
{
   Q_OBJECT
public:
   MainWindow();
   virtual ~MainWindow(); 
   
   void GoToThisTab(int index, std::string info);
   void UpdateAccountBalances(const std::string& username);
   
public:
   
   static void RunTaskImpl(std::string const& str_command, std::string& str_result);
   static bool RunTaskParseImpl(std::string const& str_command, nlohmann::json& json_result);

   
protected:

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

   void ImportKeySlot();
   void ReplayBlockChainSlot();
   void SendDCTSlot();

   void slot_showPurchasedTab();
   void slot_showTransactionsTab(std::string const&);
   void slot_stackWidgetPush(StackLayerWidget* pWidget);
   void slot_stackWidgetPop();
   void slot_updateAccountBalance(Asset const&);
   
   void slot_connection_status_changed(Globals::ConnectionState from, Globals::ConnectionState to);
   
   void slot_enableSendButton();
      
protected:
   QStackedWidget*   m_pStackedWidget;
   
   CentralWigdet*       m_pCentralWidget;
   
   QTimer                              _downloadChecker;
   QTimer                              _balanceUpdater;
   std::set<std::string>               _activeDownloads;
};

   
}

inline void RunTask(std::string const& str_command, std::string& str_result)
{
   gui_wallet::MainWindow::RunTaskImpl(str_command, str_result);
}
inline bool RunTaskParse(std::string const& str_command, nlohmann::json& json_result)
{
   return gui_wallet::MainWindow::RunTaskParseImpl(str_command, json_result);
}


