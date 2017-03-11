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

#define __INFO__ 0
#define __WARN__ 1
#define __ERRR__ 2

#include "gui_wallet_mainwindow.hpp"
#include "gui_wallet_global.hpp"
#include <QMenuBar>
//#include "connected_api_instance.hpp"
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



static bool FindStringByKey(const char* a_cpcInput, const char* a_key, std::string* a_pToFind)
{
    std::string& csToParse = *a_pToFind;
    const char *cpcStrBegin, *cpcStrEnd, *cpcValueFld = strstr(a_cpcInput,a_key);

    if(!cpcValueFld++){return false;}
    cpcValueFld = strchr(cpcValueFld,':');
    if(!cpcValueFld++){return false;}
    cpcStrBegin = strchr(cpcValueFld,'\"');
    if(!cpcStrBegin++){return false;}
    cpcStrEnd = strchr(cpcStrBegin,'\"');
    if(!cpcStrEnd){return false;}
    csToParse = std::string(cpcStrBegin,((size_t)cpcStrEnd)-((size_t)cpcStrBegin));
    return true;


}

typedef const char* TypeConstChar;

static bool GetJsonVectorNextElem(const char* a_cpcJsonStr,TypeConstChar* a_beg, TypeConstChar* a_end)
{
    const char* cpcNext = *a_beg = strchr(a_cpcJsonStr,'{');

    if(!(*a_beg)){return false;}

    int nOpen(1), nClose(0);

    while(nOpen>nClose)
    {
        cpcNext = strpbrk(++cpcNext,"{}");
        if(!cpcNext){break;}
        switch(cpcNext[0])
        {
        case '{':++nOpen;break;
        default:++nClose;break;
        }
    }

    *a_end = cpcNext;

    return (nOpen<=nClose);
}




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
        m_SetPasswordDialog(true)
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

}

