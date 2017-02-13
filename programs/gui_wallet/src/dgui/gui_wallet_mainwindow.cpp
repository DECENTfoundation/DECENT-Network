/*
 *	File: gui_wallet_mainwindow.cpp
 *
 *	Created on: Nov 11, 2016
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
#include "gui_wallet_mainwindow.hpp"
#include <QMenuBar>
#include "connected_api_instance.hpp"
#include <QMoveEvent>

using namespace gui_wallet;
extern int g_nDebugApplication;

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
        m_ConnectDlg(this),
        m_info_dialog(),
        m_PasswdDialog2(this)
{
    m_barLeft = new QMenuBar;
    m_barRight = new QMenuBar;

    m_pCentralAllLayout = new QVBoxLayout;
    m_pMenuLayout = new QHBoxLayout;

    m_pMenuLayout->addWidget(m_barLeft);
    m_pMenuLayout->addWidget(m_barRight);

    m_pMenuLayout->setAlignment(m_barLeft, Qt::AlignLeft);
    m_pMenuLayout->setAlignment(m_barRight, Qt::AlignRight);

    m_pCentralAllLayout->addLayout(m_pMenuLayout);
    /*mainMenuLayout0->addWidget(new QWidget);

    QWidget *central = new QWidget;
    central->setLayout(mainMenuLayout0);*/

    m_pCentralWidget = new CentralWigdet(m_pCentralAllLayout);
    m_pCentralWidget->setLayout(m_pCentralAllLayout);
    setCentralWidget(m_pCentralWidget);
    CreateActions();
    CreateMenues();
    resize(900,550);

    setCentralWidget(m_pCentralWidget);

    m_info_dialog.resize(0,0);

    m_nError = 0;
    m_error_string = "";

    QComboBox* pUsersCombo = &(m_pCentralWidget->usersCombo());

    connect(this, SIGNAL(TaskDoneSig(int,std::string, std::string)), this, SLOT(TaskDoneSlot(int,std::string, std::string)) );
    connect(this, SIGNAL(WalletContentReadySig(int)), this, SLOT(WalletContentReadySlot(int)) );
    connect(&m_ConnectDlg, SIGNAL(ConnectDoneSig()), this, SLOT(ConnectDoneSlot()) );
    //connect(pUsersCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(CurrentUserBalanceSlot(int)) );

}

#ifndef LIST_ACCOUNT_BALANCES_DIRECT_CALL
void Mainwindow_gui_wallet::__EmitWalletcontentReadyFnc(int a_det)
{
    emit WalletContentReadySig(a_det);
}

struct SAccountBalanceStruct{
    Mainwindow_gui_wallet* pWnd;int index,nUpdate;
    SAccountBalanceStruct(Mainwindow_gui_wallet* a_pWnd,int a_index,int nUp):pWnd(a_pWnd),index(a_index),nUpdate(nUp){}
};
static void acount_balance_done_fnc(void* a_owner,int /*err*/,const std::string& /*a_task*/, const std::string& a_task_result)
{
    struct SAccountBalanceStruct* pStr = (struct SAccountBalanceStruct*)a_owner;
    Mainwindow_gui_wallet* pWnd = pStr->pWnd;
    long long int llnDecents = strtoll(a_task_result.c_str(),NULL,10);
    int nIndex = pStr->index;
    int nUpdate = pStr->nUpdate;
    std::vector<asset> vAssets;

    if(llnDecents){vAssets.push_back(asset(llnDecents)); pWnd->m_vAccountsBalances[nIndex]=vAssets;}

    if(g_nDebugApplication){printf("llnDecents=%lld, nIndex=%d, string=\"%s\"\n", llnDecents,nIndex,a_task_result.c_str());}

    delete pStr;

    if(g_nDebugApplication){printf("nIndex=%d, nUpdate=%d\n",nIndex,nUpdate);}
    if(nUpdate){pWnd->__EmitWalletcontentReadyFnc(0);}
}
#endif // LIST_ACCOUNT_BALANCES_DIRECT_CALL


void Mainwindow_gui_wallet::CurrentUserBalanceSlot(int a_nIndex)
{
    if(a_nIndex>=0){UseConnectedApiInstance(this,&Mainwindow_gui_wallet::CurrentUserBalanceFunction);}
}


