/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
#pragma once

#include <QMainWindow>
#include <string>
#include <set>

class QCloseEvent;
class QStackedWidget;
class QComboBox;
class QTimer;
class QAction;
class CProgBar;// updating
class CDetectUpdateThreadParams;// updating

namespace gui_wallet
{
class Asset;
class DecentLabel;
class DecentLineEdit;
class DecentButton;
class TabContentManager;

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
   void slot_resyncBlockChain();
   void slot_importKey();
   void slot_checkDownloads();
   void slot_getContents();
   void slot_PreviousPage();
   void slot_ResetPage();
   void slot_NextPage();
   void slot_BrowseToggled(bool toggled);
   void slot_TransactionsToggled(bool toggled);
   void slot_PublishToggled(bool toggled);
   void slot_UsersToggled(bool toggled);
   void slot_PurchasedToggled(bool toggled);
   
   void DisplayWalletContentGUI();

signals:
   void signal_setSplashMainText(QString const&);

protected:
   void closeSplash(bool bGonnaCoverAgain);
   virtual void closeEvent(QCloseEvent* event) override;
   TabContentManager* activeTable() const;

protected:
   size_t m_iSplashWidgetIndex;
   QTimer* m_pTimerDownloads;
   QTimer* m_pTimerBalance;
   QTimer* m_pTimerContents;
   
   QStackedWidget* m_pStackedWidget;
   QComboBox* m_pAccountList;
   DecentLabel* m_pBalance;

   DecentButton* m_pButtonBrowse;
   DecentButton* m_pButtonTransactions;
   DecentButton* m_pButtonPublish;
   DecentButton* m_pButtonUsers;
   DecentButton* m_pButtonPurchased;

   DecentButton* m_pPreviousPage;
   DecentButton* m_pResetPage;
   DecentButton* m_pNextPage;
   
   DecentLineEdit* m_pFilterBrowse;
   DecentLineEdit* m_pFilterTransactions;
   DecentLineEdit* m_pFilterPublish;
   DecentLineEdit* m_pFilterUsers;
   DecentLineEdit* m_pFilterPurchased;

   DecentButton* m_pPublish;

   TabContentManager* m_pTabBrowse;
   TabContentManager* m_pTabTransactions;
   TabContentManager* m_pTabPublish;
   TabContentManager* m_pTabUsers;
   TabContentManager* m_pTabPurchased;

   QAction* m_pActionImportKey;
   QAction* m_pActionReplayBlockchain;
   QAction* m_pActionResyncBlockchain;

   std::set<std::string> m_activeDownloads;
   
   // updating:   this needs to go to separate class
public:
   void EmitStartRevHistoryDlg(const std::string& revHistory, uint32_t& returnValue); 
   void EmitProgBarSetPos(int pos);
   void EmitProgBarDestroy(void);
   void ProxyCreateProgBar(int upperBorder, uint32_t* abort); 
   void ProxyDestroyProgBar(void);
   void ProxySetProgBarTitle(const QString& title);

protected:
   bool m_updateProgBarCreate;
   bool m_updateProgBarDestroy;
   int m_proxyUpdateProgBarUpperBorder;
   QString m_proxyUpdateProgBarSetTitle;
   uint32_t* m_proxyUpdateProgBarAbort;
   CProgBar* m_progBar;
   CDetectUpdateThreadParams* m_updateThreadParams;
   void* m_updateThread;
   QTimer* m_pTimerUpdateProxy;

   void progBarCreate(int upperBorder, uint32_t* abort);
   void progBarSetTitle(const QString& title);
   void progBarDestroy(void);

signals:
   void signal_startRevHistoryDlg(const QString& revHistory, long* returnValue);
   void signal_progBarSetPos(int pos);

protected slots:
   void slot_startRevHistoryDlg(const QString& revHistory, long* returnValue); 
   void slot_updateProxy();
   void slot_progBarSetPos(int pos);

};

}