void Mainwindow_gui_wallet::ContentWasBoughtSlot() {
    
    m_pCentralWidget->SetMyCurrentTabIndex(4);
    
    std::string csUserName = GlobalEvents::instance().getCurrentUser();
    std::string csLineToRun = "list_account_balances " + csUserName;
    SetNewTask(csLineToRun,this,NULL,&Mainwindow_gui_wallet::TaskDoneFuncGUI);
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

    std::string csUserName = StringFromQString(a_new_user);
    std::string csLineToRun = "list_account_balances " + csUserName;
    SetNewTask(csLineToRun,this,NULL,&Mainwindow_gui_wallet::TaskDoneFuncGUI);
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

void Mainwindow_gui_wallet::LockSlot()
{
    m_ActionLock.setDisabled(true);
    m_ActionUnlock.setDisabled(true);
    const std::string csLine = "lock";
    SetNewTask(csLine, this, NULL, &Mainwindow_gui_wallet::TaskDoneFuncGUI);
}


void Mainwindow_gui_wallet::UnlockSlot()
{
    
    QPoint thisPos = pos();
    thisPos.rx() += size().width() / 2;
    thisPos.ry() += size().height() / 2;

    
    
    std::string cvsPassword;
    bool rtRet = m_UnlockDialog.execRD(thisPos, cvsPassword);
    if(rtRet) {
        m_ActionLock.setDisabled(true);
        m_ActionUnlock.setDisabled(true);
        const std::string csPassLine = "unlock " + cvsPassword;
        SetNewTask(csPassLine, this, NULL, &Mainwindow_gui_wallet::TaskDoneFuncGUI);
    }
}


void Mainwindow_gui_wallet::UpdateLockedStatus()
{
    const std::string csLine = "is_locked";
    SetNewTask(csLine, this, NULL, &Mainwindow_gui_wallet::TaskDoneFuncGUI);
}



void Mainwindow_gui_wallet::ResumeDownloads()
{
    
    auto& global_instance = gui_wallet::GlobalEvents::instance();
    std::string str_current_username = global_instance.getCurrentUser();

    std::string a_result;
    RunTask("get_buying_history_objects_by_consumer_term \"" + str_current_username +"\" \"\" ", a_result);
    
    try {
        auto contents = json::parse(a_result);
        for (int i = 0; i < contents.size(); ++i) {
            
            auto content = contents[i];
            std::string URI = contents[i]["URI"].get<std::string>();
            std::string ignore_string;
            RunTask("download_package \"" + URI +"\" ", ignore_string);            
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
    SetNewTask("list_my_accounts",this,NULL,&Mainwindow_gui_wallet::TaskDoneFuncGUI);
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
    SetNewTask(csTaskStr,this,NULL,&Mainwindow_gui_wallet::TaskDoneFuncGUI);
}


void Mainwindow_gui_wallet::InfoSlot()
{
    SetNewTask("info",this,NULL,&Mainwindow_gui_wallet::TaskDoneFuncGUI);
}


void Mainwindow_gui_wallet::AboutSlot()
{
    SetNewTask("about",this,NULL,&Mainwindow_gui_wallet::TaskDoneFuncGUI);
}


void Mainwindow_gui_wallet::HelpSlot()
{
    SetNewTask("help",this,NULL,&Mainwindow_gui_wallet::TaskDoneFuncGUI);
}




void Mainwindow_gui_wallet::TaskDoneFuncGUI(void* a_clbkArg,int64_t a_err,const std::string& a_task,const std::string& a_result)
{
    const char* cpcOccur;

    __DEBUG_APP2__(1,"just_conn=%d, err=%d, a_clbkArg=%p, task=%s, result=%s\n",
                   m_nJustConnecting,(int)a_err,a_clbkArg,a_task.c_str(),a_result.c_str());

    //m_nJustConnecting = 0;

    if(a_clbkArg == CLI_WALLET_CODE)
    {
        if(a_err)
        {
            m_cCliWalletDlg->setTextColor(Qt::red);
        }
        else
        {
            //
        }
    }
    else if(a_clbkArg == WALLET_CONNECT_CODE)
    {
        if(a_err)
        {
            QMessageBox aMessageBox(QMessageBox::Critical,
                                    QObject::tr("error"),QObject::tr(a_task.c_str()),
                                    QMessageBox::Ok,this);
            aMessageBox.setDetailedText(QObject::tr(a_result.c_str()));
            aMessageBox.exec();
            m_ConnectDlg.GetTableWidget(ConnectDlg::CONNECT_BUTTON_FIELD, 1)->setEnabled(true);
        }
        else
        {
            DisplayWalletContentGUI();
        }
        return;
    }



    if(strstr(a_task.c_str(),__CONNECTION_CLB_) == a_task.c_str()){__DEBUG_APP2__(0,"this should not work!");}
    else if( strstr(a_task.c_str(),"info") == a_task.c_str())
    {
        QString aStrToDisplay(tr(a_task.c_str()));

        aStrToDisplay += tr("(err=");
        aStrToDisplay += QString::number(a_err,10);
        aStrToDisplay += tr(")\n");
        aStrToDisplay += tr(a_result.c_str());

        m_info_dialog.setFixedSize(600,500);
        m_info_dialog->setText(aStrToDisplay);
        m_info_dialog.exec();
    }
    else if(strstr(a_task.c_str(),"about") == a_task.c_str())
    {
        m_info_dialog.setFixedSize(500,300);
        m_info_dialog->setText(tr(a_result.c_str()));
        m_info_dialog.exec();
    }
    else if(strstr(a_task.c_str(),"help") == a_task.c_str())
    {
        m_info_dialog.setMaximumSize(QSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX));
        m_info_dialog.resize(500,500);
        m_info_dialog->setText(tr(a_result.c_str()));
        m_info_dialog.exec();
    }
    else if(strstr(a_task.c_str(),"list_my_accounts") == a_task.c_str())
    {
        m_user_ids.clear();
        std::string sId;
        int nNumbOfUsers(0);
        std::string csUserName;
        QComboBox& userCombo = *m_pCentralWidget->usersCombo();
        const char *cpcBegin, *cpcEnd, *cpcNextUser(a_result.c_str());

        __DEBUG_APP2__(1,"res=\n%s\n",a_result.c_str());
        userCombo.clear();

        while(GetJsonVectorNextElem(cpcNextUser,&cpcBegin,&cpcEnd))
        {
            if(FindStringByKey(++cpcBegin,"id",&sId))
            {
                m_user_ids.push_back(sId);
            }
            if(FindStringByKey(++cpcBegin,"name",&csUserName))
            {
                ++nNumbOfUsers;
                userCombo.addItem(tr(csUserName.c_str()));
            }
            cpcNextUser = cpcEnd+1;
        }
        if(nNumbOfUsers)
        {
            std::string newLine = std::string("list_account_balances ") + StringFromQString(userCombo.itemText(0));
            userCombo.setCurrentIndex(0);
            SetNewTask(newLine,this,NULL,&Mainwindow_gui_wallet::TaskDoneFuncGUI);
        }
    }
    else if((cpcOccur=strstr(a_task.c_str(),"list_account_balances ")) == a_task.c_str())
    {
        __DEBUG_APP2__(2,"%s",a_result.c_str());
        QComboBox& userCombo = *m_pCentralWidget->usersCombo();
        std::vector<std::string> cvAccountBalances;
        std::string aAcoountBalanceStr;
        const char* cpcFirstStart = a_result.c_str();
        const char* cpcFirstEnd;

        do
        {
            cpcFirstEnd = strchr(cpcFirstStart,'\n');
            if(cpcFirstEnd){aAcoountBalanceStr = std::string(cpcFirstStart,((size_t)cpcFirstEnd)-((size_t)cpcFirstStart));}
            else{aAcoountBalanceStr = std::string(cpcFirstStart);}
            if(aAcoountBalanceStr.length()){cvAccountBalances.push_back(aAcoountBalanceStr);}
            cpcFirstStart = cpcFirstEnd+1;
        }
        while(cpcFirstEnd);

        m_pCentralWidget->SetAccountBalancesFromStrGUI(cvAccountBalances);
        cpcOccur += strlen("list_account_balances ");
        for(;*cpcOccur && *cpcOccur==' ';++cpcOccur);
        int nIndexBefore = userCombo.currentIndex();
        int nIndex = userCombo.findText(tr(cpcOccur));
        __DEBUG_APP2__(2,"cpcOccur=%s, index=%d",cpcOccur,nIndex);
        if(nIndexBefore != nIndex)
        {
            m_nUserComboTriggeredInGui = 1;
            userCombo.setCurrentIndex(nIndex);
        }

    }
    
    else if(strstr(a_task.c_str(),"import_key ") == a_task.c_str())
    {
        if (a_result.find("exception") != std::string::npos) {
            QMessageBox aMessageBox(QMessageBox::Critical,
                                    QObject::tr("Error"),
                                    QObject::tr("Can not import key."),
                                    QMessageBox::Ok,
                                    this);
            aMessageBox.setDetailedText(QObject::tr(a_result.c_str()));
            aMessageBox.exec();
        }
        DisplayWalletContentGUI();
    }
    else if(strstr(a_task.c_str(),"unlock ") == a_task.c_str())
    {
        if (a_err) {
            QMessageBox aMessageBox(QMessageBox::Critical,
                                    QObject::tr("Error"),
                                    QObject::tr("Unable to unlock the wallet."),
                                    QMessageBox::Ok,
                                    this);
            aMessageBox.setDetailedText(QObject::tr(a_result.c_str()));
            aMessageBox.exec();
        }
        UpdateLockedStatus();
        
        ResumeDownloads();
    }
    else if(strstr(a_task.c_str(),"lock") == a_task.c_str())
    {
        if (a_err) {
            QMessageBox aMessageBox(QMessageBox::Critical,
                                    QObject::tr("Error"),
                                    QObject::tr("Unable to lock the wallet."),
                                    QMessageBox::Ok,
                                    this);
            aMessageBox.setDetailedText(QObject::tr(a_result.c_str()));
            aMessageBox.exec();
        }
        UpdateLockedStatus();
    }
    else if(strstr(a_task.c_str(),"is_locked") == a_task.c_str())
    {
        if (a_err) {
            QMessageBox aMessageBox(QMessageBox::Critical,
                                    QObject::tr("error"),
                                    QObject::tr("Unable to get wallet lock status!"),
                                    QMessageBox::Ok,
                                    this);
            aMessageBox.setDetailedText(QObject::tr(a_result.c_str()));
            aMessageBox.exec();
            m_locked = true; // Assume disabled.
        }
        else {
            m_locked = (a_result == "true");
        }

        m_ActionLock.setDisabled(m_locked);
        m_ActionUnlock.setEnabled(m_locked);
        if (m_locked) {
            UnlockSlot();
        }
    }
    else if(strstr(a_task.c_str(),"get_account_history ") == a_task.c_str())
    {

    }

    //donePoint:
    if(a_clbkArg == CLI_WALLET_CODE)
    {
        m_cCliWalletDlg.appentText(a_result);
        if(a_err){ m_cCliWalletDlg->setTextColor(Qt::black);}
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

    pThis->execRD(thisPos, *pcsPassword);
    
}

