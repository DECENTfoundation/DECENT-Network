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

const char* StringFromQString(const QString& a_cqsString);

namespace gui_wallet
{
class Mainwindow_gui_wallet;
    
class PasswordDialog : public QDialog
{
    Q_OBJECT
        
public:
    PasswordDialog(Mainwindow_gui_wallet* pParent, bool isSet);
    bool execRD(QPoint centerPosition, std::string& pass);
        
protected:
    virtual void resizeEvent (QResizeEvent * event);
        
protected slots:
    void unlock_slot();
        
private:
    bool ret_value;
    Mainwindow_gui_wallet* m_pParent;
    QVBoxLayout m_main_layout;
    //QTableWidget m_main_table;
    QLineEdit password_box;
};

/*
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
   */ 
    

}

#endif // GUI_WALLET_CONNECTDLG_HPP
