
#include <mutex>
#include <stdio.h>
#include <stdarg.h>
#include <iostream>
#include <thread>

#include <graphene/wallet/wallet.hpp>

#include <fc/network/http/websocket.hpp>
#include <fc/rpc/websocket_api.hpp>
#include <fc/thread/thread.hpp>

#include <boost/filesystem.hpp>


#include "decent_gui_inguiloopcaller.hpp"
#include "decent_tools_rwlock.hpp"
#include "ui_wallet_functions.hpp"
#include "fc_rpc_gui.hpp"
#include "unnamedsemaphorelite.hpp"
#include "decent_tool_fifo.hpp"


#include <QCoreApplication>
#include <QEventLoop>
#include <QTime>

#include <chrono>

#ifdef WIN32
   #include <windows.h>
#else
   #include <unistd.h>
   #define Sleep(__x__) usleep(1000*(__x__))
#endif

#ifndef __DLL_EXPORT__
   #ifdef __MSC_VER
      #define __DLL_EXPORT__ _declspec(dllexport)
   #else
      #define __DLL_EXPORT__
   #endif
#endif




using namespace gui_wallet;


struct StructApi {
    StructApi(): wal_api(NULL), gui_api(NULL) {

    } 

    graphene::wallet::wallet_api* wal_api; 
    fc::rpc::gui* gui_api;
};


static int                          s_nLibraryInited = 0;

static RWLock*                                                  s_pMutex_for_cur_api; // It is better to use here rw mutex
static FiFo<ConnectListItem>*                                   s_pConnectionRequestFifo;
static UnnamedSemaphoreLite*                                    s_pSema_for_connection_thread;
static graphene::wallet::wallet_data*                           s_wdata_ptr;
static std::thread*                                             s_pConnectionThread ;


static volatile int s_nConThreadRun;
static StructApi      s_CurrentApi;

static void ConnectionThreadFunction(void);
static int SaveWalletFile_private(const SConnectionStruct& a_WalletData);
static int ConnectToNewWitness(const ConnectListItem& item);




int WarnAndWaitFunc(void* a_pOwner,WarnYesOrNoFuncType a_fpYesOrNo, void* a_pDataForYesOrNo,const char* a_form,...);


void QtDelay( int millisecondsToWait )
{
    QTime dieTime = QTime::currentTime().addMSecs( millisecondsToWait );
    while( QTime::currentTime() < dieTime )
    {
        QCoreApplication::processEvents( QEventLoop::AllEvents, 100 );
    }
}




void WalletInterface::initialize() {

   int nLibInited(s_nLibraryInited++);
    s_nLibraryInited = 1;
   
    if(!nLibInited)  // should be done in real atomic manner
    {
        s_pMutex_for_cur_api = new RWLock;
        s_pConnectionRequestFifo = new FiFo<ConnectListItem>;

        s_pSema_for_connection_thread = new UnnamedSemaphoreLite;
        
        s_wdata_ptr = new graphene::wallet::wallet_data;
       
       s_pConnectionThread = new std::thread(&ConnectionThreadFunction);
       
    }

}


void WalletInterface::startConnecting(SConnectionStruct* a_conn_str_ptr, void *a_owner, void*a_clbData) {
   s_pConnectionRequestFifo->AddNewTask(a_conn_str_ptr,a_owner,a_clbData,a_conn_str_ptr->fpDone);
   s_pSema_for_connection_thread->post();
}



int WalletInterface::callFunctionInGuiLoop(void* a_clbData, int64_t a_err, const std::string& a_inp, const std::string& a_result,void* a_owner,TypeCallbackSetNewTaskGlb2 a_fpFunc) {
   InGuiLoopCaller* caller = InGuiLoopCaller::instance();
   if (caller == NULL) {
      return -1;
   }
   caller->CallFunctionInGuiLoop2(a_clbData,a_err,a_inp,a_result,a_owner,a_fpFunc);
   return 0;
}


void WalletInterface::destroy() {
    int nLibInited(s_nLibraryInited--);
    s_nLibraryInited = 0;
    if(nLibInited>0)  // should be done in real atomic manner
    {
        s_nConThreadRun = 0;
        s_pSema_for_connection_thread->post();
        s_pConnectionThread->join();
        delete s_pConnectionThread;
        delete s_wdata_ptr;
        delete s_pSema_for_connection_thread;
        delete s_pConnectionRequestFifo;
        delete s_pMutex_for_cur_api;
    }
}



__DLL_EXPORT__ int gui_wallet::SetNewTask(const std::string& a_inp_line, void* a_owner, void* a_clbData,
                               TypeCallbackSetNewTaskGlb2 a_clbkFunction)
{
    return SetNewTask_base(a_inp_line, a_owner, a_clbData, a_clbkFunction);
}



