/*
 *	File: ui_wallet_functions.cpp
 *
 *	Created on: 04 Feb 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */

#include "ui_wallet_functions.hpp"
#include <mutex>
#include <stdio.h>
#include <stdarg.h>
#include "fc_rpc_gui.hpp"
#include <graphene/wallet/wallet.hpp>
#include <thread>
#include "unnamedsemaphorelite.hpp"
#include "decent_tool_fifo.hpp"
#include <fc/network/http/websocket.hpp>
#include <fc/rpc/websocket_api.hpp>
#include "decent_gui_inguiloopcaller_glb.hpp"
#include "decent_tools_rwlock.hpp"

#ifdef WIN32
#include <windows.h>
#else  // #ifdef WIN32
#include <unistd.h>
#define Sleep(__x__) usleep(1000*(__x__))
#endif  // #ifdef WIN32

#define CONN_FNC_TYPE   TypeCallbackSetNewTaskGlb

static void __THISCALL__ MenegementClbkDefault(void* a_owner,SetNewTask_last_args);

extern int g_nDebugApplication ;
static graphene::wallet::wallet_data s_wdata;
static volatile int s_nManagerThreadRun ;
static volatile int s_nConThreadRun;
static TypeCallbackSetNewTaskGlb    s_fpMenegmentClbk = &MenegementClbkDefault;
static void*                        s_pManagerOwner = NULL;
static void*                        s_pManagerClbData = NULL;

static void gui_wallet_application_MenegerThreadFunc(void);
static int SaveWalletFile_private(const SConnectionStruct& a_WalletData);

struct StructApi{StructApi():wal_api(NULL),gui_api(NULL){} graphene::wallet::wallet_api* wal_api; fc::rpc::gui* gui_api;};

class NewTestMutex2 : public std::mutex{
public:
    void lock(){if(g_nDebugApplication>1){printf("++locking!\n");}std::mutex::lock();if(g_nDebugApplication>1){printf("++locked!\n");}}
    void unlock(){if(g_nDebugApplication>1){printf("--unlocking!\n");}std::mutex::unlock();if(g_nDebugApplication>1){printf("--unlocked!\n");}}
};

#define NewTestMutex decent::tools::RWLock
//#define NewTestMutex std::mutex
//#define NewTestMutex NewTestMutex2


static NewTestMutex*   s_pMutex_for_cur_api; // It is better to use here rw mutex

static decent::tools::FiFo<SConnectionStruct,CONN_FNC_TYPE>*  s_pConnectionRequestFifo;
static decent::tools::UnnamedSemaphoreLite*     s_pSema_for_connection_thread;
static std::thread*                             s_pConnectionThread ;
static std::thread*                             s_pManagementThread ;
static void ConnectionThreadFunction(void);
class ConnectionThreadStarter{
public:
    ConnectionThreadStarter(){
        s_pMutex_for_cur_api = new NewTestMutex;
        s_pConnectionRequestFifo = new decent::tools::FiFo<SConnectionStruct,TypeCallbackSetNewTaskGlb>;
        //if(!s_pConnectionRequestFifo){throw "Low memory!\n" __FILE__ ;}
        s_pSema_for_connection_thread = new decent::tools::UnnamedSemaphoreLite;
        //if(!s_pSema_for_connection_thread){throw "Low memory!\n" __FILE__ ;}
        s_pConnectionThread = new std::thread(&ConnectionThreadFunction);
        //if(!s_pConnectionThread){throw "Low memory!\n" __FILE__ ;}
        s_pManagementThread = new std::thread(&gui_wallet_application_MenegerThreadFunc);
        //if(!s_pManagementThread){throw "Low memory!\n" __FILE__ ;}
    }

    ~ConnectionThreadStarter(){
        s_nManagerThreadRun = 0;
        s_nConThreadRun = 0;
        s_pSema_for_connection_thread->post();
        s_pManagementThread->join();
        s_pConnectionThread->join();
        delete s_pManagementThread;
        delete s_pConnectionThread;
        delete s_pSema_for_connection_thread;
        delete s_pConnectionRequestFifo;
        delete s_pMutex_for_cur_api;
    }
};
static ConnectionThreadStarter s_ConnectionThreadStarter;

