
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
#include "gui_wallet_global.hpp"
#include "gui_wallet_application.hpp"

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


using namespace gui_wallet;


namespace {
   
class task_exception : public std::exception
{
public:
   task_exception(std::string const& str_info) noexcept : m_str_info(str_info) {}
   virtual ~task_exception() {}
   
   char const* what() const noexcept override
   {
      return m_str_info.c_str();
   }
private:
   std::string m_str_info;
};

struct task_result {
   bool        m_bDone = false;
   int64_t     m_error = 0;
   std::string m_strResult;
};

}




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


static volatile int   s_nConThreadRun;
static StructApi      s_CurrentApi;





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
       
       s_pConnectionThread = new std::thread(&WalletInterface::connectionThreadFunction);
       
    }

}


void WalletInterface::connectedCallback(void* owner, void* a_clbData, int64_t a_err, const std::string& a_inp, const std::string& a_result) {
   if(a_err) {
      GlobalEvents::instance().setWalletError(a_result);
      return;
   }
   
   GlobalEvents::instance().setWalletConnected();
}



void WalletInterface::startConnecting(SConnectionStruct* connectionInfo) {
   s_pConnectionRequestFifo->AddNewTask(connectionInfo, connectionInfo->owner, NULL, &WalletInterface::connectedCallback);
   s_pSema_for_connection_thread->post();
}



void WalletInterface::connectionThreadFunction() {
   ConnectListItem aTaskItem(NULL,NULL);
   s_nConThreadRun = 1;
   std::thread* pConnectionThread;
   
   while(s_nConThreadRun)
   {
      s_pSema_for_connection_thread->wait();
      
      while(s_pConnectionRequestFifo->GetFirstTask(&aTaskItem))
      {
         pConnectionThread = new std::thread(&WalletInterface::connectToNewWitness, aTaskItem);
         if(pConnectionThread){
            pConnectionThread->detach();
            delete pConnectionThread;
         }
      }
      
   }
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
   
    if(nLibInited > 0) {
       
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




int WalletInterface::setNewTask(const std::string& a_inp_line, void* a_owner, void* a_clbData, TypeCallbackSetNewTaskGlb2 fpTaskDone) {
    int nReturn = 0;
   
    std::lock_guard<RWLock> lock(*s_pMutex_for_cur_api);
    if(s_CurrentApi.gui_api)
    {
        nReturn = 0;
        (s_CurrentApi.gui_api)->SetNewTask(a_inp_line,a_owner,a_clbData,fpTaskDone);
    }
    else if(strstr(a_inp_line.c_str(),"load_wallet_file "))
    {
        static SConnectionStruct aConStr;
        const char* cpcWlFlName = a_inp_line.c_str() + strlen("load_wallet_file ");
        for(;*cpcWlFlName == ' ' && *cpcWlFlName != 0;++cpcWlFlName);

        if(*cpcWlFlName)
        {
            aConStr.wallet_file_name = cpcWlFlName;
            nReturn = WalletInterface::loadWalletFile(&aConStr);

            if(!nReturn)
            {
                s_pConnectionRequestFifo->AddNewTask(&aConStr,a_owner,a_clbData, fpTaskDone);
                s_pSema_for_connection_thread->post();
            }

        } else {
           nReturn = WRONG_ARGUMENT;
        }
    } else {
        nReturn = NO_API_INITED;
    }

    return nReturn;
}


void WalletInterface::runTask(std::string const& str_command, std::string& str_result)
{
   task_result result;
   setNewTask(str_command,
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




int WalletInterface::loadWalletFile(SConnectionStruct* a_pWalletData) {
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



int WalletInterface::saveWalletFile(const SConnectionStruct& a_WalletData) {
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










int WalletInterface::connectToNewWitness(const ConnectListItem& a_con_data) {
   try {
      
      SConnectionStruct* pStruct = a_con_data.input;

      
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

      FC_ASSERT( remote_api->login( pStruct->ws_user, pStruct->ws_password ) );
      
      auto wapiptr = std::make_shared<graphene::wallet::wallet_api>( *s_wdata_ptr, remote_api );
      wapiptr->set_wallet_filename( wallet_file.generic_string() );
      wapiptr->load_wallet_file();
      aApiToCreate.wal_api = wapiptr.get();
      
      fc::api<graphene::wallet::wallet_api> wapi(wapiptr);
      
      auto wallet_gui = std::make_shared<fc::rpc::gui>();
      aApiToCreate.gui_api = wallet_gui.get();
      
      
      s_pMutex_for_cur_api->write_lock();
      memcpy(&s_CurrentApi, &aApiToCreate,sizeof(StructApi));
      s_pMutex_for_cur_api->unlock();
      
      for( auto& name_formatter : wapiptr->get_result_formatters() )
         wallet_gui->format_result( name_formatter.first, name_formatter.second );
      
      boost::signals2::scoped_connection closed_connection(con->closed.connect([=]{
         
         WalletInterface::callFunctionInGuiLoop(a_con_data.callbackArg,UNABLE_TO_CONNECT, "", "Server has disconnected us.", a_con_data.owner,a_con_data.callback);
         wallet_gui->stop();
      }));
      (void)(closed_connection);
      
      
      
      wallet_gui->register_api( wapi );
      wallet_gui->start();
      
      if( wapiptr->is_new() )
      {
         std::string aPassword;
         QString aString = "Please use the set_password method to initialize a new wallet before continuing";
         
         InGuiThreatCaller::instance()->m_pParent2 = (QWidget*)a_con_data.owner;
         InGuiThreatCaller::instance()->EmitShowMessageBox(aString, pStruct->setPasswordFn, &aPassword);
         InGuiThreatCaller::instance()->m_sema.wait();
         
         if(aPassword != "") {
            wapiptr->set_password(aPassword);
            wapiptr->unlock(aPassword);
         }
      }
      
      
      WalletInterface::loadWalletFile(pStruct);
      
      
      if(pStruct->wallet_file_name != "" ) {
         WalletInterface::callFunctionInGuiLoop(a_con_data.callbackArg, 0, "load_wallet_file " + pStruct->wallet_file_name, "true", a_con_data.owner,a_con_data.callback);
         
      }
      
      wallet_gui->wait();
      
      wapi->save_wallet_file(wallet_file.generic_string());
      closed_connection.disconnect();
   }
   catch(const fc::exception& a_fc) {
      
      int64_t llnErr = a_fc.code() ? a_fc.code() : -2;
      WalletInterface::callFunctionInGuiLoop(a_con_data.callbackArg,llnErr, a_fc.to_string(),
                                             a_fc.to_detail_string(),
                                             a_con_data.owner,a_con_data.callback);
      
   } catch(...) {
      
      WalletInterface::callFunctionInGuiLoop(a_con_data.callbackArg, UNKNOWN_EXCEPTION, "",
                                             "Unknown exception!",
                                             a_con_data.owner,a_con_data.callback);
   }
   
   return 0;
}




