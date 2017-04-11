/*
 *	File: richdialog.hpp
 *
 *	Created on: 27 Jan 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
#ifndef RICHDIALOG_HPP
#define RICHDIALOG_HPP

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <vector>
#include <string>
#include "decent_button.hpp"


namespace gui_wallet {


enum RET_TYPE {RDB_OK, RDB_CANCEL};

class RichDialogBase : protected QDialog
{
    Q_OBJECT
public:
    RichDialogBase(QString title);
    virtual ~RichDialogBase(){}

    virtual RET_TYPE execRB(const QPoint* pMove);
    virtual void AddLayout(QLayout* pLayout);
    virtual void AddWidget(QWidget* pWidget);

protected slots:
    void set_ok_and_closeSlot();

protected:
    QVBoxLayout   m_main_layout;
    QVBoxLayout   m_controls_layout;
    QHBoxLayout   m_buttons_layout;
    DecentButton  m_ok_button;
    DecentButton  m_cancel_button;
    RET_TYPE      m_ret_value;
};

/********************************************/
class RichDialog : protected RichDialogBase
{
public:
    RichDialog(int num_of_text_boxes , QString title);
    virtual ~RichDialog();

    virtual RET_TYPE execRD(const QPoint* pMove, std::vector<std::string>& results);

protected:
    int         m_nNumOfTextBoxes;
    QLineEdit*  m_pTextBoxes;

};

   

class SendDialogBase : protected QDialog
{
   Q_OBJECT
public:
   SendDialogBase(QString title);
   virtual ~SendDialogBase(){}
   
   virtual RET_TYPE execRB(const QPoint* pMove);
   virtual void AddLayout(QLayout* pLayout);
   virtual void AddWidget(QWidget* pWidget);
   
   protected slots:
   void set_ok_and_closeSlot();
   
protected:
   QVBoxLayout   m_main_layout;
   QVBoxLayout   m_controls_layout;
   QHBoxLayout   m_buttons_layout;
   DecentButton  m_ok_button;
   DecentButton  m_cancel_button;
   RET_TYPE      m_ret_value;
};

/********************************************/
class SendDialog : protected SendDialogBase
{
public:
   SendDialog(int num_of_text_boxes , QString title);
   virtual ~SendDialog();
   
   virtual RET_TYPE execRD(const QPoint* pMove, std::vector<std::string>& results);
   
protected:
   int         m_nNumOfTextBoxes;
   QLineEdit*  m_pTextBoxes;
   
};
   
}


#endif // RICHDIALOG_HPP