void Mainwindow_gui_wallet::CurrentUserBalanceFunction(struct StructApi* a_pApi)
{
    int nCurUserIndex ( m_pCentralWidget->usersCombo().currentIndex() );
    const int cnSize ( m_pCentralWidget->usersCombo().count() );
    if((nCurUserIndex>=0) && cnSize)
    {

        m_vAccountsBalances.resize(cnSize);
        QString cqsUserName = m_pCentralWidget->usersCombo().currentText();
        QByteArray cLatin = cqsUserName.toLatin1();
        std::string csAccName = cLatin.data();
        SAccountBalanceStruct* pStr = new SAccountBalanceStruct(this,nCurUserIndex,1);
        std::string csTaskString = "list_account_balances " + csAccName;
        if(a_pApi && a_pApi->gui_api){(a_pApi->gui_api)->SetNewTask(pStr,acount_balance_done_fnc,csTaskString);}
    }
}


Mainwindow_gui_wallet::~Mainwindow_gui_wallet()
{
}


void Mainwindow_gui_wallet::CreateActions()
{
    //m_pActionLoadIniFile = new QAction( tr("&Load ini"), this );
    //m_pActionLoadIniFile->setIcon( QIcon(":/images/open.png") );
    //m_pActionLoadIniFile->setShortcut( QKeySequence::Open );
    //m_pActionLoadIniFile->setStatusTip( tr("Load ini file") );
    //connect( m_pActionLoadIniFile, SIGNAL(triggered()), this, SLOT(LoadIniFileSlot()) );

    /**************************************************************************///m_pActionPrint

    //m_pActionPrint = new QAction( tr("&Print"), this );
    //m_pActionLoadIniFile->setIcon( QIcon(":/images/open.png") );
    //m_pActionLoadIniFile->setShortcut( QKeySequence::Open );
    //m_pActionPrint->setStatusTip( tr("Print") );
    //connect( m_pActionLoadIniFile, SIGNAL(triggered()), this, SLOT(LoadIniFileSlot()) );

    /**************************************************************************///m_pActionGuiConf

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

    m_ActionWalletContent.setDisabled(true);
    m_ActionWalletContent.setStatusTip( tr("Wallet content") );
    connect( &m_ActionWalletContent, SIGNAL(triggered()), this, SLOT(ShowWalletContentSlot()) );

    m_ActionUnlock.setDisabled(true);
    m_ActionUnlock.setStatusTip( tr("Unlock account") );
    connect( &m_ActionUnlock, SIGNAL(triggered()), this, SLOT(UnlockSlot()) );

    m_ActionImportKey.setDisabled(true);
    m_ActionImportKey.setStatusTip( tr("Import key") );
    connect( &m_ActionImportKey, SIGNAL(triggered()), this, SLOT(ImportKeySlot()) );

    /**************************************************************************/

    /**************************************************************************/

    /**************************************************************************/
}


void Mainwindow_gui_wallet::CreateMenues()
{
#define ADD_ACTION_TO_MENU_HELP(__action_ptr__) \
    do{m_pMenuHelpL->addAction( (__action_ptr__) );m_pMenuHelpR->addAction( (__action_ptr__) );}while(0)

    QMenuBar* pMenuBar = m_barLeft;

    m_pMenuFile = pMenuBar->addMenu( tr("&File") );
    //m_pMenuFile->addAction( m_pActionLoadIniFile );
    //m_pMenuFile->addAction( m_pActionPrint );
    m_pMenuFile->addAction( &m_ActionExit );
    m_pMenuFile->addAction( &m_ActionConnect );
    m_pMenuFile->addAction( &m_ActionUnlock );
    m_pMenuFile->addAction( &m_ActionImportKey );

    m_pMenuSetting = pMenuBar->addMenu( tr("&Setting") );
    m_pMenuHelpL = pMenuBar->addMenu( tr("&Help") );
    //m_pMenuHelpL->addAction( &m_ActionAbout );
    //m_pMenuHelpL->addAction( &m_ActionInfo );

    m_pMenuContent = pMenuBar->addMenu( tr("Content") );
    m_pMenuContent->addAction( &m_ActionWalletContent );

    //QMenu*          m_pMenuHelpR;
    //QMenu*          m_pMenuCreateTicket;
    pMenuBar = m_barRight;
    m_pMenuHelpR = pMenuBar->addMenu( tr("Help") );
    //m_pMenuHelpR->addAction( &m_ActionAbout );
    //m_pMenuHelpR->addAction( &m_ActionInfo );
    m_pMenuCreateTicket = pMenuBar->addMenu( tr("Create ticket") );

    ADD_ACTION_TO_MENU_HELP(&m_ActionAbout);
    ADD_ACTION_TO_MENU_HELP(&m_ActionInfo);
    ADD_ACTION_TO_MENU_HELP(&m_ActionHelp);
}


