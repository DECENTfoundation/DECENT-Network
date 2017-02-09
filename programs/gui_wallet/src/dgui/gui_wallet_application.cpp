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

//#include "connected_api_instance.hpp"
//#include <fc/network/http/websocket.hpp>
//#include <fc/rpc/websocket_api.hpp>
//#include <graphene/egenesis/egenesis.hpp>
#include <mutex>
#include <QMessageBox>
#include <QWidget>
#include <stdarg.h>
#include <thread>

#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#define Sleep(__ms__) usleep(1000*(__ms__))
#endif

//using namespace graphene::wallet;
//using namespace fc::http;
extern int g_nDebugApplication ;

static InGuiThreatCaller* s_pWarner = NULL;


int WarnAndWaitFunc(void* a_pOwner,WarnYesOrNoFuncType a_fpYesOrNo,
                           void* a_pDataForYesOrNo,const char* a_form,...)
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


gui_wallet::application::application(int& argc, char** argv)
    :
      QApplication(argc,argv)
{
    qRegisterMetaType<std::string>( "std::string" );
    qRegisterMetaType<WarnYesOrNoFuncType>( "WarnYesOrNoFuncType" );
    qRegisterMetaType<int64_t>( "int64_t" );
    qRegisterMetaType<TypeCallbackSetNewTaskGlb>( "TypeCallbackSetNewTaskGlb" );//TypeCallbackSetNewTaskGlb

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



#if 0

void UseConnectedApiInstance(void* a_pUserData,void* a_callbackArg,WaletFncType a_fpFunction)
{
    UseConnectedApiInstance_base(a_pUserData,a_callbackArg,a_fpFunction);
}


void UseConnectedApiInstance_base(void* a_pUserData,void* a_callbackArg,...)
{
    WaletFncType fpFunction;
    va_list aFunc;

    va_start( aFunc, a_callbackArg );  /* Initialize variable arguments. */
    fpFunction = va_arg( aFunc, WaletFncType);
    va_end( aFunc );                /* Reset variable arguments.      */

    std::lock_guard<NewTestMutex> lock(s_mutex_for_cur_api);
    (*fpFunction)(a_pUserData,a_callbackArg,&s_CurrentApi);
    //s_mutex_for_cur_api.unlock();

}

#endif  // #if 0


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
    __DEBUG_APP2__(_DEF_LOG_LEVEL_,"a_fpYesOrNo=%p",a_fpYesOrNo);
    (*a_fpYesOrNo)(m_pParent2,m_nRes,a_pDataForYesOrNo);

    m_sema.post();
}


void InGuiThreatCaller::MakeCallFuncSlot(SInGuiThreadCallInfo a_call_info)
{
    (*a_call_info.fnc)(a_call_info.data);
}
