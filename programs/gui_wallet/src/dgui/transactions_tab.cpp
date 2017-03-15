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
#include <QStringList>

#include <graphene/chain/config.hpp>
#include "gui_wallet_global.hpp"
#include "qt_commonheader.hpp"
#include "ui_wallet_functions.hpp"
#include "json.hpp"

using namespace gui_wallet;
using namespace nlohmann;

static const char* firsItemNames[]={"ID", "Info", "Memo", "Fee"};
const int numTransactionCols = sizeof(firsItemNames) / sizeof(const char*);



HTableWidget::HTableWidget() : QTableWidget()
{
    this->setMouseTracking(true);
}


void HTableWidget::mouseMoveEvent(QMouseEvent *event)
{
    mouseMoveEventDid();
}





Transactions_tab::Transactions_tab() : green_row(0)
{
    QFont font( "Open Sans Bold", 14, QFont::Bold);

    //create table (widget)
    tablewidget = new HTableWidget();
    //tablewidget->setRowCount(0);//add first row in table
    tablewidget->setColumnCount(numTransactionCols);
    
    tablewidget->verticalHeader()->setDefaultSectionSize(35);
    tablewidget->horizontalHeader()->setDefaultSectionSize(230);
    tablewidget->verticalHeader()->hide();
    tablewidget->setStyleSheet("QTableView{border : 1px solid lightGray}");
    tablewidget->setSelectionMode(QAbstractItemView::NoSelection);
    
    tablewidget->setHorizontalHeaderLabels(QStringList() << "ID" << "Info" << "Memo" << "Fee");
    tablewidget->horizontalHeader()->setFixedHeight(35);
    tablewidget->horizontalHeader()->setFont(font);

    tablewidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    tablewidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    user.setStyleSheet("border: 0px solid white");
    user.setPlaceholderText("Enter user name to see transaction history");
    user.setAttribute(Qt::WA_MacShowFocusRect, 0);
    user.setMaximumHeight(40);
    user.setFixedHeight(40);
    user.setFrame(false);
    

//    for (int i = 0; i < numTransactionCols; ++i)
//    {
//        tablewidget->setItem(0, i, new QTableWidgetItem(tr(firsItemNames[i])));
//        tablewidget->item(0, i)->setFont(f);
//        tablewidget->item(0, i)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
//        tablewidget->item(0, i)->setBackground(QColor(228,227,228));
//        tablewidget->item(0, i)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
//        tablewidget->item(0, i)->setForeground(QColor::fromRgb(51,51,51));
//    }

    connect(tablewidget,SIGNAL(mouseMoveEventDid()),this,SLOT(doRowColor()));

    QHBoxLayout* search_lay = new QHBoxLayout();
    QPixmap image(":/icon/images/search.svg");
    search_label.setSizeIncrement(100,40);
    search_label.setPixmap(image);

    search_lay->addWidget(new QLabel());
    search_lay->addWidget(new QLabel());
    search_lay->addWidget(new QLabel());
    search_lay->addWidget(&search_label);
    search_lay->addWidget(&user);

    tablewidget->horizontalHeader()->setStretchLastSection(true);
    main_layout.setContentsMargins(0, 0, 0, 0);
    main_layout.addLayout(search_lay);
    main_layout.addWidget(tablewidget);
    setLayout(&main_layout);
    
    
    connect(&user, SIGNAL(textChanged(QString)), this, SLOT(onTextChanged(QString)));

    m_contentUpdateTimer.connect(&m_contentUpdateTimer, SIGNAL(timeout()), this, SLOT(maybeUpdateContent()));
    m_contentUpdateTimer.setInterval(1000);
    m_contentUpdateTimer.start();
    Connects();
}






void Transactions_tab::maybeUpdateContent() {
    if (!m_doUpdate) {
        return;
    }
    
    m_doUpdate = false;
    updateContents();
}

void Transactions_tab::onTextChanged(const QString& text) {
    
    m_doUpdate = true;
}


void Transactions_tab::createNewRow()
{
    tablewidget->setRowCount(100);
}

