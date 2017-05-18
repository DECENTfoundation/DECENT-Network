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
#include "stdafx.h"

#include "gui_wallet_global.hpp"
#include "gui_design.hpp"
#include "gui_wallet_connectdlg.hpp"
//#include "connected_api_instance.hpp"
#ifndef _MSC_VER
#include <QLineEdit>
#include <QPushButton>
#include <QHeaderView>
#include <thread>
#include <QLabel>
#include <stdio.h>
#include <QMessageBox>
#endif

#include "richdialog.hpp"
#include "gui_wallet_mainwindow.hpp"


namespace gui_wallet
{
    PasswordDialog::PasswordDialog(Mainwindow_gui_wallet* pParent, bool isSet)
    : QDialog((QWidget*)pParent, Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint)
    , ret_value()
    , m_pParent(pParent)
    {       
        QLabel* text = new QLabel(this);
        text->setText(tr("The password must be limited to 50 characters"));
        DecentButton* unlockButton = new DecentButton(this);

        if (isSet) {
            unlockButton->setText(tr("Set Password"));
        } else {
            unlockButton->setText(tr("Unlock"));
        }
        unlockButton->setFixedWidth(140);
        unlockButton->setFixedHeight(40);
        password_box.setFixedSize(300, 44);
        password_box.setEchoMode(QLineEdit::Password);
        password_box.setAttribute(Qt::WA_MacShowFocusRect, 0);
        password_box.setPlaceholderText(QString(tr("Password")));
        password_box.setStyleSheet(d_pass);
        password_box.setMaxLength(50);
       
        if (isSet) {
           setWindowTitle(tr("Set Password"));
        } else {
           text->hide();
           setWindowTitle(tr("Unlock your wallet"));
        }
       
        QHBoxLayout* button_layout = new QHBoxLayout();
        button_layout->addWidget(unlockButton, Qt::AlignCenter);
        QHBoxLayout* text_layout = new QHBoxLayout();
        text_layout->addWidget(text, Qt::AlignCenter);
        connect(unlockButton, SIGNAL(clicked()), this, SLOT(unlock_slot()));
        connect(&password_box, SIGNAL(returnPressed()), unlockButton, SIGNAL(clicked()));
        m_main_layout.setContentsMargins(40, 40, 40, 40);
        m_main_layout.addLayout(text_layout);
        m_main_layout.addWidget(&password_box);
        m_main_layout.addLayout(button_layout);
        m_main_layout.setSizeConstraint(QLayout::SetFixedSize);
        setLayout(&m_main_layout);
        
        QTimer::singleShot(0, &password_box, SLOT(setFocus()));
#ifdef _MSC_VER
        int height = style()->pixelMetric(QStyle::PM_TitleBarHeight);
        setWindowIcon(height > 32 ? QIcon(":/icon/images/windows_decent_icon_32x32.png")
           : QIcon(":/icon/images/windows_decent_icon_16x16.png"));
#endif
    }
    
    bool PasswordDialog::execRD(QPoint centerPosition, std::string& pass)
    {
        ret_value = false;
        
        centerPosition.rx() -= size().width() / 2;
        centerPosition.ry() -= size().height() / 2;
        
        //QDialog::move(centerPosition);
        QDialog::exec();
        pass = password_box.text().toStdString();
        password_box.setText("");
        
        return ret_value;
    }
    
    void PasswordDialog::resizeEvent(QResizeEvent * event)
    {
        //QSize tableSize = m_main_table.size();
        //m_main_table.setColumnWidth(0, tableSize.width() - 10);
    }
    
    void PasswordDialog::unlock_slot() {
        ret_value = true;
        close();
    }
}
