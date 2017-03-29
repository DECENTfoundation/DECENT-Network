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
        resize(570, 220);
       
       
        m_main_table.horizontalHeader()->hide();
        m_main_table.verticalHeader()->hide();
        m_main_table.setShowGrid(false);
        m_main_table.setFrameShape(QFrame::NoFrame);
        
        QPalette plt_tbl = m_main_table.palette();
        plt_tbl.setColor(QPalette::Base, palette().color(QPalette::Window));
        m_main_table.setPalette(plt_tbl);
        
        DecentButton* unlockButton = new DecentButton();

        if (isSet) {
            unlockButton->setText("Set Password");
        } else {
            unlockButton->setText("Unlock");
        }
        unlockButton->setFixedWidth(185);
        unlockButton->setFixedHeight(38);
        password_box.setFixedSize(383, 44);
        password_box.setEchoMode(QLineEdit::Password);
        password_box.setAttribute(Qt::WA_MacShowFocusRect, 0);
        password_box.setPlaceholderText(QString("Password"));
        password_box.setStyleSheet("border: 1px solid rgb(143,143,143);padding-left:25px;");
       
        if (isSet) {
           setWindowTitle("Set Password");
        } else {
           setWindowTitle("Unlock your wallet");
        }
       
        m_main_table.setCellWidget(1, 0, &password_box);
        QHBoxLayout* button_layout = new QHBoxLayout();
        button_layout->addWidget(unlockButton);
        connect(unlockButton, SIGNAL(LabelClicked()), this, SLOT(unlock_slot()));
        connect(&password_box, SIGNAL(returnPressed()), unlockButton, SIGNAL(LabelClicked()));
        m_main_layout.setContentsMargins(93, 30, 93, 60);
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