static StructApi      s_CurrentApi;

static int ConnectToNewWitness(const decent::tools::taskListItem<SConnectionStruct,CONN_FNC_TYPE>&);


void StartConnectionProcedure(const SConnectionStruct& a_conn_str, void* a_owner, void* a_clbData,TypeCallbackSetNewTaskGlb a_clbkFunction)
{
    StartConnectionProcedure_base(a_conn_str,a_owner,a_clbData,a_clbkFunction);
}


void StartConnectionProcedure_base(const SConnectionStruct& a_conn_str,void *a_owner, void*a_clbData,...)
{
    TypeCallbackSetNewTaskGlb fpTaskDone;
    va_list aFunc;

    va_start( aFunc, a_clbData );
    fpTaskDone = va_arg( aFunc, TypeCallbackSetNewTaskGlb);
    va_end( aFunc );

    s_pConnectionRequestFifo->AddNewTask(a_conn_str,a_owner,a_clbData,fpTaskDone);
    s_pSema_for_connection_thread->post();

}


int SetNewTask(const std::string& a_inp_line, void* a_owner, void* a_clbData, TypeCallbackSetNewTaskGlb a_clbkFunction)
{
    return SetNewTask_base(a_inp_line, a_owner, a_clbData, a_clbkFunction);
}


int SetNewTask_base(const std::string& a_inp_line, void* a_owner, void* a_clbData, ...)
{

    TypeCallbackSetNewTaskGlb fpTaskDone;
    va_list aFunc;

    va_start( aFunc, a_clbData );
    fpTaskDone = va_arg( aFunc, TypeCallbackSetNewTaskGlb);
    va_end( aFunc );

    std::lock_guard<NewTestMutex> lock(*s_pMutex_for_cur_api);
    if(s_CurrentApi.gui_api)
    {
        (s_CurrentApi.gui_api)->SetNewTask(a_inp_line,a_owner,a_clbData,fpTaskDone);
    }
    else
    {
        (*fpTaskDone)(a_owner,a_clbData,NO_API_INITED, a_inp_line, "First connet to witness node");
        return NO_API_INITED;
    }

    return 0;

}



void SetManagementCallback(void* a_pOwner,void* a_pClbData,TypeCallbackSetNewTaskGlb a_fpClbk)
{
    SetManagementCallback_base(a_pOwner,a_pClbData, a_fpClbk);
}


void SetManagementCallback_base(void* a_pOwner,void* a_pClbData,...)
{
    va_list aFunc;

    va_start( aFunc, a_pClbData );
    s_fpMenegmentClbk = va_arg( aFunc, TypeCallbackSetNewTaskGlb);
    va_end( aFunc );

    s_pManagerOwner = a_pOwner;
    s_pManagerClbData = a_pClbData;
}


int LoadWalletFile(SConnectionStruct* a_pWalletData)
{
    int nReturn(0);

    if(!a_pWalletData)
    {
        return WRONG_ARGUMENT;
    }

    FILE* fpFile= fopen(a_pWalletData->wallet_file_name.c_str(),"r");
    if(!fpFile){
        return FILE_DOES_NOT_EXIST;
    }
    fclose(fpFile);

    try{
        s_wdata = fc::json::from_file( a_pWalletData->wallet_file_name ).as< graphene::wallet::wallet_data >();
        a_pWalletData->ws_server = s_wdata.ws_server;
        a_pWalletData->ws_user = s_wdata.ws_user;
        a_pWalletData->ws_password = s_wdata.ws_password;
        a_pWalletData->chain_id = s_wdata.chain_id.str().c_str();
    }
    catch(...)
    {
        nReturn = UNKNOWN_EXCEPTION;
    }

    return nReturn;
}



