/*
 *	File      : overview_tab.cpp
 *
 *	Created on: 21 Nov 2016
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
#include "overview_tab.hpp"
#include "gui_wallet_mainwindow.hpp"
#include <QPixmap>

using namespace gui_wallet;



Overview_tab::Overview_tab(class Mainwindow_gui_wallet* a_pPar)
    : m_pPar(a_pPar)
{



    table_widget.setColumnCount(3);
    table_widget.setRowCount(1);

    table_widget.setItem(0,0,new QTableWidgetItem(tr("Info")));
    table_widget.setItem(0,1,new QTableWidgetItem(tr("Asset ID")));
    table_widget.setItem(0,2,new QTableWidgetItem(tr("Author")));

    table_widget.item(0,0)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
    table_widget.item(0,1)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
    table_widget.item(0,2)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);

    table_widget.item(0,0)->setBackground(Qt::lightGray);
    table_widget.item(0,1)->setBackground(Qt::lightGray);
    table_widget.item(0,2)->setBackground(Qt::lightGray);

    table_widget.horizontalHeader()->hide();
    table_widget.verticalHeader()->hide();

    table_widget.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    table_widget.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    table_widget.resize(table_widget.width(), table_widget.height());
    table_widget.horizontalHeader()->setStretchLastSection(true);
    table_widget.horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    QVBoxLayout* main = new QVBoxLayout();
    main->setContentsMargins(0, 5, 0, 0);
    search.setPlaceholderText(QString("Search"));

    main->addWidget(&search);
    main->addWidget(&table_widget);


    setLayout(main);
}

void Overview_tab::my_slot(int i)
{
    QString str = QString::fromStdString("get_account ") + accounts_names[i];
    std::string tInpuString = str.toStdString();
    SetNewTask(tInpuString,m_pPar,NULL,&Mainwindow_gui_wallet::TaskDoneOverrviewGUI);
}

void Overview_tab::CreateTable()
{
    table_widget.setRowCount(accounts_names.size() + 1);

    for(int i = 0; i < accounts_names.size(); ++i)
    {
        table_widget.setCellWidget(i + 1,0,new NewButton(i));
        table_widget.setItem(i + 1,1,new QTableWidgetItem((accounts_names[i])));
        table_widget.setItem(i + 1,2,new QTableWidgetItem((accounts_id[i])));
        table_widget.item(i + 1,1)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        table_widget.item(i + 1,2)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);

        ((NewButton*)table_widget.cellWidget(i + 1,0))->setScaledContents(true);
        QPixmap image("/Users/vahe/Desktop/aaa.png");
        ((NewButton*)table_widget.cellWidget(i + 1,0))->setPixmap(image);
    }

    Connects();
}

void Overview_tab::Connects()
{
    for(int i = 1; i < accounts_names.size() + 1; ++i)
    {
        connect(table_widget.cellWidget(i,0), SIGNAL(ButtonPushedSignal(int)), this , SLOT(my_slot(int)));
    }
}

void Overview_tab::ArrangeSize()
{
    QSize tqsTableSize = table_widget.size();
    int nSizeForOne = tqsTableSize.width()/(DCF::NUM_OF_DIG_CONT_FIELDS)-1;
    for(int i = 0; i < DCF::NUM_OF_DIG_CONT_FIELDS; ++i)
    {
        table_widget.setColumnWidth(i,nSizeForOne);
    }
}

void Overview_tab::resizeEvent(QResizeEvent *a_event)
{
    QWidget::resizeEvent(a_event);
    ArrangeSize();
}

void Overview_tab::mouseMoveEvent(QMouseEvent *a_event)
{

}


Overview_tab::~Overview_tab()
{

}

