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
#include "gui_wallet_mainwindow.hpp"


namespace gui_wallet
{
    PasswordDialog::PasswordDialog(Mainwindow_gui_wallet* pParent, bool isSet)
    : QDialog((QWidget*)pParent, Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint)
    , ret_value()
    , m_pParent(pParent)
    , m_main_table(2, 1)
    {
        resize(350, 150);
        
        m_main_table.horizontalHeader()->hide();
        m_main_table.verticalHeader()->hide();
        m_main_table.setShowGrid(false);
        m_main_table.setFrameShape(QFrame::NoFrame);
        
        QPalette plt_tbl = m_main_table.palette();
        plt_tbl.setColor(QPalette::Base, palette().color(QPalette::Window));
        m_main_table.setPalette(plt_tbl);
        
        DecentButton* unlockButton = new DecentButton();
        DecentButton* cencel = new DecentButton();
        
        // set hight
        cencel->setStyleSheet("QLabel { background-color :rgb(255,255,255); color : rgb(0,0,0);}");
        if (isSet) {
            unlockButton->setText("Set Password");
            unlockButton->setFixedWidth(150);
        } else {
            unlockButton->setText("Unlock");
            cencel->setText("Cencel");
        }
        unlockButton->setFixedWidth(120);
        cencel->setFixedWidth(120);
        unlockButton->setFixedHeight(30);
        cencel->setFixedHeight(30);
        password_box.setEchoMode(QLineEdit::Password);
        password_box.setAttribute(Qt::WA_MacShowFocusRect, 0);
        password_box.setPlaceholderText(QString("Password"));
        
        if (isSet) {
            m_main_table.setCellWidget(0, 0, new QLabel(tr("Choose password to encrypt your wallet.")));
        } else {
           setWindowTitle("Unlock your wallet");
        }
       
        m_main_table.setCellWidget(1, 0, &password_box);
        //m_main_table.setCellWidget(2, 0, unlockButton);
        QHBoxLayout* button_layout = new QHBoxLayout();
        button_layout->addWidget(unlockButton);
        button_layout->addWidget(cencel);
        connect(unlockButton, SIGNAL(LabelClicked()), this, SLOT(unlock_slot()));
        connect(cencel, SIGNAL(LabelClicked()), this, SLOT(close()));
        connect(&password_box, SIGNAL(returnPressed()), unlockButton, SIGNAL(LabelClicked()));
        button_layout->setContentsMargins(0, 0, 0, 0);
        m_main_layout.setContentsMargins(40, 0, 40, 30);
        m_main_layout.addWidget(&m_main_table);
        m_main_layout.addLayout(button_layout);
        setLayout(&m_main_layout);
        
        QTimer::singleShot(0, &password_box, SLOT(setFocus()));
    }
    
    bool PasswordDialog::execRD(QPoint centerPosition, std::string& pass)
    {
        ret_value = false;
        
        centerPosition.rx() -= size().width() / 2;
        centerPosition.ry() -= size().height() / 2;
        
        QDialog::move(centerPosition);
        QDialog::exec();
        pass = password_box.text().toStdString();
        password_box.setText("");
        
        return ret_value;
    }
    
    void PasswordDialog::resizeEvent(QResizeEvent * event)
    {
        QSize tableSize = m_main_table.size();
        m_main_table.setColumnWidth(0, tableSize.width() - 10);
    }
    
    void PasswordDialog::unlock_slot() {
        ret_value = true;
        close();
    }
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
    m_ret_value = RDB_CANCEL;
    
    QString tqsString;
    ((QLineEdit*)m_main_table.cellWidget(RPC_ENDPOINT_FIELD,1))->setText(tr(a_pData->ws_server.c_str()));
    ((QLineEdit*)(m_main_table.cellWidget(WALLET_FILE_FIELD,1)))->setText(tr(a_pData->wallet_file_name.c_str()));
    ((QLineEdit*)m_main_table.cellWidget(CHAIN_ID_FIELD,1))->setText(tr(a_pData->chain_id.c_str()));
    
    
    QDialog::exec();

    tqsString = ((QLineEdit*)m_main_table.cellWidget(RPC_ENDPOINT_FIELD,1))->text();
    a_pData->ws_server = tqsString.toStdString();
    tqsString = ((QLineEdit*)m_main_table.cellWidget(WALLET_FILE_FIELD,1))->text();
    a_pData->wallet_file_name = tqsString.toStdString();
    tqsString = ((QLineEdit*)m_main_table.cellWidget(CHAIN_ID_FIELD,1))->text();
    a_pData->chain_id = tqsString.toStdString();

    return m_ret_value;
}


void gui_wallet::ConnectDlg::ConnectPushedSlot()
{
    m_ret_value = RDB_OK;
    close();
}
