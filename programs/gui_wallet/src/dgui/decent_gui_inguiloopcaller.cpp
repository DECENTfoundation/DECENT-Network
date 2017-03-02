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
    connect(this,SIGNAL(NewFunctionToCallSig2(void*,int64_t, std::string, std::string,void*,TypeCallbackSetNewTaskGlb2)),
            this,SLOT(NextFunctionToCallSlot2(void*,int64_t, std::string, std::string,void*,TypeCallbackSetNewTaskGlb2)));

    connect(this,SIGNAL(NewFunctionToCallSig3(void*,int64_t, std::string, fc::variant,void*,TypeCallbackSetNewTaskGlb3)),
            this,SLOT(NextFunctionToCallSlot3(void*,int64_t, std::string, fc::variant,void*,TypeCallbackSetNewTaskGlb3)));
}


decent::gui::InGuiLoopCaller::~InGuiLoopCaller()
{
    disconnect(this,SIGNAL(NewFunctionToCallSig3(void*,int64_t, std::string, fc::variant,void*,TypeCallbackSetNewTaskGlb3)),
               this,SLOT(NextFunctionToCallSlot3(void*,int64_t, std::string, fc::variant,void*,TypeCallbackSetNewTaskGlb3)));

    disconnect(this,SIGNAL(NewFunctionToCallSig2(void*,int64_t, std::string, std::string,void*,TypeCallbackSetNewTaskGlb2)),
               this,SLOT(NextFunctionToCallSlot2(void*,int64_t, std::string, std::string,void*,TypeCallbackSetNewTaskGlb2)));
}


void decent::gui::InGuiLoopCaller::CallFunctionInGuiLoop2(SetNewTask_last_args2,const std::string& a_result,
                                                          void* a_owner,TypeCallbackSetNewTaskGlb2 a_fpFnc)
{
    //(*a_fpFnc)(a_owner,a_clbData,a_err,a_inp,a_result);
    __DEBUG_APP2__(2," ");
    emit NewFunctionToCallSig2(a_clbData,a_err,a_inp,a_result,a_owner,a_fpFnc);
}


void decent::gui::InGuiLoopCaller::CallFunctionInGuiLoop3(SetNewTask_last_args2,const fc::variant& a_result,
                                                          void* a_owner,TypeCallbackSetNewTaskGlb3 a_fpFnc)
{
    //(*a_fpFnc)(a_owner,a_clbData,a_err,a_inp,a_result);
    __DEBUG_APP2__(2," ");
    emit NewFunctionToCallSig3(a_clbData,a_err,a_inp,a_result,a_owner,a_fpFnc);
}


void decent::gui::InGuiLoopCaller::NextFunctionToCallSlot2(void* a_clbData,int64_t a_err,
                                                           std::string a_inp, std::string a_result,
                                                           void* a_owner,TypeCallbackSetNewTaskGlb2 a_fpFnc)
{
    __DEBUG_APP2__(2,"inp=\"%s\",a_fpFnc=%p",a_inp.c_str(),a_fpFnc);
    (*a_fpFnc)(a_owner,a_clbData,a_err,a_inp,a_result);
}


void decent::gui::InGuiLoopCaller::NextFunctionToCallSlot3(void* a_clbData,int64_t a_err,
                                                           std::string a_inp, fc::variant a_result,
                                                           void* a_owner,TypeCallbackSetNewTaskGlb3 a_fpFnc)
{
    __DEBUG_APP2__(2,"inp=\"%s\",a_fpFnc=%p",a_inp.c_str(),a_fpFnc);
    (*a_fpFnc)(a_owner,a_clbData,a_err,a_inp,a_result);
}





/*///////////////////////////////////////////////////////////////////////////////////////////////////////*/

int CallFunctionInGuiLoop2(SetNewTask_last_args2,const std::string& a_result,void* a_owner,
                           TypeCallbackSetNewTaskGlb2 a_fpFunc)
{
    if(!s_pInGuiThreadCaller){return -1;}
    s_pInGuiThreadCaller->CallFunctionInGuiLoop2(a_clbData,a_err,a_inp,a_result,a_owner,a_fpFunc);
    return 0;
}


int CallFunctionInGuiLoop3(SetNewTask_last_args2,const fc::variant& a_result,void* a_owner,
                           TypeCallbackSetNewTaskGlb3 a_fpFunc)
{
    if(!s_pInGuiThreadCaller){return -1;}
    s_pInGuiThreadCaller->CallFunctionInGuiLoop3(a_clbData,a_err,a_inp,a_result,a_owner,a_fpFunc);
    return 0;
}
