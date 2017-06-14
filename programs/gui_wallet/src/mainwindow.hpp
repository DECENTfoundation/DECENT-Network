#pragma once

#include <QMainWindow>
#include <string>
#include <set>

class QCloseEvent;
class QStackedWidget;
class QComboBox;
class QTimer;
class QAction;

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
   virtual void closeEvent(QCloseEvent* event) override;
   TabContentManager* activeTable() const;


protected:
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

   std::set<std::string> m_activeDownloads;
};

}