__DLL_EXPORT__ int gui_wallet::SetNewTask_base(const std::string& a_inp_line, void* a_owner, void* a_clbData, ...)
{
    int nReturn = 0;
    std::string errStr("error accured!");
    void* fpTaskDone3;
    va_list aFunc;

    va_start( aFunc, a_clbData );
    fpTaskDone3 = va_arg( aFunc, void*);
    va_end( aFunc );

    std::lock_guard<RWLock> lock(*s_pMutex_for_cur_api);
    if(s_CurrentApi.gui_api)
    {
        nReturn = 0;
        (s_CurrentApi.gui_api)->SetNewTask(a_inp_line,a_owner,a_clbData,fpTaskDone3);
    }
    else if(strstr(a_inp_line.c_str(),"load_wallet_file "))
    {
        static SConnectionStruct aConStr;
        const char* cpcWlFlName = a_inp_line.c_str() + strlen("load_wallet_file ");
        for(;*cpcWlFlName == ' ' && *cpcWlFlName != 0;++cpcWlFlName);

        if(*cpcWlFlName)
        {
            aConStr.wallet_file_name = cpcWlFlName;
            nReturn = WalletInterface::LoadWalletFile(&aConStr);

            if(!nReturn)
            {
                aConStr.action = WAT::CONNECT;
                aConStr.fpDone = (TypeCallbackSetNewTaskGlb2)fpTaskDone3;
                s_pConnectionRequestFifo->AddNewTask(&aConStr,a_owner,a_clbData,fpTaskDone3);
                s_pSema_for_connection_thread->post();
            }

        }
        else{nReturn = WRONG_ARGUMENT;}
    }
    else
    {
        nReturn = NO_API_INITED;
        errStr = "First connect to witness node";
    }

    return nReturn;
}


int WalletInterface::LoadWalletFile(SConnectionStruct* a_pWalletData) {
    int nReturn = 0;

    if(!a_pWalletData) {
        return WRONG_ARGUMENT;
    }

    if(!boost::filesystem::exists(a_pWalletData->wallet_file_name)) {
        return FILE_DOES_NOT_EXIST;
    }
   

    try {
        *s_wdata_ptr = fc::json::from_file( a_pWalletData->wallet_file_name ).as< graphene::wallet::wallet_data >();
        a_pWalletData->ws_server = s_wdata_ptr->ws_server;
        a_pWalletData->ws_user = s_wdata_ptr->ws_user;
        a_pWalletData->ws_password = s_wdata_ptr->ws_password;
        a_pWalletData->chain_id = s_wdata_ptr->chain_id.str().c_str();
    }
    catch(...)
    {
        nReturn = UNKNOWN_EXCEPTION;
    }

    return nReturn;
}



int WalletInterface::SaveWalletFile(const SConnectionStruct& a_WalletData) {
    int nReturn = 0;
    s_pMutex_for_cur_api->lock();
   
    try {
       s_wdata_ptr->ws_server = a_WalletData.ws_server;
       s_wdata_ptr->ws_user = a_WalletData.ws_user ;
       s_wdata_ptr->ws_password = a_WalletData.ws_password;
       s_wdata_ptr->chain_id = chain_id_type(a_WalletData.chain_id);
       
       if (s_CurrentApi.wal_api) {
          (s_CurrentApi.wal_api)->save_wallet_file(a_WalletData.wallet_file_name);
       } else {
          nReturn = NO_API_INITED;
       }

    } catch(...) {
       nReturn = UNKNOWN_EXCEPTION;
    }
    s_pMutex_for_cur_api->unlock();
    return nReturn;
}





