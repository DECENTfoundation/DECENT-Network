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

using namespace gui_wallet;



Overview_tab::Overview_tab()
{
    QHBoxLayout* up = new QHBoxLayout();
    up->addWidget(&search);

    QVBoxLayout* main = new QVBoxLayout();
    main->addLayout(up);
    main->addWidget(&text);

    table_widget.setColumnCount(2);
    table_widget.setRowCount(0);
    table_widget.setHorizontalHeaderLabels(QStringList() << "waiting for design" << " ");
    for(int i = 0; i < accounts_names.size(); ++i)
    {
        connect(table_widget.cellWidget(i,1), SIGNAL(clicked()), this , SLOT(func(i)));
    }

    main->addWidget(&table_widget);
    setLayout(main);
}

void Overview_tab::func(int i)
{
    QString str = QString::fromStdString("get_account ") + accounts_names[i];
    std::string tInpuString = str.toStdString();
    SetNewTask(tInpuString,this,NULL,&Mainwindow_gui_wallet::TaskDoneOverrviewGUI);
}

void Overview_tab::CreateTable()
{
    table_widget.setRowCount(accounts_names.size());
    for(int i = 0; i < accounts_names.size(); ++i)
    {
        table_widget.setItem(i,0,new QTableWidgetItem((accounts_names[i])));
        table_widget.setCellWidget(i,1,new QPushButton("info"));
    }
}

Overview_tab::~Overview_tab()
{

}

