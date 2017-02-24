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
#include <QPalette>

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
        std::string id_s = "id\n";
        int pos = a_result.find("id");
        std::cout<<pos<<std::endl;
        pos += 6;
        for(int i = pos; a_result[i] != '"'; ++i)
            id_s.push_back(a_result[i]);

        std::string registrar_s = "\nregistrar\n  ";
        pos = a_result.find("registrar");
        std::cout<<pos<<std::endl;
        pos += 13;
        for(int i = pos; a_result[i] != '"'; ++i)
            registrar_s.push_back(a_result[i]);

        std::string referrer_s = "\nreferrer\n ";
        pos = a_result.find("referrer");
        std::cout<<pos<<std::endl;
        pos += 12;
        for(int i = pos; a_result[i] != '"'; ++i)
            referrer_s.push_back(a_result[i]);

        std::string lifetime_referrer_s = "\nlifetime_referrer\n";
        pos = a_result.find("lifetime_referrer");
        std::cout<<pos<<std::endl;
        pos += 21;
        for(int i = pos; a_result[i] != '"'; ++i)
            lifetime_referrer_s.push_back(a_result[i]);

        std::string network_fee_percentage_s = "\nnetwork_fee_percentage\n ";
        pos = a_result.find("network_fee_percentage");
        std::cout<<pos<<std::endl;
        pos += 24;
        for(int i = pos; a_result[i] != ','; ++i)
            network_fee_percentage_s.push_back(a_result[i]);

        std::string lifetime_referrer_fee_percentage_s = "\nlifetime_referrer_fee_percentage\n";
        pos = a_result.find("lifetime_referrer_fee_percentage");
        std::cout<<pos<<std::endl;
        pos += 34;
        for(int i = pos; a_result[i] != ','; ++i)
            lifetime_referrer_fee_percentage_s.push_back(a_result[i]);


        std::string name_s = "\nname\n  ";
        pos = a_result.find("name");
        std::cout<<pos<<std::endl;
        pos += 8;
        for(int i = pos; a_result[i] != '"'; ++i)
            name_s.push_back(a_result[i]);

        std::string referrer_rewards_percentage_s = "\nreferrer_rewards_percentage\n";
        pos = a_result.find("referrer_rewards_percentage");
        std::cout<<pos<<std::endl;
        pos += 29;
        for(int i = pos; a_result[i] != ','; ++i)
            referrer_rewards_percentage_s.push_back(a_result[i]);

        std::vector<std::string> infos;
        infos.push_back(id_s);
        infos.push_back(registrar_s);
        infos.push_back(referrer_s);
        infos.push_back(lifetime_referrer_s);
        infos.push_back(network_fee_percentage_s);
        infos.push_back(lifetime_referrer_fee_percentage_s);
        infos.push_back(referrer_rewards_percentage_s);
        infos.push_back(name_s);


        QZebraWidget* info_window = new QZebraWidget(infos);
        info_window->show();
    }
    m_pCentralWidget->m_Overview_tab.ArrangeSize();
}


void gui_wallet::Mainwindow_gui_wallet::TaskDoneOverrviewGUI3(void* a_clbkArg,int64_t a_err,const std::string& a_task,const fc::variant& a_result)
{
    __DEBUG_APP2__(0, " ");
}
