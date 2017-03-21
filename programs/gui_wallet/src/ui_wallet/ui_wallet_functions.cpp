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
#include <iostream>
#include "fc_rpc_gui.hpp"
#include <graphene/wallet/wallet.hpp>
#include <thread>
#include "unnamedsemaphorelite.hpp"
#include "decent_tool_fifo.hpp"
#include <fc/network/http/websocket.hpp>
#include <fc/rpc/websocket_api.hpp>
#include <fc/thread/thread.hpp>

#include "decent_gui_inguiloopcaller_glb.hpp"
#include "decent_tools_rwlock.hpp"


#include <QCoreApplication>
#include <QEventLoop>
#include <QTime>

#include <chrono>
#include <thread>
#ifdef WIN32
#include <windows.h>
#else  // #ifdef WIN32
#include <unistd.h>
#define Sleep(__x__) usleep(1000*(__x__))
#endif  // #ifdef WIN32

#ifndef __DLL_EXPORT__
#ifdef __MSC_VER
#define __DLL_EXPORT__ _declspec(dllexport)
#else  // #ifdef __MSC_VER
#define __DLL_EXPORT__
#endif  // #ifdef __MSC_VER
#endif  // #ifndef __DLL_EXPORT__

#define CONN_FNC_TYPE   TypeCallbackSetNewTaskGlb2
#define NewTestMutex decent::tools::RWLock
//#define NewTestMutex std::mutex
//#define NewTestMutex NewTestMutex2

struct StructApi {
    StructApi(): wal_api(NULL), gui_api(NULL) {

    } 

    graphene::wallet::wallet_api* wal_api; 
    fc::rpc::gui* gui_api;
};


/*//// Variables those should be inited!///*/
static int                          s_nLibraryInited = 0;
static TypeWarnAndWaitFunc          s_fpWarnAndWaitFunc;
static void*                        s_pManagerOwner;
static void*                        s_pManagerClbData;
static TypeManagementClbk          s_fpMenegmentClbk;
static TypeCallFunctionInGuiLoop2    s_fpCorrectUiCaller2;
static TypeCallFunctionInGuiLoop3    s_fpCorrectUiCaller3;
static NewTestMutex*                                            s_pMutex_for_cur_api; // It is better to use here rw mutex
static decent::tools::FiFo<SConnectionStruct*,CONN_FNC_TYPE>*   s_pConnectionRequestFifo;
static decent::tools::UnnamedSemaphoreLite*                     s_pSema_for_connection_thread;
static graphene::wallet::wallet_data*                           s_wdata_ptr;
static std::thread*                                             s_pConnectionThread ;
static std::thread*                                             s_pManagementThread ;
/* /// Variables those do not need initialization */
static volatile int s_nManagerThreadRun;
static volatile int s_nConThreadRun;
static StructApi      s_CurrentApi;

static void ConnectionThreadFunction(void);
static void gui_wallet_application_MenegerThreadFunc(void);
static int SaveWalletFile_private(const SConnectionStruct& a_WalletData);
static int ConnectToNewWitness(const decent::tools::taskListItem<SConnectionStruct*,CONN_FNC_TYPE>&);

#ifdef __cplusplus
//extern "C"{
#endif






void QtDelay( int millisecondsToWait )
{
    QTime dieTime = QTime::currentTime().addMSecs( millisecondsToWait );
    while( QTime::currentTime() < dieTime )
    {
        QCoreApplication::processEvents( QEventLoop::AllEvents, 100 );
    }
}





__DLL_EXPORT__ void* GetFunctionPointerAsVoid(int a_first,...)
{
    va_list aFunc;

    va_start( aFunc, a_first );
    void* pReturn = va_arg( aFunc, void*);
    va_end( aFunc );

    return pReturn;
}


