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
#include <QComboBox>
#include <QStyleFactory>
#include <QTimer>
#include <QButtonGroup>
#endif

#include "mainwindow.hpp"
#include "gui_design.hpp"
#include "decent_label.hpp"
#include "decent_button.hpp"
#include "decent_line_edit.hpp"
#include "richdialog.hpp"
#include "browse_content_tab.hpp"
#include "transactions_tab.hpp"
#include "upload_tab.hpp"
#include "overview_tab.hpp"
#include "purchased_tab.hpp"

#include "gui_wallet_centralwidget.hpp"

#include "json.hpp"

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
, m_pTimerDownloads(new QTimer(this))
, m_pTimerBalance(new QTimer(this))
, m_pTimerContents(new QTimer(this))
, m_pStackedWidget(new QStackedWidget(this))
, m_pAccountList(nullptr)
, m_pBalance(nullptr)
, m_pPreviousPage(nullptr)
, m_pResetPage(nullptr)
, m_pNextPage(nullptr)
, m_pFilterBrowse(nullptr)
, m_pFilterTransactions(nullptr)
, m_pFilterPublish(nullptr)
, m_pFilterUsers(nullptr)
, m_pFilterPurchased(nullptr)
, m_pPublish(nullptr)
, m_pTabBrowse(nullptr)
, m_pTabTransactions(nullptr)
, m_pTabPublish(nullptr)
, m_pTabUsers(nullptr)
, m_pTabPurchased(nullptr)
, m_pActionImportKey(new QAction(tr("Import key"), this))
, m_pActionReplayBlockchain(new QAction(tr("Replay Blockchain"), this))
{
   setWindowTitle(tr("DECENT - Blockchain Content Distribution"));

   QWidget* pContainerWidget = new QWidget(this);
   QMenuBar* pMenuBar = new QMenuBar(pContainerWidget);
   QWidget* pMainWidget = new QWidget(pContainerWidget);
   //
   // 1st row controls
   //
   DecentLabel* pDecentLogo = new DecentLabel(pMainWidget, DecentLabel::DecentLogo);
   DecentLabel* pAccount = new DecentLabel(pMainWidget, DecentLabel::Account);
   DecentLabel* pRow1Spacer = new DecentLabel(pMainWidget, DecentLabel::Row1Spacer);
   m_pAccountList = new QComboBox(pMainWidget);
   m_pAccountList->setStyle(QStyleFactory::create("fusion"));
   m_pBalance = new DecentLabel(pMainWidget, DecentLabel::Balance);
   DecentButton* pTransferButton = new DecentButton(pMainWidget, DecentButton::Send);
   //
   // 2nd row controls
   //
   m_pButtonBrowse = new DecentButton(pMainWidget, DecentButton::TabChoice);
   m_pButtonBrowse->setText(tr("Browse Content"));
   m_pButtonBrowse->setCheckable(true);
   m_pButtonBrowse->setChecked(true);

   m_pButtonTransactions = new DecentButton(pMainWidget, DecentButton::TabChoice);
   m_pButtonTransactions->setText(tr("Transactions"));
   m_pButtonTransactions->setCheckable(true);

   m_pButtonPublish = new DecentButton(pMainWidget, DecentButton::TabChoice);
   m_pButtonPublish->setText(tr("Publish"));
   m_pButtonPublish->setCheckable(true);

   m_pButtonUsers = new DecentButton(pMainWidget, DecentButton::TabChoice);
   m_pButtonUsers->setText(tr("Users"));
   m_pButtonUsers->setCheckable(true);

   m_pButtonPurchased = new DecentButton(pMainWidget, DecentButton::TabChoice);
   m_pButtonPurchased->setText(tr("Purchased"));
   m_pButtonPurchased->setCheckable(true);

   QButtonGroup* pGroup = new QButtonGroup(pMainWidget);
   pGroup->addButton(m_pButtonBrowse);
   pGroup->addButton(m_pButtonTransactions);
   pGroup->addButton(m_pButtonPublish);
   pGroup->addButton(m_pButtonUsers);
   pGroup->addButton(m_pButtonPurchased);
   //
   // 3rd row controls
   //
   m_pPreviousPage = new DecentButton(pMainWidget, DecentButton::DialogTextButton);
   m_pPreviousPage->setText(tr("Previous"));
   m_pResetPage = new DecentButton(pMainWidget, DecentButton::DialogTextButton);
   m_pResetPage->setText(tr("Reset"));
   m_pNextPage = new DecentButton(pMainWidget, DecentButton::DialogTextButton);
   m_pNextPage->setText(tr("Next"));

   DecentLabel* pSearchLabel = new DecentLabel(pMainWidget, DecentLabel::TableSearch);

   m_pFilterBrowse = new DecentLineEdit(pMainWidget, DecentLineEdit::TableSearch);
   m_pFilterBrowse->setAttribute(Qt::WA_MacShowFocusRect, 0);
   m_pFilterBrowse->setPlaceholderText(tr("Content path"));
   m_pFilterTransactions = new DecentLineEdit(pMainWidget, DecentLineEdit::TableSearch);
   m_pFilterTransactions->setAttribute(Qt::WA_MacShowFocusRect, 0);
   m_pFilterTransactions->setPlaceholderText(tr("Enter user name to see transaction history"));
   m_pFilterTransactions->hide();
   m_pFilterPublish = new DecentLineEdit(pMainWidget, DecentLineEdit::TableSearch);
   m_pFilterPublish->setAttribute(Qt::WA_MacShowFocusRect, 0);
   m_pFilterPublish->setPlaceholderText(tr("Search Content"));
   m_pFilterPublish->hide();
   m_pFilterUsers = new DecentLineEdit(pMainWidget, DecentLineEdit::TableSearch);
   m_pFilterUsers->setAttribute(Qt::WA_MacShowFocusRect, 0);
   m_pFilterUsers->setPlaceholderText(tr("Search"));
   m_pFilterUsers->hide();
   m_pFilterPurchased = new DecentLineEdit(pMainWidget, DecentLineEdit::TableSearch);
   m_pFilterPurchased->setAttribute(Qt::WA_MacShowFocusRect, 0);
   m_pFilterPurchased->setPlaceholderText(tr("Search Content"));
   m_pFilterPurchased->hide();

   m_pPublish = new DecentButton(pMainWidget, DecentButton::DialogAction);
   m_pPublish->setText(tr("Publish"));
   m_pPublish->hide();
   //
   // 4th row controls
   //
   m_pTabBrowse = new BrowseContentTab(pMainWidget, m_pFilterBrowse);
   m_pTabBrowse->show();
   m_pTabTransactions = new TransactionsTab(pMainWidget, m_pFilterTransactions);
   m_pTabTransactions->hide();
   m_pTabPublish = new Upload_tab(pMainWidget, m_pFilterPublish, m_pPublish);
   m_pTabPublish->hide();
   m_pTabUsers = new Overview_tab(pMainWidget, m_pFilterUsers);
   m_pTabUsers->hide();
   m_pTabPurchased = new PurchasedTab(pMainWidget, m_pFilterPurchased);
   m_pTabPurchased->hide();
   //
   // 1st row layout
   //
   QHBoxLayout* pSpacerLayout = new QHBoxLayout;
   pSpacerLayout->addWidget(m_pAccountList, Qt::AlignLeft);
   pSpacerLayout->addStretch();
   pRow1Spacer->setLayout(pSpacerLayout);
   pSpacerLayout->setSpacing(0);
   pSpacerLayout->setContentsMargins(0, 0, 0, 0);

   QHBoxLayout* pRow1Layout = new QHBoxLayout;
   pRow1Layout->addWidget(pDecentLogo, Qt::AlignLeft);
   pRow1Layout->addWidget(pAccount, Qt::AlignLeft);
   pRow1Layout->addWidget(pRow1Spacer, Qt::AlignLeft);
   pRow1Layout->addWidget(m_pBalance, Qt::AlignRight);
   pRow1Layout->addWidget(pTransferButton, Qt::AlignRight);
   //
   // 2nd row layout
   //
   QHBoxLayout* pRow2Layout = new QHBoxLayout;
   pRow2Layout->addWidget(m_pButtonBrowse);
   pRow2Layout->addWidget(m_pButtonTransactions);
   pRow2Layout->addWidget(m_pButtonPublish);
   pRow2Layout->addWidget(m_pButtonUsers);
   pRow2Layout->addWidget(m_pButtonPurchased);
   //
   // 3rd row layout
   //
   QHBoxLayout* pRow3Layout = new QHBoxLayout;
   pRow3Layout->addWidget(m_pPreviousPage);
   pRow3Layout->addWidget(m_pResetPage);
   pRow3Layout->addWidget(m_pNextPage);
   pRow3Layout->addWidget(pSearchLabel);
   pRow3Layout->addWidget(m_pFilterBrowse);
   pRow3Layout->addWidget(m_pFilterTransactions);
   pRow3Layout->addWidget(m_pFilterPublish);
   pRow3Layout->addWidget(m_pFilterUsers);
   pRow3Layout->addWidget(m_pFilterPurchased);
   pRow3Layout->addWidget(m_pPublish);
   pRow3Layout->setContentsMargins(5, 0, 5, 0);
   //
   // 4th row layout
   //
   QHBoxLayout* pRow4Layout = new QHBoxLayout;
   pRow4Layout->addWidget(m_pTabBrowse);
   pRow4Layout->addWidget(m_pTabTransactions);
   pRow4Layout->addWidget(m_pTabPublish);
   pRow4Layout->addWidget(m_pTabUsers);
   pRow4Layout->addWidget(m_pTabPurchased);
   pRow4Layout->setSpacing(0);
   pRow4Layout->setContentsMargins(5, 0, 5, 5);
   //
   //
   QVBoxLayout* pMainLayout = new QVBoxLayout;
   pMainLayout->setContentsMargins(0, 0, 0, 0);
   pMainLayout->setSpacing(0);

   pMainLayout->addLayout(pRow1Layout, Qt::AlignLeft);
   pMainLayout->addLayout(pRow2Layout, Qt::AlignLeft);
   pMainLayout->addLayout(pRow3Layout, Qt::AlignLeft);
   pMainLayout->addLayout(pRow4Layout, Qt::AlignLeft);
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
   slot_setSplash();

   setUnifiedTitleAndToolBarOnMac(false);

   QObject::connect(m_pAccountList, (void(QComboBox::*)(QString const&))&QComboBox::currentIndexChanged,
                    &Globals::instance(), &Globals::slot_setCurrentUser);
   QObject::connect(m_pAccountList, (void(QComboBox::*)(QString const&))&QComboBox::currentIndexChanged,
                    this, &MainWindow::slot_getContents);
   QObject::connect(pTransferButton, &QPushButton::clicked,
                    &Globals::instance(), (void(Globals::*)())&Globals::slot_showTransferDialog);

   QObject::connect(m_pButtonBrowse, &QPushButton::toggled,
                    this, &MainWindow::slot_BrowseToggled);
   QObject::connect(m_pButtonTransactions, &QPushButton::toggled,
                    this, &MainWindow::slot_TransactionsToggled);
   QObject::connect(m_pButtonPublish, &QPushButton::toggled,
                    this, &MainWindow::slot_PublishToggled);
   QObject::connect(m_pButtonUsers, &QPushButton::toggled,
                    this, &MainWindow::slot_UsersToggled);
   QObject::connect(m_pButtonPurchased, &QPushButton::toggled,
                    this, &MainWindow::slot_PurchasedToggled);

   QObject::connect(m_pPreviousPage, &QPushButton::clicked,
                     this, &MainWindow::slot_PreviousPage);
   QObject::connect(m_pResetPage, &QPushButton::clicked,
                     this, &MainWindow::slot_ResetPage);
   QObject::connect(m_pNextPage, &QPushButton::clicked,
                     this, &MainWindow::slot_NextPage);
   
   {
      QAction* pActionExit = new QAction(tr("&Exit"), this);

      pActionExit->setStatusTip(tr("Exit Program"));
      m_pActionImportKey->setStatusTip(tr("Import key"));

      QObject::connect(pActionExit, &QAction::triggered,
                       this, &QMainWindow::close);

      QObject::connect(m_pActionImportKey, &QAction::triggered,
                       this, &MainWindow::slot_importKey);

      QObject::connect(m_pActionReplayBlockchain, &QAction::triggered,
                       this, &MainWindow::slot_replayBlockChain);

      QMenu* pMenuFile = pMenuBar->addMenu(tr("&File"));
      pMenuFile->addAction(pActionExit);
      pMenuFile->addAction(m_pActionImportKey);
      pMenuFile->addAction(m_pActionReplayBlockchain);
   }


   QObject::connect(&Globals::instance(), &Globals::walletConnectionStatusChanged,
                    this, &MainWindow::slot_connectionStatusChanged);

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


   m_pTimerBalance->setInterval(10000);
   QObject::connect(m_pTimerBalance, &QTimer::timeout,
                    &Globals::instance(), &Globals::slot_updateAccountBalance);


   m_pTimerDownloads->setInterval(5000);
   QObject::connect(m_pTimerDownloads, &QTimer::timeout,
                    this, &MainWindow::slot_checkDownloads);

   m_pTimerContents->setInterval(1000);
   QObject::connect(m_pTimerContents, &QTimer::timeout,
                    this, &MainWindow::slot_getContents);

   resize(900, 600);


#ifdef _MSC_VER
    int height = style()->pixelMetric(QStyle::PM_TitleBarHeight);
    setWindowIcon(height > 32 ? QIcon(":/icon/images/windows_decent_icon_32x32.png")
         : QIcon(":/icon/images/windows_decent_icon_16x16.png"));
#endif
}

