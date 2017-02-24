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
#include <QMessageBox>

#ifndef DEFAULT_WALLET_FILE_NAME
#define DEFAULT_WALLET_FILE_NAME       "wallet.json"
#endif

using namespace gui_wallet;

static gui_wallet::Mainwindow_gui_wallet*  s_pMainWindowInstance = NULL;

std::string FindImagePath(bool& a_bRet,const char* a_image_name);

int WarnAndWaitFunc(void* a_pOwner,WarnYesOrNoFuncType a_fpYesOrNo,
                           void* a_pDataForYesOrNo,const char* a_form,...);
int CallFunctionInGuiLoop2(SetNewTask_last_args2,const std::string& a_result,void* a_owner,TypeCallbackSetNewTaskGlb2 a_fpFunc);
int CallFunctionInGuiLoop3(SetNewTask_last_args2,const fc::variant& a_result,void* owner,TypeCallbackSetNewTaskGlb3 fpFnc);


/*//////////////////////////////////////////////////////////////////////////////////*/

static const char* FindValueStringByKey(const char* a_cpcInput, const char* a_key)
{
    const char* cpcValueFld = strstr(a_cpcInput,a_key);

    if(!cpcValueFld){return NULL;}
    cpcValueFld = strchr(cpcValueFld+1,':');
    if(!cpcValueFld){return NULL;}
    return cpcValueFld + 1;
}


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


#if 1
void ParseDigitalContentFromGetContentString(decent::wallet::ui::gui::SDigitalContent* a_pContent, const std::string& a_str)
{
    const char* cpcStrToGet;
    __DEBUG_APP2__(3,"str_to_parse is: \"\n%s\n\"",a_str.c_str());
    //std::string created;
    //std::string expiration;
    FindStringByKey(a_str.c_str(),"created",&a_pContent->created);
    FindStringByKey(a_str.c_str(),"expiration",&a_pContent->expiration);
    cpcStrToGet = FindValueStringByKey(a_str.c_str(),"size");
    if(cpcStrToGet)
    {
        char* pcTerm;
        a_pContent->size = strtod(cpcStrToGet,&pcTerm);
    }
    cpcStrToGet = FindValueStringByKey(a_str.c_str(),"times_bought");
    if(cpcStrToGet)
    {
        char* pcTerm;
        a_pContent->times_bougth = (int64_t)strtol(cpcStrToGet,&pcTerm,10);
    }
    a_pContent->get_content_str = a_str;
}
#endif


void SetNewTaskQtMainWnd2Glb(const std::string& a_inp_line, void* a_clbData)
{
    if(s_pMainWindowInstance)s_pMainWindowInstance->SetNewTaskQtMainWnd2(a_inp_line,a_clbData);
}

void SetNewTaskQtMainWnd3Glb(const std::string& a_inp_line, void* a_clbData)
{
    if(s_pMainWindowInstance)s_pMainWindowInstance->SetNewTaskQtMainWnd3(a_inp_line,a_clbData);
}


/*//////////////////////////////////////////////////////////////////////////////////*/

