/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
#pragma once

#include "gui_wallet_global.hpp"

#ifndef STDAFX_H
#include <QMainWindow>

class QStackedWidget;
class QComboBox;
class QCheckBox;
class QTimer;
class QAction;
class QProgressBar;
#endif

class UpdateManager;

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
   MainWindow(const std::string &wallet_file, const graphene::wallet::server_data &ws);
   virtual ~MainWindow();

   const std::string& walletFile() const { return m_wallet_file; }

public slots:
   void slot_daemonFinished(int ret);

protected slots:
   void slot_setSplash();
   void slot_closeSplash();
   void slot_showPurchasedTab();
   void slot_showTransactionsTab(std::string const&);
   void slot_stackWidgetPush(StackLayerWidget* pWidget);
   void slot_stackWidgetPop();
   void slot_updateAccountAssets(const QList<Asset>& assets);
   void slot_updateAccountBalance(QAction *pAsset);
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
   void slot_SyncProgressUpdate(const QString& time_text, int);
   void slot_CommonTextProgressUpdate(const QString& time_text);
   void slot_BlockchainUpdate(int value, int max);
   void slot_currentAccountChanged(int iIndex);
   void slot_MinerVotingToggled(bool toggled);
   void slot_advancedMinerVoting();

   void DisplayWalletContentGUI();

signals:
   void signal_setSplashMainText(const QString & );

protected:
   void closeSplash(bool bGonnaCoverAgain);
   TabContentManager* activeTable() const;
   void updateActiveTable();

   void resizeEvent(QResizeEvent* event) override;

private:
   void updateBalance(const QString& balance);

   int m_daemon_restart = 0;
   std::string m_wallet_file;
   graphene::wallet::server_data m_ws;
   size_t m_iSplashWidgetIndex;
   QTimer* m_pTimerBalance;
   QTimer* m_pOneShotUpdateTimer;

   QStackedWidget* m_pStackedWidget;
   QComboBox* m_pAccountList;
   DecentLabel* m_pBalance;
   DecentButton* m_pAssetSymbol;

   DecentButton* m_pButtonBrowse;
   DecentButton* m_pButtonTransactions;
   DecentButton* m_pButtonPublish;
   DecentButton* m_pButtonUsers;
   DecentButton* m_pButtonPurchased;
   DecentButton* m_pButtonMinerVoting;

   DecentButton* m_pPreviousPage;
   DecentButton* m_pResetPage;
   DecentButton* m_pNextPage;
   
   DecentLineEdit* m_pFilterBrowse;
   DecentLineEdit* m_pFilterTransactions;
   DecentLineEdit* m_pFilterPublish;
   DecentLineEdit* m_pFilterUsers;
   DecentLineEdit* m_pFilterPurchased;

   DecentButton* m_pPublish;
   QCheckBox* m_pOnlyMyVotes;

   TabContentManager* m_pTabBrowse;
   TabContentManager* m_pTabTransactions;
   TabContentManager* m_pTabPublish;
   TabContentManager* m_pTabUsers;
   TabContentManager* m_pTabPurchased;
   TabContentManager* m_pTabMinerVoting;

   QAction* m_pActionImportKey;
   QAction* m_pActionReplayBlockchain;
   QAction* m_pActionResyncBlockchain;
   QAction* m_pAdvancedMinerVoting;

   UpdateManager* m_pUpdateManager;

   std::set<std::string> m_activeDownloads;

   QProgressBar* m_pConnectingProgress;
   DecentLabel* m_pConnectingLabel;
};

}