MainWindow::~MainWindow()
{
}

void MainWindow::slot_setSplash()
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
                    this, &MainWindow::slot_closeSplash);

   slot_stackWidgetPush(pSplashScreen);

   m_pTimerBalance->stop();
   m_pTimerDownloads->stop();
   m_pTimerContents->stop();

   m_pActionImportKey->setDisabled(true);
   m_pActionReplayBlockchain->setDisabled(true);
}

void MainWindow::slot_closeSplash()
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
                       this, &MainWindow::slot_closeSplash);
   }
   else
   {
      slot_stackWidgetPop();

      Globals::instance().statusClearMessage();

      DisplayWalletContentGUI();

      m_pTimerBalance->start();
      m_pTimerDownloads->start();
      m_pTimerContents->start();

      m_pActionImportKey->setEnabled(true);
      m_pActionReplayBlockchain->setEnabled(true);

      Globals::instance().slot_updateAccountBalance();
      slot_checkDownloads();
      slot_getContents();
   }
}

void MainWindow::slot_connectionStatusChanged(Globals::ConnectionState from, Globals::ConnectionState to)
{
   if (Globals::ConnectionState::Up == to)
   {
      slot_closeSplash();
   }
   else if (Globals::ConnectionState::Up != to)
   {
      if (from == Globals::ConnectionState::Up)
         slot_setSplash();
   }
}

