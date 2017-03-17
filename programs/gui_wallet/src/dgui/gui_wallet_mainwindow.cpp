/*
 *	File: gui_wallet_mainwindow.cpp
 *
 *	Created on: Nov 11, 2016
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */


#define     CLI_WALLET_CODE         ((void*)-1)
#define     WALLET_CONNECT_CODE     ((void*)-2)


#include "gui_wallet_mainwindow.hpp"
#include "gui_wallet_global.hpp"
#include <QMenuBar>
#include <QMoveEvent>
#include "qt_commonheader.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <graphene/utilities/dirhelper.hpp>
#include <QMessageBox>
#include "json.hpp"
#include "decent_wallet_ui_gui_jsonparserqt.hpp"

#ifndef DEFAULT_WALLET_FILE_NAME
#define DEFAULT_WALLET_FILE_NAME       "wallet.json"
#endif

using namespace nlohmann;
using namespace gui_wallet;
using namespace std;
using namespace graphene;
using namespace utilities;

static gui_wallet::Mainwindow_gui_wallet*  s_pMainWindowInstance = NULL;

std::string FindImagePath(bool& a_bRet,const char* a_image_name);

int WarnAndWaitFunc(void* a_pOwner,WarnYesOrNoFuncType a_fpYesOrNo, void* a_pDataForYesOrNo,const char* a_form,...);


int CallFunctionInGuiLoop2(SetNewTask_last_args2,const std::string& a_result,void* a_owner,TypeCallbackSetNewTaskGlb2 a_fpFunc);
int CallFunctionInGuiLoop3(SetNewTask_last_args2,const fc::variant& a_result,void* owner,TypeCallbackSetNewTaskGlb3 fpFnc);


/*//////////////////////////////////////////////////////////////////////////////////*/

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
        m_ActionOpenCliWallet(tr("cli_wallet"),this),
        m_ActionOpenInfoDlg(tr("Open info dlg."),this),
        m_ConnectDlg(this),
        m_info_dialog(),
        m_locked(true),
        m_import_key_dlg(2),
        m_cqsPreviousFilter(tr("nf")),
        m_nConnected(0),
        m_SetPasswordDialog(this, true),
        m_UnlockDialog(this, false)
{
    s_pMainWindowInstance = this;
    m_default_stylesheet = styleSheet();
    //setStyleSheet("color:black;""background-color:white;");
    m_pInfoTextEdit = new QTextEdit;
    m_pInfoTextEdit->setReadOnly(true);
    m_pcInfoDlg = new CliWalletDlg(m_pInfoTextEdit);

    
    CliTextEdit* pCliTextEdit = (CliTextEdit*)m_cCliWalletDlg.operator ->();
    pCliTextEdit->SetCallbackStuff2(this,NULL,&Mainwindow_gui_wallet::CliCallbackFnc);

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
    /*mainMenuLayout0->addWidget(new QWidget);

    QWidget *central = new QWidget;
    central->setLayout(mainMenuLayout0);*/

    m_pCentralWidget = new CentralWigdet(m_pCentralAllLayout,this);
    //m_pCentralWidget->setStyleSheet("color:black;""background-color:white;");
    m_pCentralWidget->setLayout(m_pCentralAllLayout);
    setCentralWidget(m_pCentralWidget);
    CreateActions();
    CreateMenues();
    resize(900,550);

    setCentralWidget(m_pCentralWidget);

    m_info_dialog.resize(0,0);

    m_nError = 0;
    m_error_string = "";

    setUnifiedTitleAndToolBarOnMac(false);

    QComboBox* pUsersCombo = m_pCentralWidget->usersCombo();

    
    connect(pUsersCombo, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(CurrentUserChangedSlot(const QString&)) );
    m_nUserComboTriggeredInGui = 0;

    
    connect(m_pCentralWidget->GetBrowseContentTab(),
            SIGNAL(ShowDetailsOnDigContentSig(SDigitalContent)),
            this,SLOT(ShowDetailsOnDigContentSlot(SDigitalContent)));

    connect(m_pCentralWidget->GetPurchasedTab(),
            SIGNAL(ShowDetailsOnDigContentSig(SDigitalContent)),
            this,SLOT(ShowDetailsOnDigContentSlot(SDigitalContent)));

    
    
    
    setWindowTitle(tr("DECENT - Blockchain Content Distribution"));
    
    centralWidget()->layout()->setContentsMargins(0, 0, 0, 0);
    setStyleSheet("QMainWindow{color:black;""background-color:white;}");
    
    
    
    InitializeUiInterfaceOfWallet_base(&WarnAndWaitFunc,
                                       &CallFunctionInGuiLoop2,
                                       &CallFunctionInGuiLoop3, this, NULL,
                                       GetFunctionPointerAsVoid(0, &Mainwindow_gui_wallet::ManagementNewFuncGUI));
    m_nJustConnecting = 1;
    ConnectSlot();
    
    m_pdig_cont_detailsGenDlg = nullptr;
    m_pdig_cont_detailsBougDlg = nullptr;
    
    _downloadChecker.setSingleShot(false);
    _downloadChecker.setInterval(5000);
    connect(&_downloadChecker, SIGNAL(timeout()), this, SLOT(CheckDownloads()));
    _downloadChecker.start();
    
}

