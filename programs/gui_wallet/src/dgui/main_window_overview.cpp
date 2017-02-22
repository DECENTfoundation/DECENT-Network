//main_window_overview

/*
 *	File: main_window_overview.cpp
 *
 *	Created on: 20 Feb 2017
 *	Created by:
 *
 *  This file implements ...
 *
 */

#include "gui_wallet_mainwindow.hpp"

void gui_wallet::Mainwindow_gui_wallet::ManagementOverviewGUI()
{
    QString tFilterStr = m_pCentralWidget->FilterStr();
    QString tNewTaskInp = tr("list_accounts ") + tFilterStr + tr(" 5"); // To do, number should be taken from gui
    std::string tInpuString = StringFromQString(tNewTaskInp);
    __DEBUG_APP2__(0,"task=%s",tInpuString.c_str());
    SetNewTask(tInpuString,this,NULL,&Mainwindow_gui_wallet::TaskDoneFuncGUI);
}


void gui_wallet::Mainwindow_gui_wallet::TaskDoneOverrviewGUI(void* a_clbkArg,int64_t a_err,const std::string& a_task,const std::string& a_result)
{
    int nCurTab(m_pCentralWidget->GetMyCurrentTabIndex());
    if(nCurTab != OVERVIEW){return;}

    if(a_task.find("list_accounts ") == 0)
    {
        m_pCentralWidget->m_Overview_tab.accounts_names.clear();
        for(int i = 0 ; i < a_result.size() - 1 ; ++i)
        {
            if(a_result[i] == '[')
            {
                if(a_result[i + 1] != '[')
                {
                    int j = i+7;
                    std::string name;
                    while(a_result[j] != '"')
                    {
                        name.push_back(a_result[j]);
                        ++j;
                    }
                       m_pCentralWidget->m_Overview_tab.accounts_names.push_back(QString::fromStdString(name));
                }
            }
        }
        m_pCentralWidget->m_Overview_tab.CreateTable();
    }
    else if(a_task.find("get_account ") == 0)
    {
        m_pCentralWidget->m_Overview_tab.text.setText(QString::fromStdString(a_result));
    }
}


void gui_wallet::Mainwindow_gui_wallet::TaskDoneOverrviewGUI3(void* a_clbkArg,int64_t a_err,const std::string& a_task,const fc::variant& a_result)
{
    __DEBUG_APP2__(0, " ");
}
