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
#include <QMessageBox>

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
        for(int i = 1 ; i < a_result.size() - 1 ; ++i)
        {
            if(a_result[i] == '[')
            {
                if(a_result[i + 1] != '[')
                {
                    int j = i+7;
                    std::string name , id;
                    while(a_result[j] != '"')
                    {
                        name.push_back(a_result[j]);
                        ++j;
                    }
                    j += 8;
                    while(a_result[j] != '"')
                    {
                        id.push_back(a_result[j]);
                        ++j;
                    }
                       m_pCentralWidget->m_Overview_tab.accounts_names.push_back(QString::fromStdString(name));
                       m_pCentralWidget->m_Overview_tab.accounts_id.push_back(QString::fromStdString(id));
                }
            }
        }
        m_pCentralWidget->m_Overview_tab.CreateTable();
    }
    else if(a_task.find("get_account ") == 0)
    {
        std::string id_s = "id    ";
        int pos = a_result.find("id");
        std::cout<<pos<<std::endl;
        pos += 6;
        for(int i = pos; a_result[i] != '"'; ++i)
            id_s.push_back(a_result[i]);






        QMessageBox messig_info;
        messig_info.setWindowTitle("More Info about account");
        messig_info.setText(QString::fromStdString(a_result));
        messig_info.exec();
        //m_pCentralWidget->m_Overview_tab.text.setText(QString::fromStdString(a_result));
    }
    m_pCentralWidget->m_Overview_tab.table_widget.resize(m_pCentralWidget->m_Overview_tab.table_widget.width(), m_pCentralWidget->m_Overview_tab.table_widget.height());
    m_pCentralWidget->m_Overview_tab.table_widget.horizontalHeader()->setStretchLastSection(true);
    m_pCentralWidget->m_Overview_tab.table_widget.horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_pCentralWidget->m_Overview_tab.ArrangeSize();

}


void gui_wallet::Mainwindow_gui_wallet::TaskDoneOverrviewGUI3(void* a_clbkArg,int64_t a_err,const std::string& a_task,const fc::variant& a_result)
{
    __DEBUG_APP2__(0, " ");
}