void MainWindow::slot_showPurchasedTab()
{
   m_pButtonPurchased->setChecked(true);
   slot_getContents();
}

void MainWindow::slot_showTransactionsTab(std::string const& account_name)
{
   m_pButtonTransactions->setChecked(true);
   m_pFilterTransactions->setText(account_name.c_str());
   slot_getContents();
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
   m_pBalance->setText(balance.getStringBalance().c_str());
}

void MainWindow::slot_replayBlockChain()
{
   Globals::instance().stopDaemons();
   Globals::instance().startDaemons(true);
}

void MainWindow::slot_importKey()
{
   ImportKeyWidget* import_key = new ImportKeyWidget(nullptr);
   slot_stackWidgetPush(import_key);
}

void MainWindow::slot_BrowseToggled(bool toggled)
{
   QWidget* pSender = qobject_cast<QWidget*>(sender());
   //
   // really a stupid hack to have the state change visible
   pSender->setEnabled(false);
   pSender->setEnabled(true);
   //
   if (toggled)
   {
      m_pFilterBrowse->show();
      m_pTabBrowse->show();
      slot_getContents();
   }
   else
   {
      m_pFilterBrowse->hide();
      m_pTabBrowse->hide();
   }
}

void MainWindow::slot_TransactionsToggled(bool toggled)
{
   QWidget* pSender = qobject_cast<QWidget*>(sender());
   //
   // really a stupid hack to have the state change visible
   pSender->setEnabled(false);
   pSender->setEnabled(true);
   //
   if (toggled)
   {
      m_pFilterTransactions->show();
      m_pTabTransactions->show();
      slot_getContents();
   }
   else
   {
      m_pFilterTransactions->hide();
      m_pTabTransactions->hide();
   }
}

