/*
 *	File: decent_gui_inguiloopcaller.cpp
 *
 *	Created on: 04 Feb 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */


#include "decent_gui_inguiloopcaller.hpp"
#include <stdio.h>
#include <stdarg.h>
#include <iostream>


using namespace gui_wallet;


static InGuiLoopCaller* s_pInGuiThreadCaller = NULL;


namespace {
   

class InGuiLoopCallerIniter{
public:
    InGuiLoopCallerIniter(){
        s_pInGuiThreadCaller =  new InGuiLoopCaller;
    }

    ~InGuiLoopCallerIniter(){
        InGuiLoopCaller* pInGuiThreadCaller = s_pInGuiThreadCaller;
        s_pInGuiThreadCaller = NULL;
        delete pInGuiThreadCaller;
    }
};

}

static InGuiLoopCallerIniter   s_InGuiLoopCallerIniter;



InGuiLoopCaller::InGuiLoopCaller() {
   
    connect(this,SIGNAL(NewFunctionToCallSig2(void*,int64_t, std::string, std::string,void*,TypeCallbackSetNewTaskGlb2)),
            this,SLOT(NextFunctionToCallSlot2(void*,int64_t, std::string, std::string,void*,TypeCallbackSetNewTaskGlb2)));
}


InGuiLoopCaller::~InGuiLoopCaller() {
    disconnect(this,SIGNAL(NewFunctionToCallSig2(void*,int64_t, std::string, std::string,void*,TypeCallbackSetNewTaskGlb2)),
               this,SLOT(NextFunctionToCallSlot2(void*,int64_t, std::string, std::string,void*,TypeCallbackSetNewTaskGlb2)));
}

InGuiLoopCaller* InGuiLoopCaller::instance() {
   return s_pInGuiThreadCaller;
}

void InGuiLoopCaller::CallFunctionInGuiLoop2(void* a_clbData, int64_t a_err, const std::string& a_inp, const std::string& a_result,
                                                          void* a_owner,TypeCallbackSetNewTaskGlb2 a_fpFnc) {
   
    emit NewFunctionToCallSig2(a_clbData,a_err,a_inp,a_result,a_owner,a_fpFnc);
}



void InGuiLoopCaller::NextFunctionToCallSlot2(void* a_clbData,int64_t a_err,
                                                           std::string a_inp, std::string a_result,
                                                           void* a_owner,TypeCallbackSetNewTaskGlb2 a_fpFnc)
{
    try {
        (*a_fpFnc)(a_owner,a_clbData,a_err,a_inp,a_result);
    } catch (const std::exception& ex) {
        std::cout << "Exception running " << a_inp << "\n";
        std::cout << ex.what() << "\n";
    }
}



