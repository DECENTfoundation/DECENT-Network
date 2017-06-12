#include "stdafx.h"

#include "gui_wallet_global.hpp"
#include "richdialog.hpp"

#ifndef _MSC_VER
#include <QMenuBar>
#include <QMoveEvent>
#include <QMessageBox>
#include <QDateTime>
#include <QProgressBar>
#include <QGridLayout>
#include <QStackedWidget>
#endif

#include "mainwindow.hpp"
#include "gui_design.hpp"
#include "decent_label.hpp"

#ifndef _MSC_VER
#include <stdio.h>
#include <stdlib.h>

#include <graphene/utilities/dirhelper.hpp>
#include <graphene/wallet/wallet.hpp>
#endif

#include <QCloseEvent>

using namespace nlohmann;
using namespace gui_wallet;
using namespace std;
using namespace graphene;
using namespace utilities;

MainWindow::MainWindow()
: QMainWindow()
, m_pStackedWidget(new QStackedWidget(this))
{
   setWindowTitle(tr("DECENT - Blockchain Content Distribution"));

   QWidget* pContainerWidget = new QWidget(this);
   QMenuBar* pMenuBar = new QMenuBar(pContainerWidget);
   QWidget* pMainWidget = new QWidget(pContainerWidget);
   //DecentLabel* pDecentLogo = new DecentLabel(this, DecentLabel::DecentLogo);
   //pDecentLogo->setText("alsdkmalsd");

   m_pCentralWidget = new CentralWigdet(pMainWidget);

   QVBoxLayout* pMainLayout = new QVBoxLayout;
   pMainLayout->setContentsMargins(0, 0, 0, 0);
   //pMainLayout->setSpacing(1);
   //pMainLayout->addWidget(pDecentLogo);
   pMainLayout->addWidget(m_pCentralWidget);
   pMainWidget->setLayout(pMainLayout);

   QVBoxLayout* pContainerLayout = new QVBoxLayout;
   pContainerLayout->setContentsMargins(0, 0, 0, 0);
   pContainerLayout->setSpacing(0);
#ifdef _MSC_VER
   pContainerLayout->addWidget(pMenuBar);
#endif
   pContainerLayout->addWidget(m_pStackedWidget);
   pContainerWidget->setLayout(pContainerLayout);

   setCentralWidget(pContainerWidget);
   m_pStackedWidget->addWidget(pMainWidget);
   //
   // The blocking splash screen
   //
   SetSplash();

   setUnifiedTitleAndToolBarOnMac(false);
   
   {
      QAction* pActionExit = new QAction(tr("&Exit"), this);
      QAction* pActionImportKey = new QAction(tr("Import key"), this);
      QAction* pActionReplayBlockchain = new QAction(tr("Replay Blockchain"), this);

      pActionExit->setStatusTip(tr("Exit Program"));
      pActionImportKey->setStatusTip(tr("Import key"));

      QObject::connect(pActionExit, &QAction::triggered,
                       this, &QMainWindow::close);

      QObject::connect(pActionImportKey, &QAction::triggered,
                       this, &MainWindow::ImportKeySlot);

      QObject::connect(pActionReplayBlockchain, &QAction::triggered,
                       this, &MainWindow::ReplayBlockChainSlot);

      QMenu* pMenuFile = pMenuBar->addMenu(tr("&File"));
      pMenuFile->addAction(pActionExit);
      pMenuFile->addAction(pActionImportKey);
      pMenuFile->addAction(pActionReplayBlockchain);
   }

   QComboBox*   pUsersCombo = m_pCentralWidget->usersCombo();
   DecentButton* pImportButton = m_pCentralWidget->importButton();
   pUsersCombo->hide();

   connect(pImportButton, SIGNAL(clicked()), this, SLOT(ImportKeySlot()));


   QObject::connect(&Globals::instance(), &Globals::walletConnectionStatusChanged,
                    this, &MainWindow::slot_connection_status_changed);

   QObject::connect(&Globals::instance(), &Globals::signal_stackWidgetPush,
                    this, &MainWindow::slot_stackWidgetPush);

   QObject::connect(&Globals::instance(), &Globals::signal_showPurchasedTab,
                    this, &MainWindow::slot_showPurchasedTab);

   QObject::connect(&Globals::instance(), &Globals::signal_showTransactionsTab,
                    this, &MainWindow::slot_showTransactionsTab);

   QObject::connect(&Globals::instance(), &Globals::signal_updateAccountBalance,
                    this, &MainWindow::slot_updateAccountBalance);
   
   QObject::connect(&Globals::instance(), &Globals::signal_keyImported,
                    this, &MainWindow::DisplayWalletContentGUI);
   QObject::connect(&Globals::instance(), &Globals::signal_keyImported,
                    this, &MainWindow::slot_enableSendButton);

   connect(pUsersCombo, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(CurrentUserChangedSlot(const QString&)) );

   _balanceUpdater.setSingleShot(false);
   _balanceUpdater.setInterval(10000);
   connect(&_balanceUpdater, SIGNAL(timeout()), this, SLOT( currentUserBalanceUpdate() ));

   _downloadChecker.setInterval(5000);
   connect(&_downloadChecker, SIGNAL(timeout()), this, SLOT(CheckDownloads()));

   connect(m_pCentralWidget, SIGNAL(sendDCT()), this, SLOT(SendDCTSlot()));


#ifdef _MSC_VER
    int height = style()->pixelMetric(QStyle::PM_TitleBarHeight);
    setWindowIcon(height > 32 ? QIcon(":/icon/images/windows_decent_icon_32x32.png")
         : QIcon(":/icon/images/windows_decent_icon_16x16.png"));
#endif
}