void MainWindow::slot_PublishToggled(bool toggled)
{
   QWidget* pSender = qobject_cast<QWidget*>(sender());
   //
   // really a stupid hack to have the state change visible
   pSender->setEnabled(false);
   pSender->setEnabled(true);
   //
   if (toggled)
   {
      m_pFilterPublish->show();
      m_pTabPublish->show();
      m_pPublish->show();
      slot_getContents();
   }
   else
   {
      m_pFilterPublish->hide();
      m_pTabPublish->hide();
      m_pPublish->hide();
   }
}

void MainWindow::slot_UsersToggled(bool toggled)
{
   QWidget* pSender = qobject_cast<QWidget*>(sender());
   //
   // really a stupid hack to have the state change visible
   pSender->setEnabled(false);
   pSender->setEnabled(true);
   //
   if (toggled)
   {
      m_pFilterUsers->show();
      m_pTabUsers->show();
      slot_getContents();
   }
   else
   {
      m_pFilterUsers->hide();
      m_pTabUsers->hide();
   }
}

void MainWindow::slot_PurchasedToggled(bool toggled)
{
   QWidget* pSender = qobject_cast<QWidget*>(sender());
   //
   // really a stupid hack to have the state change visible
   pSender->setEnabled(false);
   pSender->setEnabled(true);
   //
   if (toggled)
   {
      m_pFilterPurchased->show();
      m_pTabPurchased->show();
      slot_getContents();
   }
   else
   {
      m_pFilterPurchased->hide();
      m_pTabPurchased->hide();
   }
}

