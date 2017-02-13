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
#include "connected_api_instance.hpp"
#include <QLineEdit>
#include <QPushButton>
#include <QHeaderView>
#include <thread>
#include <QLabel>
#include <stdio.h>

#ifndef DEFAULT_WALLET_FILE_NAME
#define DEFAULT_WALLET_FILE_NAME       "wallet.json"
#endif

extern int g_nDebugApplication ;


/* ///////////////////////////////////  */
gui_wallet::ConnectDlg::ConnectDlg(QWidget* a_parent)
    :
      QDialog(a_parent),
      m_main_table(NUM_OF_FIELDS,2),
      m_wallet_file_name(DEFAULT_WALLET_FILE_NAME),
      m_PasswdDialog(this)
{
    //m_wdata.chain_id = (chain_id_type)0;

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
    connect(this, SIGNAL(ConnectDoneSig()), this, SLOT(ConnectDoneSlot()) );
    connect(this, SIGNAL(ConnectErrorSig(std::string, std::string)), this, SLOT(ConnectErrorSlot(std::string, std::string)) );
}


gui_wallet::ConnectDlg::~ConnectDlg()
{
    UseConnectedApiInstance(this,&gui_wallet::ConnectDlg::CallSaveWalletFile);
}


void gui_wallet::ConnectDlg::CallSaveWalletFile(struct StructApi* a_pApi)
{
    try{
        if(a_pApi && a_pApi->wal_api)
        {
            (a_pApi->wal_api)->save_wallet_file(m_wallet_file_name);
        }
    }
    catch(const fc::exception& a_fc)
    {
        if(g_nDebugApplication){printf("file:\"" __FILE__ "\",line:%d\n",__LINE__);}
        //(*a_fpErr)(a_pOwner,a_fc.to_string(),a_fc.to_detail_string());
        if(g_nDebugApplication){printf("%s\n",(a_fc.to_detail_string()).c_str());}
    }
    catch(...)
    {
        if(g_nDebugApplication){printf("file:\"" __FILE__ "\",line:%d\n",__LINE__);}
        if(g_nDebugApplication){printf("Unknown exception\n");}
    }
}


void gui_wallet::ConnectDlg::resizeEvent ( QResizeEvent * event )
{
    QWidget::resizeEvent(event);

    QSize aInfWidgSize = m_main_table.size();

    m_main_table.setColumnWidth(0,40*aInfWidgSize.width()/100);
    m_main_table.setColumnWidth(1,59*aInfWidgSize.width()/100);

}


#include <QMessageBox>

void gui_wallet::ConnectDlg::error_function(void* a_pOwner, const std::string& a_err, const std::string& a_details)
{
    ((ConnectDlg*)a_pOwner)->error_function(a_err, a_details);
}

void gui_wallet::ConnectDlg::error_function(const std::string& a_err, const std::string& a_details)
{
    emit ConnectErrorSig(a_err,a_details);
}


void gui_wallet::ConnectDlg::done_function(void* a_pOwner)
{
    ((ConnectDlg*)a_pOwner)->done_function();
}

void gui_wallet::ConnectDlg::done_function()
{
    emit ConnectDoneSig();
}


void gui_wallet::ConnectDlg::ConnectErrorSlot(const std::string a_err, const std::string a_details)
{
    //ConnectDlg* pParent = (ConnectDlg*)a_pOwner;
    QMessageBox aMessageBox(QMessageBox::Critical,QObject::tr("error"),QObject::tr(a_err.c_str()),
                            QMessageBox::Ok,this);
    aMessageBox.setDetailedText(QObject::tr(a_details.c_str()));
    aMessageBox.exec();
}


void gui_wallet::ConnectDlg::ConnectDoneSlot()
{
    std::string csWalletFileName = GetWalletFileName();
    FILE* fpWallFile = fopen(csWalletFileName.c_str(),"r");
    if(g_nDebugApplication){printf("fpWallFile=%p, chain_id=%s\n",fpWallFile,m_wdata.chain_id.str().c_str());}

    if(fpWallFile)
    {
        fclose(fpWallFile);
        m_wdata = fc::json::from_file( csWalletFileName ).as< graphene::wallet::wallet_data >();
        //m_wdata.chain_id.str();
    }
    else
    {
        //save_wallet_file
    }

    ((QLineEdit*)(m_main_table.cellWidget(CHAIN_ID_FIELD,1)))->setText(tr(m_wdata.chain_id.str().c_str()));

    //ConnectDlg* pParent = (ConnectDlg*)a_pOwner;
    QMessageBox aMessageBox(QMessageBox::Information,QObject::tr("connected"),QObject::tr("connected"),
                            QMessageBox::Ok,this);
    aMessageBox.setDetailedText(QObject::tr("Connected"));
    aMessageBox.exec();
}


std::string gui_wallet::ConnectDlg::GetWalletFileName()const
{
    if(isVisible())
    {
        QString cWalletFileName = ((QLineEdit*)m_main_table.cellWidget(WALLET_FILE_FIELD,1))->text();
        QByteArray cLatin = cWalletFileName.toLatin1();
        m_wallet_file_name = cLatin.data();
    }

    return m_wallet_file_name;
}


void gui_wallet::ConnectDlg::SetWalletFileName(const std::string& a_wallet_file_name)
{
    m_wallet_file_name = a_wallet_file_name;
    ((QLineEdit*)m_main_table.cellWidget(WALLET_FILE_FIELD,1))->setText(tr(m_wallet_file_name.c_str()));
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
        *pcsPassword = pThis->execN();
    }
        break;

    default:
        break;
    }

}


void gui_wallet::ConnectDlg::ConnectPushedSlot()
{
    DoneFuncType fpDone = &ConnectDlg::done_function;
    ErrFuncType fpErr = &ConnectDlg::error_function;
    WarnYesOrNoFuncType fpPasswdFunc = &ConnectDlg::SetPassword;
    QString aRpcEndPointAStr = ((QLineEdit*)m_main_table.cellWidget(RPC_ENDPOINT_FIELD,1))->text();
    QByteArray aLatin=aRpcEndPointAStr.toLatin1();
    std::string csWalletFileName = GetWalletFileName();
    FILE* fpWallFile = fopen(csWalletFileName.c_str(),"r");

    if(fpWallFile)
    {
        fclose(fpWallFile);
        m_wdata = fc::json::from_file( csWalletFileName ).as< graphene::wallet::wallet_data >();
    }
    else
    {
        m_wdata.ws_server = aLatin.data();
        m_wdata.chain_id = chain_id_type( std::string( (((QLineEdit*)m_main_table.cellWidget(CHAIN_ID_FIELD,1))->text()).toLatin1().data() ) );
    }

    std::thread aThread(&CreateConnectedApiInstance,&m_wdata,csWalletFileName,this,fpDone, fpErr,fpPasswdFunc);
    aThread.detach();
    //close();
}

