/*
 *	File: ui_wallet_functions.hpp
 *
 *	Created on: 04 Feb 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
#ifndef UI_WALLET_FUNCTIONS_HPP
#define UI_WALLET_FUNCTIONS_HPP

#include "ui_wallet_functions_base.hpp"

// WAT stands for Wallet Acctions Type
namespace WAT {enum _WAT_TP{CONNECT,SAVE2,LOAD2,EXIT};}

// WAS stands for wallet Api State
namespace WAS{enum _API_STATE{DEFAULT_ST=0,CONNECTED_ST,_API_STATE_SIZE};}

static void __THISCALL__ WarnYesOrNoFunc_static(void*,int,void*){}
static void __THISCALL__ CallbackSetNewTaskGlb_static(void* owner,SetNewTask_last_args2,const std::string&){}

typedef struct SConnectionStruct{
    SConnectionStruct():fpDone(&CallbackSetNewTaskGlb_static),setPasswordFn(&WarnYesOrNoFunc_static){}
    ~SConnectionStruct(){__DEBUG_APP2__(1,"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! ");}
    WAT::_WAT_TP   action;
    std::string   wallet_file_name;
    TypeCallbackSetNewTaskGlb2 fpDone;
    WarnYesOrNoFuncType setPasswordFn;
    
    std::string  ws_server;
    std::string  ws_user;
    std::string  ws_password;
    std::string  chain_id;
}SConnectionStruct;

void InitializeUiInterfaceOfWallet_base(TypeWarnAndWaitFunc a_fpWarnAndWait,
                                        TypeCallFunctionInGuiLoop2 a_fpCorrectUiCaller2,TypeCallFunctionInGuiLoop3 a_fpCorrectUiCaller3,
                                        void* a_pMngOwner,void* a_pMngClb,...);


void DestroyUiInterfaceOfWallet(void);


int LoadWalletFile(SConnectionStruct* a_pWalletData);
int SaveWalletFile2(const SConnectionStruct& a_pWalletData);



void StartConnectionProcedure(SConnectionStruct* a_conn_str,void *owner, void*clbData);

#define SetNewTask SetNewTask2

int SetNewTask_base(int a_nType,const std::string& inp_line, void* ownr, void* clbData, ...);
int SetNewTask2(const std::string& inp_line, void* ownr, void* clbData, TypeCallbackSetNewTaskGlb2 clbkFunction);
template <typename Type>
static int SetNewTask2(const std::string& a_inp_line, Type* a_memb, void* a_clbData,
                       void (Type::*a_clbkFunction)(SetNewTask_last_args2,const std::string&))
{
    return SetNewTask_base(TIT::AS_STR,a_inp_line, a_memb, a_clbData, a_clbkFunction);
}

template <typename Type>
static int SetNewTask3(const std::string& a_inp_line, Type* a_memb, void* a_clbData,
                       void (Type::*a_clbkFunction)(SetNewTask_last_args2,const fc::variant&))
{
    return SetNewTask_base(TIT::AS_VARIANT,a_inp_line, a_memb, a_clbData, a_clbkFunction);
}


void* GetFunctionPointerAsVoid(int,...);

void RunTask(std::string const& str_command, std::string& str_result);

#endif // UI_WALLET_FUNCTIONS_HPP
