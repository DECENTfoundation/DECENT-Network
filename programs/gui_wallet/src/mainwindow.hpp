#pragma once

#include <QMainWindow>
#include <string>
#include <set>

class QCloseEvent;
class QStackedWidget;
class QComboBox;
class QTimer;

namespace gui_wallet
{
class Asset;
class DecentLabel;
class CentralWigdet;

class MainWindow : public QMainWindow
{
   Q_OBJECT
public:
   MainWindow();
   virtual ~MainWindow();

protected slots:
   void slot_setSplash();
   void slot_closeSplash();
   void slot_showPurchasedTab();
   void slot_showTransactionsTab(std::string const&);
   void slot_stackWidgetPush(StackLayerWidget* pWidget);
   void slot_stackWidgetPop();
   void slot_updateAccountBalance(Asset const&);
   void slot_connectionStatusChanged(Globals::ConnectionState from, Globals::ConnectionState to);
   void slot_replayBlockChain();
   void slot_importKey();
   void slot_checkDownloads();

   void DisplayWalletContentGUI();

signals:
   void signal_setSplashMainText(QString const&);

protected:
   virtual void closeEvent(QCloseEvent* event) override;


protected:
   QTimer* m_pTimerDownloads;
   QTimer* m_pTimerBalance;
   QStackedWidget* m_pStackedWidget;
   QComboBox* m_pAccountList;
   DecentLabel* m_pBalance;
   CentralWigdet*       m_pCentralWidget;

   std::set<std::string>               _activeDownloads;


public:
   
   void GoToThisTab(int index, std::string info);
   
public:
   
   static void RunTaskImpl(std::string const& str_command, std::string& str_result);
   static bool RunTaskParseImpl(std::string const& str_command, nlohmann::json& json_result);

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


