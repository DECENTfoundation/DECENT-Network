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
#include "decent_gui_inguiloopcaller_glb.hpp"
#include <stdio.h>
#include <stdarg.h>
#include <iostream>

static decent::gui::InGuiLoopCaller* s_pInGuiThreadCaller = NULL;

namespace decent{ namespace gui{

class InGuiLoopCallerIniter{
public:
    InGuiLoopCallerIniter(){
        decent::gui::InGuiLoopCaller* pInGuiThreadCaller = new decent::gui::InGuiLoopCaller;
        if(!pInGuiThreadCaller){} // make deleyed error
        s_pInGuiThreadCaller = pInGuiThreadCaller;
    }

    ~InGuiLoopCallerIniter(){
        decent::gui::InGuiLoopCaller* pInGuiThreadCaller = s_pInGuiThreadCaller;
        s_pInGuiThreadCaller = NULL;
        delete pInGuiThreadCaller;
    }
};
static InGuiLoopCallerIniter   s_InGuiLoopCallerIniter;

}}

decent::gui::InGuiLoopCaller::InGuiLoopCaller()
{
    connect(this,SIGNAL(NewFunctionToCallSig(void*,int64_t, std::string, std::string,void*,TypeCallbackSetNewTaskGlb)),
            this,SLOT(NextFunctionToCallSlot(void*,int64_t, std::string, std::string,void*,TypeCallbackSetNewTaskGlb)));
}


decent::gui::InGuiLoopCaller::~InGuiLoopCaller()
{
    disconnect(this,SIGNAL(NewFunctionToCallSig(void*,int64_t, std::string, std::string,void*,TypeCallbackSetNewTaskGlb)),
               this,SLOT(NextFunctionToCallSlot(void*,int64_t, std::string, std::string,void*,TypeCallbackSetNewTaskGlb)));
}


void decent::gui::InGuiLoopCaller::CallFunctionInGuiLoop(SetNewTask_last_args,void* a_owner,TypeCallbackSetNewTaskGlb a_fpFnc)
{
    //(*a_fpFnc)(a_owner,a_clbData,a_err,a_inp,a_result);
    emit NewFunctionToCallSig(a_clbData,a_err,a_inp,a_result,a_owner,a_fpFnc);
}


void decent::gui::InGuiLoopCaller::NextFunctionToCallSlot(void* a_clbData,int64_t a_err, std::string a_inp, std::string a_result,void* a_owner,TypeCallbackSetNewTaskGlb a_fpFnc)
{
    //std::cout << "Input: " << a_inp << std::endl;
    //std::cout << "Output: " << a_result << std::endl;
    (*a_fpFnc)(a_owner,a_clbData,a_err,a_inp,a_result);
}





/*///////////////////////////////////////////////////////////////////////////////////////////////////////*/
int CallFunctionInGuiLoop_base(SetNewTask_last_args,void* a_owner,...)
{
    if(!s_pInGuiThreadCaller){return -1;}
    TypeCallbackSetNewTaskGlb fpClbkFnc;
    va_list aFunc;

    va_start( aFunc,a_owner );
    fpClbkFnc = va_arg( aFunc, TypeCallbackSetNewTaskGlb);
    va_end( aFunc );

    s_pInGuiThreadCaller->CallFunctionInGuiLoop(a_clbData,a_err,a_inp,a_result,a_owner,fpClbkFnc);

    return 0;
}


int CallFunctionInGuiLoop(SetNewTask_last_args,void* a_owner,TypeCallbackSetNewTaskGlb a_fpFunc){
    return CallFunctionInGuiLoop_base(a_clbData,a_err,a_inp,a_result,a_owner,a_fpFunc);
}
