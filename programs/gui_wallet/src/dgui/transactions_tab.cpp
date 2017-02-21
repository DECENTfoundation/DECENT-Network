/*
 *	File      : transactions_tab.cpp
 *
 *	Created on: 21 Nov 2016
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
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

    //close button
//    push = new QPushButton("signal");
//    push->pos().setX(500);
//    push->pos().setY(500);
//    push->show();

    //signals
//    QObject::connect(push, SIGNAL(clicked()), push, SLOT(close()));
//    QObject::connect(push, SIGNAL(clicked()), this, SLOT(close()));
//    QObject::connect(push, SIGNAL(clicked()), this, SLOT(aftherSignal()));

        //copy file
    if( loadFile() )
    {
        //wallet return jstr, jstr -> on tables
        setOnGrids();
    }


    main_layout.addWidget(tablewidget);
    setLayout(&main_layout);
}

void Transactions_tab::setOnGrids()
{
#if 0
    int count = 0;
    int row = 0; //tablewidget->rowCount();
    int col = 0; //tablewidget->columnCount();
    int n = 0;
    while(true)
    {
        n++;
        col = 0;
        QString loops = 0;
        for (int i = 0; i < 20; ++i)
        {
            count++;
            loops += jstr[i];
        }
        itm = new QTableWidgetItem(tr("%1").arg(loops));
        tablewidget->setItem(row, col++, itm);

        QString loops2 = 0;
        for (int i = count; ; ++i)
        {
            if( (jstr[i + 4] >= '0' && jstr[i + 4] <= '9') && jstr[i + 5] == '-')
            {
                break;
            }
            count++;
            loops2 += jstr[i];
        }

        itm = new QTableWidgetItem(tr("%1").arg(loops2));
        tablewidget->setItem(row, col, itm);

        col = 0;
        ++row;
        if(n == 4)
            break;
        createNewRow();

//    std::cout << n << std::endl;
    }
#endif // #if 0
}

void Transactions_tab::createNewRow()
{
    tablewidget->setRowCount(tablewidget->rowCount() + 1);
}

bool Transactions_tab::loadFile()
{
#if 0
    QFile json("/Users/vahe/Desktop/myfile.txt");
    if(!json.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        mess.setText("Error::Can't read file");
        mess.exec();
        return false;
    }
    else
    {
        QTextStream qts(&json);
        jstr = qts.readAll();

        json.close();

        return true;
    }
#endif
    return true;

}

void Transactions_tab::aftherSignal()
{
    mess.setText("SIGNAL IS DONE");
    mess.exec();
}

Transactions_tab::~Transactions_tab()
{
    delete tablewidget;
    delete push;
    //delete itm; //deleting automaticly ???
}
