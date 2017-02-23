/*
 *	File      : overview_tab.hpp
 *
 *	Created on: 21 Nov 2016
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
#ifndef OVERVIEW_TAB_HPP
#define OVERVIEW_TAB_HPP

#include <QWidget>
#include <QPushButton>
#include <QTextBrowser>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <vector>
#include <QTableWidget>
#include <map>
#include <QStringList>
#include <stdio.h>
#include <stdarg.h>


//class NewButton : public QPushButton
//{
//   Q_OBJECT
//public:
//   NewButton(int id) : m_id(id){connect(this,SIGNAL(clicked()),this,SLOT(ButtonPushedSlot()));}
//private:
//   int m_id;
//private slots:
//   void ButtonPushedSlot(){emit ButtonPushedSignal(m_id);}
//public:
//signals:
//   void ButtonPushedSignal(int);
//};

class NewButton : public QLabel
{
   Q_OBJECT
public:
   NewButton(int id) : m_id(id){connect(this,SIGNAL(LabelWosClicked()),this,SLOT(ButtonPushedSlot()));}
private:
   int m_id;
private slots:
   void ButtonPushedSlot(){emit ButtonPushedSignal(m_id);}
private:
signals:
   void LabelWosClicked();
public:
signals:
   void ButtonPushedSignal(int);
private:
   virtual void mouseReleaseEvent(QMouseEvent * event)
   {
        LabelWosClicked();
   }
};

namespace gui_wallet
{
    class Overview_tab : public QWidget
    {
        Q_OBJECT
    public:
        Overview_tab(class Mainwindow_gui_wallet* pPar);
        virtual ~Overview_tab();
        void CreateTable();
        void Connects();
        void ArrangeSize();
    public slots:
        void my_slot(int);
    public:
        QLineEdit search;
        QTableWidget table_widget;
        std::vector<QString> accounts_names;
        std::vector<QString> accounts_id;
        std::map<std::string , std::string> info;
        int button_number;
    protected:
        class Mainwindow_gui_wallet* m_pPar;
        virtual void resizeEvent ( QResizeEvent * a_event );
        virtual void mouseMoveEvent(QMouseEvent *);
    };
}


#endif // OVERVIEW_TAB_HPP
