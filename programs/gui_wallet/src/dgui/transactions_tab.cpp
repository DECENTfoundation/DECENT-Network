//*
// *	File      : transactions_tab.cpp
// *
// *	Created on: 21 Nov 2016
// *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
// *
// *  This file implements ...
// *
// */
#include "transactions_tab.hpp"
#include <QFrame>
#include <Qt>

using namespace gui_wallet;

static const char* firsItemNames[]={"Time","Type","Info","Fee"};

Transactions_tab::Transactions_tab()
{
    //create table (widget)
    tablewidget = new QTableWidget();
    tablewidget->setRowCount(1);//add first row in table
    tablewidget->setColumnCount(4);
    tablewidget->verticalHeader()->setDefaultSectionSize(34);
    tablewidget->horizontalHeader()->setDefaultSectionSize(220);
    tablewidget->horizontalHeader()->hide();
    tablewidget->verticalHeader()->hide();
   // tablewidget->setFrameStyle(0);
    tablewidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    tablewidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    main_layout.setContentsMargins(0, 5, 0, 0);

    user.setPlaceholderText("Search");

    for (int i = 0; i < 4; ++i)
    {
        tablewidget->setItem(0, i, new QTableWidgetItem(tr(firsItemNames[i])));
        tablewidget->item(0, i)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        tablewidget->item(0, i)->setBackground(Qt::lightGray);
        tablewidget->item(0, i)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    }

    main_layout.addWidget(&user);
    main_layout.addWidget(tablewidget);
    setLayout(&main_layout);
}

void Transactions_tab::createNewRow(const int str)
{
    int count = str/50;
    tablewidget->setRowCount(count);
}

void Transactions_tab::ArrangeSize()
{
  QSize tqsTableSize = tablewidget->size();
  tablewidget->setColumnWidth(0,(tqsTableSize.width()*25)/100);
  tablewidget->setColumnWidth(1,(tqsTableSize.width()*25)/100);
  tablewidget->setColumnWidth(2,(tqsTableSize.width()*25)/100);
  tablewidget->setColumnWidth(3,(tqsTableSize.width()*25)/100);
}

void Transactions_tab::resizeEvent(QResizeEvent *a_event)
{
  QWidget::resizeEvent(a_event);
  ArrangeSize();
}

void Transactions_tab::deleteEmptyRows()
{
   for (int i = tablewidget->rowCount(); tablewidget->item(i, 0) == 0; --i) // !!!!!!!!!!!!!!!!!add
   {
       tablewidget->removeRow(i);
   }
}


Transactions_tab::~Transactions_tab()
{
    main_layout.removeWidget(tablewidget);
    delete tablewidget;
}