void MainWindow::slot_checkDownloads()
{
   auto& global_instance = gui_wallet::Globals::instance();
   std::string str_current_username = global_instance.getCurrentUser();

   if (str_current_username.empty())
   {
      m_activeDownloads.clear();
      return;
   }

   json contents;

   try
   {
      contents = Globals::instance().runTaskParse("search_my_purchases "
                                                  "\"" + str_current_username + "\" "
                                                  "\"\" "
                                                  "\"\" "
                                                  "\"\" "
                                                  "\"-1\" ");
   }
   catch(...)
   {
      return;
   }

   for (size_t i = 0; i < contents.size(); ++i)
   {
      auto const& content = contents[i];
      std::string URI = contents[i]["URI"].get<std::string>();
      std::string hash = contents[i]["hash"].get<std::string>();

      if (URI.empty())
         continue;

      if (m_activeDownloads.find(URI) == m_activeDownloads.end())
      {
         try
         {
            Globals::instance().runTask("download_package "
                                        "\"" + URI + "\" "
                                        "\"" + hash + "\" ");

            m_activeDownloads.insert(URI);
         }
         catch(...)
         {
            std::cout << "Cannot resume download: " << URI << std::endl;
         }
      }
   }
}

void MainWindow::slot_getContents()
{
   TabContentManager* pTab = activeTable();

   if (pTab)
   {
      pTab->m_i_page_size = pTab->size().height() / 35;

      pTab->tryToUpdate();

      if (pTab->is_first())
      {
         m_pPreviousPage->setDisabled(true);
         m_pResetPage->setDisabled(true);
      }
      else
      {
         m_pPreviousPage->setEnabled(true);
         m_pResetPage->setEnabled(true);
      }
      if (pTab->is_last())
         m_pNextPage->setDisabled(true);
      else
         m_pNextPage->setEnabled(true);
   }
}

void MainWindow::slot_PreviousPage()
{
   TabContentManager* pTab = activeTable();

   if (pTab)
   {
      pTab->previous();
      pTab->tryToUpdate();
   }
}

void MainWindow::slot_ResetPage()
{
   TabContentManager* pTab = activeTable();

   if (pTab)
   {
      pTab->reset();
      pTab->tryToUpdate();
   }
}

void MainWindow::slot_NextPage()
{
   TabContentManager* pTab = activeTable();

   if (pTab)
   {
      pTab->next();
      pTab->tryToUpdate();
   }
}

TabContentManager* MainWindow::activeTable() const
{
   TabContentManager* pTab = nullptr;

   if (m_pTabBrowse->isVisible())
      pTab = m_pTabBrowse;
   else if (m_pTabTransactions->isVisible())
      pTab = m_pTabTransactions;
   else if (m_pTabPublish->isVisible())
      pTab = m_pTabPublish;
   else if (m_pTabUsers->isVisible())
      pTab = m_pTabUsers;
   else if (m_pTabPurchased->isVisible())
      pTab = m_pTabPurchased;

   return pTab;
}

void MainWindow::DisplayWalletContentGUI()
{
   Globals::instance().setWalletUnlocked();
   Globals::instance().getWallet().SaveWalletFile();

   try
   {
      auto accs = Globals::instance().runTaskParse("list_my_accounts");
      m_pAccountList->clear();
      
      for (int i = 0; i < accs.size(); ++i)
      {
         std::string id = accs[i]["id"].get<std::string>();
         std::string name = accs[i]["name"].get<std::string>();

         m_pAccountList->addItem(name.c_str());
      }

      if (accs.size() > 0)
      {
         m_pAccountList->setCurrentIndex(0);
      }
   }
   catch (const std::exception& ex)
   {
      //ALERT_DETAILS("Failed to get account information", ex.what());
      QMessageBox::critical(this, "Error", QString(tr("Failed to get account information - %1")).arg(ex.what()));
   }
}


void MainWindow::closeEvent(QCloseEvent* event)
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