void Mainwindow_gui_wallet::ContentWasBoughtSlot()
{
    m_pCentralWidget->SetMyCurrentTabIndex(4);
    UpdateAccountBalances(GlobalEvents::instance().getCurrentUser());
    //  this is a demonstration of wallet_api direct usage
    try
    {
        //
        //  just enable below lines - create wallet, set password and unlock
        //  and everything gets messed up!
        //
        //m_ptr_wallet_utility = decent::wallet_utility::create_wallet_api();
        //if (m_ptr_wallet_utility->is_new())
        //    m_ptr_wallet_utility->set_password("hardcode the password");
        //m_ptr_wallet_utility->unlock("hardcode the password");
        
        /*vector<asset> arr_asset = m_ptr_wallet_utility->list_account_balances(GlobalEvents::instance().getCurrentUser());
        for (auto const& asset : arr_asset)
        {
            QMessageBox::information(this, "DCT-Satoshi", QString("%1").arg(asset.amount.value));
        }*/
    }
    catch (std::exception const& e)
    {
        QMessageBox::critical(this, "Error", e.what());
    }
    catch(...)
    {
        QMessageBox::critical(this, "Error", "...");
    }
}


Mainwindow_gui_wallet::~Mainwindow_gui_wallet()
{
    SaveWalletFile2(m_wdata2);
    DestroyUiInterfaceOfWallet();
    delete m_pInfoTextEdit;
    delete m_pcInfoDlg;
    
    if (m_pdig_cont_detailsGenDlg)
        delete m_pdig_cont_detailsGenDlg;
    if (m_pdig_cont_detailsBougDlg)
        delete m_pdig_cont_detailsBougDlg;
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

    m_ActionOpenCliWallet.setStatusTip( tr("Open CLI wallet dialog") );
    connect( &m_ActionOpenCliWallet, SIGNAL(triggered()), this, SLOT(OpenCliWalletDlgSlot()) );

    m_ActionOpenInfoDlg.setStatusTip( tr("Open Info dialog") );
    connect( &m_ActionOpenInfoDlg, SIGNAL(triggered()), this, SLOT(OpenInfoDlgSlot()) );

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

#ifndef NDEBUG
    m_pMenuDebug = pMenuBar->addMenu( tr("Debug") );
    m_pMenuDebug->addAction(&m_ActionOpenCliWallet);
    m_pMenuDebug->addAction(&m_ActionOpenInfoDlg);
    m_pMenuTempFunctions = m_pMenuDebug->addMenu(tr("temp. functions start"));
#endif


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
    if(m_nUserComboTriggeredInGui)
    {
        m_nUserComboTriggeredInGui = 0;
        return;
    }

    GlobalEvents::instance().setCurrentUser(a_new_user.toStdString());
    UpdateAccountBalances(a_new_user.toStdString());
}



void Mainwindow_gui_wallet::CliCallbackFnc(void*/*arg*/,const std::string& a_task)
{
    m_cli_line = a_task;
    SetNewTask(a_task,this,CLI_WALLET_CODE,&Mainwindow_gui_wallet::TaskDoneFuncGUI);
}


void Mainwindow_gui_wallet::OpenCliWalletDlgSlot()
{
    m_cCliWalletDlg.exec();
}


void Mainwindow_gui_wallet::OpenInfoDlgSlot()
{
    m_pcInfoDlg->exec();
}


void Mainwindow_gui_wallet::ShowDetailsOnDigContentSlot(SDigitalContent a_dig_cont)
{
    switch(a_dig_cont.type)
    {
    case DCT::GENERAL:
        if (m_pdig_cont_detailsGenDlg)
            delete m_pdig_cont_detailsGenDlg;
        m_pdig_cont_detailsGenDlg = new ContentDetailsGeneral();
        
        connect(m_pdig_cont_detailsGenDlg, SIGNAL(ContentWasBought()), this, SLOT(ContentWasBoughtSlot()));

        m_pdig_cont_detailsGenDlg->execCDD(a_dig_cont);
        break;
    case DCT::BOUGHT:
    case DCT::WAITING_DELIVERY:
        if (nullptr == m_pdig_cont_detailsBougDlg)
            delete m_pdig_cont_detailsBougDlg;
        m_pdig_cont_detailsBougDlg = new ContentDetailsBase();
        m_pdig_cont_detailsBougDlg->execCDB(a_dig_cont);
        break;
    default:
        break;
    }
}