MainWindow::~MainWindow()
{
}

void MainWindow::SetSplash()
{
   StackLayerWidget* pSplashScreen = new StackLayerWidget(this);
   QProgressBar* pConnectingProgress = new QProgressBar(pSplashScreen);
   pConnectingProgress->setStyleSheet("QProgressBar"
                                      "{"
#ifdef WINDOWS_HIGH_DPI
                                          "border-radius: 10px;"
                                          "border: 1px;"
                                          "height: 20px;"
                                          "max-height: 20px;"
                                          "width: 230px;"
                                          "max-width: 230px;"
#else
                                         "border-radius: 5px;"
                                          "border: 1px;"
                                          "height: 10px;"
                                          "max-height: 10px;"
                                          "width: 176px;"
                                          "max-width: 176px;"
#endif
                                          "background: rgb(224, 229, 235);"
                                          "qproperty-textVisible: false;"
                                      "}"
                                      "QProgressBar:chunk"
                                      "{"
                                         "border-radius: 5px;"
                                          "border: 1px;"
                                          "background: rgb(31, 218, 129);"
                                      "}");
   pConnectingProgress->setValue(70);
   QLabel* pConnectingLabel = new QLabel(pSplashScreen);
   pConnectingLabel->setText(tr("Please wait, we are syncing with network…"));
   StatusLabel* pSyncUpLabel = new StatusLabel(pSplashScreen);
   DecentButton* pButton = new DecentButton(this);

   QString labelStyle =
   "QLabel"
   "{"
#ifdef WINDOWS_HIGH_DPI
      "font-size: 10pt;"
#else
      "font-size: 10px;"
#endif
      "color: rgb(127, 138, 158);"
   "}";
   pConnectingLabel->setStyleSheet(labelStyle);
   pSyncUpLabel->setStyleSheet(labelStyle);
   pButton->setStyleSheet(
                           "QPushButton"
                           "{"
                              "border: 0px;"
                              "background-color: rgb(31, 218, 129);"
                              "color: white;"
                              "width: 5em;"
                              "height: 1.5em;"
                              "max-width: 5em;"
                              "max-height: 1.5em;"
                           "}"
                          );
   pButton->hide();
   pButton->setText(tr("Proceed"));
   
   QGridLayout* pLayoutSplash = new QGridLayout;
   pLayoutSplash->addWidget(pConnectingProgress, 0, 0, Qt::AlignVCenter | Qt::AlignCenter);
   pLayoutSplash->addWidget(pConnectingLabel, 1, 0, Qt::AlignVCenter | Qt::AlignCenter);
   pLayoutSplash->addWidget(pSyncUpLabel, 2, 0, Qt::AlignVCenter | Qt::AlignCenter);
   pLayoutSplash->addWidget(pButton, 3, 0, Qt::AlignVCenter | Qt::AlignCenter);
   
   pLayoutSplash->setSizeConstraint(QLayout::SetFixedSize);
   pLayoutSplash->setSpacing(10);
   pLayoutSplash->setContentsMargins(0, 0, 0, 0);
   
   pSplashScreen->setLayout(pLayoutSplash);
   
   QObject::connect(&Globals::instance(), &Globals::statusShowMessage,
                    pSyncUpLabel, &StatusLabel::showMessage);
   QObject::connect(&Globals::instance(), &Globals::statusClearMessage,
                    pSyncUpLabel, &StatusLabel::clearMessage);
   
   QObject::connect(this, &MainWindow::signal_setSplashMainText,
                    pConnectingLabel, &QLabel::setText);
   QObject::connect(this, &MainWindow::signal_setSplashMainText,
                    pButton, &QWidget::show);
   
   QObject::connect(pButton, &QPushButton::clicked,
                    this, &MainWindow::CloseSplash);

   slot_stackWidgetPush(pSplashScreen);
}