void Transactions_tab::ArrangeSize()
{
  QSize tqsTableSize = tablewidget->size();
  tablewidget->setColumnWidth(0,(tqsTableSize.width()*10)/100);
  tablewidget->setColumnWidth(1,(tqsTableSize.width()*40)/100);
  tablewidget->setColumnWidth(2,(tqsTableSize.width()*40)/100);
  tablewidget->setColumnWidth(3,(tqsTableSize.width()*10)/100);
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
    if(tablewidget->rowCount() < 1) {return;}
    if(tablewidget->item(green_row,0) != NULL)
    {
        tablewidget->item(green_row,0)->setBackgroundColor(QColor(255,255,255));
        tablewidget->item(green_row,0)->setForeground(QColor::fromRgb(0,0,0));

    }
    if(tablewidget->item(green_row,1) != NULL)
    {
        tablewidget->item(green_row,1)->setBackgroundColor(QColor(255,255,255));
        tablewidget->item(green_row,1)->setForeground(QColor::fromRgb(0,0,0));
        
    }
    if(tablewidget->item(green_row,2) != NULL)
    {
        tablewidget->item(green_row,2)->setBackgroundColor(QColor(255,255,255));
        tablewidget->item(green_row,2)->setForeground(QColor::fromRgb(0,0,0));
        
    }
    if(tablewidget->item(green_row,3) != NULL)
    {
        tablewidget->item(green_row,3)->setBackgroundColor(QColor(255,255,255));
        tablewidget->item(green_row,3)->setForeground(QColor::fromRgb(0,0,0));
        
    }

    QPoint mouse_pos = tablewidget->mapFromGlobal(QCursor::pos());
    mouse_pos.setY(mouse_pos.y() - 41);
    QTableWidgetItem *ite = tablewidget->itemAt(mouse_pos);
    if(ite != NULL)
    {

        int row = ite->row();
        if(row < 0) {return;}
        if(tablewidget->item(row,0) != NULL)
        {
            tablewidget->item(row,0)->setBackgroundColor(QColor(27,176,104));
            tablewidget->item(row,0)->setForeground(QColor::fromRgb(255,255,255));
        }
        if(tablewidget->item(row,1) != NULL)
        {
            tablewidget->item(row,1)->setBackgroundColor(QColor(27,176,104));
            tablewidget->item(row,1)->setForeground(QColor::fromRgb(255,255,255));
        }
        if(tablewidget->item(row,2) != NULL)
        {
            tablewidget->item(row,2)->setBackgroundColor(QColor(27,176,104));
            tablewidget->item(row,2)->setForeground(QColor::fromRgb(255,255,255));
        }
        if(tablewidget->item(row,3) != NULL)
        {
            tablewidget->item(row,3)->setBackgroundColor(QColor(27,176,104));
            tablewidget->item(row,3)->setForeground(QColor::fromRgb(255,255,255));
        }
            green_row = row;
    }
    else
    {
        green_row = 0;
    }
}

void Transactions_tab::Connects()
{
    connect(tablewidget,SIGNAL(mouseMoveEventDid()),this,SLOT(doRowColor()));
}





void Transactions_tab::updateContents() {
    if (user.text().toStdString().empty()) {
        return;
    }
    
    //tablewidget->setRowCount(1); //Remove everything but header

    

    
    SetNewTask("get_account_history \"" + user.text().toStdString() +"\" 100", this, NULL, +[](void* owner, void* a_clbkArg, int64_t a_err, const std::string& a_task, const std::string& a_result) {
        
        if (a_err != 0) {
            return;
        }
        
        Transactions_tab* obj = (Transactions_tab*)owner;
        
        try {
            auto contents = json::parse(a_result);
            
            obj->tablewidget->setRowCount(contents.size());
            for (int i = 0; i < contents.size(); ++i) {
                auto content = contents[i];
                auto op = content["op"];

                //ID
                obj->tablewidget->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(op["id"].get<std::string>())));
                obj->tablewidget->item(i, 0)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
                obj->tablewidget->item(i, 0)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
                
                //description
                std::string cont = contents[i]["description"].get<std::string>();
                std::string desc = "";
                for(int i = 0; i < cont.size(); ++i)
                {
                    if((cont[i + 1] == 'F' || cont[i + 1] == 'f') && cont[i + 2] == 'e' && cont[i + 3] == 'e')   { break; }
                    
                    desc += cont[i];
                }
                obj->tablewidget->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(desc)));
                obj->tablewidget->item(i, 1)->setTextAlignment(Qt::AlignLeft|Qt::AlignVCenter);
                obj->tablewidget->item(i, 1)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
                
                //memo
                obj->tablewidget->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(contents[i]["memo"].get<std::string>())));
                obj->tablewidget->item(i, 2)->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
                obj->tablewidget->item(i, 2)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);

                auto amount = op["op"][1]["fee"]["amount"];
                QString amountText = "";
                
                if (amount.is_number()){
                    amountText = QString::number(amount.get<double>() / GRAPHENE_BLOCKCHAIN_PRECISION) + " DCT";
                } else {
                    amountText = QString::number(std::stod(amount.get<std::string>()) / GRAPHENE_BLOCKCHAIN_PRECISION) + " DCT";

                }
                obj->tablewidget->setItem(i, 3, new QTableWidgetItem(amountText));
                obj->tablewidget->item(i, 3)->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
                obj->tablewidget->item(i, 3)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);

            }
        } catch (std::exception& ex) {
        }
    });
    Connects();
}

void Transactions_tab::SetInfo(std::string info_from_overview)
{
    user.setText(QString::fromStdString(info_from_overview));
}


