/*
 *	File: gui_wallet_connectdlg.hpp
 *
 *	Created on: 12 Dec, 2016
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This is the header file for the
 *   class ConnectDlg. This class implements
 *   functionality necessary to connect to witness node
 *
 */

#ifndef GUI_WALLET_CONNECTDLG_HPP
#define GUI_WALLET_CONNECTDLG_HPP

#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QTimer>
#include <string>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTableWidget>
#include <QHeaderView>

#include <thread>
#include <vector>

#include "decent_button.hpp"
#include "richdialog.hpp"
#include "ui_wallet_functions.hpp"

const char* StringFromQString(const QString& a_cqsString);

namespace gui_wallet
{

    
class PasswordDialog : public QDialog {
    Q_OBJECT
    
public:
    PasswordDialog(bool isSet = false) : QDialog(Q_NULLPTR, Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint), m_main_table(2, 1) {
        resize(350, 150);

        m_main_table.horizontalHeader()->hide();
        m_main_table.verticalHeader()->hide();
        m_main_table.setShowGrid(false);
        m_main_table.setFrameShape(QFrame::NoFrame);
        
        QPalette plt_tbl = m_main_table.palette();
        plt_tbl.setColor(QPalette::Base, palette().color(QPalette::Window));
        m_main_table.setPalette(plt_tbl);

        DecentButton* unlockButton = new DecentButton();
        
        // set hight
        unlockButton->setFixedHeight(30);
        
        if (isSet) {
            unlockButton->setText("Set Password");
            unlockButton->setFixedWidth(150);
        } else {
            unlockButton->setText("Unlock");
            unlockButton->setFixedWidth(100);
        }
        unlockButton->setFixedHeight(30);
        unlockButton->setFixedWidth(120);
        password_box.setEchoMode(QLineEdit::Password);
        password_box.setAttribute(Qt::WA_MacShowFocusRect, 0);
        
        if (isSet) {
            m_main_table.setCellWidget(0, 0, new QLabel(tr("Choose password to encrypt your wallet.")));
        } else {
            m_main_table.setCellWidget(0, 0, new QLabel(tr("Please unlock your wallet to continue.")));
        }
        
        m_main_table.setCellWidget(1, 0, &password_box);
        //m_main_table.setCellWidget(2, 0, unlockButton);
        QHBoxLayout* button_layout = new QHBoxLayout();
        button_layout->addWidget(unlockButton);
        connect(unlockButton, SIGNAL(LabelClicked()),this, SLOT(unlock_slot()));
        connect(&password_box, SIGNAL(returnPressed()), unlockButton, SIGNAL(LabelClicked()));
        
        m_main_layout.addWidget(&m_main_table);
        //button_layout->setContentsMargins(50, 0, 50, 0);
        m_main_layout.addLayout(button_layout);
        setLayout(&m_main_layout);

        QTimer::singleShot(0, &password_box, SLOT(setFocus()));

    }
    
    bool execRD(QPoint centerPosition, std::string& pass)
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
    
    
protected:
    virtual void resizeEvent ( QResizeEvent * event ) {
        QSize tableSize = m_main_table.size();
        m_main_table.setColumnWidth(0, tableSize.width() - 10);
    }
    
    


protected slots:
    
    void unlock_slot() {
        ret_value = true;
        close();
    }
    
    
private:
    QVBoxLayout                     m_main_layout;
    QTableWidget                    m_main_table;
    QLineEdit                       password_box;
    bool                            ret_value;
    
};

class ConnectDlg : public QDialog
{
    Q_OBJECT
public:
    enum{RPC_ENDPOINT_FIELD,WALLET_FILE_FIELD,CHAIN_ID_FIELD,CONNECT_BUTTON_FIELD,NUM_OF_FIELDS};

public:
    ConnectDlg(QWidget* parent);
    virtual ~ConnectDlg();

    int execNew(SConnectionStruct* pData);

    QWidget* GetTableWidget(int column, int row);

protected:
    void resizeEvent ( QResizeEvent * event );

protected slots:
    void ConnectPushedSlot();

private:
    SConnectionStruct*   m_wdata_ptr;
    QHBoxLayout                     m_main_layout;
    QTableWidget                    m_main_table;
    int                             m_ret_value;
};
    
    

}

#endif // GUI_WALLET_CONNECTDLG_HPP
