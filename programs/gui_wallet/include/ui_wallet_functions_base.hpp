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

#define SetNewTask_last_args    void* a_clbData,int64_t a_err, const std::string& a_inp, const std::string& a_result

typedef void (*ConnErrFuncType)(void*owner, void* clbData,const std::string& err,const std::string& details);
typedef void (__THISCALL__ *TypeCallbackSetNewTaskGlb)(void* owner,SetNewTask_last_args);
typedef void (*WarnYesOrNoFuncType)(void*owner,int answer,/*string**/void* str_ptr);

#include "debug_decent_application.h"

#endif // UI_WALLET_FUNCTIONS_BASE_HPP