static void ConnectionThreadFunction(void)
{
    ConnectListItem aTaskItem(NULL,NULL);
    s_nConThreadRun = 1;
    std::thread* pConnectionThread;

    while(s_nConThreadRun)
    {
        s_pSema_for_connection_thread->wait();

        while(s_pConnectionRequestFifo->GetFirstTask(&aTaskItem))
        {
            pConnectionThread = new std::thread(&ConnectToNewWitness, aTaskItem);
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




static int ConnectToNewWitness(const ConnectListItem& a_con_data) {
    try {
       
        SConnectionStruct* pStruct = a_con_data.input;

        if(pStruct->action == WAT::SAVE2)
        {
           int nReturn = WalletInterface::SaveWalletFile(*pStruct);
            WalletInterface::callFunctionInGuiLoop(a_con_data.callbackArg,(int64_t)nReturn, __CONNECTION_CLB_, "Saving procedure",a_con_data.owner,a_con_data.callback);
            return nReturn;
        }
        else if(pStruct->action == WAT::LOAD2)
        {
           
            int nReturn = WalletInterface::LoadWalletFile((SConnectionStruct*)a_con_data.callbackArg);
           
           WalletInterface::callFunctionInGuiLoop(a_con_data.callbackArg,(int64_t)nReturn, __CONNECTION_CLB_, "Loading procedure",a_con_data.owner,a_con_data.callback);
            return nReturn;
        }
        else if(pStruct->action != WAT::CONNECT)
        {
            WalletInterface::callFunctionInGuiLoop(a_con_data.callbackArg,UNKNOWN_EXCEPTION, __CONNECTION_CLB_, "Unknown option",a_con_data.owner,a_con_data.callback);
            return UNKNOWN_EXCEPTION;
        }

        StructApi aApiToCreate;
        s_wdata_ptr->ws_server = pStruct->ws_server;
        s_wdata_ptr->ws_user = pStruct->ws_user;
        s_wdata_ptr->ws_password = pStruct->ws_password;
        fc::path wallet_file( pStruct->wallet_file_name );

        fc::http::websocket_client client;
        fc::http::websocket_connection_ptr con;
        
        
        // Try to connect to server again and again and again and again and again and again and again and ...
        bool connection_error = true;
        while (connection_error) {
            try {
               con = client.connect( pStruct->ws_server );
                connection_error = false;
            } catch (...) {
                connection_error = true;
                QtDelay(1000);
            }
        }
        auto apic = std::make_shared<fc::rpc::websocket_api_connection>(*con);

        auto remote_api = apic->get_remote_api< login_api >(1);
        //edump((wdata.ws_user)(wdata.ws_password) );
        // TODO:  Error message here
        FC_ASSERT( remote_api->login( pStruct->ws_user, pStruct->ws_password ) );

        auto wapiptr = std::make_shared<graphene::wallet::wallet_api>( *s_wdata_ptr, remote_api );
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

            WalletInterface::callFunctionInGuiLoop(a_con_data.callbackArg,UNABLE_TO_CONNECT, __CONNECTION_CLB_, "Server has disconnected us.", a_con_data.owner,a_con_data.callback);
           wallet_gui->stop();
        }));
        (void)(closed_connection);

       

         wallet_gui->register_api( wapi );
         wallet_gui->start();

       if( wapiptr->is_new() )
        {
           std::string aPassword("");
           
           WarnAndWaitFunc(a_con_data.owner,pStruct->setPasswordFn, &aPassword, "Please use the set_password method to initialize a new wallet before continuing\n");
           if(aPassword != "")
           {
               wapiptr->set_password(aPassword);
               wapiptr->unlock(aPassword);
           }
        }
       
        boost::signals2::scoped_connection locked_connection(wapiptr->lock_changed.connect([&](bool /*locked*/) {
           //wallet_cli->set_prompt(  locked ? "locked >>> " : "unlocked >>> " );
        }));



       WalletInterface::LoadWalletFile(pStruct);

       std::string possible_input = __CONNECTION_CLB_;
       if(pStruct->wallet_file_name != "" ){possible_input = "load_wallet_file " + pStruct->wallet_file_name;}

       WalletInterface::callFunctionInGuiLoop(a_con_data.callbackArg,0, possible_input, "true", a_con_data.owner,a_con_data.callback);
       wallet_gui->wait();

        wapi->save_wallet_file(wallet_file.generic_string());
        locked_connection.disconnect();
        closed_connection.disconnect();
    }
    catch(const fc::exception& a_fc) {
       
        int64_t llnErr = a_fc.code() ? a_fc.code() : -2;
        WalletInterface::callFunctionInGuiLoop(a_con_data.callbackArg,llnErr, a_fc.to_string(),
                              a_fc.to_detail_string(),
                              a_con_data.owner,a_con_data.callback);
       
    } catch(...) {
       
        WalletInterface::callFunctionInGuiLoop(a_con_data.callbackArg, UNKNOWN_EXCEPTION, __CONNECTION_CLB_,
                              "Unknown exception!",
                              a_con_data.owner,a_con_data.callback);
    }

    return 0;
}
















class task_exception : public std::exception
{
public:
    task_exception(std::string const& str_info) noexcept
    : m_str_info(str_info) {}
    virtual ~task_exception() {}
    
    char const* what() const noexcept override
    {
        return m_str_info.c_str();
    }
private:
    std::string m_str_info;
};

struct task_result
{
    task_result()
    : m_bDone(false)
    , m_error(0)
    , m_strResult() {}
    
    bool m_bDone;
    int64_t m_error;
    std::string m_strResult;
};

void gui_wallet::RunTask(std::string const& str_command, std::string& str_result)
{
    task_result result;
    SetNewTask(str_command,
               nullptr,
               static_cast<void*>(&result),
               +[](void* /*owner*/,
                   void* a_clbkArg,
                   int64_t a_err,
                   std::string const& /*a_task*/,
                   std::string const& a_result)
               {
                   task_result& result = *static_cast<task_result*>(a_clbkArg);
                   result.m_bDone = true;
                   result.m_error = a_err;
                   result.m_strResult = a_result;
               });
    
    bool volatile& bDone = result.m_bDone;
    while (false == bDone)
    {
        std::this_thread::sleep_for(chrono::milliseconds(0));
        QCoreApplication::processEvents();
    }
    
    if (0 == result.m_error)
        str_result = result.m_strResult;
    else
        throw task_exception(result.m_strResult);
}

