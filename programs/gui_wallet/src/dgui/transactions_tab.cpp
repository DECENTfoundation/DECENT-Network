///*
// *	File      : transactions_tab.cpp
// *
// *	Created on: 21 Nov 2016
// *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
// *
// *  This file implements ...
// *
// */
#include "transactions_tab.hpp"

using namespace gui_wallet;

Transactions_tab::Transactions_tab()
{
    //create table (widget)
    tablewidget = new QTableWidget();
    tablewidget->setRowCount(tablewidget->rowCount() + 1);//add first row in table
    tablewidget->setColumnCount(4);
    tablewidget->verticalHeader()->setDefaultSectionSize(50);
    tablewidget->horizontalHeader()->setDefaultSectionSize(200);
    tablewidget->setHorizontalHeaderLabels(QStringList() << "Date" << "Type" << "Fee" << "");

    more = new QPushButton();
    more->setText("More info");
    tablewidget->setCellWidget(0, 3, more);

    main_layout.addWidget(tablewidget);
    setLayout(&main_layout);
}

void Transactions_tab::createNewRow()
{
    static int m = 0;
    tablewidget->setRowCount(tablewidget->rowCount() + 1);

    more = new QPushButton();
    more->setText("More info");
    tablewidget->setCellWidget(++m, 3, more);
}


Transactions_tab::~Transactions_tab()
{
    delete tablewidget;
}
