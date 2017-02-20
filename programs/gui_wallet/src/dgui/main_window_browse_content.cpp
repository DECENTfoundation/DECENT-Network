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

void gui_wallet::Mainwindow_gui_wallet::ManagementBrowseContentGUI()
{
    __DEBUG_APP2__(2,"WAS::CONNECTED_ST");
    QString cqsNewFilter = m_pCentralWidget->getFilterText();
    if(cqsNewFilter==m_cqsPreviousFilter){return;}

    else if(cqsNewFilter==tr(""))
    {
        // may be in the case of empty filter all contents should be displayed?
        m_cqsPreviousFilter = cqsNewFilter;
        return;
    }

    m_cqsPreviousFilter = cqsNewFilter;
    ShowDigitalContextesGUI(cqsNewFilter);
}


void gui_wallet::Mainwindow_gui_wallet::TaskDoneBrowseContentGUI(void* a_clbkArg,int64_t a_err,const std::string& a_task,const std::string& a_result)
{}
