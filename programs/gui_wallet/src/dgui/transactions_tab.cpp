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
    tablewidget->setColumnCount(2);
    tablewidget->verticalHeader()->setDefaultSectionSize(30);
    tablewidget->horizontalHeader()->setDefaultSectionSize(400);
    tablewidget->setHorizontalHeaderLabels(QStringList() << "Date" << "Info");

    main_layout.addWidget(tablewidget);
    setLayout(&main_layout);
}

void Transactions_tab::createNewRow()
{
    tablewidget->setRowCount(tablewidget->rowCount() + 1);
}


Transactions_tab::~Transactions_tab()
{
    delete tablewidget;
}
