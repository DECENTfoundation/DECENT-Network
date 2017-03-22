/*
 *	File: gui_wallet_mainwindow.cpp
 *
 *	Created on: Nov 11, 2016
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */



#include <QMenuBar>
#include <QMoveEvent>
#include <QMessageBox>

#include "qt_commonheader.hpp"
#include "gui_wallet_mainwindow.hpp"
#include "gui_wallet_global.hpp"

#include <stdio.h>
#include <stdlib.h>

#include <graphene/utilities/dirhelper.hpp>
#include "json.hpp"

#ifndef DEFAULT_WALLET_FILE_NAME
#define DEFAULT_WALLET_FILE_NAME       "wallet.json"
#endif

using namespace nlohmann;
using namespace gui_wallet;
using namespace std;
using namespace graphene;
using namespace utilities;

static gui_wallet::Mainwindow_gui_wallet*  s_pMainWindowInstance = NULL;



Mainwindow_gui_wallet::Mainwindow_gui_wallet()
        :
        m_ActionExit(tr("&Exit"),this),
        m_ActionConnect(tr("Connect"),this),
        m_ActionAbout(tr("About"),this),
        m_ActionInfo(tr("Info"),this),
        m_ActionHelp(tr("Help"),this),
        m_ActionLock(tr("Lock"),this),
        m_ActionUnlock(tr("Unlock"),this),
        m_ActionImportKey(tr("Import key"),this),
        m_info_dialog(),
        m_locked(true),
        m_import_key_dlg(2),
        m_nConnected(0),
        m_SetPasswordDialog(this, true),
        m_UnlockDialog(this, false)
{
    s_pMainWindowInstance = this;
   
    m_barLeft = new QMenuBar;
    m_barRight = new QMenuBar;

    m_pCentralAllLayout = new QVBoxLayout;
    m_pMenuLayout = new QHBoxLayout;
    
    fc::path wallet_path = decent_path_finder::instance().get_decent_home() / DEFAULT_WALLET_FILE_NAME;
    m_wdata2.wallet_file_name = wallet_path.string().c_str();
    
    m_wdata2.ws_server = "ws://127.0.0.1:8090";
    m_wdata2.chain_id = "0000000000000000000000000000000000000000000000000000000000000000";

    m_pMenuLayout->addWidget(m_barLeft);
    m_pMenuLayout->addWidget(m_barRight);

    m_pMenuLayout->setAlignment(m_barLeft, Qt::AlignLeft);
    m_pMenuLayout->setAlignment(m_barRight, Qt::AlignRight);

    m_pCentralAllLayout->addLayout(m_pMenuLayout);


    m_pCentralWidget = new CentralWigdet(m_pCentralAllLayout,this);
    m_pCentralWidget->setLayout(m_pCentralAllLayout);
    setCentralWidget(m_pCentralWidget);
    CreateActions();
    CreateMenues();
    resize(900,550);

    setCentralWidget(m_pCentralWidget);

    m_info_dialog.resize(0,0);

    setUnifiedTitleAndToolBarOnMac(false);

    QComboBox* pUsersCombo = m_pCentralWidget->usersCombo();

    
    connect(pUsersCombo, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(CurrentUserChangedSlot(const QString&)) );
    
    
    setWindowTitle(tr("DECENT - Blockchain Content Distribution"));
    
    centralWidget()->layout()->setContentsMargins(0, 0, 0, 0);
    setStyleSheet("QMainWindow{color:black;""background-color:white;}");
    
    
    
    WalletInterface::initialize();
    ConnectSlot();
   
    _downloadChecker.setSingleShot(false);
    _downloadChecker.setInterval(5000);
    connect(&_downloadChecker, SIGNAL(timeout()), this, SLOT(CheckDownloads()));
    _downloadChecker.start();
   
   
   connect(&GlobalEvents::instance(), SIGNAL(walletConnected()), this, SLOT(DisplayWalletContentGUI()));
   connect(&GlobalEvents::instance(), SIGNAL(walletConnectionError(std::string)), this, SLOT(DisplayConnectionError(std::string)));
   
}

Mainwindow_gui_wallet::~Mainwindow_gui_wallet() {
   
   WalletInterface::saveWalletFile(m_wdata2);
   WalletInterface::destroy();
}



