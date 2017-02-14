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
#include <string>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTableWidget>
//#include <graphene/wallet/wallet.hpp>
#include <thread>
#include <vector>
//#include "connected_api_instance.hpp"
#include "richdialog.hpp"
#include "ui_wallet_functions.hpp"

const char* StringFromQString(const QString& a_cqsString);

namespace gui_wallet
{

#if 0
class PasswordDialog : private QDialog
{
public:
    PasswordDialog(QWidget* a_pMove):m_pMove(a_pMove),m_password_lab(tr("password:")){
        m_password.setEchoMode(QLineEdit::Password);
        m_layout.addWidget(&m_password_lab);
        m_layout.addWidget(&m_password);
        setLayout(&m_layout);
    }

    std::string execN(){
        if(m_pMove){move(m_pMove->pos());}
        QDialog::exec();
        QString cqsPassword = m_password.text();
        QByteArray cLatin = cqsPassword.toLatin1();
        return cLatin.data();
    }
private:
    QWidget*    m_pMove;
    QLabel      m_password_lab;
    QLineEdit   m_password;
    QHBoxLayout m_layout;
};
#endif

class PasswordDialog : public decent::gui::tools::RichDialog
{
public:
    PasswordDialog() : decent::gui::tools::RichDialog(1){m_pTextBoxes[0].setEchoMode(QLineEdit::Password);}
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
