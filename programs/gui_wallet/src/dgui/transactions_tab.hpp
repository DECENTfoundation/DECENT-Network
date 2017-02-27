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



namespace gui_wallet
{
    class Transactions_tab : public QWidget
    {

        Q_OBJECT
    public:
        Transactions_tab();
        ~Transactions_tab();

        QVBoxLayout main_layout;
        QLabel search_label;
        QTableWidget* tablewidget;
        QTableWidgetItem* itm;
        QPushButton* more;
        QLineEdit user;
        void createNewRow(const int);
        void deleteEmptyRows();
        void ArrangeSize();
virtual void resizeEvent(QResizeEvent *a_event);

    };
}


#endif // TRANSACTIONS_TAB_HPP