int SaveWalletFile2(const SConnectionStruct& a_WalletData)
{
    int nReturn(0);
    s_pMutex_for_cur_api->lock();
    try{SaveWalletFile_private(a_WalletData);}
    catch(...){nReturn=UNKNOWN_EXCEPTION;}
    s_pMutex_for_cur_api->unlock();
    return nReturn;
}




/*///////////////////////////////////////////////////////////*/

//void (__THISCALL__ *TypeCallbackSetNewTaskGlb)(void* owner,SetNewTask_last_args)


static int SaveWalletFile_private(const SConnectionStruct& a_WalletData)
{
    int nReturn(0);
    s_wdata.ws_server = a_WalletData.ws_server;
    s_wdata.ws_user = a_WalletData.ws_user ;
    s_wdata.ws_password = a_WalletData.ws_password;
    //m_wdata.chain_id = chain_id_type( std::string( (((QLineEdit*)m_main_table.cellWidget(CHAIN_ID_FIELD,1))->text()).toLatin1().data() ) );
    s_wdata.chain_id = chain_id_type(a_WalletData.chain_id);

    if(s_CurrentApi.wal_api){(s_CurrentApi.wal_api)->save_wallet_file(a_WalletData.wallet_file_name);}
    else{nReturn=NO_API_INITED;}
    return nReturn;
}


static void gui_wallet_application_MenegerThreadFunc(void)
{
    char vnOpt[WAS::_API_STATE_SIZE];
    int i;

    s_nManagerThreadRun = 1;
    if(g_nDebugApplication){printf("!!!!!!! fn:%s, ln:%d\n",__FUNCTION__,__LINE__);}

    memset(vnOpt,0,sizeof(vnOpt));

    while(s_nManagerThreadRun)
    {
        // make checks
        s_pMutex_for_cur_api->lock();

        if(s_CurrentApi.wal_api )
        {
            vnOpt[WAS::CONNECTED_ST] = 1;
        }

        s_pMutex_for_cur_api->unlock();

        for(i=0;i<WAS::_API_STATE_SIZE;++i)
        {
            if(vnOpt[i])
            {
                //if(g_nDebugApplication){printf("emit UpdateGuiStateSig(%d)\n",i);}
                //emit UpdateGuiStateSig(i);
                CallFunctionInGuiLoop(s_pManagerClbData,(int64_t)i, __MANAGER_CLB_,
                                      __FILE__ "\nManagement",s_pManagerOwner,s_fpMenegmentClbk);
                vnOpt[i] = 0;
            }
        }  // for(i=0;i<_API_STATE_SIZE;++i)


        Sleep(1000);
    } // while(s_nManagerThreadRun)
}


static void __THISCALL__ MenegementClbkDefault(void* a_owner,SetNewTask_last_args)
{
    // a_clbData,int64_t a_err, const std::string& a_inp, const std::string& a_result
    if(g_nDebugApplication)
    {
        printf("owner=%p, clbData=%p, err=%d, inp=%s, res=%s\n",a_owner,a_clbData,(int)a_err,a_inp.c_str(), a_result.c_str());
    }
}


static void ConnectionThreadFunction(void)
{
    decent::tools::taskListItem<SConnectionStruct,CONN_FNC_TYPE> aTaskItem(NULL,SConnectionStruct());
    //int nConnectReturn;
    s_nConThreadRun = 1;
    std::thread* pConnectionThread;

    while(s_nConThreadRun)
    {
        s_pSema_for_connection_thread->wait();
        __DEBUG_APP2__(1,"!!!!!!!!!!!!\n");
        while(s_pConnectionRequestFifo->GetFirstTask(&aTaskItem))
        {
            //nConnectReturn = ConnectToNewWitness(aTaskItem);
            pConnectionThread = new std::thread(&ConnectToNewWitness,aTaskItem);
            if(pConnectionThread){
                pConnectionThread->detach();
                delete pConnectionThread;
            }
        }

    }
}