void Mainwindow_gui_wallet::CreateActions()
{
    m_ActionExit.setStatusTip( tr("Exit Program") );
    connect( &m_ActionExit, SIGNAL(triggered()), this, SLOT(close()) );

    m_ActionAbout.setStatusTip( tr("About") );
    connect( &m_ActionAbout, SIGNAL(triggered()), this, SLOT(AboutSlot()) );

    m_ActionHelp.setStatusTip( tr("Help") );
    connect( &m_ActionHelp, SIGNAL(triggered()), this, SLOT(HelpSlot()) );

    m_ActionInfo.setStatusTip( tr("Info") );
    connect( &m_ActionInfo, SIGNAL(triggered()), this, SLOT(InfoSlot()) );

    m_ActionConnect.setStatusTip( tr("Connect to witness node") );
    connect( &m_ActionConnect, SIGNAL(triggered()), this, SLOT(ConnectSlot()) );

   
    m_ActionLock.setDisabled(m_locked);
    m_ActionLock.setStatusTip( tr("Lock account") );
    connect( &m_ActionLock, SIGNAL(triggered()), this, SLOT(LockSlot()) );

    m_ActionUnlock.setEnabled(m_locked);
    m_ActionUnlock.setStatusTip( tr("Unlock account") );
    connect( &m_ActionUnlock, SIGNAL(triggered()), this, SLOT(UnlockSlot()) );

    m_ActionImportKey.setDisabled(true);
    m_ActionImportKey.setStatusTip( tr("Import key") );
    connect( &m_ActionImportKey, SIGNAL(triggered()), this, SLOT(ImportKeySlot()) );


}


void Mainwindow_gui_wallet::CreateMenues()
{

    QMenuBar* pMenuBar = m_barLeft;
    m_pMenuFile = pMenuBar->addMenu( tr("&File") );
    m_pMenuFile->addAction( &m_ActionExit );
    m_pMenuFile->addAction( &m_ActionConnect );
    m_pMenuFile->addAction( &m_ActionLock );
    m_pMenuFile->addAction( &m_ActionUnlock );
    m_pMenuFile->addAction( &m_ActionImportKey );





    m_pMenuView = pMenuBar->addMenu( tr("&View") );

    QAction* browseAction = new QAction(tr("Browse Content"), this);
    m_pMenuView->addAction(browseAction);
    browseAction->setProperty("index", 0);

    QAction* transactionsAction = new QAction(tr("Transactions"), this);
    m_pMenuView->addAction(transactionsAction);
    transactionsAction->setProperty("index", 1);

    QAction* uploadAction = new QAction(tr("Upload"), this);
    m_pMenuView->addAction(uploadAction);
    uploadAction->setProperty("index", 2);

    QAction* overviewAction = new QAction(tr("Overview"), this);
    m_pMenuView->addAction(overviewAction);
    overviewAction->setProperty("index", 3);

    QAction* purchasedAction = new QAction(tr("Purchased"), this);
    m_pMenuView->addAction(purchasedAction);
    purchasedAction->setProperty("index", 4);

    connect( browseAction, SIGNAL(triggered()), this, SLOT(ViewAction()) );
    connect( transactionsAction, SIGNAL(triggered()), this, SLOT(ViewAction()) );
    connect( uploadAction, SIGNAL(triggered()), this, SLOT(ViewAction()) );
    connect( overviewAction, SIGNAL(triggered()), this, SLOT(ViewAction()) );
    connect( purchasedAction, SIGNAL(triggered()), this, SLOT(ViewAction()) );




    m_pMenuHelpL = pMenuBar->addMenu( tr("&Help") );


    /******************************************************/
    m_pMenuHelpL->addAction(&m_ActionAbout);
    m_pMenuHelpL->addAction(&m_ActionInfo);
    m_pMenuHelpL->addAction(&m_ActionHelp);
}

void Mainwindow_gui_wallet::ViewAction() {
    QAction* act = dynamic_cast<QAction*>(sender());
    int index = act->property("index").toInt();

    m_pCentralWidget->SetMyCurrentTabIndex(index);
}

void Mainwindow_gui_wallet::CurrentUserChangedSlot(const QString& a_new_user)
{
    GlobalEvents::instance().setCurrentUser(a_new_user.toStdString());
    UpdateAccountBalances(a_new_user.toStdString());
}


