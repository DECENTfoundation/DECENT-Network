//main_window_transactions
/*
 *	File: main_window_transactions.cpp
 *
 *	Created on: 20 Feb 2017
 *	Created by:
 *
 *  This file implements ...
 *
 */


#include "gui_wallet_mainwindow.hpp"

void gui_wallet::Mainwindow_gui_wallet::ManagementUploadGUI()
{
    //SetNewTask("get_account_history hayq 4",this,NULL,&Mainwindow_gui_wallet::TaskDoneFuncGUI);
}


void gui_wallet::Mainwindow_gui_wallet::TaskDoneUploadGUI(void* a_clbkArg,int64_t a_err,
                                                          const std::string& a_task,const std::string& a_result)
{
    //
}


void gui_wallet::Mainwindow_gui_wallet::TaskDoneUploadGUI3(void* a_clbkArg,int64_t a_err,
                                                           const std::string& a_task,const fc::variant& a_result)
{
    __DEBUG_APP2__(0, " ");
}