static void SetCurrentApis(const StructApi* a_pApis)
{
    s_pMutex_for_cur_api->write_lock();
    memcpy(&s_CurrentApi,a_pApis,sizeof(StructApi));
    s_pMutex_for_cur_api->unlock();
}


static int ConnectToNewWitness(const decent::tools::taskListItem<SConnectionStruct,CONN_FNC_TYPE>& a_con_data)
{
    try
    {
        const SConnectionStruct& aStrct = a_con_data.input;

        if(aStrct.action == WAT::SAVE2)
        {
            int nReturn(SaveWalletFile2(aStrct));
            CallFunctionInGuiLoop(a_con_data.callbackArg,(int64_t)nReturn, __CONNECTION_CLB_,
                                  __FILE__ "\nSaving procedure",a_con_data.owner,a_con_data.fn_tsk_dn2);
            return nReturn;
        }
        else if(aStrct.action == WAT::LOAD2)
        {
            //m_wdata = fc::json::from_file( csWalletFileName ).as< graphene::wallet::wallet_data >();
            int nReturn(LoadWalletFile((SConnectionStruct*)a_con_data.callbackArg));
            CallFunctionInGuiLoop(a_con_data.callbackArg,(int64_t)nReturn, __CONNECTION_CLB_,
                                  __FILE__ "\nLoading procedure",a_con_data.owner,a_con_data.fn_tsk_dn2);
            return nReturn;
        }
        else if(aStrct.action != WAT::CONNECT)
        {
            CallFunctionInGuiLoop(a_con_data.callbackArg,UNKNOWN_EXCEPTION, __CONNECTION_CLB_,
                                  __FILE__ "Unknown option",a_con_data.owner,a_con_data.fn_tsk_dn2);
            return UNKNOWN_EXCEPTION;
        }

        void* pOwner = a_con_data.owner; // ???
        StructApi aApiToCreate;
        //const graphene::wallet::wallet_data wdata; wdata.ws_password;
        s_wdata.ws_server = aStrct.ws_server;
        s_wdata.ws_user = aStrct.ws_user;
        s_wdata.ws_password = aStrct.ws_password;
        fc::path wallet_file( aStrct.wallet_file_name );

        fc::http::websocket_client client;
        //idump((wdata.ws_server));
        auto con  = client.connect( aStrct.ws_server );
        auto apic = std::make_shared<fc::rpc::websocket_api_connection>(*con);

        auto remote_api = apic->get_remote_api< login_api >(1);
        //edump((wdata.ws_user)(wdata.ws_password) );
        // TODO:  Error message here
        FC_ASSERT( remote_api->login( aStrct.ws_user, aStrct.ws_password ) );

        auto wapiptr = std::make_shared<graphene::wallet::wallet_api>( s_wdata, remote_api );
        wapiptr->set_wallet_filename( wallet_file.generic_string() );
        wapiptr->load_wallet_file();
        aApiToCreate.wal_api = wapiptr.get();

        fc::api<graphene::wallet::wallet_api> wapi(wapiptr);

        auto wallet_gui = std::make_shared<fc::rpc::gui>();
        aApiToCreate.gui_api = wallet_gui.get();
        SetCurrentApis(&aApiToCreate);
        for( auto& name_formatter : wapiptr->get_result_formatters() )
           wallet_gui->format_result( name_formatter.first, name_formatter.second );

        boost::signals2::scoped_connection closed_connection(con->closed.connect([=]{
           //cerr << "Server has disconnected us.\n";
           // int CallFunctionInGuiLoop(SetNewTask_last_args,void* owner,TypeCallbackSetNewTaskGlb fpFnc);
           //(*(a_con_data.fn_tsk_dn))(a_con_data.owner,a_con_data.callbackArg,(int64_t)UNABLE_TO_CONNECT,
           //                          __CONNECTION_CLB_, __FILE__ "\nServer has disconnected us.");
           CallFunctionInGuiLoop(a_con_data.callbackArg,UNABLE_TO_CONNECT, __CONNECTION_CLB_,
                                 __FILE__ "\nServer has disconnected us.",
                                 a_con_data.owner,a_con_data.fn_tsk_dn2);
           wallet_gui->stop();
        }));
        (void)(closed_connection);

        if( wapiptr->is_new() )
        {
           std::string aPassword("");
           //std::cout << "Please use the set_password method to initialize a new wallet before continuing\n";
           //wallet_cli->set_prompt( "new >>> " );
           //(*(aStrct.fpWarnFunc))(a_con_data.owner,int answer,/*string**/void* str_ptr);
           WarnAndWaitFunc(a_con_data.owner,aStrct.fpWarnFunc,&aPassword,
                           "Please use the set_password method to initialize a new wallet before continuing\n");
           if(aPassword != "")
           {
               wapiptr->set_password(aPassword);
               wapiptr->unlock(aPassword);
           }
        } else
           {/*wallet_cli->set_prompt( "locked >>> " );*/}

        boost::signals2::scoped_connection locked_connection(wapiptr->lock_changed.connect([&](bool /*locked*/) {
           //wallet_cli->set_prompt(  locked ? "locked >>> " : "unlocked >>> " );
        }));


        //if( !options.count( "daemon" ) )
        if(1)
        {
           wallet_gui->register_api( wapi );
           wallet_gui->start();
           //(*a_fpDone)(a_pOwner);
           //(*(a_con_data.fn_tsk_dn))(a_con_data.owner,a_con_data.callbackArg,0,
           //                          __CONNECTION_CLB_, __FILE__ "\nConnection is ok");

           LoadWalletFile((SConnectionStruct*)a_con_data.callbackArg);

           CallFunctionInGuiLoop(a_con_data.callbackArg,0, __CONNECTION_CLB_,
                                 __FILE__ "\nConnection is ok",
                                 a_con_data.owner,a_con_data.fn_tsk_dn2);
           wallet_gui->wait();
        }
        else
        {
#if 0
          fc::promise<int>::ptr exit_promise = new fc::promise<int>("UNIX Signal Handler");
          fc::set_signal_handler([&exit_promise](int signal) {
             exit_promise->set_value(signal);
          }, SIGINT);

          ilog( "Entering Daemon Mode, ^C to exit" );
          exit_promise->wait();
#endif
        }

        wapi->save_wallet_file(wallet_file.generic_string());
        locked_connection.disconnect();
        closed_connection.disconnect();
    }
    catch(const fc::exception& a_fc)
    {
        if(g_nDebugApplication){printf("file:\"" __FILE__ "\",line:%d\n",__LINE__);}
        //(*a_fpErr)(a_pOwner,a_fc.to_string(),a_fc.to_detail_string());
        //(*(a_con_data.fn_tsk_dn))(a_con_data.owner,a_con_data.callbackArg,a_fc.code(),
        //                          __CONNECTION_CLB_, a_fc.to_detail_string());
        CallFunctionInGuiLoop(a_con_data.callbackArg,a_fc.code(), __CONNECTION_CLB_,
                              a_fc.to_detail_string(),
                              a_con_data.owner,a_con_data.fn_tsk_dn2);
        if(g_nDebugApplication){printf("%s\n",(a_fc.to_detail_string()).c_str());}
    }
    catch(...)
    {
        if(g_nDebugApplication){printf("file:\"" __FILE__ "\",line:%d\n",__LINE__);}
        //(*(a_con_data.fn_tsk_dn))(a_con_data.owner,a_con_data.callbackArg,UNKNOWN_EXCEPTION,
        //                          __CONNECTION_CLB_, __FILE__ "\nUnknown exception!");
        CallFunctionInGuiLoop(a_con_data.callbackArg,UNKNOWN_EXCEPTION, __CONNECTION_CLB_,
                              __FILE__ "\nUnknown exception!",
                              a_con_data.owner,a_con_data.fn_tsk_dn2);
        if(g_nDebugApplication){printf("Unknown exception\n");}
    }

    return 0;
}