void Mainwindow_gui_wallet::UpdateAccountBalances(const std::string& username) {

    try {
    
        std::string assetsResult;
        std::string getAssetsCommand = "list_assets \"\" 100";
        RunTask(getAssetsCommand, assetsResult);
        
        
        std::string csLineToRun = "list_account_balances " + username;
        std::string result;
        
        RunTask(csLineToRun, result);
        
        
        auto allAssets = json::parse(assetsResult);
        auto allBalances = json::parse(result);
        
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
            
            QString str = QString::number(amount) + tr(" ") + QString::fromStdString(assetName);
            
            balances.push_back(str.toStdString());
        }
        m_pCentralWidget->SetAccountBalancesFromStrGUI(balances);
        
    } catch (const std::exception& ex) {
        ALERT_DETAILS("Could not get account balances", ex.what());
    }

}

void Mainwindow_gui_wallet::LockSlot()
{
    m_ActionLock.setDisabled(true);
    m_ActionUnlock.setDisabled(true);
    
    const std::string csLine = "lock";
    std::string dummy;
    
    try {
        RunTask(csLine, dummy);
    } catch (std::exception& ex) {
        ALERT_DETAILS("Unable to lock the wallet", ex.what());
    }
    
    UpdateLockedStatus();
    
}


void Mainwindow_gui_wallet::UnlockSlot()
{
    
    QPoint thisPos = pos();
    thisPos.rx() += size().width() / 2;
    thisPos.ry() += size().height() / 2;

    
    
    std::string cvsPassword;
    
   if(!m_UnlockDialog.execRD(thisPos, cvsPassword)) {
      UpdateLockedStatus();
      return;
   }
   
    m_ActionLock.setDisabled(true);
    m_ActionUnlock.setDisabled(true);
    
    const std::string csPassLine = "unlock " + cvsPassword;
    std::string result;
    
    try {
       RunTask(csPassLine, result);
       GlobalEvents::instance().setWalletUnlocked();
    } catch (const std::exception& ex) {
        ALERT_DETAILS("Unable to unlock the wallet", ex.what());
    }
    
    UpdateLockedStatus();
    
}


void Mainwindow_gui_wallet::UpdateLockedStatus()
{
    const std::string csLine = "is_locked";
    std::string a_result;
    
    try {
        
        RunTask(csLine, a_result);
        m_locked = (a_result == "true");
        
    } catch (const std::exception& ex) {
        ALERT_DETAILS("Unable to get wallet lock status", ex.what());
        m_locked = true;
    }
    
    
    m_ActionLock.setDisabled(m_locked);
    m_ActionUnlock.setEnabled(m_locked);
    if (m_locked) {
        UnlockSlot();
    }
}



void Mainwindow_gui_wallet::CheckDownloads()
{
    
    auto& global_instance = gui_wallet::GlobalEvents::instance();
    std::string str_current_username = global_instance.getCurrentUser();

    if (str_current_username == "") {
        _activeDownloads.clear();
        return;
    }
    
    try {
        
        std::string a_result;
        RunTask("get_buying_history_objects_by_consumer_term \"" + str_current_username +"\" \"\" ", a_result);
    
        
        auto contents = json::parse(a_result);
        for (int i = 0; i < contents.size(); ++i) {
            
            auto content = contents[i];
            std::string URI = contents[i]["URI"].get<std::string>();
            
            if (URI == "") {
                continue;
            }
            
            if (_activeDownloads.find(URI) == _activeDownloads.end()) {
                std::string ignore_string;
                try {
                    RunTask("download_package \"" + URI +"\" ", ignore_string);
                    _activeDownloads.insert(URI);
                } catch (const std::exception& ex) {
                    std::cout << "Can not resume download: " << URI << std::endl;
                    std::cout << "Error: " << ex.what() << std::endl;
                }
                
            }
            
        }
        
        
    } catch (std::exception& ex) {
        std::cout << ex.what() << std::endl;
    }
    

}


void Mainwindow_gui_wallet::DisplayConnectionError(std::string errorMessage) {
   ALERT_DETAILS("Could not connect to wallet", errorMessage.c_str());
}


void Mainwindow_gui_wallet::DisplayWalletContentGUI()
{
    m_ActionLock.setDisabled(true);
    m_ActionUnlock.setDisabled(true);
    UpdateLockedStatus();

    m_ActionImportKey.setEnabled(true);
    QComboBox& userCombo = *m_pCentralWidget->usersCombo();

    try {
        std::string a_result;
        RunTask("list_my_accounts", a_result);

        
        auto accs = json::parse(a_result);
        
        for (int i = 0; i < accs.size(); ++i) {
            std::string id = accs[i]["id"].get<std::string>();
            std::string name = accs[i]["name"].get<std::string>();
            
            userCombo.addItem(tr(name.c_str()));
        }
        
        if (accs.size() > 0)
        {
            userCombo.setCurrentIndex(0);
            UpdateAccountBalances(userCombo.itemText(0).toStdString());
        }
    } catch (const std::exception& ex) {
        ALERT_DETAILS("Faild to get account information", ex.what());
    }
}


