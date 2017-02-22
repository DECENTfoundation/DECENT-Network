/*
 *	File: ui_wallet_functions_base.hpp
 *
 *	Created on: 04 Feb 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
#ifndef UI_WALLET_FUNCTIONS_BASE_HPP
#define UI_WALLET_FUNCTIONS_BASE_HPP

#include <stdint.h>
#include <string>
#include "debug_decent_application.h"
#include <fc/variant.hpp>

#define __CONNECTION_CLB_ "__CONNECTION_CLB_"
#define __MANAGER_CLB_ "__MANAGER_CLB_"

#ifndef __THISCALL__
#ifdef __MSC_VER
#define __THISCALL__ __thiscall
#else  // #ifdef __MSC_VER
#define __THISCALL__
#endif  // #ifdef __MSC_VER
#endif  // #ifndef __THISCALL__

#define NO_API_INITED -1
#define UNABLE_TO_CONNECT -2
#define UNKNOWN_EXCEPTION -3
#define WRONG_ARGUMENT      -4
#define FILE_DOES_NOT_EXIST      -5

#ifdef __cplusplus
//extern "C"{
#endif

namespace TIT{enum {AS_STR,AS_VARIANT};}

#define SetNewTask_last_args2    void* a_clbData,int64_t a_err, const std::string& a_inp

//typedef void (*ConnErrFuncType)(void*owner, void* clbData,const std::string& err,const std::string& details);
typedef void (__THISCALL__ *TypeCallbackSetNewTaskGlb2)(void* owner,SetNewTask_last_args2,const std::string& a_result);
typedef void (__THISCALL__ *TypeCallbackSetNewTaskGlb3)(void* owner,SetNewTask_last_args2,const fc::variant& a_result);
typedef void (__THISCALL__ *TypeManagementClbk)(void* owner,SetNewTask_last_args2,const std::string& a_result);
typedef void (__THISCALL__ *WarnYesOrNoFuncType)(void*owner,int answer,/*string**/void* str_ptr);
typedef int (__THISCALL__ *TypeWarnAndWaitFunc)(void* owner,
                                                WarnYesOrNoFuncType fpYesOrNo,
                                                void* a_pDataForYesOrNo,const char* a_form,...);
typedef int (__THISCALL__ *TypeCallFunctionInGuiLoop2)(SetNewTask_last_args2,const std::string& a_result,void* owner,TypeCallbackSetNewTaskGlb2 fpFnc);
typedef int (__THISCALL__ *TypeCallFunctionInGuiLoop3)(SetNewTask_last_args2,const fc::variant& a_result,void* owner,TypeCallbackSetNewTaskGlb3 fpFnc);

#ifdef __cplusplus
//}
#endif

#endif // UI_WALLET_FUNCTIONS_BASE_HPP
