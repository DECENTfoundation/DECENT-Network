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
#include <graphene/wallet/wallet.hpp>
#include <thread>
#include <vector>
#include "connected_api_instance.hpp"

namespace gui_wallet
{

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

class ConnectDlg : public QDialog
{
    Q_OBJECT

    enum{RPC_ENDPOINT_FIELD,WALLET_FILE_FIELD,CHAIN_ID_FIELD,CONNECT_BUTTON_FIELD,NUM_OF_FIELDS};

    //friend int CreateWallepApiInstance( void* dataContainer );
public:
    ConnectDlg(QWidget* parent);
    virtual ~ConnectDlg();

    std::string GetWalletFileName()const;
    void SetWalletFileName(const std::string& wallet_file_name);

private:
    void CallSaveWalletFile(struct StructApi* pApi);

    static void error_function(void* a_pOwner, const std::string& a_err, const std::string& a_details);
    void error_function(const std::string& a_err, const std::string& a_details);

    static void done_function(void* a_pOwner);
    void done_function();
    static void SetPassword(void* a_owner,int a_answer,/*string**/void* a_str_ptr);

protected:
    void resizeEvent ( QResizeEvent * event );

protected slots:
    void ConnectPushedSlot();
    void ConnectDoneSlot();
    void ConnectErrorSlot(const std::string a_err, const std::string a_details);

private:
signals:
    void ConnectDoneSig();
    void ConnectErrorSig(const std::string a_err, const std::string a_details);

public:
    //mutable void*                   m_pTmp; // used for callbacks
private:
    graphene::wallet::wallet_data   m_wdata;
    QHBoxLayout                     m_main_layout;
    QTableWidget                    m_main_table;
    mutable std::string             m_wallet_file_name;
    //graphene::wallet::wallet_api*   m_pCurApi;
    //fc::rpc::gui*                   m_pCurGuiApi;
    //std::vector<graphene::wallet::wallet_api*>   m_vAllApis;

private:
    PasswordDialog m_PasswdDialog;
};

}

#endif // GUI_WALLET_CONNECTDLG_HPP