Mainwindow_gui_wallet::Mainwindow_gui_wallet()
        :
        m_ActionExit(tr("&Exit"),this),
        m_ActionConnect(tr("Connect"),this),
        m_ActionAbout(tr("About"),this),
        m_ActionInfo(tr("Info"),this),
        m_ActionHelp(tr("Help"),this),
        m_ActionWalletContent(tr("Wallet content"),this),
        m_ActionUnlock(tr("Unlock"),this),
        m_ActionImportKey(tr("Import key"),this),
        m_ActionOpenCliWallet(tr("cli_wallet"),this),
        m_ActionOpenInfoDlg(tr("Open info dlg."),this),
        m_ConnectDlg(this),
        m_info_dialog(),
        m_import_key_dlg(2),
        m_cqsPreviousFilter(tr("nf")),
        m_nConnected(0)
{
    s_pMainWindowInstance = this;
    m_default_stylesheet = styleSheet();
    //setStyleSheet("color:black;""background-color:white;");
    m_pInfoTextEdit = new QTextEdit;
    if(!m_pInfoTextEdit){throw "Low memory";}
    m_pInfoTextEdit->setReadOnly(true);
    m_pcInfoDlg = new CliWalletDlg(m_pInfoTextEdit);
    if(!m_pcInfoDlg){throw "Low memory";}

    CliTextEdit* pCliTextEdit = (CliTextEdit*)m_cCliWalletDlg.operator ->();
    pCliTextEdit->SetCallbackStuff2(this,NULL,&Mainwindow_gui_wallet::CliCallbackFnc);

    m_barLeft = new QMenuBar;
    m_barRight = new QMenuBar;

    m_pCentralAllLayout = new QVBoxLayout;
    m_pMenuLayout = new QHBoxLayout;

    m_wdata2.wallet_file_name = DEFAULT_WALLET_FILE_NAME;
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

    m_pCentralWidget = new CentralWigdet(m_pCentralAllLayout);
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

    //connect(this, SIGNAL(WalletContentReadySig(int)), this, SLOT(WalletContentReadySlot(int)) );
    //connect(&m_ConnectDlg, SIGNAL(ConnectDoneSig()), this, SLOT(ConnectDoneSlot()) );
    connect(pUsersCombo, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(CurrentUserChangedSlot(const QString&)) );
    m_nUserComboTriggeredInGui = 0;
    //void GuiWalletInfoWarnErrSlot(std::string);
    connect(m_pCentralWidget->GetBrowseContentTab(),
            SIGNAL(ShowDetailsOnDigContentSig(decent::wallet::ui::gui::SDigitalContent)),
            this,SLOT(ShowDetailsOnDigContentSlot(decent::wallet::ui::gui::SDigitalContent)));

    connect(m_pCentralWidget->GetPurchasedTab(),
            SIGNAL(ShowDetailsOnDigContentSig(decent::wallet::ui::gui::SDigitalContent)),
            this,SLOT(ShowDetailsOnDigContentSlot(decent::wallet::ui::gui::SDigitalContent)));

    //static void InitializeUiInterfaceOfWallet(TypeWarnAndWaitFunc a_fpWarnAndWait,
    //     TypeCallFunctionInGuiLoop a_fpCorrectUiCaller,
    //     Type* a_pMngOwner,void* a_pMngClb,void (Type::*a_clbkFunction)(SetNewTask_last_args))

    //__DEBUG_APP2__(0,"fpMan=%p",&Mainwindow_gui_wallet::ManagementNewFuncGUI);
    __DEBUG_APP2__(1,"fpWarn=%p, fpCaller2=%p, fpCaller3=%p, pMnOwner=%p, pMngrClb=%p, fpMngClb=%p",
                   GetFunctionPointerAsVoid(0,&WarnAndWaitFunc),GetFunctionPointerAsVoid(0,&CallFunctionInGuiLoop2),
                   GetFunctionPointerAsVoid(0,&CallFunctionInGuiLoop3),this,(void*)0,
                   GetFunctionPointerAsVoid(0,&Mainwindow_gui_wallet::ManagementNewFuncGUI));

    InitializeUiInterfaceOfWallet_base(&WarnAndWaitFunc,&CallFunctionInGuiLoop2,
                                  &CallFunctionInGuiLoop3,this,NULL,
                                  GetFunctionPointerAsVoid(0,&Mainwindow_gui_wallet::ManagementNewFuncGUI));
    m_nJustConnecting = 1;
    ConnectSlot();
    setWindowTitle(tr("Decent - Blockchain Content Distributor"));

    centralWidget()->layout()->setContentsMargins(0, 0, 0, 0);
    //statusBar()->hide();
    //mainToolBar->hide();
    setStyleSheet("QMainWindow{color:black;""background-color:white;}");

    //change
//    QString str = m_pCentralWidget->m_Overview_tab.search.text();
//    connect(&m_pCentralWidget->m_Overview_tab.find,SIGNAL(clicked()),this,SLOT(listAccountsSlot(str)));
}


Mainwindow_gui_wallet::~Mainwindow_gui_wallet()
{
    SaveWalletFile2(m_wdata2);
    DestroyUiInterfaceOfWallet();
    delete m_pInfoTextEdit;
    delete m_pcInfoDlg;
}


void Mainwindow_gui_wallet::SetNewTaskQtMainWnd2(const std::string& a_inp_line, void* a_clbData)
{
    SetNewTask2(a_inp_line,this,a_clbData,&Mainwindow_gui_wallet::TaskDoneFuncGUI);
}


void Mainwindow_gui_wallet::SetNewTaskQtMainWnd3(const std::string& a_inp_line, void* a_clbData)
{
    SetNewTask3(a_inp_line,this,a_clbData,&Mainwindow_gui_wallet::TaskDoneFuncGUI3);
}