int Mainwindow_gui_wallet::GetDigitalContentsFromVariant(DCT::DIG_CONT_TYPES a_type,
                                                         std::vector<SDigitalContent>& a_vcContents,
                                                         const fc::variant& a_contents_var)
{
    SDigitalContent aDigContent;
    JsonParserQt aParser;
    const JsonParserQt* pNext;

    a_contents_var.visit(aParser);
    const int cnSize(aParser.size());
    aDigContent.type = a_type;

    for(int i(0);i<cnSize;++i)
    {
        pNext = &(aParser.GetByIndex(i));
        aDigContent.URI = pNext->GetByKey("URI").value();
        aDigContent.author = pNext->GetByKey("author").value();
        aDigContent.price.amount = std::stod(pNext->GetByKey("price").GetByKey("amount").value());
        aDigContent.price.asset_id = pNext->GetByKey("price").GetByKey("asset_id").value();
        
        a_vcContents.push_back(aDigContent);
    }

    return 0;
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
    
    if(!m_UnlockDialog.execRD(thisPos, cvsPassword))
        return;
    
    m_ActionLock.setDisabled(true);
    m_ActionUnlock.setDisabled(true);
    
    const std::string csPassLine = "unlock " + cvsPassword;
    std::string result;
    
    try {
        RunTask(csPassLine, result);
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



void Mainwindow_gui_wallet::moveEvent(QMoveEvent * a_event)
{
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
    m_nError = 0;
    m_error_string = "";

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
    decent::gui::tools::RET_TYPE aRet = m_import_key_dlg.execRD(&thisPos,cvsUsKey);
    
    if(aRet == decent::gui::tools::RDB_CANCEL){
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




void Mainwindow_gui_wallet::TaskDoneFuncGUI(void* a_clbkArg,int64_t a_err,const std::string& a_task,const std::string& a_result)
{

    
    if(a_clbkArg == WALLET_CONNECT_CODE) {
        if(a_err)
        {
            ALERT_DETAILS("Could not connect to wallet", a_result.c_str());
            m_ConnectDlg.GetTableWidget(ConnectDlg::CONNECT_BUTTON_FIELD, 1)->setEnabled(true);
            return;
        }
        
        DisplayWalletContentGUI();
        return;
    }

    if(a_clbkArg == CLI_WALLET_CODE) {
        
        if (a_err) {
            m_cCliWalletDlg->setTextColor(Qt::red);
        }
        
        m_cCliWalletDlg.appentText(a_result);
        
        if (a_err) {
            m_cCliWalletDlg->setTextColor(Qt::black);
        }
        
        m_cCliWalletDlg.appentText(">>>");
    }
}


void Mainwindow_gui_wallet::ManagementNewFuncGUI(void* a_clbkArg,int64_t a_err,const std::string& a_task,const std::string& a_result)
{

    
}


void Mainwindow_gui_wallet::ConnectSlot()
{
    int nRet(decent::gui::tools::RDB_OK);
    m_nError = 0;
    m_error_string = "";

    LoadWalletFile(&m_wdata2);
    if(m_nJustConnecting==1)
    {
        m_nJustConnecting = 0;
    }
    else
    {
        nRet = m_ConnectDlg.execNew(&m_wdata2);
    }


    if(nRet == decent::gui::tools::RDB_CANCEL){return;}

    m_ActionConnect.setEnabled(false);
    m_wdata2.action = WAT::CONNECT;
    
    m_wdata2.setPasswordFn = +[](void*owner, int answer, void* str_ptr) {
        ((Mainwindow_gui_wallet*)owner)->SetPassword(owner, str_ptr);
    };
    
    m_wdata2.fpDone = (TypeCallbackSetNewTaskGlb2)GetFunctionPointerAsVoid(1,&Mainwindow_gui_wallet::TaskDoneFuncGUI);
    StartConnectionProcedure(&m_wdata2,this,WALLET_CONNECT_CODE);
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

    }
    
    
    
}

void Mainwindow_gui_wallet::GoToThisTab(int index , std::string info)
{
    m_pCentralWidget->SetTransactionInfo(info);
    m_pCentralWidget->SetMyCurrentTabIndex(index);
}

