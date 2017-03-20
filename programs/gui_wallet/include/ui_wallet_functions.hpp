
#pragma once

#include "ui_wallet_functions_base.hpp"

namespace gui_wallet {
   
   
   
// WAT stands for Wallet Acctions Type
namespace WAT {
   enum _WAT_TP { CONNECT,SAVE2,LOAD2,EXIT};
}

// WAS stands for wallet Api State
namespace WAS{
   enum _API_STATE {DEFAULT_ST=0, CONNECTED_ST, _API_STATE_SIZE};
}

static void __THISCALL__ WarnYesOrNoFunc_static(void*,int,void*){}
static void __THISCALL__ CallbackSetNewTaskGlb_static(void* owner,SetNewTask_last_args2,const std::string&){}

struct SConnectionStruct {
    SConnectionStruct() : fpDone(&CallbackSetNewTaskGlb_static), setPasswordFn(&WarnYesOrNoFunc_static) {
    }

   WAT::_WAT_TP   action;
   std::string   wallet_file_name;
   TypeCallbackSetNewTaskGlb2 fpDone;
   WarnYesOrNoFuncType setPasswordFn;

   std::string  ws_server;
   std::string  ws_user;
   std::string  ws_password;
   std::string  chain_id;
};

void InitializeUiInterfaceOfWallet_base(TypeWarnAndWaitFunc a_fpWarnAndWait,
                                        TypeCallFunctionInGuiLoop2 a_fpCorrectUiCaller2);


void DestroyUiInterfaceOfWallet(void);


int LoadWalletFile(SConnectionStruct* a_pWalletData);
int SaveWalletFile2(const SConnectionStruct& a_pWalletData);

void StartConnectionProcedure(SConnectionStruct* a_conn_str,void *owner, void*clbData);


int SetNewTask_base(const std::string& inp_line, void* ownr, void* clbData, ...);

int SetNewTask(const std::string& inp_line, void* ownr, void* clbData, TypeCallbackSetNewTaskGlb2 clbkFunction);

template <typename Type>
static int SetNewTask(const std::string& a_inp_line, Type* a_memb, void* a_clbData,
                       void (Type::*a_clbkFunction)(SetNewTask_last_args2,const std::string&))
{
    return SetNewTask_base(a_inp_line, a_memb, a_clbData, a_clbkFunction);
}


void RunTask(std::string const& str_command, std::string& str_result);

}


// This function should not exist. Remove this when you can
inline void* GetFunctionPointerAsVoid(int a_first,...) {
   va_list aFunc;
   
   va_start( aFunc, a_first );
   void* pReturn = va_arg( aFunc, void*);
   va_end( aFunc );
   
   return pReturn;
}

