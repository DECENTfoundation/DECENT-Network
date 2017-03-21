
#pragma once

#include "ui_wallet_functions_base.hpp"
#include <thread>

namespace gui_wallet {
   
   

static void __THISCALL__ WarnYesOrNoFunc_static(void*,int,void*){}
   
   
   
   
struct SConnectionStruct {
    SConnectionStruct() : setPasswordFn(&WarnYesOrNoFunc_static) {
    }

   std::string   wallet_file_name;
   WarnYesOrNoFuncType setPasswordFn;

   std::string  ws_server;
   std::string  ws_user;
   std::string  ws_password;
   std::string  chain_id;
};

   
   
class WalletInterface {
public:
   static void initialize();
   static void startConnecting(SConnectionStruct* connectionInfo);
   static void destroy();
   static int callFunctionInGuiLoop(void* a_clbData, int64_t a_err, const std::string& a_inp, const std::string& a_result,void* a_owner,TypeCallbackSetNewTaskGlb2 a_fpFunc);
   
public:
   static int loadWalletFile(SConnectionStruct* a_pWalletData);
   static int saveWalletFile(const SConnectionStruct& a_pWalletData);

public:
   
   
   static int SetNewTask(const std::string& inp_line, void* ownr, void* clbData, TypeCallbackSetNewTaskGlb2 fpTaskDone);
   static void RunTask(std::string const& str_command, std::string& str_result);
   
private:
   static void connectionThreadFunction();
   static void connectedCallback(void* owner, void* a_clbData, int64_t a_err, const std::string& a_inp, const std::string& a_result);


   
};
   





#define    AsyncTask    WalletInterface::SetNewTask
#define    RunTask      WalletInterface::RunTask


}


