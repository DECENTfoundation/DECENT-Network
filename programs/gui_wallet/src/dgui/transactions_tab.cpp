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
#include <QTableWidgetItem>

#include <iostream>
#include <QResource>

using namespace gui_wallet;

static const char* firsItemNames[]={"Time","Type","Info","Fee"};

Transactions_tab::Transactions_tab() : green_row(0)
{
    connect(tablewidget,SIGNAL(mouseMoveEventDid()),this,SLOT(doRowColor()));

    //create table (widget)
    tablewidget = new HTableWidget();
    tablewidget->setRowCount(1);//add first row in table
    tablewidget->setColumnCount(4);
    tablewidget->verticalHeader()->setDefaultSectionSize(35);
    tablewidget->horizontalHeader()->setDefaultSectionSize(230);
    tablewidget->horizontalHeader()->hide();
    tablewidget->verticalHeader()->hide();
    tablewidget->setStyleSheet("QTableView{border : 1px solid lightGray}");
    //tablewidget->setShowGrid(false);

    tablewidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    tablewidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    user.setStyleSheet("border: 0px solid white");
    user.setPlaceholderText("Search");
    user.setMaximumHeight(40);
    user.setFixedHeight(40);

    QFont f( "Open Sans Bold", 10, QFont::Bold);
    for (int i = 0; i < 4; ++i)
    {
        tablewidget->setItem(0, i, new QTableWidgetItem(tr(firsItemNames[i])));
        tablewidget->item(0, i)->setFont(f);
        tablewidget->item(0, i)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        tablewidget->item(0, i)->setBackground(QColor(228,227,228));
        tablewidget->item(0, i)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        tablewidget->item(0, i)->setForeground(QColor::fromRgb(51,51,51));
    }

    QResource icon;
    icon.registerResource("/Users/vahe/dev/DECENTfoundation/DECENT-Network/programs/gui_wallet/resource.rcc");

    QHBoxLayout* search_lay = new QHBoxLayout();
    QPixmap image(":/icon/search.svg");
    search_label.setSizeIncrement(100,40);
    search_label.setPixmap(image);

    search_lay->addWidget(new QLabel());
    search_lay->addWidget(new QLabel());
    search_lay->addWidget(new QLabel());
    search_lay->addWidget(&search_label);
    search_lay->addWidget(&user);

    main_layout.setContentsMargins(0, 0, 0, 0);
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

void Transactions_tab::doRowColor()
{
    if(green_row != 0)
    {
        tablewidget->item(green_row,0)->setBackgroundColor(QColor(255,255,255));
        tablewidget->item(green_row,1)->setBackgroundColor(QColor(255,255,255));
        tablewidget->item(green_row,2)->setBackgroundColor(QColor(255,255,255));
        tablewidget->item(green_row,3)->setBackgroundColor(QColor(255,255,255));

        tablewidget->item(green_row,0)->setForeground(QColor::fromRgb(0,0,0));
        tablewidget->item(green_row,1)->setForeground(QColor::fromRgb(0,0,0));
        tablewidget->item(green_row,2)->setForeground(QColor::fromRgb(0,0,0));
        tablewidget->item(green_row,3)->setForeground(QColor::fromRgb(0,0,0));
    }
    QPoint mouse_pos = tablewidget->mapFromGlobal(QCursor::pos());
    QTableWidgetItem *ite = tablewidget->itemAt(mouse_pos);

    if(ite != NULL)
    {

        int a = ite->row();
        if(a != 0)
        {
            tablewidget->item(a,0)->setBackgroundColor(QColor(27,176,104));
            tablewidget->item(a,1)->setBackgroundColor(QColor(27,176,104));
            tablewidget->item(a,2)->setBackgroundColor(QColor(27,176,104));
            tablewidget->item(a,3)->setBackgroundColor(QColor(27,176,104));

            tablewidget->item(a,0)->setForeground(QColor::fromRgb(255,255,255));
            tablewidget->item(a,1)->setForeground(QColor::fromRgb(255,255,255));
            tablewidget->item(a,2)->setForeground(QColor::fromRgb(255,255,255));
            tablewidget->item(a,3)->setForeground(QColor::fromRgb(255,255,255));
            green_row = a;
        }
    }
    else
    {
        green_row == 0;
    }
}

void Transactions_tab::Connects()
{
    connect(tablewidget,SIGNAL(mouseMoveEventDid()),this,SLOT(doRowColor()));
}


HTableWidget::HTableWidget() : QTableWidget()
{
    this->setMouseTracking(true);
}


void HTableWidget::mouseMoveEvent(QMouseEvent *event)
{
    mouseMoveEventDid();
}


