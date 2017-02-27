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
#include <QHeaderView>
#include <QFont>

using namespace gui_wallet;

static const char* firsItemNames[]={"Time","Type","Info","Fee"};

Transactions_tab::Transactions_tab()
{
    //create table (widget)
    tablewidget = new QTableWidget();
    tablewidget->setRowCount(1);//add first row in table
    tablewidget->setColumnCount(4);
    tablewidget->verticalHeader()->setDefaultSectionSize(35);
    tablewidget->horizontalHeader()->setDefaultSectionSize(230);
    tablewidget->horizontalHeader()->hide();
    tablewidget->verticalHeader()->hide();
    tablewidget->setStyleSheet("QTableView{border : 1px solid lightGray}");

    tablewidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    tablewidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    main_layout.setContentsMargins(0, 5, 0, 0);

    user.setStyleSheet("border: 1px solid white");
    user.setPlaceholderText("Search");
    user.setMaximumHeight(40);
    user.setFixedHeight(40);

    QFont font( "Arial", 14, QFont::Bold);
    for (int i = 0; i < 4; ++i)
    {
        tablewidget->setItem(0, i, new QTableWidgetItem(tr(firsItemNames[i])));
        tablewidget->item(0, i)->setFont(font);
        tablewidget->item(0, i)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        tablewidget->item(0, i)->setBackground(QColor(228,227,228));
        tablewidget->item(0, i)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    }

    QHBoxLayout* search_lay = new QHBoxLayout();


    QPixmap image("../../../../png_files/search.png");
    QPixmap image1 = image.scaled(QSize(15, 15),  Qt::KeepAspectRatio);
    search_label.setSizeIncrement(100,40);
    search_label.setPixmap(image1);

    search_lay->addWidget(new QLabel());
    search_lay->addWidget(new QLabel());
    search_lay->addWidget(new QLabel());
    search_lay->addWidget(&search_label);
    search_lay->addWidget(&user);

    main_layout.addLayout(search_lay);
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
   for (int i = tablewidget->rowCount(); tablewidget->item(i, 0) == 0; --i)
   {
       tablewidget->removeRow(i);
   }
}

Transactions_tab::~Transactions_tab()
{
    main_layout.removeWidget(tablewidget);
    delete tablewidget;
}
