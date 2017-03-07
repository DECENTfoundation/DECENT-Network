/*
 *	File: gui_wallet_connectdlg.cpp
 *
 *	Created on: 12 Dec, 2016
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  Implementation for
 *   class ConnectDlg. This class implements
 *   functionality necessary to connect to witness node
 *
 */

#include "gui_wallet_connectdlg.hpp"
//#include "connected_api_instance.hpp"
#include <QLineEdit>
#include <QPushButton>
#include <QHeaderView>
#include <thread>
#include <QLabel>
#include <stdio.h>
#include <QMessageBox>
#include "richdialog.hpp"

const char* StringFromQString(const QString& a_cqsString)
{
    QByteArray cLatin = a_cqsString.toLatin1();
    return cLatin.data();
}

/* ///////////////////////////////////  */
gui_wallet::ConnectDlg::ConnectDlg(QWidget* a_parent)
    :
      QDialog(a_parent),
      m_main_table(NUM_OF_FIELDS,2)
{
    m_main_table.setItem(RPC_ENDPOINT_FIELD,0,new QTableWidgetItem(tr("rpc-endpoint")));
    m_main_table.setCellWidget(RPC_ENDPOINT_FIELD,1,new QLineEdit(tr("ws://127.0.0.1:8090")));

    m_main_table.setItem(WALLET_FILE_FIELD,0,new QTableWidgetItem(tr("wallet file")));
    m_main_table.setCellWidget(WALLET_FILE_FIELD,1,new QLineEdit(tr("")));

    m_main_table.setItem(CHAIN_ID_FIELD,0,new QTableWidgetItem(tr("chain-id")));
    //m_main_table.setCellWidget(CHAIN_ID_FIELD,1,new QLineEdit(tr("d9561465fd1aab95eb6fec9a60705e983b7759ea4c9892ac4acd30737f5079b4")));
    m_main_table.setCellWidget(CHAIN_ID_FIELD,1,new QLineEdit(tr("0000000000000000000000000000000000000000000000000000000000000000")));

    //m_main_table.setItem(FIELDS-1,0,new QTableWidgetItem(tr("rpc-endpoint")));
    m_main_table.setCellWidget(CONNECT_BUTTON_FIELD,0,new QLabel);
    m_main_table.setCellWidget(CONNECT_BUTTON_FIELD,1,new QPushButton(tr("Connect")));

    QPalette aPalette = m_main_table.cellWidget(CONNECT_BUTTON_FIELD,0)->palette();
    aPalette.setColor(QPalette::Base,Qt::gray);
    m_main_table.cellWidget(CONNECT_BUTTON_FIELD,0)->setPalette(aPalette);

    m_main_table.horizontalHeader()->hide();
    m_main_table.verticalHeader()->hide();
    m_main_table.resize(size());
    m_main_layout.addWidget(&m_main_table);
    setLayout(&m_main_layout);

    /* Initing signal-slot pairs*/
    connect( m_main_table.cellWidget(CONNECT_BUTTON_FIELD,1), SIGNAL(clicked()), this, SLOT(ConnectPushedSlot()) );
}


gui_wallet::ConnectDlg::~ConnectDlg()
{
}


void gui_wallet::ConnectDlg::resizeEvent ( QResizeEvent * event )
{
    QWidget::resizeEvent(event);

    QSize aInfWidgSize = m_main_table.size();

    m_main_table.setColumnWidth(0,40*aInfWidgSize.width()/100);
    m_main_table.setColumnWidth(1,59*aInfWidgSize.width()/100);

}


QWidget* gui_wallet::ConnectDlg::GetTableWidget(int a_column, int a_row)
{
    return m_main_table.cellWidget(a_column,a_row);
}


int gui_wallet::ConnectDlg::execNew(SConnectionStruct* a_pData)
{
    m_ret_value = decent::gui::tools::RDB_CANCEL;
    
    QString tqsString;
    ((QLineEdit*)m_main_table.cellWidget(RPC_ENDPOINT_FIELD,1))->setText(tr(a_pData->ws_server.c_str()));
    ((QLineEdit*)(m_main_table.cellWidget(WALLET_FILE_FIELD,1)))->setText(tr(a_pData->wallet_file_name.c_str()));
    ((QLineEdit*)m_main_table.cellWidget(CHAIN_ID_FIELD,1))->setText(tr(a_pData->chain_id.c_str()));
    
    
    QDialog::exec();

    tqsString = ((QLineEdit*)m_main_table.cellWidget(RPC_ENDPOINT_FIELD,1))->text();
    a_pData->ws_server = StringFromQString(tqsString);
    tqsString = ((QLineEdit*)m_main_table.cellWidget(WALLET_FILE_FIELD,1))->text();
    a_pData->wallet_file_name = StringFromQString(tqsString);
    tqsString = ((QLineEdit*)m_main_table.cellWidget(CHAIN_ID_FIELD,1))->text();
    a_pData->chain_id = StringFromQString(tqsString);

    return m_ret_value;
}


void gui_wallet::ConnectDlg::ConnectPushedSlot()
{
    m_ret_value = decent::gui::tools::RDB_OK;
    close();
}
