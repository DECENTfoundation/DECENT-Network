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
#include <QLocale>
#include <QInputMethod>
#include <QApplication>
#include <vector>
#include <string>
#include "decent_button.hpp"



namespace gui_wallet {
               //SendDialog
/********************************************/
   
class TransferDialog : public QDialog
{
   Q_OBJECT
public:
   TransferDialog(QWidget* parent, QString const& userName = QString());
   
public slots:
   void nameChanged(const QString &);
   void amountChanged(const QString &);
   void memoChanged(const QString &);
   void Transfer();
private:
   QString  m_toUserName;
   double   m_amount;
   QString  m_memo;
   QString  m_fromUserName;
};
   
/********************************************/
   
   
            //ImportDialog
/********************************************/

class ImportDialog : public QDialog
{
   Q_OBJECT
public:
   ImportDialog(QWidget* parent);
   
   public slots:
   void nameChanged(const QString &);
   void keyChanged(const QString &);
   void Import();
public:
signals:
   void isOk();
private:
   QString  m_userName;
   QString  m_key;
};

/********************************************/
   
}


#endif // RICHDIALOG_HPP
