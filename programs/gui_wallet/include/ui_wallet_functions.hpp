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
static void __THISCALL__ CallbackSetNewTaskGlb_static(void* owner,SetNewTask_last_args){}

typedef struct SConnectionStruct{
    SConnectionStruct():fpDone(&CallbackSetNewTaskGlb_static),fpWarnFunc(&WarnYesOrNoFunc_static){}
    ~SConnectionStruct(){__DEBUG_APP2__(1,"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! ");}
    WAT::_WAT_TP   action;
    std::string   wallet_file_name;
    TypeCallbackSetNewTaskGlb fpDone; WarnYesOrNoFuncType fpWarnFunc;
    //std::string  ws_server = "ws://localhost:8090";
    std::string  ws_server;
    std::string  ws_user;
    std::string  ws_password;
    std::string  chain_id;
}SConnectionStruct;

void InitializeUiInterfaceOfWallet_base(TypeWarnAndWaitFunc a_fpWarnAndWait,TypeCallFunctionInGuiLoop a_fpCorrectUiCaller,
                                        void* a_pMngOwner,void* a_pMngClb,...);
void InitializeUiInterfaceOfWallet(TypeWarnAndWaitFunc a_fpWarnAndWait,TypeCallFunctionInGuiLoop a_fpCorrectUiCaller,
                                   void* a_pMngOwner,void* a_pMngClb,TypeCallbackSetNewTaskGlb a_fpMngClbk);
template <typename Type>
static void InitializeUiInterfaceOfWallet(TypeWarnAndWaitFunc a_fpWarnAndWait,TypeCallFunctionInGuiLoop a_fpCorrectUiCaller,
                                          Type* a_pMngOwner,void* a_pMngClb,void (Type::*a_clbkFunction)(SetNewTask_last_args))
{
    InitializeUiInterfaceOfWallet_base(a_fpWarnAndWait,a_fpCorrectUiCaller,a_pMngOwner,a_pMngClb,a_clbkFunction);
}

void DestroyUiInterfaceOfWallet(void);


int LoadWalletFile(SConnectionStruct* a_pWalletData);
int SaveWalletFile2(const SConnectionStruct& a_pWalletData);



void StartConnectionProcedure(SConnectionStruct* a_conn_str,void *owner, void*clbData);

int SetNewTask_base(const std::string& inp_line, void* ownr, void* clbData, ...);
int SetNewTask(const std::string& inp_line, void* ownr, void* clbData, TypeCallbackSetNewTaskGlb clbkFunction);
template <typename Type>
static int SetNewTask(const std::string& a_inp_line, Type* a_memb, void* a_clbData, void (Type::*a_clbkFunction)(SetNewTask_last_args))
{
    return SetNewTask_base(a_inp_line, a_memb, a_clbData, a_clbkFunction);
}


void* GetFunctionPointerAsVoid(int,...);


#endif // UI_WALLET_FUNCTIONS_HPP