void Mainwindow_gui_wallet::ConnectDoneSlot()
{
    WalletContentReadySlot(0);
    m_ActionWalletContent.setEnabled(true);
    m_ActionUnlock.setEnabled(true);
    m_ActionImportKey.setEnabled(true);
}


void Mainwindow_gui_wallet::UnlockSlot()
{
    UseConnectedApiInstance(this,&Mainwindow_gui_wallet::UnlockFunction);
    //wapiptr->unlock(aPassword);
}


void Mainwindow_gui_wallet::UnlockFunction(struct StructApi* a_pApi)
{
    graphene::wallet::wallet_api* pWapi = a_pApi ? a_pApi->wal_api : NULL;

    try
    {
        if(pWapi)
        {
            std::string csPassword = m_PasswdDialog2.execN();
            pWapi->unlock(csPassword);
        } // if(pWapi)
    }
    catch(const fc::exception& a_fc)
    {
        if(g_nDebugApplication){printf("file:\"" __FILE__ "\",line:%d\n",__LINE__);}
        m_nError = 1;
        m_error_string = a_fc.to_detail_string();
        if(g_nDebugApplication){printf("%s\n",(a_fc.to_detail_string()).c_str());}
    }
    catch(...)
    {
        if(g_nDebugApplication){printf("file:\"" __FILE__ "\",line:%d\n",__LINE__);}
        m_nError = 2;
        m_error_string = "Unknown exception!";
        if(g_nDebugApplication){printf("Unknown exception\n");}
    }

}


void Mainwindow_gui_wallet::moveEvent(QMoveEvent * a_event)
{
    //m_wallet_content_dlg.move( mapToGlobal(a_event->pos()));
    m_wallet_content_dlg.move( /*mapToGlobal*/(a_event->pos()));
}


void Mainwindow_gui_wallet::ListAccountThreadFunc(int a_nDetailed)
{
    UseConnectedApiInstance(this,&Mainwindow_gui_wallet::CallShowWalletContentFunction);
    emit WalletContentReadySig(a_nDetailed);
}


void Mainwindow_gui_wallet::WalletContentReadySlot(int a_nDetailed)
{
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

    if(a_nDetailed){m_wallet_content_dlg.exec(m_vAccounts,m_vAccountsBalances,m_nError,m_error_string);}

}




void Mainwindow_gui_wallet::CallShowWalletContentFunction(struct StructApi* a_pApi)
{
    graphene::wallet::wallet_api* pWapi = a_pApi ? a_pApi->wal_api : NULL;

    try
    {
        if(pWapi)
        {
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
                    SAccountBalanceStruct* pStr = new SAccountBalanceStruct(this,i,nUpdate);
                    std::string csTaskString = "list_account_balances " + ((std::string)(pAcc->id));
                    (a_pApi->gui_api)->SetNewTask(pStr,acount_balance_done_fnc,csTaskString);
                }
#endif
            }
        }
    }
    catch(const fc::exception& a_fc)
    {
        if(g_nDebugApplication){printf("file:\"" __FILE__ "\",line:%d\n",__LINE__);}
        m_nError = 1;
        m_error_string = a_fc.to_detail_string();
        if(g_nDebugApplication){printf("%s\n",(a_fc.to_detail_string()).c_str());}
    }
    catch(...)
    {
        if(g_nDebugApplication){printf("file:\"" __FILE__ "\",line:%d\n",__LINE__);}
        m_nError = 2;
        m_error_string = "Unknown exception!";
        if(g_nDebugApplication){printf("Unknown exception\n");}
    }

}


