// main_window_browse_content
/*
 *	File: main_window_browse_content.cpp
 *
 *	Created on: 20 Feb 2017
 *	Created by:
 *
 *  This file implements ...
 *
 */

#include "gui_wallet_mainwindow.hpp"
#include <fc/variant_object.hpp>
#include "decent_wallet_ui_gui_jsonparserqt.hpp"


/*///////////////////////////////////////////////////////*/

static int s_nActive = 0;

void gui_wallet::Mainwindow_gui_wallet::ManagementBrowseContentGUI()
{
    QString cqsNewFilter = m_pCentralWidget->getFilterText();

    if(s_nActive==0)
    {
//#if DEFAULT_LOG_LEVEL==0
        SetNewTask3("list_content a 10",this,NULL,&Mainwindow_gui_wallet::TaskDoneBrowseContentGUI3);
//#endif
        s_nActive = 1;
    }

#if 1

    if(cqsNewFilter==tr(""))
    {
        //char vcFilter[2] = {(char)1,'\0'};
        char vcFilter[2] = {'a','\0'};
        cqsNewFilter = tr("URI_start:") + tr(vcFilter);
    }

    __DEBUG_APP2__(DEFAULT_LOG_LEVEL,"filter=%s",cqsNewFilter.toStdString().c_str());

    if(cqsNewFilter==m_cqsPreviousFilter){return;}

    m_cqsPreviousFilter = cqsNewFilter;
    ShowDigitalContextesGUI(cqsNewFilter);
#endif
}


void gui_wallet::Mainwindow_gui_wallet::TaskDoneBrowseContentGUI(void* a_clbkArg,int64_t a_err,
                                                                 const std::string& a_task,const std::string& a_result)
{
    //
}


void gui_wallet::Mainwindow_gui_wallet::TaskDoneBrowseContentGUI3(void* a_clbkArg,int64_t a_err,
                                                                  const std::string& a_task,
                                                                  const fc::variant& a_result)
{
    decent::wallet::ui::gui::JsonParserQt aVisitor;
    aVisitor.m_inp = a_task;
    __DEBUG_APP2__(2," ");

    a_result.visit(aVisitor);
    aVisitor.PrintValues();
    const decent::wallet::ui::gui::JsonParserQt& visRes = aVisitor.GetByIndex(0).GetByKey("author");
    std::string aRes = visRes.value();
    printf("!!!!!!!!!!!!!!!! type=%s, val=%s\n",visRes.TypeToString(), aRes.c_str());
    //printf("!!!!!!!!!!!!!!!!!!!!!!!!!! g_nCreateAndDelete=%d\n",g_nCreateAndDelete);
    s_nActive = 0;
}



/*////////////////////////////////////////////////////*/

//void FinalResult::Clear2(){m_values.clear();}
