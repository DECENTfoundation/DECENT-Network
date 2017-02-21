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

void gui_wallet::Mainwindow_gui_wallet::ManagementTransactionsGUI()
{
    SetNewTask("get_account_history nathan 4",this,NULL,&Mainwindow_gui_wallet::TaskDoneFuncGUI);
}


void gui_wallet::Mainwindow_gui_wallet::TaskDoneTransactionsGUI(void* a_clbkArg,int64_t a_err,const std::string& a_task,const std::string& a_result)
{
    int count = 0;
    int row = 0; //tablewidget->rowCount();
    int col = 0; //tablewidget->columnCount();
    int n = 0;

    QString loops = 0;
    //loops = a_result;
    loops = QString::fromStdString(a_result);

    m_pCentralWidget->m_trans_tab.itm = new QTableWidgetItem(tr("%1").arg(loops));
    m_pCentralWidget->m_trans_tab.tablewidget->setItem(0, 0,  m_pCentralWidget->m_trans_tab.itm);

//    while(true)
//    {
//        n++;
//        col = 0;
//        QString loops = 0;
//        for (int i = 0; i < 20; ++i)
//        {
//            count++;
//            loops += a_result[i];
//        }
//        m_pCentralWidget->m_trans_tab.itm = new QTableWidgetItem(tr("%1").arg(loops));
//        m_pCentralWidget->m_trans_tab.tablewidget->setItem(row, col++,  m_pCentralWidget->m_trans_tab.itm);

//        QString loops2 = 0;
//        for (int i = count; ; ++i)
//        {
//            if( (a_result[i + 4] >= '0' && a_result[i + 4] <= '9') && a_result[i + 5] == '-')
//            {
//                break;
//            }
//            count++;
//            loops2 += a_result[i];
//        }

//        m_pCentralWidget->m_trans_tab.itm = new QTableWidgetItem(tr("%1").arg(loops2));
//        m_pCentralWidget->m_trans_tab.tablewidget->setItem(row, col,  m_pCentralWidget->m_trans_tab.itm);

//        col = 0;
//        ++row;
//        if(n == 4)
//            break;
//        m_pCentralWidget->m_trans_tab.createNewRow();
//    }
}






