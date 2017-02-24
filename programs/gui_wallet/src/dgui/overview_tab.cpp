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
#include <QStackedWidget>
#include <QRect>

using namespace gui_wallet;



Overview_tab::Overview_tab(class Mainwindow_gui_wallet* a_pPar)
    : m_pPar(a_pPar)
{


//    QLabel * cl = new QLabel();
//    int posx = ((NewButton*)table_widget.cellWidget(i + 1 , 0))->pos().x();
//    int posy = ((NewButton*)table_widget.cellWidget(i + 1 , 0))->pos().y();
//    int posrx = ((NewButton*)table_widget.cellWidget(i + 1 , 0))->pos().rx();
//    int posry = ((NewButton*)table_widget.cellWidget(i + 1 , 0))->pos().ry();
//    QPixmap image("/Users/vahe/Desktop/info_icon.png");
//    cl->setGeometry(QRect(20 , 20 , 30 , 10));
//    cl->setScaledContents(true);
//    cl->setPixmap(image);




    table_widget.setColumnCount(3);
    table_widget.setRowCount(1);

    QSize tqsTableSize = table_widget.size();
    table_widget.setColumnWidth(0,(tqsTableSize.width()*10)/100);
    table_widget.setColumnWidth(1,(tqsTableSize.width()*45)/100);
    table_widget.setColumnWidth(2,(tqsTableSize.width()*45)/100);

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

    QVBoxLayout* main = new QVBoxLayout();
    main->setContentsMargins(0, 5, 0, 0);
    search.setPlaceholderText(QString("Search"));




//    QStackedWidget * s = new QStackedWidget();
//    s->addWidget(&search);
//    s->setSizeIncrement(search.size());
//    s->setStyleSheet("background-image: url(/Users/vahe/Desktop/search.png)");

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
        table_widget.setItem(i + 1,1,new QTableWidgetItem((accounts_id[i])));
        table_widget.setItem(i + 1,2,new QTableWidgetItem((accounts_names[i])));
        //table_widget.item(i + 1,0)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        table_widget.item(i + 1,1)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        table_widget.item(i + 1,2)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);

       // ((NewButton*)table_widget.cellWidget(i + 1,0))->setScaledContents(true);
        QPixmap image("/Users/vahe/Desktop/info_icon.png");
        QPixmap image1 = image.scaled(QSize(30,30),  Qt::KeepAspectRatio);

        ((NewButton*)table_widget.cellWidget(i + 1,0))->setPixmap(image1);

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
    table_widget.setColumnWidth(0,(tqsTableSize.width()*6)/100);
    table_widget.setColumnWidth(1,(tqsTableSize.width()*47)/100);
    table_widget.setColumnWidth(2,(tqsTableSize.width()*47)/100);
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