void Mainwindow_gui_wallet::ImportKeySlot()
{
    m_nError = 0;
    m_error_string = "";
    UseConnectedApiInstance(this,&Mainwindow_gui_wallet::CallImportKeyFunction);
}


void Mainwindow_gui_wallet::ShowWalletContentSlot()
{
    //m_wallet_content_dlg.exec();

    m_nError = 0;
    m_error_string = "";
    std::thread aListAccountThread(&Mainwindow_gui_wallet::ListAccountThreadFunc,this,1);
    aListAccountThread.detach();
}


void Mainwindow_gui_wallet::CallInfoFunction(struct StructApi* a_pApi)
{
    try
    {
        if(a_pApi && (a_pApi->gui_api)){(a_pApi->gui_api)->SetNewTask(this,TaskDoneFunc,"info");}
    }
    catch(const fc::exception& a_fc)
    {
        if(g_nDebugApplication){printf("file:\"" __FILE__ "\",line:%d\n",__LINE__);}
        //(*a_fpErr)(a_pOwner,a_fc.to_string(),a_fc.to_detail_string());
        if(g_nDebugApplication){printf("%s\n",(a_fc.to_detail_string()).c_str());}
    }
    catch(...)
    {
        if(g_nDebugApplication){printf("file:\"" __FILE__ "\",line:%d\n",__LINE__);}
        if(g_nDebugApplication){printf("Unknown exception\n");}
    }
}


static void task_done_static_function(void* a_pOwner,int a_err,const std::string& a_task, const std::string& a_result)
{
    if(strstr(a_task.c_str(),"import_key"))
    {
        if(a_pOwner){((Mainwindow_gui_wallet*)a_pOwner)->task_done_function(a_err,a_task,a_result);}
    }
}


void Mainwindow_gui_wallet::task_done_function(int /*err*/,const std::string& a_task, const std::string& /*result*/)
{
    if(strstr(a_task.c_str(),"import_key"))
    {
        std::thread aListAccountThread(&Mainwindow_gui_wallet::ListAccountThreadFunc,this,0);
        aListAccountThread.detach();
    }
}


void Mainwindow_gui_wallet::CallImportKeyFunction(struct StructApi* a_pApi)
{
    try
    {
        if(a_pApi && (a_pApi->wal_api))
        {
            QByteArray cLatin;
            QString cqsUserName(tr("nathan"));
            QString cqsKey(tr("5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3"));
            std::string csUser_name, csKey;
            QComboBox& cUsersCombo = m_pCentralWidget->GetUsersList();

            if(cUsersCombo.count())
            {
                cqsUserName = cUsersCombo.currentText();
            }

            m_import_key_dlg.exec(pos(),cqsUserName,cqsKey);
            cLatin = cqsUserName.toLatin1();
            csUser_name = cLatin.data();
            cLatin = cqsKey.toLatin1();
            csKey = cLatin.data();
#ifdef WALLET_API_DIRECT_CALLS
            (a_pApi->wal_api)->import_key(csUser_name,csKey);
#else  // #ifdef WALLET_API_DIRECT_CALLS
            if(a_pApi->gui_api)
            {
                std::string csTaskStr = "import_key " + csUser_name + " " + csKey;
                if(g_nDebugApplication){printf("!!!task: %s\n",csTaskStr.c_str());}
                (a_pApi->gui_api)->SetNewTask(this,task_done_static_function,csTaskStr);
            }
#endif  // #ifdef WALLET_API_DIRECT_CALLS

        } // if(a_pApi && (a_pApi->wal_api))
    } // try
    catch(const fc::exception& a_fc)
    {
        //(*a_fpErr)(a_pOwner,a_fc.to_string(),a_fc.to_detail_string());
        if(g_nDebugApplication){printf("%s\n",(a_fc.to_detail_string()).c_str());}
    }
    catch(...)
    {
        if(g_nDebugApplication){printf("Unknown exception\n");}
    }

    emit WalletContentReadySig(0);
}


void Mainwindow_gui_wallet::InfoSlot()
{
    UseConnectedApiInstance<Mainwindow_gui_wallet>(this,&Mainwindow_gui_wallet::CallInfoFunction);
}