void MainWindow::CloseSplash()
{
   StackLayerWidget* pLayer = nullptr;

   signal_setSplashMainText(QString());
   Globals::instance().statusShowMessage(QString());

   if (Globals::instance().getWallet().IsNew())
   {
      pLayer = new PasswordWidget(nullptr, PasswordWidget::eSetPassword);
      signal_setSplashMainText(tr("Please set a password to encrypt your wallet"));
   }
   else if (Globals::instance().getWallet().IsLocked())
   {
      pLayer = new PasswordWidget(nullptr, PasswordWidget::eUnlock);
      signal_setSplashMainText(tr("Please unlock your wallet"));
   }
   else
   {
      auto accounts = Globals::instance().runTaskParse("list_my_accounts");
      if (accounts.empty())
      {
         pLayer = new ImportKeyWidget(nullptr);
         signal_setSplashMainText(tr("Please import your account in order to proceed"));
      }
   }

   if (pLayer)
   {
      slot_stackWidgetPush(pLayer);
      QObject::connect(pLayer, &StackLayerWidget::accepted,
                       this, &MainWindow::CloseSplash);
   }
   else
   {
      slot_stackWidgetPop();

      Globals::instance().statusClearMessage();

      DisplayWalletContentGUI();
   }
}

void MainWindow::slot_connection_status_changed(Globals::ConnectionState from, Globals::ConnectionState to)
{
   if (Globals::ConnectionState::Up == to)
   {
      CloseSplash();

      _balanceUpdater.start();
      _downloadChecker.start();
   }
   else if (Globals::ConnectionState::Up != to)
   {
      _balanceUpdater.stop();
      _downloadChecker.stop();

      if (from == Globals::ConnectionState::Up)
         SetSplash();
   }
}

void MainWindow::currentUserBalanceUpdate()
{
   std::string userBalanceUpdate = Globals::instance().getCurrentUser();

   if (userBalanceUpdate.empty())
      return;
   else
      UpdateAccountBalances(userBalanceUpdate);
//      m_pCentralWidget->getSendButton()->setEnabled(true);
}

void MainWindow::slot_showPurchasedTab()
{
   GoToThisTab(4, std::string());
}

void MainWindow::slot_showTransactionsTab(std::string const& account_name)
{
   GoToThisTab(1, std::string());
   m_pCentralWidget->SetTransactionInfo(account_name);
}

