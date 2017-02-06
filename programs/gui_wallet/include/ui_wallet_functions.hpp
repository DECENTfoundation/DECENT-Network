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

typedef struct SConnectionStruct{
    ~SConnectionStruct(){__DEBUG_APP2__(1,"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! ");}
    WAT::_WAT_TP   action;
    std::string   wallet_file_name;
    ConnErrFuncType fpErr; WarnYesOrNoFuncType fpWarnFunc;
    //std::string  ws_server = "ws://localhost:8090";
    std::string  ws_server;
    std::string  ws_user;
    std::string  ws_password;
    std::string  chain_id;
}SConnectionStruct;


int LoadWalletFile(SConnectionStruct* a_pWalletData);
int SaveWalletFile2(const SConnectionStruct& a_pWalletData);


void SetManagementCallback_base(void* a_pOwner,void* a_pClbData,...);
void SetManagementCallback(void* a_pOwner,void* a_pClbData,TypeCallbackSetNewTaskGlb a_fpClbk);
template <typename Type>
static void SetManagementCallback(Type* a_memb,void* a_pClbData,void (Type::*a_clbkFunction)(SetNewTask_last_args))
{
    SetManagementCallback_base(a_memb,a_pClbData,a_clbkFunction);
}



void StartConnectionProcedure_base(const SConnectionStruct& a_conn_str,void *owner, void*clbData,...);
void StartConnectionProcedure(const SConnectionStruct& a_conn_str,void *owner, void*clbData,TypeCallbackSetNewTaskGlb connectionDone);
template <typename Type>
static void StartConnectionProcedure(const SConnectionStruct& a_conn_str, Type* a_memb, void* a_clbData,
                             void (Type::*a_clbkFunction)(SetNewTask_last_args))
{
    StartConnectionProcedure_base(a_conn_str,a_memb,a_clbData,a_clbkFunction);
}

int SetNewTask_base(const std::string& inp_line, void* ownr, void* clbData, ...);
int SetNewTask(const std::string& inp_line, void* ownr, void* clbData, TypeCallbackSetNewTaskGlb clbkFunction);
template <typename Type>
static int SetNewTask(const std::string& a_inp_line, Type* a_memb, void* a_clbData, void (Type::*a_clbkFunction)(SetNewTask_last_args))
{
    return SetNewTask_base(a_inp_line, a_memb, a_clbData, a_clbkFunction);
}


#endif // UI_WALLET_FUNCTIONS_HPP
