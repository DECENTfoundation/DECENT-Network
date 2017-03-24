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


InGuiThreatCaller* s_pWarner = NULL;


gui_wallet::application::application(int argc, char** argv)
    :
      QApplication(argc,argv)
{
    qRegisterMetaType<std::string>( "std::string" );
    qRegisterMetaType<WarnYesOrNoFuncType>( "WarnYesOrNoFuncType" );
    qRegisterMetaType<int64_t>( "int64_t" );
    qRegisterMetaType<TypeCallbackSetNewTaskGlb2>( "TypeCallbackSetNewTaskGlb2" );
    qRegisterMetaType<SDigitalContent>( "SDigitalContent" );

    setApplicationDisplayName("Decent");
    
    s_pWarner = new InGuiThreatCaller;
    
}


gui_wallet::application::~application() {
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

InGuiThreatCaller* InGuiThreatCaller::instance() {
   return s_pWarner;
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
    (*a_call_info.function)(a_call_info.data);
}