void Mainwindow_gui_wallet::CallAboutFunction(struct StructApi* a_pApi)
{
    graphene::wallet::wallet_api* pApi = NULL;
    bool bCreatedHere = false;
    std::string aStr;

    try
    {
        aStr = "";
        fc::variant_object var_obj_about;
        pApi = a_pApi ? a_pApi->wal_api : NULL;
#if 0
        if(!pApi){
            graphene::wallet::wallet_data wdata;
            get_remote_api
            pApi = new graphene::wallet::wallet_api(wdata,);
            bCreatedHere=true;
        }
#endif

        if(pApi)
        {
            var_obj_about = pApi->about();
            fc::variant_object::iterator itCur;
            fc::variant_object::iterator itBeg = var_obj_about.begin();
            fc::variant_object::iterator itEnd = var_obj_about.end();

            for(itCur=itBeg;itCur < itEnd;++itCur)
            {
                aStr += "\"";
                aStr += (*itCur).key();
                aStr += "\": \"";
                aStr += (*itCur).value().as_string();
                aStr += "\"\n";
            }

        } // if(pApi)
        else
        {
            //var_obj_about = static_about();
            aStr = "First connect to the witness node, then require again info!";
        }


    }
    catch(const fc::exception& a_fc)
    {
        aStr += a_fc.to_detail_string();
        //(*a_fpErr)(a_pOwner,a_fc.to_string(),a_fc.to_detail_string());
        if(g_nDebugApplication){printf("%s\n",(a_fc.to_detail_string()).c_str());}
    }
    catch(...)
    {
        aStr += "Unknown Exception thrown!";
        if(g_nDebugApplication){printf("Unknown exception\n");}
    }

    m_info_dialog.setFixedSize(500,300);
    m_info_dialog->setText(tr(aStr.c_str()));
    m_info_dialog.exec();

    if(bCreatedHere){delete pApi;}

}

void Mainwindow_gui_wallet::AboutSlot()
{
    UseConnectedApiInstance<Mainwindow_gui_wallet>(this,&Mainwindow_gui_wallet::CallAboutFunction);
}


void Mainwindow_gui_wallet::CallHelpFunction(struct StructApi* a_pApi)
{
    graphene::wallet::wallet_api* pApi = NULL;
    bool bCreatedHere = false;
    std::string aStr;

    try
    {
        aStr = "";
        //fc::variant_object var_obj_about;
        pApi = a_pApi ? a_pApi->wal_api : NULL;

        if(pApi)
        {
            aStr = pApi->help();
        } // if(pApi)
        else
        {
            //var_obj_about = static_about();
            aStr = "First connect to the witness node, then require again info!";
        }


    }
    catch(const fc::exception& a_fc)
    {
        aStr += a_fc.to_detail_string();
        //(*a_fpErr)(a_pOwner,a_fc.to_string(),a_fc.to_detail_string());
        if(g_nDebugApplication){printf("%s\n",(a_fc.to_detail_string()).c_str());}
    }
    catch(...)
    {
        aStr += "Unknown Exception thrown!";
        if(g_nDebugApplication){printf("Unknown exception\n");}
    }

    m_info_dialog.setMaximumSize(QSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX));
    m_info_dialog.resize(500,500);
    m_info_dialog->setText(tr(aStr.c_str()));
    m_info_dialog.exec();

    if(bCreatedHere){delete pApi;}
}

void Mainwindow_gui_wallet::HelpSlot()
{
    UseConnectedApiInstance<Mainwindow_gui_wallet>(this,&Mainwindow_gui_wallet::CallHelpFunction);
}


void Mainwindow_gui_wallet::TaskDoneFunc(int a_err,const std::string& a_task,const std::string& a_result)
{
    emit TaskDoneSig(a_err,a_task,a_result);
}


void Mainwindow_gui_wallet::TaskDoneFunc(void* a_owner,int a_err,const std::string& a_task,const std::string& a_result)
{
    Mainwindow_gui_wallet* pOwner = (Mainwindow_gui_wallet*)a_owner;
    pOwner->TaskDoneFunc(a_err,a_task,a_result);
}


void Mainwindow_gui_wallet::TaskDoneSlot(int a_err,std::string a_task, std::string a_result)
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



void Mainwindow_gui_wallet::ConnectSlot()
{
    m_nError = 0;
    m_error_string = "";
    m_ConnectDlg.exec();
}
