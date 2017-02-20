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
#include <QIODevice>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QApplication>
#include <QVBoxLayout>
#include <QString>
#include <QMessageBox>
#include <QFile>
#include <QHeaderView>
#include <QTextStream>
#include <QLabel>
#include <QPushButton>

namespace gui_wallet
{
    class Transactions_tab : public QWidget
    {
        Q_OBJECT
    public:
        Transactions_tab();
        ~Transactions_tab();

    private:
        QVBoxLayout main_layout;
        QTableWidget* tablewidget;
        QTableWidgetItem* itm;
        QPushButton* push;
        QMessageBox mess;
        QString jstr;
        bool loadFile();
        void setOnGrids();
        void createNewRow();

    private slots:
        void aftherSignal();

    };
}


#endif // TRANSACTIONS_TAB_HPP
