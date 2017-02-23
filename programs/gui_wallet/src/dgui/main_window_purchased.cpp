//main_window_purchased
// main_window_purchased
/*
 *	File: main_window_purchased.cpp
 *
 *	Created on: 20 Feb 2017
 *	Created by:
 *
 *  This file implements ...
 *
 */

#include "gui_wallet_mainwindow.hpp"

void ParseDigitalContentFromGetContentString(decent::wallet::ui::gui::SDigitalContent* a_pContent, const std::string& a_str);

void gui_wallet::Mainwindow_gui_wallet::ManagementPurchasedGUI()
{
    SetNewTask("list_content_by_bought 100",this,NULL,&gui_wallet::Mainwindow_gui_wallet::TaskDonePurchasedGUI);
    SetNewTask3("list_content_by_bought 100",this,NULL,&gui_wallet::Mainwindow_gui_wallet::TaskDonePurchasedGUI3);
}


void gui_wallet::Mainwindow_gui_wallet::TaskDonePurchasedGUI(void* a_clbkArg,int64_t a_err,
                                                             const std::string& a_task,const std::string& a_result)
{
    if(a_err)
    {
        //
    }
    else if(strstr(a_task.c_str(),"list_content_by_bought "))
    {
        //QTableWidget& cContents = m_pCentralWidget->getDigitalContentsTable();
        std::string csGetContStr;
        m_vcDigContent.clear();
        GetDigitalContentsFromString(DCT::BOUGHT,m_vcDigContent,a_result.c_str());
        const int cnContsNumber(m_vcDigContent.size());

        for(int i(0); i<cnContsNumber; ++i)
        {
            csGetContStr = std::string("get_content \"") + m_vcDigContent[i].URI + "\"";
            SetNewTask(csGetContStr,this,(void*)((size_t)i),&Mainwindow_gui_wallet::TaskDoneFuncGUI);
        }

    }
    else if(strstr(a_task.c_str(),"get_content "))
    {
        const int cnIndex (  (int)(  (size_t)a_clbkArg  )     );
        const int cnContsNumber(m_vcDigContent.size());
        if(cnIndex>=cnContsNumber){return;}
        ParseDigitalContentFromGetContentString(&m_vcDigContent[cnIndex],a_result);
        if(cnIndex==(cnContsNumber-1)){m_pCentralWidget->m_Purchased_tab.SetDigitalContentsGUI(m_vcDigContent);}
    }
}


void gui_wallet::Mainwindow_gui_wallet::TaskDonePurchasedGUI3(void* a_clbkArg,int64_t a_err,
                                                              const std::string& a_task,const fc::variant& a_result)
{
    __DEBUG_APP2__(1," ");

    if(a_err)
    {
        //
    }
    else if(strstr(a_task.c_str(),"list_content_by_bought "))
    {
        __DEBUG_APP2__(1," ");
    }
    else if(strstr(a_task.c_str(),"get_content "))
    {
        __DEBUG_APP2__(0," ");
    }
}