void MainWindow::slot_stackWidgetPush(StackLayerWidget* pWidget)
{
   QObject::connect(pWidget, &StackLayerWidget::closed,
                    this, &MainWindow::slot_stackWidgetPop);

   QWidget* pLayer = new QWidget(nullptr);
   QGridLayout* pLayoutHolder = new QGridLayout;
   pLayoutHolder->addWidget(pWidget, 0, 0, Qt::AlignVCenter | Qt::AlignCenter);

   pLayoutHolder->setSizeConstraint(QLayout::SetFixedSize);
   pLayoutHolder->setSpacing(0);
   pLayoutHolder->setContentsMargins(0, 0, 0, 0);

   QWidget* pHolder = new QWidget(pLayer);
   pHolder->setLayout(pLayoutHolder);

   QHBoxLayout* pLayoutLayer = new QHBoxLayout;
   pLayoutLayer->addWidget(pHolder);

   pLayer->setLayout(pLayoutLayer);

   m_pStackedWidget->addWidget(pLayer);
   m_pStackedWidget->setCurrentWidget(pLayer);
}

void MainWindow::slot_stackWidgetPop()
{
   int iCount = m_pStackedWidget->count();
   if (iCount > 1)
   {
      QWidget* pWidget = m_pStackedWidget->widget(iCount - 1);
      m_pStackedWidget->removeWidget(pWidget);
      pWidget->deleteLater();
      m_pStackedWidget->setCurrentIndex(iCount - 2);
   }
}

void MainWindow::slot_updateAccountBalance(Asset const& balance)
{
   // use old function needs to be reviewed
   UpdateAccountBalances(Globals::instance().getCurrentUser());
}

void MainWindow::RunTaskImpl(std::string const& str_command, std::string& str_result)
{
   str_result = Globals::instance().runTask(str_command);
}

bool MainWindow::RunTaskParseImpl(std::string const& str_command, nlohmann::json& json_result) {
   try {
      std::string str_result;
      MainWindow::RunTaskImpl(str_command, str_result);
      json_result = json::parse(str_result);
      return true;
   } catch (const std::exception& ex) {
      json_result = json(ex.what());
   } catch (...) {
      json_result = json("Unhandled exception");
   }
   return false;
}

void MainWindow::CurrentUserChangedSlot(const QString& a_new_user)
{
   if(m_pCentralWidget->usersCombo()->count())
   {
      Globals::instance().setCurrentUser(a_new_user.toStdString());
      UpdateAccountBalances(a_new_user.toStdString());
   }
}


void MainWindow::UpdateAccountBalances(const std::string& username)
{
   auto& global_instance = gui_wallet::Globals::instance();

   json allAssets;
   std::string getAssetsCommand = "list_assets \"\" 100";
   if (!RunTaskParse(getAssetsCommand, allAssets)) {
      ALERT_DETAILS(tr("Could not get account balances").toStdString(), allAssets.get<string>().c_str());
      return;
   }


   std::string csLineToRun = "list_account_balances " + username;
   json allBalances;

   if (!RunTaskParse(csLineToRun, allBalances)) {
      ALERT_DETAILS(tr("Could not get account balances").toStdString(), allBalances.get<string>().c_str());
      return;
   }
   
   if(!allBalances.size())
   {
      m_pCentralWidget->usersCombo()->hide();
      m_pCentralWidget->importButton()->show();
   }
   else
   {
      m_pCentralWidget->importButton()->hide();
      m_pCentralWidget->usersCombo()->show();
   }
   if(!m_pCentralWidget->usersCombo()->count())
   {
      m_pCentralWidget->usersCombo()->hide();
      m_pCentralWidget->importButton()->show();
   }
   else
   {
      m_pCentralWidget->importButton()->hide();
      m_pCentralWidget->usersCombo()->show();
   }

   
   std::vector<std::string> balances;
   for (int i = 0; i < allBalances.size(); ++i) {
      
      std::string assetName = "Unknown";
      int precision = 1;

      for (int assInd = 0; assInd < allAssets.size(); ++assInd) {
         if (allAssets[assInd]["id"].get<std::string>() == allBalances[i]["asset_id"]) {
            assetName = allAssets[assInd]["symbol"].get<std::string>();
            precision = allAssets[assInd]["precision"].get<int>();
            break;
         }
      }
      
      double amount = 0;
      if (allBalances[i]["amount"].is_number()) {
         amount = allBalances[i]["amount"].get<double>();
      } else {
         amount = std::stod(allBalances[i]["amount"].get<std::string>());
      }
      amount = amount / pow(10, precision);
      
      QString str = QString::number(amount) + " " + QString::fromStdString(assetName);
      
      balances.push_back(str.toStdString());
   }
   m_pCentralWidget->SetAccountBalancesFromStrGUI(balances);
   
}

