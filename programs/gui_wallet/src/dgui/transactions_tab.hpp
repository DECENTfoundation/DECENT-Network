/*
 *	File      : transactions_tab.hpp
 *
 *	Created on: 21 Nov 2016
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
#ifndef TRANSACTIONS_TAB_HPP
#define TRANSACTIONS_TAB_HPP

#include <QWidget>
#include <iostream>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QString>
#include <QHeaderView>
#include <QTextStream>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QFrame>
#include <Qt>
#include <QColor>
#include <QMouseEvent>
#include <QTimer>

#include "gui_wallet_tabcontentmanager.hpp"



class HTableWidget : public QTableWidget
{
    Q_OBJECT
public:
    HTableWidget();

    virtual void mouseMoveEvent(QMouseEvent * event);
public:
signals:
    void mouseMoveEventDid();
};




namespace gui_wallet
{
    class Transactions_tab : public TabContentManager
    {

        Q_OBJECT
    public:
        Transactions_tab();
        ~Transactions_tab();

        QVBoxLayout main_layout;
        QLabel search_label;
        HTableWidget* tablewidget;
        QTableWidgetItem* itm;
        QPushButton* more;
        QLineEdit user;
        int green_row;
        void createNewRow();

        void ArrangeSize();
        void Connects();
        
    public:
        virtual void content_activated() {}
        virtual void content_deactivated() {}
        virtual void resizeEvent(QResizeEvent *a_event);
    private:
        std::string getAccountName(std::string accountId);

    public slots:
        void doRowColor();
        void onTextChanged(const QString& text);
        void updateContents();
        void maybeUpdateContent();

    private:
        QTimer  m_contentUpdateTimer;
        bool m_doUpdate = true;
        
        std::map<std::string, std::string> _userIdCache;
    };
}


#endif // TRANSACTIONS_TAB_HPP
