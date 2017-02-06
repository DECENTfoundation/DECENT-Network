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

#ifndef DEFAULT_WALLET_FILE_NAME
#define DEFAULT_WALLET_FILE_NAME       "wallet.json"
#endif

extern int g_nDebugApplication ;


/* ///////////////////////////////////  */
gui_wallet::ConnectDlg::ConnectDlg(QWidget* a_parent)
    :
      QDialog(a_parent),
      m_main_table(NUM_OF_FIELDS,2)
{
    //m_wdata.chain_id = (chain_id_type)0;
    m_wdata.wallet_file_name = DEFAULT_WALLET_FILE_NAME;

    m_main_table.setItem(RPC_ENDPOINT_FIELD,0,new QTableWidgetItem(tr("rpc-endpoint")));
    m_main_table.setCellWidget(RPC_ENDPOINT_FIELD,1,new QLineEdit(tr("ws://127.0.0.1:8090")));

    m_main_table.setItem(WALLET_FILE_FIELD,0,new QTableWidgetItem(tr("wallet file")));
    m_main_table.setCellWidget(WALLET_FILE_FIELD,1,new QLineEdit(tr(DEFAULT_WALLET_FILE_NAME)));

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
    SaveWalletFile2(m_wdata);
}


void gui_wallet::ConnectDlg::resizeEvent ( QResizeEvent * event )
{
    QWidget::resizeEvent(event);

    QSize aInfWidgSize = m_main_table.size();

    m_main_table.setColumnWidth(0,40*aInfWidgSize.width()/100);
    m_main_table.setColumnWidth(1,59*aInfWidgSize.width()/100);

}


void gui_wallet::ConnectDlg::ErrorMsgBoxFnc(void* a_owner, void* /*a_clbData*/,const std::string& a_err,const std::string& a_details)
{
    ((gui_wallet::ConnectDlg*)a_owner)->ConnectErrorFncGui(a_err,a_details);
}


void gui_wallet::ConnectDlg::ConnectErrorFncGui(const std::string& a_err, const std::string& a_details)
{
    //ConnectDlg* pParent = (ConnectDlg*)a_pOwner;
    QMessageBox aMessageBox(QMessageBox::Critical,QObject::tr("error"),QObject::tr(a_err.c_str()),
                            QMessageBox::Ok,this);
    aMessageBox.setDetailedText(QObject::tr(a_details.c_str()));
    aMessageBox.exec();
}


std::string gui_wallet::ConnectDlg::GetWalletFileName()
{
    if(isVisible())
    {
        QString cWalletFileName = ((QLineEdit*)m_main_table.cellWidget(WALLET_FILE_FIELD,1))->text();
        QByteArray cLatin = cWalletFileName.toLatin1();
        m_wdata.wallet_file_name = cLatin.data();
    }

    return m_wdata.wallet_file_name;
}


void gui_wallet::ConnectDlg::SetWalletFileName(const std::string& a_wallet_file_name)
{
    m_wdata.wallet_file_name = a_wallet_file_name;
    ((QLineEdit*)m_main_table.cellWidget(WALLET_FILE_FIELD,1))->setText(tr(m_wdata.wallet_file_name.c_str()));
}


void gui_wallet::ConnectDlg::SetPassword(void* a_owner,int a_answer,/*string**/void* a_str_ptr)
{
    std::string* pcsPassword = (std::string*)a_str_ptr;
    *pcsPassword = "";

    switch(a_answer)
    {
    case QMessageBox::Yes: case QMessageBox::Ok:
    {
        gui_wallet::ConnectDlg* pThisCon = (gui_wallet::ConnectDlg*)a_owner;
        PasswordDialog* pThis = &pThisCon->m_PasswdDialog;
        /*pThis->move(pThisCon->pos());
        pThis->exec();
        QString cqsPassword = pThis->m_password.text();
        QByteArray cLatin = cqsPassword.toLatin1();
        *pcsPassword = cLatin.data();*/
        std::vector<std::string> vsPassword(1);
        QPoint thisPos = pThisCon->pos();
        decent::gui::tools::RET_TYPE rtRet = pThis->execRD(&thisPos,vsPassword);
        if(rtRet != decent::gui::tools::RDB_CANCEL){*pcsPassword = vsPassword[0];}
    }
        break;

    default:
        break;
    }

}


//tatic void StartConnectionProcedure(const SConnectionStruct& a_conn_str, Type* a_memb, void* a_clbData,
//  void (Type::*a_clbkFunction)(SetNewTask_last_args))

void gui_wallet::ConnectDlg::ConnectPushedSlot()
{
    m_wdata.wallet_file_name = GetWalletFileName();
    if(g_nDebugApplication)
    {
        printf("fn:%s, ln:%d -> wal_fl_name=%s\n",__FUNCTION__,__LINE__,m_wdata.wallet_file_name.c_str());
    }

    if(LoadWalletFile(&m_wdata))
    { // File does not exist
        QString  cqsData = ((QLineEdit*)m_main_table.cellWidget(RPC_ENDPOINT_FIELD,1))->text();
        QByteArray aLatin = cqsData.toLatin1();
        m_wdata.ws_server = aLatin.data();
        //m_wdata.chain_id = chain_id_type( std::string( (((QLineEdit*)m_main_table.cellWidget(CHAIN_ID_FIELD,1))->text()).toLatin1().data() ) );
        m_wdata.chain_id = (((QLineEdit*)m_main_table.cellWidget(CHAIN_ID_FIELD,1))->text()).toLatin1().data() ;
    }

    if(g_nDebugApplication)
    {
        printf("fn:%s, ln:%d \n",__FUNCTION__,__LINE__);
    }

    m_wdata.action = WAT::CONNECT;
    m_wdata.fpWarnFunc = &SetPassword;
    m_wdata.fpErr = &ErrorMsgBoxFnc;
    StartConnectionProcedure(m_wdata,this,&m_wdata,&gui_wallet::ConnectDlg::SaveAndConnectDoneFncGUI);

}


void gui_wallet::ConnectDlg::SaveAndConnectDoneFncGUI(void* /*clbkArg*/,int64_t a_err,const std::string& a_task,const std::string& a_result)
{
    if(a_err)
    {
        ConnectErrorFncGui(a_task,a_result);
        return ;
    }

    m_wdata.wallet_file_name = GetWalletFileName();
    //FILE* fpWallFile = fopen(csWalletFileName.c_str(),"r");
    //int LoadWalletFile(const std::string& a_file_name, SConnectionStruct* a_pWalletData);
    LoadWalletFile(&m_wdata);
    if(g_nDebugApplication){printf("chain_id=%s\n",m_wdata.chain_id.c_str());}

    ((QLineEdit*)(m_main_table.cellWidget(CHAIN_ID_FIELD,1)))->setText(tr(m_wdata.chain_id.c_str()));

    //ConnectDlg* pParent = (ConnectDlg*)a_pOwner;
    QMessageBox aMessageBox(QMessageBox::Information,QObject::tr("connected"),QObject::tr("connected"),
                            QMessageBox::Ok,this);
    aMessageBox.setDetailedText(QObject::tr("Connected"));
    aMessageBox.exec();
    emit ConnectDoneSig();
}