void Mainwindow_gui_wallet::ImportKeySlot()
{

    std::vector<std::string> cvsUsKey(2);
    QComboBox& cUsersCombo = *m_pCentralWidget->usersCombo();

    cvsUsKey[0] = "";
    cvsUsKey[1] = "";

    if(cUsersCombo.count()&&(cUsersCombo.currentIndex()>0))
    {
        QString cqsUserName = cUsersCombo.currentText();
        QByteArray cbaResult = cqsUserName.toLatin1();
        cvsUsKey[0] = cbaResult.data();
    }

    QPoint thisPos = pos();
    RET_TYPE aRet = m_import_key_dlg.execRD(&thisPos,cvsUsKey);
    
    if(aRet == RDB_CANCEL){
        return ;
    }

    std::string csTaskStr = "import_key " + cvsUsKey[0] + " " + cvsUsKey[1];
    std::string result;
    bool hasError = false;
    
    try {
        RunTask(csTaskStr, result);
        hasError = result.find("exception") != std::string::npos;
    } catch (...) {
        hasError = true;
    }
    
    if (hasError) {
        ALERT_DETAILS("Can not import key.", result.c_str());
    } else {
        DisplayWalletContentGUI();

    }

    
}


void Mainwindow_gui_wallet::InfoSlot()
{
    try {
        std::string a_result;
        
        RunTask("info", a_result);
        QString aStrToDisplay(tr(a_result.c_str()));
        
        m_info_dialog.setFixedSize(600,500);
        m_info_dialog->setText(aStrToDisplay);
        m_info_dialog.exec();
    } catch (...) {
        
    }

}


void Mainwindow_gui_wallet::AboutSlot()
{
    try {
        std::string a_result;
        
        RunTask("about", a_result);
        
        m_info_dialog.setFixedSize(500,300);
        m_info_dialog->setText(tr(a_result.c_str()));
        m_info_dialog.exec();
    } catch (...) {
        
    }
    
}


void Mainwindow_gui_wallet::HelpSlot()
{
    
    try {
        std::string a_result;
        
        RunTask("help", a_result);
        
        m_info_dialog.setFixedSize(500,500);
        m_info_dialog->setText(tr(a_result.c_str()));
        m_info_dialog.exec();
    } catch (...) {
        
    }
}





void Mainwindow_gui_wallet::ConnectSlot()
{
    int nRet = RDB_OK;

    WalletInterface::loadWalletFile(&m_wdata2);

    if(nRet == RDB_CANCEL) {
       return;
    }

    m_ActionConnect.setEnabled(false);
   m_wdata2.owner = this;
    m_wdata2.setPasswordFn = +[](void*owner, int answer, void* str_ptr) {
        ((Mainwindow_gui_wallet*)owner)->SetPassword(owner, str_ptr);
    };
   
   WalletInterface::startConnecting(&m_wdata2);
}


void Mainwindow_gui_wallet::SetPassword(void* a_owner, void* a_str_ptr)
{
    std::string* pcsPassword = (std::string*)a_str_ptr;
    *pcsPassword = "";


    Mainwindow_gui_wallet* pThisCon = (Mainwindow_gui_wallet*)a_owner;
    PasswordDialog* pThis = &pThisCon->m_SetPasswordDialog;
    
    QPoint thisPos = pThisCon->pos();
    thisPos.rx() += this->size().width() / 2;
    thisPos.ry() += this->size().height() / 2;

    if (pThis->execRD(thisPos, *pcsPassword)) {
        m_ActionImportKey.setEnabled(true);
        m_ActionUnlock.setEnabled(false);
        m_ActionLock.setEnabled(true);
       GlobalEvents::instance().setWalletUnlocked();
    }
    
    
    
}

void Mainwindow_gui_wallet::GoToThisTab(int index , std::string info)
{
    m_pCentralWidget->SetTransactionInfo(info);
    m_pCentralWidget->SetMyCurrentTabIndex(index);
}

