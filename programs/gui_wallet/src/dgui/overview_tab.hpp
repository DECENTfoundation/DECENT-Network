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
#include <QTimer>
#include <QStringList>
#include <stdio.h>
#include <stdarg.h>
#include <iostream>
#include <QFont>

#include "gui_wallet_tabcontentmanager.hpp"


class QZebraWidget : public QWidget
{
public:
   QZebraWidget()
   {
       m_main_layout.setSpacing(0);
       m_main_layout.setContentsMargins(0, 0, 0, 0);

       setStyleSheet("background-color:white;");
       setLayout(&m_main_layout);
    }
    
    void AddInfo(std::string title, std::string info) {
        _subWidgets.push_back(new QWidget());
        _subLayouts.push_back(new QVBoxLayout());
        
        if (_subWidgets.size() % 2 == 0) {
            _subWidgets.back()->setStyleSheet("background-color:rgb(244,244,244);");
        } else {
            _subWidgets.back()->setStyleSheet("background-color:rgb(255, 255, 255);");
        }
        
        QLabel* lblTitle = new QLabel(QString::fromStdString(title));
        lblTitle->setStyleSheet("font-weight: bold");
        
        QLabel* lblInfo = new QLabel(QString::fromStdString(info));
        
        _subLayouts.back()->setSpacing(0);
        _subLayouts.back()->setContentsMargins(45,3,0,3);
        _subLayouts.back()->addWidget(lblTitle);
        _subLayouts.back()->addWidget(lblInfo);
        
        _subWidgets.back()->setLayout(_subLayouts.back());
        m_main_layout.addWidget(_subWidgets.back());
        
        
    }
private:
   QVBoxLayout     m_main_layout;
    
    std::vector<QWidget*>     _subWidgets;
    std::vector<QVBoxLayout*> _subLayouts;
    
};



class NewButton : public QLabel
{
   Q_OBJECT
public:
    NewButton(std::string id) : m_id(id){
       connect(this,SIGNAL(LabelWasClicked()),this,SLOT(ButtonPushedSlot()));
   }
private:
   std::string m_id;
    
private slots:
   void ButtonPushedSlot(){emit ButtonPushedSignal(m_id);}
    
private:
signals:
   void LabelWasClicked();
    
public:
signals:
   void ButtonPushedSignal(std::string);
   void mouseWasMoved();
    
public:
   virtual void mouseReleaseEvent(QMouseEvent * event)
   {
        LabelWasClicked();
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
        void updateContents();

        void maybeUpdateContent();
        void onTextChanged(const QString& text);

        void buttonPressed(std::string accountName);
        void doRowColor();
    public:
        QLineEdit search;
        TableWidget table_widget;
        QLabel search_label;

        std::map<std::string , std::string> info;
        int button_number;
    protected:
        class Mainwindow_gui_wallet* m_pPar;
        virtual void resizeEvent ( QResizeEvent * a_event );
        
    private:
        QTimer  m_contentUpdateTimer;
        bool m_doUpdate = true;
    };
}



#endif // OVERVIEW_TAB_HPP
