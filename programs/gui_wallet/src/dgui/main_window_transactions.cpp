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
#include <Qt>

void gui_wallet::Mainwindow_gui_wallet::ManagementTransactionsGUI()
{
    QString tFilterStr = m_pCentralWidget->FilterStr();
    std::string usr = StringFromQString(tFilterStr);
    SetNewTask("get_account_history " + usr +  " 100",this,NULL,&Mainwindow_gui_wallet::TaskDoneFuncGUI);
}


void gui_wallet::Mainwindow_gui_wallet::TaskDoneTransactionsGUI(void* a_clbkArg,int64_t a_err,const std::string& a_task,const std::string& a_result)
{
    int index = 0;
    int row = 1;
    int col = 0;


    m_pCentralWidget->m_trans_tab.createNewRow(a_result.size());

    //false username
    if( a_result[3] == 'a' && a_result[4] == 's' && a_result[5] == 's')
    {
        for(int i = row; i < m_pCentralWidget->m_trans_tab.tablewidget->rowCount(); ++i )
        {
            for (int j = 0; j < 4; ++j)
            {
                QString aa = 0;
                aa += '-';
                m_pCentralWidget->m_trans_tab.itm = new QTableWidgetItem(tr("%1").arg(aa));
                m_pCentralWidget->m_trans_tab.tablewidget->setItem(i, j,  m_pCentralWidget->m_trans_tab.itm);
            }
            return;
        }
    }

    while(a_result.size() > index)
    {
    //TIME
        QString date = 0;
        for (int i = index; a_result[i] != 'T'; ++i)
        {
            index++;
            date += a_result[i];
        }
        //walking on TYPE section
        for (int i = index; a_result[i - 1] != ' '; ++i){      index++;    }
        QTableWidgetItem* Idate = new QTableWidgetItem(tr("%1").arg(date));
        Idate->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        Idate->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        m_pCentralWidget->m_trans_tab.tablewidget->setItem(row, col++, Idate);

    //TYPE
        QString type = 0;
        if(a_result[index] >= 'A' && a_result[index] <= 'Z')
        {
            int word = 0;
            for (int i = index; word < 3; ++i)
            {
                index++;
                type += a_result[i];
                if(a_result[i] == ' ')
                {
                    word ++;
                    //index++;
                if(a_result[i+1] < 'A' || a_result[i+1] > 'Z') {   break; }
                }
            }
        }
        else if(a_result[index] >= 'a' && a_result[index] <= 'z')
        {
            for(int i = index; a_result[i] != ' '; ++i)
            {
                index++;
                type += a_result[i];
            }
        }
        QTableWidgetItem* Itype = new QTableWidgetItem(tr("%1").arg(type));
        Itype->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        Itype->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        m_pCentralWidget->m_trans_tab.tablewidget->setItem(row, col++, Itype);

    //INFO
        QString info = 0;
        for(int i = index; ; ++i)
        {
            if(a_result[i] == '(')
            {
                index += 5;
                break;
            }
            else if( (a_result[i] == 'F' || a_result[i] == 'f') && a_result[i + 1] == 'e' && a_result[i + 2] == 'e')
                 {
                     index += 4;
                     break;
                 }
            index++;
            info += a_result[i];
        }
        QTableWidgetItem* Iinfo = new QTableWidgetItem(tr("%1").arg(info));
        Iinfo->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        Iinfo->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        m_pCentralWidget->m_trans_tab.tablewidget->setItem(row, col++, Iinfo);
    //FEE
        QString fee = 0;
        for (int i = index; ; ++i)
        {
            if(a_result[i] >= 'A' && a_result[i] <= 'Z')
            {
                break;
            }
            index++;
            fee += a_result[i];
        }
        QTableWidgetItem* Ifee = new QTableWidgetItem(tr("%1").arg(fee));
        Ifee->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        Ifee->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        m_pCentralWidget->m_trans_tab.tablewidget->setItem(row, col++, Ifee);
        //=====
        //walking to date
        for(int i = index; ; ++i)
        {
            index++;
            if(a_result[i+1] >= '0' && a_result[i+1] <= '9')
            {
                break;
            }
        }

        row++;
        col = 0;
    }
    m_pCentralWidget->m_trans_tab.deleteEmptyRows();

    if(m_pCentralWidget->m_trans_tab.green_row != 0)
    {
        m_pCentralWidget->m_trans_tab.tablewidget->item(m_pCentralWidget->m_trans_tab.green_row,0)->setBackgroundColor(QColor(27,176,104));
        m_pCentralWidget->m_trans_tab.tablewidget->item(m_pCentralWidget->m_trans_tab.green_row,1)->setBackgroundColor(QColor(27,176,104));
        m_pCentralWidget->m_trans_tab.tablewidget->item(m_pCentralWidget->m_trans_tab.green_row,2)->setBackgroundColor(QColor(27,176,104));
        m_pCentralWidget->m_trans_tab.tablewidget->item(m_pCentralWidget->m_trans_tab.green_row,3)->setBackgroundColor(QColor(27,176,104));

        m_pCentralWidget->m_trans_tab.tablewidget->item(m_pCentralWidget->m_trans_tab.green_row,0)->setForeground(QColor::fromRgb(255,255,255));
        m_pCentralWidget->m_trans_tab.tablewidget->item(m_pCentralWidget->m_trans_tab.green_row,1)->setForeground(QColor::fromRgb(255,255,255));
        m_pCentralWidget->m_trans_tab.tablewidget->item(m_pCentralWidget->m_trans_tab.green_row,2)->setForeground(QColor::fromRgb(255,255,255));
        m_pCentralWidget->m_trans_tab.tablewidget->item(m_pCentralWidget->m_trans_tab.green_row,3)->setForeground(QColor::fromRgb(255,255,255));
    }
}


void gui_wallet::Mainwindow_gui_wallet::TaskDoneTransactionsGUI3(void* a_clbkArg,int64_t a_err,
                                                                 const std::string& a_task,const fc::variant& a_result)
{
    __DEBUG_APP2__(0, " ");
}


