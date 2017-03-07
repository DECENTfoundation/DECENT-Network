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
#include <QFont>

#include "gui_wallet_tabcontentmanager.hpp"

#define NUMBER_OF_SUB_LAYOUTS   6

class QZebraWidget : public QWidget
{
public:
   QZebraWidget(std::vector<std::string> a_info)
   {
       int i, nIndexKent(1), nIndexZuyg(0);

       m_main_layout.setSpacing(0);
       m_main_layout.setContentsMargins(0,0,0,0);


       for(i=1;i<=NUMBER_OF_SUB_LAYOUTS;++i,nIndexZuyg+=2,nIndexKent+=2)
       {
           std::string name;
           std::string value;
           if(i%2==0){m_vSub_Widgets[i].setStyleSheet("background-color:rgb(244,244,244);");}
           else{m_vSub_Widgets[i].setStyleSheet("background-color:white;");}
           m_vLabels[nIndexZuyg].setStyleSheet("font-weight: bold");
           int slash_position = a_info[i - 1].find("\n");
           for(int j = 0; j < slash_position; ++j)
           {
               name.push_back(a_info[i - 1][j]);
           }
           for(int j = slash_position + 1; j < a_info[i - 1].size(); ++j)
           {
               value.push_back(a_info[i - 1][j]);
           }
           //value = std::to_string(slash_position);
           m_vLabels[nIndexZuyg].setText(QString::fromStdString(name));
           m_vLabels[nIndexKent].setText(QString::fromStdString(value));
           m_vSub_layouts[i].setSpacing(0);
           m_vSub_layouts[i].setContentsMargins(45,3,0,3);
           m_vSub_layouts[i].addWidget(&m_vLabels[nIndexZuyg]);
           m_vSub_layouts[i].addWidget(&m_vLabels[nIndexKent]);
           m_vSub_Widgets[i].setLayout(&m_vSub_layouts[i]);
           m_main_layout.addWidget(&m_vSub_Widgets[i]);
       }
       setLayout(&m_main_layout);
       setStyleSheet("background-color:white;");
    }

private:
   QVBoxLayout     m_main_layout;
   QHBoxLayout     m_free_for_child;
   QWidget         m_vSub_Widgets[NUMBER_OF_SUB_LAYOUTS + 1];
   QVBoxLayout     m_vSub_layouts[NUMBER_OF_SUB_LAYOUTS + 1];
   QLabel          m_vLabels[NUMBER_OF_SUB_LAYOUTS*2 + 1];
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
    class Overview_tab : public TabContentManager
    {
        Q_OBJECT
    public:
        Overview_tab(class Mainwindow_gui_wallet* pPar);
        virtual ~Overview_tab();
        void CreateTable();
        void Connects();
        void ArrangeSize();
        
    public:
        virtual void content_activated() {}
        virtual void content_deactivated() {}

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
    };
}



#endif // OVERVIEW_TAB_HPP