__DLL_EXPORT__ void InitializeUiInterfaceOfWallet_base(TypeWarnAndWaitFunc a_fpWarnAndWait,
                                                       TypeCallFunctionInGuiLoop2 a_fpCorrectUiCaller2,
                                                       TypeCallFunctionInGuiLoop3 a_fpCorrectUiCaller3,
                                                       void* a_pMngOwner,void* a_pMngClb,...)
{
    int nLibInited(s_nLibraryInited++);
    s_nLibraryInited = 1;

    va_list aFunc;

    va_start( aFunc, a_pMngClb );
    TypeManagementClbk fpMenegmentClbk2 = (TypeManagementClbk)va_arg( aFunc, void*);
    va_end( aFunc );

    __DEBUG_APP2__(1,"fpWarn=%p, fpCaller2=%p, fpCaller3=%p, pMnOwner=%p, pMngrClb=%p, fpMngClb=%p",
                   GetFunctionPointerAsVoid(0,a_fpWarnAndWait),GetFunctionPointerAsVoid(0,a_fpCorrectUiCaller2),
                   GetFunctionPointerAsVoid(0,a_fpCorrectUiCaller3),a_pMngOwner,a_pMngClb,
                   GetFunctionPointerAsVoid(0,fpMenegmentClbk2));

    if(!nLibInited)  // should be done in real atomic manner
    {
        s_fpWarnAndWaitFunc = a_fpWarnAndWait;
        s_pManagerOwner = a_pMngOwner;
        s_pManagerClbData = a_pMngClb;
        s_fpMenegmentClbk = fpMenegmentClbk2;
        s_fpCorrectUiCaller2 = a_fpCorrectUiCaller2;
        s_fpCorrectUiCaller3 = a_fpCorrectUiCaller3;

        s_pMutex_for_cur_api = new NewTestMutex;
        s_pConnectionRequestFifo = new decent::tools::FiFo<SConnectionStruct*,TypeCallbackSetNewTaskGlb2>;

        s_pSema_for_connection_thread = new decent::tools::UnnamedSemaphoreLite;
        
        s_wdata_ptr = new graphene::wallet::wallet_data;
        
        
        /* after all data is inited we can create the threads */
        s_pConnectionThread = new std::thread(&ConnectionThreadFunction);
        
        s_pManagementThread = new std::thread(&gui_wallet_application_MenegerThreadFunc);
        
    }

}


__DLL_EXPORT__ void DestroyUiInterfaceOfWallet(void)
{
    int nLibInited(s_nLibraryInited--);
    s_nLibraryInited = 0;
    if(nLibInited>0)  // should be done in real atomic manner
    {
        s_nManagerThreadRun = 0;
        s_nConThreadRun = 0;
        s_pSema_for_connection_thread->post();
        s_pManagementThread->join();
        s_pConnectionThread->join();
        delete s_pManagementThread;
        delete s_pConnectionThread;
        delete s_wdata_ptr;
        delete s_pSema_for_connection_thread;
        delete s_pConnectionRequestFifo;
        delete s_pMutex_for_cur_api;
    }
}


__DLL_EXPORT__ void StartConnectionProcedure(SConnectionStruct* a_conn_str_ptr,void *a_owner, void*a_clbData)
{
    s_pConnectionRequestFifo->AddNewTask(TIT::AS_STR, a_conn_str_ptr,a_owner,a_clbData,a_conn_str_ptr->fpDone);
    s_pSema_for_connection_thread->post();

}


#if 1
__DLL_EXPORT__ int SetNewTask2(const std::string& a_inp_line, void* a_owner, void* a_clbData,
                               TypeCallbackSetNewTaskGlb2 a_clbkFunction)
{
    return SetNewTask_base(TIT::AS_STR,a_inp_line, a_owner, a_clbData, a_clbkFunction);
}
#endif

typedef void*  void_ptr;

