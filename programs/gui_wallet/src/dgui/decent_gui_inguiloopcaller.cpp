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
    connect(this,SIGNAL(NewFunctionToCallSig(void*,int64_t, std::string, std::string,void*,TypeCallbackSetNewTaskGlb2)),
            this,SLOT(NextFunctionToCallSlot(void*,int64_t, std::string, std::string,void*,TypeCallbackSetNewTaskGlb2)));
}


decent::gui::InGuiLoopCaller::~InGuiLoopCaller()
{
    disconnect(this,SIGNAL(NewFunctionToCallSig(void*,int64_t, std::string, std::string,void*,TypeCallbackSetNewTaskGlb2)),
               this,SLOT(NextFunctionToCallSlot(void*,int64_t, std::string, std::string,void*,TypeCallbackSetNewTaskGlb2)));
}


void decent::gui::InGuiLoopCaller::CallFunctionInGuiLoop(SetNewTask_last_args2,const std::string& a_result,void* a_owner,TypeCallbackSetNewTaskGlb2 a_fpFnc)
{
    //(*a_fpFnc)(a_owner,a_clbData,a_err,a_inp,a_result);
    __DEBUG_APP2__(2," ");
    emit NewFunctionToCallSig(a_clbData,a_err,a_inp,a_result,a_owner,a_fpFnc);
}


void decent::gui::InGuiLoopCaller::NextFunctionToCallSlot(void* a_clbData,int64_t a_err, std::string a_inp, std::string a_result,void* a_owner,TypeCallbackSetNewTaskGlb2 a_fpFnc)
{
    //std::cout << "Input: " << a_inp << std::endl;
    //std::cout << "Output: " << a_result << std::endl;
    __DEBUG_APP2__(2,"inp=\"%s\",a_fpFnc=%p",a_inp.c_str(),a_fpFnc);
    (*a_fpFnc)(a_owner,a_clbData,a_err,a_inp,a_result);
}





/*///////////////////////////////////////////////////////////////////////////////////////////////////////*/
int CallFunctionInGuiLoop_base(SetNewTask_last_args2,const std::string& a_result,void* a_owner,...)
{
    if(!s_pInGuiThreadCaller){return -1;}
    TypeCallbackSetNewTaskGlb2 fpClbkFnc;
    va_list aFunc;

    va_start( aFunc,a_owner );
    fpClbkFnc = va_arg( aFunc, TypeCallbackSetNewTaskGlb2);
    va_end( aFunc );

    s_pInGuiThreadCaller->CallFunctionInGuiLoop(a_clbData,a_err,a_inp,a_result,a_owner,fpClbkFnc);

    return 0;
}


int CallFunctionInGuiLoop2(SetNewTask_last_args2,const std::string& a_result,void* a_owner,TypeCallbackSetNewTaskGlb2 a_fpFunc){
    return CallFunctionInGuiLoop_base(a_clbData,a_err,a_inp,a_result,a_owner,a_fpFunc);
}


int CallFunctionInGuiLoop3(SetNewTask_last_args2,void* a_result,void* owner,TypeCallbackSetNewTaskGlb3 fpFnc)
{
    return 0;
}