void Mainwindow_gui_wallet::CreateActions()
{
    //m_pActionLoadIniFile = new QAction( tr("&Load ini"), this );
    //m_pActionLoadIniFile->setIcon( QIcon(":/images/open.png") );
    //m_pActionLoadIniFile->setShortcut( QKeySequence::Open );
    //m_pActionLoadIniFile->setStatusTip( tr("Load ini file") );
    //connect( m_pActionLoadIniFile, SIGNAL(triggered()), this, SLOT(LoadIniFileSlot()) );

    /**************************************************************************/

    m_ActionExit.setStatusTip( tr("Exit Program") );
    connect( &m_ActionExit, SIGNAL(triggered()), this, SLOT(close()) );

    /**************************************************************************/

    m_ActionAbout.setStatusTip( tr("About") );
    connect( &m_ActionAbout, SIGNAL(triggered()), this, SLOT(AboutSlot()) );

    m_ActionHelp.setStatusTip( tr("Help") );
    connect( &m_ActionHelp, SIGNAL(triggered()), this, SLOT(HelpSlot()) );

    m_ActionInfo.setStatusTip( tr("Info") );
    connect( &m_ActionInfo, SIGNAL(triggered()), this, SLOT(InfoSlot()) );

    m_ActionConnect.setStatusTip( tr("Connect to witness node") );
    connect( &m_ActionConnect, SIGNAL(triggered()), this, SLOT(ConnectSlot()) );

    //m_ActionWalletContent.setDisabled(true);
    m_ActionWalletContent.setStatusTip( tr("Wallet content") );
    connect( &m_ActionWalletContent, SIGNAL(triggered()), this, SLOT(ShowWalletContentSlot()) );

    m_ActionUnlock.setDisabled(true);
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
#define ADD_ACTION_TO_MENU_HELP(__action_ptr__) \
    do{m_pMenuHelpL->addAction( (__action_ptr__) );m_pMenuHelpR->addAction( (__action_ptr__) );}while(0)

    QMenuBar* pMenuBar = m_barLeft;

    m_pMenuFile = pMenuBar->addMenu( tr("&File") );
    m_pMenuFile->addAction( &m_ActionExit );
    m_pMenuFile->addAction( &m_ActionConnect );
    m_pMenuFile->addAction( &m_ActionUnlock );
    m_pMenuFile->addAction( &m_ActionImportKey );

    m_pMenuSetting = pMenuBar->addMenu( tr("&Setting") );
    m_pMenuHelpL = pMenuBar->addMenu( tr("&Help") );

    m_pMenuContent = pMenuBar->addMenu( tr("Content") );
    m_pMenuContent->addAction( &m_ActionWalletContent );

    m_pMenuDebug = pMenuBar->addMenu( tr("Debug") );
    m_pMenuDebug->addAction(&m_ActionOpenCliWallet);
    m_pMenuDebug->addAction(&m_ActionOpenInfoDlg);
    m_pMenuTempFunctions = m_pMenuDebug->addMenu(tr("temp. functions start"));
    //m_pMenuTempFunctions->addAction(&m_ActionShowDigitalContextes);

    /******************************************************/
    pMenuBar = m_barRight;
    m_pMenuHelpR = pMenuBar->addMenu( tr("Help") );

    m_pMenuCreateTicket = pMenuBar->addMenu( tr("Create ticket") );

    /******************************************************/
    ADD_ACTION_TO_MENU_HELP(&m_ActionAbout);
    ADD_ACTION_TO_MENU_HELP(&m_ActionInfo);
    ADD_ACTION_TO_MENU_HELP(&m_ActionHelp);
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


// static int SetNewTask(const std::string& a_inp_line, Type* a_memb, void* a_clbData,
//                      void (Type::*a_clbkFunction)(SetNewTask_last_args))

void Mainwindow_gui_wallet::CliCallbackFnc(void*/*arg*/,const std::string& a_task)
{
    m_cli_line = a_task;
    SetNewTask(a_task,this,CLI_WALLET_CODE,&Mainwindow_gui_wallet::TaskDoneFuncGUI);
    //UseConnectedApiInstance(this,NULL,&Mainwindow_gui_wallet::CliCallbackFunction);
}


void Mainwindow_gui_wallet::OpenCliWalletDlgSlot()
{
    m_cCliWalletDlg.exec();
}


void Mainwindow_gui_wallet::OpenInfoDlgSlot()
{
    m_pcInfoDlg->exec();
}

void Mainwindow_gui_wallet::listAccountsSlot(QString str)
{
    //change
    //SetNewTask("list_accounts " + str + " 5",this,CLI_WALLET_CODE,&Mainwindow_gui_wallet::TaskDoneFuncGUI);
}

void Mainwindow_gui_wallet::ShowDetailsOnDigContentSlot(decent::wallet::ui::gui::SDigitalContent a_dig_cont)
{
    switch(a_dig_cont.type)
    {
    case DCT::GENERAL:
        m_dig_cont_detailsGenDlg.execCDD(a_dig_cont);
        break;
    case DCT::BOUGHT:
        m_dig_cont_detailsBougDlg.execCDD(m_pCentralWidget->usersCombo()->currentText(),a_dig_cont);
        break;
    default:
        __DEBUG_APP2__(0,"error!!!!!!!!!");
        break;
    }
    //m_pInfoTextEdit->setText(tr(a_get_cont_str.c_str()));
    //m_pcInfoDlg->exec();  // Shold be modified
}


//int SetNewTask(const std::string& a_inp_line, Type* a_memb, void* a_clbData, void (Type::*a_clbkFunction)(SetNewTask_last_args))
void Mainwindow_gui_wallet::ShowDigitalContextesGUI(QString a_filter)
{
    //const char* cpcCoinside;
    const char* cpcNumberPtr;
    std::string csTaskLine("");
    std::string csFilterStr = StringFromQString(a_filter);

    if(strstr(csFilterStr.c_str(),ST::s_vcpcSearchTypeStrs[ST::author]))
    {
        csTaskLine = std::string("list_content_by_author ") +
                (csFilterStr.c_str() + strlen(ST::s_vcpcSearchTypeStrs[ST::author]) + 1);
    }
    else if(strstr(csFilterStr.c_str(),ST::s_vcpcSearchTypeStrs[ST::URI_start]))
    {
        const char* cpcURIstart;
        std::string csNumber;

        cpcNumberPtr = strchr(csFilterStr.c_str(),':');
        if(!cpcNumberPtr++){return;}
        for(;*cpcNumberPtr != 0 && *cpcNumberPtr==' ';++cpcNumberPtr);
        cpcURIstart = cpcNumberPtr;
        for(;*cpcNumberPtr != 0 && *cpcNumberPtr!=' ';++cpcNumberPtr);
        if( (*cpcNumberPtr==0) || (atoi(cpcNumberPtr+1)==0)){csNumber += " 100";}
        else {csNumber = cpcNumberPtr+1;}
        csTaskLine = std::string("list_content ") + cpcURIstart + std::string(" ") + csNumber;

    }
    else if(strstr(csFilterStr.c_str(),ST::s_vcpcSearchTypeStrs[ST::content]))
    {
        __DEBUG_APP2__(0,"Displaying contents by content is not implemented yet");
    }

    __DEBUG_APP2__(3,"taskLine=%s",csTaskLine.c_str());

    SetNewTask(csTaskLine,this,NULL,&Mainwindow_gui_wallet::TaskDoneFuncGUI);
}


/*
 * return != 0 means parsing error
 */
int Mainwindow_gui_wallet::GetDigitalContentsFromString(DCT::DIG_CONT_TYPES a_type,
                                                        std::vector<decent::wallet::ui::gui::SDigitalContent>& a_vcContents,
                                                        const char* a_contents_str)
{
    decent::wallet::ui::gui::SDigitalContent aDigContent;
    const char *cpcSearchStart= a_contents_str,
            *cpcAuthorFld, *cpcAutorBeg, *cpcAutorEnd,
            *cpcAmountFld, *cpcAmountBeg,*cpcAmountEnd,
            *cpcAssetIdFld, *cpcAssetIdBeg,*cpcAssetIdEnd,
            *cpcSynopsisFld, *cpcSynopsisBeg, *cpcSynopsisEnd,
            *cpcUriFld, *cpcUriBeg, *cpcUriEnd,
            *cpcAvgRatingFld, *cpcAvgRatingBeg,*cpcAvgRatingEnd;

    while(cpcSearchStart)
    {
        cpcAuthorFld = strstr(cpcSearchStart,"\"author\"");
        if(g_nDebugApplication){printf("cpcAuthorFld=\"%.10s\"\n",cpcAuthorFld ? cpcAuthorFld : "nill");}
        if(!cpcAuthorFld){return 0;}
        //
        cpcAutorBeg = strchr(cpcAuthorFld+strlen("\"author\""),'\"');
        if(g_nDebugApplication){printf("cpcAutorBeg=\"%.10s\"\n",cpcAutorBeg ? cpcAutorBeg : "nill");}
        if(!cpcAutorBeg){return 1;}
        cpcAutorEnd = strchr(++cpcAutorBeg,'\"');
        if(g_nDebugApplication){printf("cpcAutorEnd=\"%.10s\"\n",cpcAutorEnd ? cpcAutorEnd : "nill");}
        if(!cpcAutorEnd){return 1;}
        aDigContent.type = a_type;
        aDigContent.author = std::string(cpcAutorBeg,((size_t)cpcAutorEnd)-((size_t)cpcAutorBeg));
        if(g_nDebugApplication){printf("Content.author=\"%s\"\n",aDigContent.author.c_str());}

        cpcAmountFld = strstr(cpcAutorEnd+1,"\"amount\"");
        if(g_nDebugApplication){printf("cpcAmountFld=\"%.10s\"\n",cpcAmountFld ? cpcAmountFld : "nill");}
        if(!cpcAmountFld){return 1;}
        cpcAmountBeg = strchr(cpcAmountFld+strlen("\"amount\""),':');
        //cpcAmountBeg = cpcAmountFld+strlen("\"amount\"");
        if(g_nDebugApplication){printf("cpcAmountBeg=\"%.10s\"\n",cpcAmountBeg ? cpcAmountBeg : "nill");}
        if(!cpcAmountBeg){return 1;}
        aDigContent.price.amount = strtod(++cpcAmountBeg,const_cast<char**>(&cpcAmountEnd));
        if(g_nDebugApplication){printf("cpcAmountEnd=\"%.10s\", Content.price.amount=%lf\n",cpcAmountEnd ? cpcAmountEnd : "nill",aDigContent.price.amount);}
        if(!cpcAmountEnd){return 1;}

        cpcAssetIdFld = strstr(cpcAmountEnd,"\"asset_id\"");
        if(g_nDebugApplication){printf("cpcAssetIdFld=\"%.10s\"\n",cpcAssetIdFld ? cpcAssetIdFld : "nill");}
        if(!cpcAssetIdFld){return 1;}
        cpcAssetIdBeg = strchr(cpcAssetIdFld+strlen("\"asset_id\""),'\"');
        if(g_nDebugApplication){printf("cpcAssetIdBeg=\"%.10s\"\n",cpcAssetIdBeg ? cpcAssetIdBeg : "nill");}
        if(!cpcAssetIdBeg){return 1;}
        cpcAssetIdEnd = strchr(++cpcAssetIdBeg,'\"');
        if(g_nDebugApplication){printf("cpcAssetIdEnd=\"%.10s\"\n",cpcAssetIdEnd ? cpcAssetIdEnd : "nill");}
        if(!cpcAssetIdEnd){return 1;}
        aDigContent.price.asset_id = std::string(cpcAssetIdBeg,((size_t)cpcAssetIdEnd)-((size_t)cpcAssetIdBeg));
        if(g_nDebugApplication){printf("Content.price.asset_id=\"%s\"\n",aDigContent.price.asset_id.c_str());}

        cpcSynopsisFld = strstr(cpcAssetIdEnd,"\"synopsis\"");
        if(g_nDebugApplication){printf("cpcSynopsisFld=\"%.10s\"\n",cpcSynopsisFld ? cpcSynopsisFld : "nill");}
        if(!cpcSynopsisFld){return 1;}
        cpcSynopsisBeg = strchr(cpcSynopsisFld+strlen("\"synopsis\""),'\"');
        if(g_nDebugApplication){printf("cpcSynopsisBeg=\"%.10s\"\n",cpcSynopsisBeg ? cpcSynopsisBeg : "nill");}
        if(!cpcSynopsisBeg){return 1;}
        cpcSynopsisEnd = strchr(++cpcSynopsisBeg,'\"');
        if(g_nDebugApplication){printf("cpcSynopsisEnd=\"%.10s\"\n",cpcSynopsisEnd ? cpcSynopsisEnd : "nill");}
        if(!cpcSynopsisEnd){return 1;}
        aDigContent.synopsis = std::string(cpcSynopsisBeg,((size_t)cpcSynopsisEnd)-((size_t)cpcSynopsisBeg));
        if(g_nDebugApplication){printf("Content.synopsis=\"%s\"\n",aDigContent.synopsis.c_str());}

        cpcUriFld = strstr(cpcSynopsisEnd,"\"URI\"");
        if(g_nDebugApplication){printf("cpcUriFld=\"%.10s\"\n",cpcUriFld ? cpcUriFld : "nill");}
        if(!cpcSynopsisFld){return 1;}
        cpcUriBeg = strchr(cpcUriFld+strlen("\"URI\""),'\"');
        if(g_nDebugApplication){printf("cpcUriBeg=\"%.10s\"\n",cpcUriBeg ? cpcUriBeg : "nill");}
        if(!cpcUriBeg){return 1;}
        cpcUriEnd = strchr(++cpcUriBeg,'\"');
        if(g_nDebugApplication){printf("cpcUriEnd=\"%.10s\"\n",cpcUriEnd ? cpcUriEnd : "nill");}
        if(!cpcUriEnd){return 1;}
        aDigContent.URI = std::string(cpcUriBeg,((size_t)cpcUriEnd)-((size_t)cpcUriBeg));
        if(g_nDebugApplication){printf("Content.URI=\"%s\"\n",aDigContent.URI.c_str());}

        cpcAvgRatingFld = strstr(cpcUriEnd+1,"\"AVG_rating\"");
        if(g_nDebugApplication){printf("cpcAvgRatingFld=\"%.10s\"\n",cpcAvgRatingFld ? cpcAvgRatingFld : "nill");}
        if(!cpcAvgRatingFld){return 1;}
        cpcAvgRatingBeg = strchr(cpcAvgRatingFld+strlen("\"AVG_rating\""),':');
        //cpcAvgRatingBeg = cpcAvgRatingFld+strlen("\"AVG_rating\"");
        if(g_nDebugApplication){printf("cpcAvgRatingBeg=\"%.10s\"\n",cpcAvgRatingBeg ? cpcAvgRatingBeg : "nill");}
        if(!cpcAvgRatingBeg){return 1;}
        aDigContent.AVG_rating = strtod(++cpcAvgRatingBeg,const_cast<char**>(&cpcAvgRatingEnd));
        if(g_nDebugApplication){printf("cpcAvgRatingEnd=\"%.10s\", Content.AVG_rating=%lf\n",cpcAvgRatingEnd ? cpcAvgRatingEnd : "nill",aDigContent.AVG_rating);}
        if(!cpcAvgRatingEnd){return 1;}
        if(g_nDebugApplication){printf("\n*****************************************************\n");}

        a_vcContents.push_back(aDigContent);
        cpcSearchStart = cpcAvgRatingEnd+1;
    }

    __DEBUG_APP2__(1,"\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");

    return 0;
}


void Mainwindow_gui_wallet::UnlockSlot()
{
    //int SetNewTask(const std::string& a_inp_line, Type* a_memb, void* a_clbData,
    //           void (Type::*a_clbkFunction)(SetNewTask_last_args))
    //UseConnectedApiInstance(this,NULL,&Mainwindow_gui_wallet::UnlockFunction);
    //wapiptr->unlock(aPassword);
    QPoint thisPos = pos();
    std::vector<std::string> cvsPassword(1);
    decent::gui::tools::RET_TYPE rtRet = m_PasswdDialog.execRD(&thisPos,cvsPassword);
    if(rtRet == decent::gui::tools::RDB_OK)
    {
        std::string csPassLine = std::string("unlock ") + cvsPassword[0];
        SetNewTask(csPassLine,this,NULL,&Mainwindow_gui_wallet::TaskDoneFuncGUI);
    }

}


void Mainwindow_gui_wallet::moveEvent(QMoveEvent * a_event)
{
    //m_wallet_content_dlg.move( mapToGlobal(a_event->pos()));
    //m_wallet_content_dlg.move( /*mapToGlobal*/(a_event->pos()));
}



void Mainwindow_gui_wallet::DisplayWalletContentGUI()
{

    m_ActionUnlock.setEnabled(true);
    m_ActionImportKey.setEnabled(true);
    SetNewTask("list_my_accounts",this,NULL,&Mainwindow_gui_wallet::TaskDoneFuncGUI);

#if 0
    if(_LIKELY_(!m_nError))
    {
        std::string csBalanceName;
        asset* pAccountBalance;
        account_object* pAccount;
        QComboBox& cAccountsCombo = m_pCentralWidget->usersCombo();
        int nCount = cAccountsCombo.count();
        const int cnNumOfAccounts(m_vAccounts.size());
        int j,nNumbOfBalances;

        while(nCount>0){cAccountsCombo.removeItem(0);--nCount;}

        for(int i(0); i<cnNumOfAccounts;++i)
        {
            pAccount = &m_vAccounts[i];
            cAccountsCombo.addItem(tr(pAccount->name.c_str()));
            nNumbOfBalances = m_vAccountsBalances[i].size();
            for(j=0;j<nNumbOfBalances;++j)
            {
                //
            }

            if(g_nDebugApplication){printf("nNumbOfBalances=%d\n",nNumbOfBalances);}
            if(nNumbOfBalances>0)
            {
                int nIndexOfuser = m_pCentralWidget->usersCombo().currentIndex();
                pAccountBalance = &((m_vAccountsBalances[i])[nIndexOfuser>0 ? nIndexOfuser : 0]);
                //csBalanceName = (std::string)pAccountBalance->asset_id;
                csBalanceName = "DECENT";
                m_pCentralWidget->SetAccountBalanceGUI( pAccountBalance->amount.value,csBalanceName);
            }
        }
    }

    if(a_nDetailed){m_wallet_content_dlg.execWCt(m_vAccounts,m_vAccountsBalances,m_nError,m_error_string);}
#endif

}


void Mainwindow_gui_wallet::ImportKeySlot()
{
    m_nError = 0;
    m_error_string = "";

    std::vector<std::string> cvsUsKey(2);
    QComboBox& cUsersCombo = *m_pCentralWidget->usersCombo();

    cvsUsKey[0] = "nathan";
    cvsUsKey[1] = "5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3";

    if(cUsersCombo.count()&&(cUsersCombo.currentIndex()>0))
    {
        QString cqsUserName = cUsersCombo.currentText();
        QByteArray cbaResult = cqsUserName.toLatin1();
        cvsUsKey[0] = cbaResult.data();
    }

    QPoint thisPos = pos();
    decent::gui::tools::RET_TYPE aRet = m_import_key_dlg.execRD(&thisPos,cvsUsKey);
    if(aRet == decent::gui::tools::RDB_CANCEL){return ;}

    std::string csTaskStr = "import_key " + cvsUsKey[0] + " " + cvsUsKey[1];
    __DEBUG_APP2__(1,"!!!task: %s\n",csTaskStr.c_str());
    SetNewTask(csTaskStr,this,NULL,&Mainwindow_gui_wallet::TaskDoneFuncGUI);
}


void Mainwindow_gui_wallet::ShowWalletContentSlot()
{
    //m_wallet_content_dlg.exec();

#ifdef API_SHOULD_BE_DEFINED2

    m_nError = 0;
    m_error_string = "";
    //std::thread aListAccountThread(&Mainwindow_gui_wallet::ListAccountThreadFunc,this,1);
    //aListAccountThread.detach();

    //UseConnectedApiInstance(this,NULL,&Mainwindow_gui_wallet::CallShowWalletContentFunction);

    account_object* pAcc;
    m_vAccounts = pWapi->list_my_accounts();
    const int cnNumOfAccounts(m_vAccounts.size());
    //m_vAccountsBalances.reserve(cnNumOfAccounts);
    m_vAccountsBalances.resize(cnNumOfAccounts);
    for(int i(0); i<cnNumOfAccounts;++i)
    {
        pAcc = &(m_vAccounts[i]);
#ifdef LIST_ACCOUNT_BALANCES_DIRECT_CALL
        m_vAccountsBalances[i] = pWapi->list_account_balances(((std::string)(pAcc->id)));
#else
        m_vAccountsBalances[i].clear();
        if(a_pApi->gui_api)
        {
            int nUpdate = (i==(cnNumOfAccounts-1)) ? 1 : 0;
            __ulli64 ullnAccountAndUpdate = ((__ulli64)i)<<32 | nUpdate;
            std::string csTaskString = "list_account_balances " + ((std::string)(pAcc->id));
            (a_pApi->gui_api)->SetNewTask(this,(void*)ullnAccountAndUpdate,csTaskString,&Mainwindow_gui_wallet::TaskDoneFunc);
        }
#endif
    }

#endif // #ifdef API_SHOULD_BE_DEFINED
}



void Mainwindow_gui_wallet::InfoSlot()
{
    //UseConnectedApiInstance<Mainwindow_gui_wallet>(this,NULL,&Mainwindow_gui_wallet::CallInfoFunction);
    //if(a_pApi && (a_pApi->gui_api)){(a_pApi->gui_api)->SetNewTask(this,NULL,"info",&Mainwindow_gui_wallet::TaskDoneFunc);}
    SetNewTask("info",this,NULL,&Mainwindow_gui_wallet::TaskDoneFuncGUI);
}


void Mainwindow_gui_wallet::AboutSlot()
{
    //UseConnectedApiInstance<Mainwindow_gui_wallet>(this,NULL,&Mainwindow_gui_wallet::CallAboutFunction);
    SetNewTask("about",this,NULL,&Mainwindow_gui_wallet::TaskDoneFuncGUI);
}


void Mainwindow_gui_wallet::HelpSlot()
{
    //UseConnectedApiInstance<Mainwindow_gui_wallet>(this,NULL,&Mainwindow_gui_wallet::CallHelpFunction);
    SetNewTask("help",this,NULL,&Mainwindow_gui_wallet::TaskDoneFuncGUI);
}


void Mainwindow_gui_wallet::TaskDoneFuncGUI3(void* a_clbkArg,int64_t a_err,
                                             const std::string& a_task,const fc::variant& a_result)
{
    __DEBUG_APP2__(0,"just_conn=%d, err=%d, a_clbkArg=%p, task=%s",
                   m_nJustConnecting,(int)a_err,a_clbkArg,a_task.c_str());

    //enum MAIN_TABS_ENM{BROWSE_CONTENT,TRANSACTIONS,UPLOAD,OVERVIEW,PURCHASED};
    const int cnCurIndex(m_pCentralWidget->GetMyCurrentTabIndex());
    switch(cnCurIndex)
    {
    case BROWSE_CONTENT:
    {
        //BrowseContentTaskDone(void* a_clbkArg,int64_t a_err,const std::string& a_task,const std::string& a_result);
        TaskDoneBrowseContentGUI3(a_clbkArg, a_err,a_task,a_result);
        break;
    }
    case TRANSACTIONS:
    {
        TaskDoneTransactionsGUI3(a_clbkArg, a_err,a_task,a_result);
        break;
    }
    case UPLOAD:
    {
        TaskDoneUploadGUI3(a_clbkArg, a_err,a_task,a_result);
        break;
    }
    case OVERVIEW:
    {
        TaskDoneOverrviewGUI3(a_clbkArg, a_err,a_task,a_result);
        break;
    }
    case PURCHASED:
    {
        TaskDonePurchasedGUI3(a_clbkArg, a_err,a_task,a_result);
        break;
    }
    default:
    {
        break;
    }
    }
}


void Mainwindow_gui_wallet::TaskDoneFuncGUI(void* a_clbkArg,int64_t a_err,const std::string& a_task,const std::string& a_result)
{
    //emit TaskDoneSig(a_callbackArg,a_err,a_task,a_result);
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
            //aMessageBox.setStyleSheet(QMessageBox::);
            aMessageBox.setDetailedText(QObject::tr(a_result.c_str()));
            aMessageBox.exec();
            m_ConnectDlg.GetTableWidget(ConnectDlg::CONNECT_BUTTON_FIELD, 1)->setEnabled(true);
        }
        else
        {
            std::string aInfo, aDetails;

            LoadWalletFile(&m_wdata2);
            __DEBUG_APP2__(1,"chain_id=%s\n",m_wdata2.chain_id.c_str());
            aInfo = "Connected!\nserver: " + m_wdata2.ws_server;
            aDetails = aInfo + "\nchain_id: " + m_wdata2.chain_id;

            //ConnectDlg* pParent = (ConnectDlg*)a_pOwner;
            QMessageBox aMessageBox(QMessageBox::Information,
                                    QObject::tr("connected"),QObject::tr(aInfo.c_str()),
                                    QMessageBox::Ok,this);
            //aMessageBox.setStyleSheet(m_default_stylesheet);
            aMessageBox.setDetailedText(QObject::tr(aDetails.c_str()));
            aMessageBox.exec();
            DisplayWalletContentGUI();
        }
        return;
    }

    //enum MAIN_TABS_ENM{BROWSE_CONTENT,TRANSACTIONS,UPLOAD,OVERVIEW,PURCHASED};
    const int cnCurIndex(m_pCentralWidget->GetMyCurrentTabIndex());
    switch(cnCurIndex)
    {
    case BROWSE_CONTENT:
    {
        //BrowseContentTaskDone(void* a_clbkArg,int64_t a_err,const std::string& a_task,const std::string& a_result);
        TaskDoneBrowseContentGUI(a_clbkArg, a_err,a_task,a_result);
        break;
    }
    case TRANSACTIONS:
    {
        TaskDoneTransactionsGUI(a_clbkArg, a_err,a_task,a_result);
        break;
    }
    case UPLOAD:
    {
        TaskDoneUploadGUI(a_clbkArg, a_err,a_task,a_result);
        break;
    }
    case OVERVIEW:
    {
        TaskDoneOverrviewGUI(a_clbkArg, a_err,a_task,a_result);
        break;
    }
    case PURCHASED:
    {
        TaskDonePurchasedGUI(a_clbkArg, a_err,a_task,a_result);
        break;
    }
    default:
    {
        break;
    }
    }

    if(strstr(a_task.c_str(),__CONNECTION_CLB_))
    {
        __DEBUG_APP2__(2,"this should not work!");
    }
    else if( strstr(a_task.c_str(),"info"))
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
    else if(strstr(a_task.c_str(),"about"))
    {
        m_info_dialog.setFixedSize(500,300);
        m_info_dialog->setText(tr(a_result.c_str()));
        m_info_dialog.exec();
    }
    else if(strstr(a_task.c_str(),"help"))
    {
        m_info_dialog.setMaximumSize(QSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX));
        m_info_dialog.resize(500,500);
        m_info_dialog->setText(tr(a_result.c_str()));
        m_info_dialog.exec();
    }
    else if(strstr(a_task.c_str(),"list_my_accounts"))
    {
        int nNumbOfUsers(0);
        std::string csUserName;
        QComboBox& userCombo = *m_pCentralWidget->usersCombo();
        const char *cpcBegin, *cpcEnd, *cpcNextUser(a_result.c_str());

        __DEBUG_APP2__(1,"res=\n%s\n",a_result.c_str());
        userCombo.clear();

        while(GetJsonVectorNextElem(cpcNextUser,&cpcBegin,&cpcEnd))
        {
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
    else if((cpcOccur=strstr(a_task.c_str(),"list_account_balances ")))
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
    //else if(strstr(a_task.c_str(),"list_content "))
    else if(strstr(a_task.c_str(),"list_content"))
    {
        //QTableWidget& cContents = m_pCentralWidget->getDigitalContentsTable();
        std::string csGetContStr;
        m_vcDigContent.clear();
        GetDigitalContentsFromString(DCT::GENERAL, m_vcDigContent,a_result.c_str());
        const int cnContsNumber(m_vcDigContent.size());

        for(int i(0); i<cnContsNumber; ++i)
        {
            csGetContStr = std::string("get_content \"") + m_vcDigContent[i].URI + "\"";
            SetNewTask(csGetContStr,this,(void*)((size_t)i),&Mainwindow_gui_wallet::TaskDoneFuncGUI);
        }

    }
    else if(strstr(a_task.c_str(),"get_content "))
    {
        const int cnIndex (  (int)(  (size_t)a_clbkArg  )     );
        const int cnContsNumber(m_vcDigContent.size());
        if(cnIndex>=cnContsNumber){return;}
        ParseDigitalContentFromGetContentString(&m_vcDigContent[cnIndex],a_result);
        if(cnIndex==(cnContsNumber-1)){m_pCentralWidget->SetDigitalContentsGUI(m_vcDigContent);}
    }
    else if(strstr(a_task.c_str(),"info"))
    {
        //
    }
    else if(strstr(a_task.c_str(),"import_key "))
    {
        //std::thread aListAccountThread(&Mainwindow_gui_wallet::ListAccountThreadFunc,this,0);
        //aListAccountThread.detach();
        DisplayWalletContentGUI();
    }
    else if(strstr(a_task.c_str(),"unlock "))
    {
        //
    }
    else if(strstr(a_task.c_str(),"get_account_history "))
    {

    }
    else if(strstr(a_task.c_str(),"list_accounts "))
    {
        int nCurTab(m_pCentralWidget->GetMyCurrentTabIndex());
        if(nCurTab != OVERVIEW){return;}

        QString qstr = QString::fromStdString(a_result);
        m_pCentralWidget->m_Overview_tab.text.setText(qstr);

    }

    else if(strstr(a_task.c_str(),"get_account_history"))
    {
        QTableWidget* tabel = new QTableWidget();
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
    //int nCode = (int)a_err;
    int nError = (int)a_err;  // in 64 bit should be stored and error and message

    __DEBUG_APP2__(2,"clbArg=%p, task=\"%s\", res=\"%s\", err=%d",
                   a_clbkArg,a_task.c_str(),a_result.c_str(),nError);

#if 0
    switch(nCode)
    {
    case WAS::CONNECTED_ST:
    {
        if(m_nConnected==0)
        {
            __DEBUG_APP2__(1," ");
            m_nConnected = 1;
            m_ActionWalletContent.setEnabled(true);
            m_ActionUnlock.setEnabled(true);
            m_ActionImportKey.setEnabled(true);
        }

        break;
    }
    default:
        __DEBUG_APP2__(2,"default");
        break;
    }
#endif // #if 0

    int nCurentTab = m_pCentralWidget->GetMyCurrentTabIndex();
    __DEBUG_APP2__(2," ");
    //enum MAIN_TABS_ENM{BROWSE_CONTENT,TRANSACTIONS,UPLOAD,OVERVIEW,PURCHASED};
    switch(nCurentTab)
    {
    case BROWSE_CONTENT:
    {
        ManagementBrowseContentGUI();
        break;
    }
    case TRANSACTIONS:
        ManagementTransactionsGUI();
        //SetNewTask("get_account_history hayq 4",this,NULL,&Mainwindow_gui_wallet::TaskDoneFuncGUI);
        break;
    case UPLOAD:
        ManagementUploadGUI();
        break;
    case OVERVIEW:
    {
        ManagementOverviewGUI();
        break;
    }
    case PURCHASED:
        ManagementPurchasedGUI();
        break;
    default:
        break;
    }
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

    __DEBUG_APP2__(1,"nRet = %d, wallet_fl=%s",nRet, m_wdata2.wallet_file_name.c_str());

    if(nRet == decent::gui::tools::RDB_CANCEL){return;}

    m_ActionConnect.setEnabled(false);
    m_wdata2.action = WAT::CONNECT;
    m_wdata2.fpWarnFunc = &SetPassword;
    m_wdata2.fpDone = (TypeCallbackSetNewTaskGlb2)GetFunctionPointerAsVoid(1,&Mainwindow_gui_wallet::TaskDoneFuncGUI);
    StartConnectionProcedure(&m_wdata2,this,WALLET_CONNECT_CODE);
}


void Mainwindow_gui_wallet::SetPassword(void* a_owner,int a_answer,/*string**/void* a_str_ptr)
{
    std::string* pcsPassword = (std::string*)a_str_ptr;
    *pcsPassword = "";

    switch(a_answer)
    {
    case QMessageBox::Yes: case QMessageBox::Ok:
    {
        Mainwindow_gui_wallet* pThisCon = (Mainwindow_gui_wallet*)a_owner;
        PasswordDialog* pThis = &pThisCon->m_PasswdDialog;
        /*pThis->move(pThisCon->pos());
        pThis->exec();
        QString cqsPassword = pThis->m_password.text();
        QByteArray cLatin = cqsPassword.toLatin1();
        *pcsPassword = cLatin.data();*/
        std::vector<std::string> vsPassword(1);
        QPoint thisPos = pThisCon->pos();
        decent::gui::tools::RET_TYPE rtRet = pThis->execRD(&thisPos,vsPassword);
        if(rtRet != decent::gui::tools::RDB_CANCEL){*pcsPassword = vsPassword[0];}
        break;
    }

    default:
        break;
    }

}