__DLL_EXPORT__ int SetNewTask_base(int a_nType,const std::string& a_inp_line, void* a_owner, void* a_clbData, ...)
{
    int nReturn = 0;
    std::string errStr("error accured!");
    void_ptr fpTaskDone3;
    va_list aFunc;

    va_start( aFunc, a_clbData );
    fpTaskDone3 = va_arg( aFunc, void_ptr);
    va_end( aFunc );

    std::lock_guard<NewTestMutex> lock(*s_pMutex_for_cur_api);
    if(s_CurrentApi.gui_api)
    {
        nReturn = 0;
        (s_CurrentApi.gui_api)->SetNewTask_base(a_nType, a_inp_line,a_owner,a_clbData,fpTaskDone3);
    }
    else if(strstr(a_inp_line.c_str(),"load_wallet_file "))
    {
        static SConnectionStruct aConStr;
        const char* cpcWlFlName = a_inp_line.c_str() + strlen("load_wallet_file ");
        for(;*cpcWlFlName == ' ' && *cpcWlFlName != 0;++cpcWlFlName);

        if(*cpcWlFlName)
        {
            aConStr.wallet_file_name = cpcWlFlName;
            nReturn = LoadWalletFile(&aConStr);

            if(!nReturn)
            {
                aConStr.action = WAT::CONNECT;
                aConStr.fpDone = (TypeCallbackSetNewTaskGlb2)fpTaskDone3;
                s_pConnectionRequestFifo->AddNewTask(a_nType,&aConStr,a_owner,a_clbData,fpTaskDone3);
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

    if (nReturn) {
        //(*fpTaskDone)(a_owner, a_clbData,NO_API_INITED, a_inp_line, errStr);
    }

    return nReturn;
}


__DLL_EXPORT__ int LoadWalletFile(SConnectionStruct* a_pWalletData)
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



__DLL_EXPORT__ int SaveWalletFile2(const SConnectionStruct& a_WalletData)
{
    int nReturn(0);
    s_pMutex_for_cur_api->lock();
    try{SaveWalletFile_private(a_WalletData);}
    catch(...){nReturn=UNKNOWN_EXCEPTION;}
    s_pMutex_for_cur_api->unlock();
    return nReturn;
}


#ifdef __cplusplus
//extern "C"{
#endif




/*///////////////////////////////////////////////////////////*/

//void (__THISCALL__ *TypeCallbackSetNewTaskGlb)(void* owner,SetNewTask_last_args)


static int SaveWalletFile_private(const SConnectionStruct& a_WalletData)
{
    int nReturn(0);
    s_wdata_ptr->ws_server = a_WalletData.ws_server;
    s_wdata_ptr->ws_user = a_WalletData.ws_user ;
    s_wdata_ptr->ws_password = a_WalletData.ws_password;
    //m_wdata.chain_id = chain_id_type( std::string( (((QLineEdit*)m_main_table.cellWidget(CHAIN_ID_FIELD,1))->text()).toLatin1().data() ) );
    s_wdata_ptr->chain_id = chain_id_type(a_WalletData.chain_id);

    if(s_CurrentApi.wal_api){(s_CurrentApi.wal_api)->save_wallet_file(a_WalletData.wallet_file_name);}
    else{nReturn=NO_API_INITED;}
    return nReturn;
}


static void gui_wallet_application_MenegerThreadFunc(void)
{
    char vnOpt[WAS::_API_STATE_SIZE];
    int i;

    s_nManagerThreadRun = 1;

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
                
                (*s_fpCorrectUiCaller2)(s_pManagerClbData,(int64_t)i, __MANAGER_CLB_,
                                      __FILE__ "\nManagement",s_pManagerOwner,s_fpMenegmentClbk);
                vnOpt[i] = 0;
            }
        }  // for(i=0;i<_API_STATE_SIZE;++i)


       Sleep(100);
    } // while(s_nManagerThreadRun)
}


