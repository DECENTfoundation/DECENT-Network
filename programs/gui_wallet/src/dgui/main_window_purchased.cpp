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
#if 0
    else if(strstr(csFilterStr.c_str(),ST::s_vcpcSearchTypeStrs[ST::bought]))
    {

        std::string csNumber;
        cpcNumberPtr = strchr(csFilterStr.c_str(),':');
        if( !cpcNumberPtr || (atoi(cpcNumberPtr+1)==0)){csNumber += " 10";}
        else {csNumber = cpcNumberPtr+1;}
        csTaskLine = std::string("list_content_by_bought ") + csNumber;
    }
#endif
    SetNewTask("list_content_by_bought 100",this,NULL,&gui_wallet::Mainwindow_gui_wallet::TaskDonePurchasedGUI);
}


void gui_wallet::Mainwindow_gui_wallet::TaskDonePurchasedGUI(void* a_clbkArg,int64_t a_err,const std::string& a_task,const std::string& a_result)
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
        GetDigitalContentsFromString(m_vcDigContent,a_result.c_str());
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
