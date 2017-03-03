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

using namespace gui_wallet;

extern int g_nDebugApplication ;

InGuiThreatCaller* s_pWarner = NULL;


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
   s_pWarner->EmitShowMessageBox(aString, a_fpYesOrNo, a_pDataForYesOrNo);
   s_pWarner->m_sema.wait();
    
   //(*a_fpYesOrNo)((QWidget*)a_pOwner, 0, a_pDataForYesOrNo);


   //return s_pWarner->m_nRes;
    return 0;
}


gui_wallet::application::application(int& argc, char** argv)
    :
      QApplication(argc,argv)
{
    qRegisterMetaType<std::string>( "std::string" );
    qRegisterMetaType<WarnYesOrNoFuncType>( "WarnYesOrNoFuncType" );
    qRegisterMetaType<int64_t>( "int64_t" );
    qRegisterMetaType<TypeCallbackSetNewTaskGlb2>( "TypeCallbackSetNewTaskGlb2" );
    qRegisterMetaType<TypeCallbackSetNewTaskGlb3>( "TypeCallbackSetNewTaskGlb3" );
    qRegisterMetaType<fc::variant>( "fc::variant" );
    qRegisterMetaType<SDigitalContent>( "SDigitalContent" );

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
{
    emit ShowMessageBoxSig(a_str, a_fpYesOrNo, a_pDataForYesOrNo);
}

void InGuiThreatCaller::EmitCallFunc(SInGuiThreadCallInfo a_call_info) {
    emit CallFuncSig(a_call_info);
}

void InGuiThreatCaller::MakeShowMessageBoxSlot(const QString& a_str,WarnYesOrNoFuncType a_fpYesOrNo,void* a_pDataForYesOrNo)
{
    (*a_fpYesOrNo)(m_pParent2,m_nRes,a_pDataForYesOrNo);
    m_sema.post();
}


void InGuiThreatCaller::MakeCallFuncSlot(SInGuiThreadCallInfo a_call_info)
{
    (*a_call_info.fnc)(a_call_info.data);
}
