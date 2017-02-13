/*
 *	File: gui_wallet_application.cpp
 *
 *	Created on: 14 Dec 2016
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements the functions for class application
 *  this class will implement functional part necessary for the application
 *
 */

#include "gui_wallet_application.hpp"

#include "connected_api_instance.hpp"
#include <fc/network/http/websocket.hpp>
#include <fc/rpc/websocket_api.hpp>
#include <graphene/egenesis/egenesis.hpp>
#include <mutex>
#include <QMessageBox>
#include <QWidget>
#include <stdarg.h>

using namespace graphene::wallet;
using namespace fc::http;
extern int g_nDebugApplication ;

static InGuiThreatCaller* s_pWarner = NULL;


gui_wallet::application::application(int& argc, char** argv)
    :
      QApplication(argc,argv)
{
    qRegisterMetaType<std::string>( "std::string" );
    qRegisterMetaType<WarnYesOrNoFuncType>( "WarnYesOrNoFuncType" );
    s_pWarner = new InGuiThreatCaller;
    if(!s_pWarner)
    {
        throw "No enough memory";
    }
}


gui_wallet::application::~application()
{
    delete s_pWarner;
}


/* //////////////////////// */
namespace gui_wallet
{

static int InfoFunc(void*,const char*, ...)
{
    return 0;
}

static int WarnFunc(void*,const char*, ...)
{
    return 0;
}



static int WarnAndWaitFunc(void* a_pOwner,
                           WarnYesOrNoFuncType a_fpYesOrNo,void* a_pDataForYesOrNo,
                           const char* a_form,...)
{
    QString aString;

    va_list args;

    va_start( args, a_form );
    aString.vsprintf(a_form,args);
    va_end( args );

    s_pWarner->m_nRes = -1;
    s_pWarner->m_pParent2 = (QWidget*)a_pOwner;
    s_pWarner->EmitShowMessageBox(aString,a_fpYesOrNo,a_pDataForYesOrNo);
    s_pWarner->m_sema.wait();

    return s_pWarner->m_nRes;
}

#if 0
static std::string GetPasswordFromOwner(void* a_pOwner)
{
    s_pWarner->m_csRes = "";
    s_pWarner->m_pParent2 = (QWidget*)a_pOwner;
    s_pWarner->EmitShowMessageBox(aString);
    s_pWarner->m_sema.wait();

    return s_pWarner->m_nRes;
}
#endif


static int ErrorFunc(void*,const char*, ...)
{
    return 0;
}

class NewTestMutex : public std::mutex
{
public:
    void lock(){if(g_nDebugApplication){printf("++locking!\n");}std::mutex::lock();if(g_nDebugApplication){printf("++locked!\n");}}
    void unlock(){if(g_nDebugApplication){printf("--unlocking!\n");}std::mutex::unlock();if(g_nDebugApplication){printf("--unlocked!\n");}}
};

//static std::mutex   s_mutex_for_cur_api; // It is better to use here rw mutex
static NewTestMutex   s_mutex_for_cur_api; // It is better to use here rw mutex
static StructApi    s_CurrentApi;


static void SetCurrentApis(const StructApi* a_pApis)
{
    s_mutex_for_cur_api.lock();
    memcpy(&s_CurrentApi,a_pApis,sizeof(StructApi));
    s_mutex_for_cur_api.unlock();
}


int CreateConnectedApiInstance( const graphene::wallet::wallet_data* a_wdata,
                                const std::string& a_wallet_file_name,
                                void* a_pOwner,DoneFuncType a_fpDone, ErrFuncType a_fpErr,WarnYesOrNoFuncType a_fpFunc)
{
    try
    {
        void* pOwner = a_pOwner; // ???
        StructApi aApiToCreate;
        const graphene::wallet::wallet_data& wdata = *a_wdata;
        fc::path wallet_file( a_wallet_file_name );

        fc::http::websocket_client client;
        //idump((wdata.ws_server));
        auto con  = client.connect( wdata.ws_server );
        auto apic = std::make_shared<fc::rpc::websocket_api_connection>(*con);

        auto remote_api = apic->get_remote_api< login_api >(1);
        //edump((wdata.ws_user)(wdata.ws_password) );
        // TODO:  Error message here
        FC_ASSERT( remote_api->login( wdata.ws_user, wdata.ws_password ) );

        auto wapiptr = std::make_shared<wallet_api>( wdata, remote_api );
        wapiptr->set_wallet_filename( wallet_file.generic_string() );
        wapiptr->load_wallet_file();
        aApiToCreate.wal_api = wapiptr.get();

        fc::api<wallet_api> wapi(wapiptr);

        auto wallet_gui = std::make_shared<fc::rpc::gui>();
        aApiToCreate.gui_api = wallet_gui.get();
        SetCurrentApis(&aApiToCreate);
        for( auto& name_formatter : wapiptr->get_result_formatters() )
           wallet_gui->format_result( name_formatter.first, name_formatter.second );

        wallet_gui.get()->SetOwner(pOwner);  // ???
        wallet_gui.get()->SetInfoReporter(&InfoFunc);
        wallet_gui.get()->SetInfoReporter(&WarnFunc);
        wallet_gui.get()->SetInfoReporter(&ErrorFunc);

        boost::signals2::scoped_connection closed_connection(con->closed.connect([=]{
           //cerr << "Server has disconnected us.\n";
           ErrorFunc(pOwner,"Server has disconnected us.\n");
           wallet_gui->stop();
        }));
        (void)(closed_connection);

        if( wapiptr->is_new() )
        {
           std::string aPassword("");
           //std::cout << "Please use the set_password method to initialize a new wallet before continuing\n";
           //wallet_cli->set_prompt( "new >>> " );
           WarnAndWaitFunc(pOwner,a_fpFunc,&aPassword,"Please use the set_password method to initialize a new wallet before continuing\n");
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
           (*a_fpDone)(a_pOwner);
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
        (*a_fpErr)(a_pOwner,a_fc.to_string(),a_fc.to_detail_string());
        if(g_nDebugApplication){printf("%s\n",(a_fc.to_detail_string()).c_str());}
    }
    catch(...)
    {
        if(g_nDebugApplication){printf("file:\"" __FILE__ "\",line:%d\n",__LINE__);}
        (*a_fpErr)(a_pOwner,"Unknown exception","Unknown exception");
        if(g_nDebugApplication){printf("Unknown exception\n");}
    }

    return 0;
}


void UseConnectedApiInstance(void* a_pUserData,WaletFncType a_fpFunction)
{
    UseConnectedApiInstance_base(a_pUserData,a_fpFunction);
}


void UseConnectedApiInstance_base(void* a_pUserData,...)
{
    WaletFncType fpFunction;
    va_list aFunc;

    va_start( aFunc, a_pUserData );  /* Initialize variable arguments. */
    fpFunction = va_arg( aFunc, WaletFncType);
    va_end( aFunc );                /* Reset variable arguments.      */

    std::lock_guard<NewTestMutex> lock(s_mutex_for_cur_api);
    (*fpFunction)(a_pUserData,&s_CurrentApi);
    //s_mutex_for_cur_api.unlock();

}


} /* namespace gui_wallet */

InGuiThreatCaller::InGuiThreatCaller()
{
    connect( this, SIGNAL(ShowMessageBoxSig(const QString&,WarnYesOrNoFuncType,void*)),
             this, SLOT(MakeShowMessageBoxSlot(const QString&,WarnYesOrNoFuncType,void*)) );
    connect( this, SIGNAL(CallFuncSig(SInGuiThreadCallInfo)),
             this, SLOT(MakeCallFuncSlot(SInGuiThreadCallInfo)) );
}

InGuiThreatCaller::~InGuiThreatCaller()
{
    //
}

void InGuiThreatCaller::EmitShowMessageBox(const QString& a_str,WarnYesOrNoFuncType a_fpYesOrNo,void* a_pDataForYesOrNo)
{emit ShowMessageBoxSig(a_str,a_fpYesOrNo,a_pDataForYesOrNo);}

void InGuiThreatCaller::EmitCallFunc(SInGuiThreadCallInfo a_call_info)
{emit CallFuncSig(a_call_info);}

void InGuiThreatCaller::MakeShowMessageBoxSlot(const QString& a_str,WarnYesOrNoFuncType a_fpYesOrNo,void* a_pDataForYesOrNo)
{
    QMessageBox aMessageBox(QMessageBox::Warning,QObject::tr("WARNING"),
                            a_str,
                            QMessageBox::Ok|QMessageBox::Cancel,
                            m_pParent2);
    aMessageBox.setDetailedText(QObject::tr("Should be implemented"));
    m_nRes = aMessageBox.exec();

    //if(QMessageBox::Yes){}
    (*a_fpYesOrNo)(m_pParent2,m_nRes,a_pDataForYesOrNo);

    m_sema.post();
}


void InGuiThreatCaller::MakeCallFuncSlot(SInGuiThreadCallInfo a_call_info)
{
    (*a_call_info.fnc)(a_call_info.data);
}
