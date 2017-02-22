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
    SetNewTask("get_account_history vazgen 4",this,NULL,&Mainwindow_gui_wallet::TaskDoneFuncGUI);
}


void gui_wallet::Mainwindow_gui_wallet::TaskDoneTransactionsGUI(void* a_clbkArg,int64_t a_err,const std::string& a_task,const std::string& a_result)
{
    int index = 0;
    int row = 0; //tablewidget->rowCount();
    int col = 0; //tablewidget->columnCount();
    int n = 0;
    int d = 0;

//    QString s = QString::fromStdString(a_result);
//    m_pCentralWidget->m_trans_tab.itm = new QTableWidgetItem(tr("%1").arg(s));
//    m_pCentralWidget->m_trans_tab.tablewidget->setItem(row, col++,  m_pCentralWidget->m_trans_tab.itm);

    while(true)
    {
        n++;
        col = 0;

        //date
        QString date = 0;
        for (int i = index; d < 20; ++i)
        {
            d++;
            index++;
            date += a_result[i];
        }
        m_pCentralWidget->m_trans_tab.itm = new QTableWidgetItem(tr("%1").arg(date));
        m_pCentralWidget->m_trans_tab.tablewidget->setItem(row, col++,  m_pCentralWidget->m_trans_tab.itm);
        //=======

        //type
        QString type = 0;
        for (int i = index; ; ++i)
        {
            if( ( (a_result[i + 1] == 'F' || a_result[i + 1] == 'f') )
                    && a_result[i + 2] == 'e'
                    && a_result[i + 3] == 'e'
                    && a_result[i + 4] == ':')
            {
                break;
            }
            index+=6;
            type += a_result[i];
        }

        m_pCentralWidget->m_trans_tab.itm = new QTableWidgetItem(tr("%1").arg(type));
        m_pCentralWidget->m_trans_tab.tablewidget->setItem(row, col++,  m_pCentralWidget->m_trans_tab.itm);
        //======

        //fee
        QString fee = 0;
           for (int i = index; ; ++i)
           {
               if( ((a_result[i + 5] >= '0' && a_result[i + 5] <= '9')
                    && a_result[i + 6] == '-')   || a_result[i] == ')' )
               {
                   break;
               }

               index++;
               fee += a_result[i];
           }
           m_pCentralWidget->m_trans_tab.itm = new QTableWidgetItem(tr("%1").arg(fee));
           m_pCentralWidget->m_trans_tab.tablewidget->setItem(row, col,  m_pCentralWidget->m_trans_tab.itm);
           index+=2;
           //std::cout << jstr.size() << "index!!!!!!!!!-" << index << std::endl;
           //==========

        d = 0;
        col = 0;
        ++row;
        break;
        m_pCentralWidget->m_trans_tab.createNewRow();

    }

    //SetNewTask2("get_account_history hayq 4",this,NULL,&Mainwindow_gui_wallet::TaskDoneFuncGUI);
}





//    SetNewTask2("get_account_history hayq 4",this,NULL,&Mainwindow_gui_wallet::TaskDoneFuncGUI);
//}

//#if 0
//void gui_wallet::Mainwindow_gui_wallet::TaskDoneTransactionsGUI(void* a_clbkArg,int64_t a_err,
//                                                                const std::string& a_task,const std::string& a_result)
//{
//    //
//}
//#endif


void gui_wallet::Mainwindow_gui_wallet::TaskDoneTransactionsGUI3(void* a_clbkArg,int64_t a_err,
                                                                 const std::string& a_task,const fc::variant& a_result)
{
    __DEBUG_APP2__(0, " ");
}


