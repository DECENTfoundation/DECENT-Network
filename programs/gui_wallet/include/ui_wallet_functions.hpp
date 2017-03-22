
#pragma once


#include <thread>
#include <QWidget>

namespace gui_wallet {
   
   
   
#define NO_API_INITED -1
#define UNABLE_TO_CONNECT -2
#define UNKNOWN_EXCEPTION -3
#define WRONG_ARGUMENT      -4
#define FILE_DOES_NOT_EXIST      -5
   
   
   
typedef void (*TypeCallbackSetNewTaskGlb2)(void* owner, void* a_clbData, int64_t a_err, const std::string& a_inp, const std::string& a_result);
typedef void (*WarnYesOrNoFuncType)(void*owner,int answer,/*string**/void* str_ptr);
   
   
   
   
struct SConnectionStruct {
    SConnectionStruct() : setPasswordFn(NULL) {
    }

   std::string         wallet_file_name;
   WarnYesOrNoFuncType setPasswordFn;
   QWidget*            owner;

   std::string  ws_server;
   std::string  ws_user;
   std::string  ws_password;
   std::string  chain_id;
};


struct TaskListItem {
   typedef std::string            value_type;
   
   
   TaskListItem() : callback(NULL) {}
   TaskListItem(TypeCallbackSetNewTaskGlb2 callback_function, const std::string& a_inp, void* a_owner = NULL, void* a_clbArg=NULL)
   : next(NULL), owner(a_owner), callbackArg(a_clbArg), input(a_inp), callback(callback_function) {
      
   }
   
   TaskListItem*                   next;
   void*                           owner;
   void*                           callbackArg;
   std::string                     input;
   TypeCallbackSetNewTaskGlb2      callback;
};


struct ConnectListItem {
   typedef SConnectionStruct*     value_type;
   
   ConnectListItem() : input(NULL), callback(NULL) {}
   
   ConnectListItem(TypeCallbackSetNewTaskGlb2 callback_function, SConnectionStruct* a_inp, void* a_owner = NULL, void* a_clbArg=NULL)
   : next(NULL), owner(a_owner), callbackArg(a_clbArg), input(a_inp), callback(callback_function) {
      
   }
   
   ConnectListItem*                next;
   void*                           owner;
   void*                           callbackArg;
   SConnectionStruct*              input;
   TypeCallbackSetNewTaskGlb2      callback;
};




   
class WalletInterface {
public:
   static void initialize();
   static void startConnecting(SConnectionStruct* connectionInfo);
   static void destroy();
   static int  callFunctionInGuiLoop(void* a_clbData, int64_t a_err, const std::string& a_inp, const std::string& a_result,void* a_owner,TypeCallbackSetNewTaskGlb2 a_fpFunc);
   
public:
   static int loadWalletFile(SConnectionStruct* a_pWalletData);
   static int saveWalletFile(const SConnectionStruct& a_pWalletData);

public:
   
   
   static int  setNewTask(const std::string& inp_line, void* ownr, void* clbData, TypeCallbackSetNewTaskGlb2 fpTaskDone);
   static void runTask(std::string const& str_command, std::string& str_result);
   
private:
   static void connectionThreadFunction();
   static void connectedCallback(void* owner, void* a_clbData, int64_t a_err, const std::string& a_inp, const std::string& a_result);
   static int  connectToNewWitness(const ConnectListItem& item);


   
};
   





#define    AsyncTask    WalletInterface::setNewTask
#define    RunTask      WalletInterface::runTask


}