void MainWindow::CheckDownloads()
{
   auto& global_instance = gui_wallet::Globals::instance();
   std::string str_current_username = global_instance.getCurrentUser();

   if (str_current_username == "") {
      _activeDownloads.clear();
      return;
   }

   json contents;
   if (!RunTaskParse("search_my_purchases "
                     "\"" + str_current_username + "\" "
                     "\"\" "
                     "\"\" "
                     "\"\" "
                     "\"-1\" ",
                     contents))
   {
      std::cout << contents.get<string>() << std::endl;
      return;
   }


   for (int i = 0; i < contents.size(); ++i)
   {
      auto content = contents[i];
      std::string URI = contents[i]["URI"].get<std::string>();
      std::string hash = contents[i]["hash"].get<std::string>();

      if (URI == "")
         continue;

      if (_activeDownloads.find(URI) == _activeDownloads.end())
      {
         json ignore_result;
         if (RunTaskParse("download_package "
                          "\"" + URI + "\" "
                          "\"" + hash + "\" ",
                          ignore_result))
         {
            _activeDownloads.insert(URI);
         }
         else
         {
            std::cout << "Cannot resume download: " << URI << std::endl;
            std::cout << "Error: " << ignore_result.get<string>() << std::endl;
         }
      }
   }
}


void MainWindow::DisplayConnectionError(std::string errorMessage) {
   ALERT_DETAILS(tr("Could not connect to wallet").toStdString(), errorMessage.c_str());
}

void MainWindow::slot_enableSendButton()
{
   //m_pCentralWidget->getSendButton()->setEnabled(true);
}


void MainWindow::DisplayWalletContentGUI()
{
   Globals::instance().setWalletUnlocked();
   Globals::instance().getWallet().SaveWalletFile();
   QComboBox& userCombo = *m_pCentralWidget->usersCombo();

   try
   {
      std::string a_result;
      RunTask("list_my_accounts", a_result);
      userCombo.clear();
      
      auto accs = json::parse(a_result);
      
      for (int i = 0; i < accs.size(); ++i)
      {
         std::string id = accs[i]["id"].get<std::string>();
         std::string name = accs[i]["name"].get<std::string>();
         
         userCombo.addItem(tr(name.c_str()));
      }

      if (accs.size() > 0)
      {
         userCombo.setCurrentIndex(0);
         UpdateAccountBalances(userCombo.itemText(0).toStdString());
      }
   }
   catch (const std::exception& ex)
   {
      //ALERT_DETAILS("Failed to get account information", ex.what());
      QMessageBox::critical(this, "Error", QString(tr("Failed to get account information - %1")).arg(ex.what()));
   }
}


void MainWindow::ImportKeySlot()
{
   ImportKeyWidget* import_key = new ImportKeyWidget(nullptr);
   slot_stackWidgetPush(import_key);
}

void MainWindow::ReplayBlockChainSlot()
{
   Globals::instance().stopDaemons();
   Globals::instance().startDaemons(true);
}

void MainWindow::SendDCTSlot()
{
   if(!m_pCentralWidget->usersCombo()->count())
      return;
   
   DecentButton* button = (DecentButton*)sender();
   QString accountName = button->property("accountName").toString();

   Globals::instance().showTransferDialog(accountName.toStdString());
}

void MainWindow::GoToThisTab(int index , std::string)
{
    m_pCentralWidget->SetMyCurrentTabIndex(index);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
   if (Globals::instance().connected())
   {
      event->accept();
   }
   else
   {
      event->ignore();
   }
}

