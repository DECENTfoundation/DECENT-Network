#pragma once

#include <stdint.h>
#include <string>


#define __CONNECTION_CLB_ "__CONNECTION_CLB_"
#define __MANAGER_CLB_ "__MANAGER_CLB_"

#ifndef __THISCALL__
   #ifdef __MSC_VER
      #define __THISCALL__ __thiscall
   #else
      #define __THISCALL__
   #endif
#endif

#define NO_API_INITED -1
#define UNABLE_TO_CONNECT -2
#define UNKNOWN_EXCEPTION -3
#define WRONG_ARGUMENT      -4
#define FILE_DOES_NOT_EXIST      -5



#define SetNewTask_last_args2    void* a_clbData, int64_t a_err, const std::string& a_inp

typedef void (__THISCALL__ *TypeCallbackSetNewTaskGlb2)(void* owner,SetNewTask_last_args2,const std::string& a_result);
typedef void (__THISCALL__ *TypeManagementClbk)(void* owner,SetNewTask_last_args2,const std::string& a_result);
typedef void (__THISCALL__ *WarnYesOrNoFuncType)(void*owner,int answer,/*string**/void* str_ptr);
typedef int (__THISCALL__ *TypeWarnAndWaitFunc)(void* owner,
                                                WarnYesOrNoFuncType fpYesOrNo,
                                                void* a_pDataForYesOrNo,const char* a_form,...);
typedef int (__THISCALL__ *TypeCallFunctionInGuiLoop2)(SetNewTask_last_args2,const std::string& a_result,void* owner,TypeCallbackSetNewTaskGlb2 fpFnc);


