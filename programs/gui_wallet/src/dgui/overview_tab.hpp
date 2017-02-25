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
#include <iostream>


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


class QZebraWidget : public QWidget
{
public:
   QZebraWidget(std::vector<std::string> a_info)
   {
       m_main_layout.setMargin(0);
       m_main_layout.setContentsMargins(0,0,0,0);
       QWidget* pNextWidget;

       pNextWidget = new QWidget;
       pNextWidget->setStyleSheet("background-color:white;");
       QVBoxLayout* h_leyout = new QVBoxLayout();

       QLabel* inf = new QLabel();

       inf->setText(QString::fromStdString(a_info[0]));

       h_leyout->addWidget(inf);

       pNextWidget->setLayout(h_leyout);
       m_main_layout.addWidget(pNextWidget);




       pNextWidget = new QWidget;
       pNextWidget->setStyleSheet("background-color:lightGray;");
       QVBoxLayout* h_leyout1 = new QVBoxLayout();

       QLabel* inf1 = new QLabel();

       inf1->setText(QString::fromStdString(a_info[1]));

       h_leyout1->addWidget(inf1);

       pNextWidget->setLayout(h_leyout1);
       m_main_layout.addWidget(pNextWidget);





       pNextWidget = new QWidget;
       pNextWidget->setStyleSheet("background-color:white;");
       QVBoxLayout* h_leyout2 = new QVBoxLayout();

       QLabel* inf2 = new QLabel();

       inf2->setText(QString::fromStdString(a_info[2]));

       h_leyout2->addWidget(inf2);

       pNextWidget->setLayout(h_leyout2);
       m_main_layout.addWidget(pNextWidget);





       pNextWidget = new QWidget;
       pNextWidget->setStyleSheet("background-color:lightGray;");
       QVBoxLayout* h_leyout3 = new QVBoxLayout();

       QLabel* inf3 = new QLabel();

       inf3->setText(QString::fromStdString(a_info[3]));

       h_leyout3->addWidget(inf3);

       pNextWidget->setLayout(h_leyout3);
       m_main_layout.addWidget(pNextWidget);





       pNextWidget = new QWidget;
       pNextWidget->setStyleSheet("background-color:white;");
       QVBoxLayout* h_leyout4 = new QVBoxLayout();

       QLabel* inf4 = new QLabel();

       inf4->setText(QString::fromStdString(a_info[4]));

       h_leyout4->addWidget(inf4);

       pNextWidget->setLayout(h_leyout4);
       m_main_layout.addWidget(pNextWidget);





       pNextWidget = new QWidget;
       pNextWidget->setStyleSheet("background-color:lightGray;");
       QVBoxLayout* h_leyout5 = new QVBoxLayout();

       QLabel* inf5 = new QLabel();

       inf5->setText(QString::fromStdString(a_info[5]));

       h_leyout5->addWidget(inf5);

       pNextWidget->setLayout(h_leyout5);
       m_main_layout.addWidget(pNextWidget);




       pNextWidget = new QWidget;
       pNextWidget->setStyleSheet("background-color:white;");
       QVBoxLayout* h_leyout6 = new QVBoxLayout();

       QLabel* inf6 = new QLabel();

       inf6->setText(QString::fromStdString(a_info[6]));

       h_leyout6->addWidget(inf6);

       pNextWidget->setLayout(h_leyout6);
       m_main_layout.addWidget(pNextWidget);





       pNextWidget = new QWidget;
       pNextWidget->setStyleSheet("background-color:lightGray;");
       QVBoxLayout* h_leyout7 = new QVBoxLayout();

       QLabel* inf7 = new QLabel();

       inf7->setText(QString::fromStdString(a_info[7]));

       h_leyout7->addWidget(inf7);

       pNextWidget->setLayout(h_leyout7);
       m_main_layout.addWidget(pNextWidget);



        setLayout(&m_main_layout);

       resize(300,300);
   }


private:
   QVBoxLayout m_main_layout;
};



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
   void mouseWasMoved();
public:
   virtual void mouseReleaseEvent(QMouseEvent * event)
   {
        LabelWosClicked();
   }

   virtual void mouseMoveEvent(QMouseEvent * event)
   {
       printf("%s\n",__FUNCTION__);
        emit mouseWasMoved();
        QLabel::mouseMoveEvent(event);
   }
};



class TableWidget : public QTableWidget
{
    Q_OBJECT
public:
    TableWidget();

    virtual void mouseMoveEvent(QMouseEvent * event);
public:
signals:
    void mouseMoveEventDid();
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
        void doRowColor();
    public:
        QLineEdit search;
        TableWidget table_widget;
        QLabel search_label;
        std::vector<QString> accounts_names;
        std::vector<QString> accounts_id;
        std::map<std::string , std::string> info;
        int button_number;
    protected:
        class Mainwindow_gui_wallet* m_pPar;
        virtual void resizeEvent ( QResizeEvent * a_event );
       // virtual void mouseMoveEvent(QMouseEvent *);
    };
}



#endif // OVERVIEW_TAB_HPP