static void ConnectionThreadFunction(void)
{
    decent::tools::taskListItem<SConnectionStruct*,CONN_FNC_TYPE> aTaskItem(NULL,NULL);
    //int nConnectReturn;
    s_nConThreadRun = 1;
    std::thread* pConnectionThread;

    while(s_nConThreadRun)
    {
        s_pSema_for_connection_thread->wait();

        while(s_pConnectionRequestFifo->GetFirstTask(&aTaskItem))
        {
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




static int ConnectToNewWitness(const decent::tools::taskListItem<SConnectionStruct*,CONN_FNC_TYPE>& a_con_data)
{
    try
    {
        SConnectionStruct* pStruct = a_con_data.input;

        if(pStruct->action == WAT::SAVE2)
        {
            int nReturn(SaveWalletFile2(*pStruct));
            (*s_fpCorrectUiCaller2)(a_con_data.callbackArg,(int64_t)nReturn, __CONNECTION_CLB_,
                                  __FILE__ "\nSaving procedure",a_con_data.owner,a_con_data.fn_tsk_dn2);
            return nReturn;
        }
        else if(pStruct->action == WAT::LOAD2)
        {
            //m_wdata = fc::json::from_file( csWalletFileName ).as< graphene::wallet::wallet_data >();
            int nReturn(LoadWalletFile((SConnectionStruct*)a_con_data.callbackArg));
            (*s_fpCorrectUiCaller2)(a_con_data.callbackArg,(int64_t)nReturn, __CONNECTION_CLB_,
                                  __FILE__ "\nLoading procedure",a_con_data.owner,a_con_data.fn_tsk_dn2);
            return nReturn;
        }
        else if(pStruct->action != WAT::CONNECT)
        {
            (*s_fpCorrectUiCaller2)(a_con_data.callbackArg,UNKNOWN_EXCEPTION, __CONNECTION_CLB_,
                                  __FILE__ "Unknown option",a_con_data.owner,a_con_data.fn_tsk_dn2);
            return UNKNOWN_EXCEPTION;
        }

        StructApi aApiToCreate;
        //const graphene::wallet::wallet_data wdata; wdata.ws_password;
        s_wdata_ptr->ws_server = pStruct->ws_server;
        s_wdata_ptr->ws_user = pStruct->ws_user;
        s_wdata_ptr->ws_password = pStruct->ws_password;
        fc::path wallet_file( pStruct->wallet_file_name );

        fc::http::websocket_client client;
        //idump((wdata.ws_server));
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

            (*s_fpCorrectUiCaller2)(a_con_data.callbackArg,UNABLE_TO_CONNECT, __CONNECTION_CLB_,
                                 __FILE__ "\nServer has disconnected us.",
                                 a_con_data.owner,a_con_data.fn_tsk_dn2);
           wallet_gui->stop();
        }));
        (void)(closed_connection);

       

         wallet_gui->register_api( wapi );
         wallet_gui->start();

       if( wapiptr->is_new() )
        {
           std::string aPassword("");
           
           (*s_fpWarnAndWaitFunc)(a_con_data.owner,pStruct->setPasswordFn, &aPassword, "Please use the set_password method to initialize a new wallet before continuing\n");
           if(aPassword != "")
           {
               wapiptr->set_password(aPassword);
               wapiptr->unlock(aPassword);
           }
        }
       
        boost::signals2::scoped_connection locked_connection(wapiptr->lock_changed.connect([&](bool /*locked*/) {
           //wallet_cli->set_prompt(  locked ? "locked >>> " : "unlocked >>> " );
        }));



       LoadWalletFile(pStruct);

       std::string possible_input = __CONNECTION_CLB_;
       if(pStruct->wallet_file_name != "" ){possible_input = "load_wallet_file " + pStruct->wallet_file_name;}

       (*s_fpCorrectUiCaller2)(a_con_data.callbackArg,0, possible_input,
                             "true",
                             a_con_data.owner,a_con_data.fn_tsk_dn2);
       wallet_gui->wait();

        wapi->save_wallet_file(wallet_file.generic_string());
        locked_connection.disconnect();
        closed_connection.disconnect();
    }
    catch(const fc::exception& a_fc)
    {
        int64_t llnErr = a_fc.code() ? a_fc.code() : -2;
        (*s_fpCorrectUiCaller2)(a_con_data.callbackArg,llnErr, a_fc.to_string(),
                              a_fc.to_detail_string(),
                              a_con_data.owner,a_con_data.fn_tsk_dn2);
    }
    catch(...)
    {
        (*s_fpCorrectUiCaller2)(a_con_data.callbackArg,UNKNOWN_EXCEPTION, __CONNECTION_CLB_,
                              __FILE__ "\nUnknown exception!",
                              a_con_data.owner,a_con_data.fn_tsk_dn2);
    }

    return 0;
}


#if 0
static int CallFunctionInUiLoop_static(int a_nType,SetNewTask_last_args2,void* a_result,void* a_owner,void* a_fpFnc);

int CallFunctionInUiLoop2(SetNewTask_last_args2,const std::string& a_result,void* a_owner,TypeCallbackSetNewTaskGlb2 a_fpFnc)
{
    return CallFunctionInUiLoop_static(TIT::AS_STR,a_clbData,a_err,a_inp,(void*)&a_result,a_owner,GetFunctionPointerAsVoid(1,a_fpFnc));
}

int CallFunctionInUiLoop3(SetNewTask_last_args2,void* a_variant,void* a_owner,void* a_fpFnc)
{
    return CallFunctionInUiLoop_static(TIT::AS_STR,a_clbData,a_err,a_inp,a_variant,a_owner,a_fpFnc);
}
#endif



int CallFunctionInUiLoopGeneral(int a_nType,SetNewTask_last_args2,
                                const fc::variant& a_resultVar,const std::string& a_resultStr,
                                void* a_owner,void* a_fpFnc)
{
    


    switch(a_nType)
    {
    case TIT::AS_VARIANT:
    {
        TypeCallbackSetNewTaskGlb3 fpFunc3 = (TypeCallbackSetNewTaskGlb3)a_fpFnc;
        return (*s_fpCorrectUiCaller3)(a_clbData,a_err, a_inp, a_resultVar,
                                       a_owner,fpFunc3);
    }
    default:
    {

        TypeCallbackSetNewTaskGlb2 fpFunc2 = (TypeCallbackSetNewTaskGlb2)a_fpFnc;
        return (*s_fpCorrectUiCaller2)(a_clbData,a_err, a_inp, a_resultStr,
                                       a_owner,fpFunc2);
    }
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

void RunTask(std::string const& str_command, std::string& str_result)
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

