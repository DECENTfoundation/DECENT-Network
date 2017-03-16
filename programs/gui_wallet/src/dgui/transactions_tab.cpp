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

static const char* firsItemNames[]={"Type", "From", "To", "Amount", "Fee", "Description"};
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
    QFont f( "Open Sans Bold", 14, QFont::Bold);

    //create table (widget)
    tablewidget = new HTableWidget();
    tablewidget->setRowCount(1);//add first row in table
    tablewidget->setColumnCount(numTransactionCols);
    
    tablewidget->verticalHeader()->setDefaultSectionSize(35);
    tablewidget->horizontalHeader()->setDefaultSectionSize(230);
    tablewidget->verticalHeader()->hide();
    tablewidget->horizontalHeader()->hide();

    tablewidget->setStyleSheet("QTableView{border : 1px solid lightGray}");
    tablewidget->setSelectionMode(QAbstractItemView::NoSelection);
    
    tablewidget->setHorizontalHeaderLabels(QStringList() << "ID" << "Info" << "Memo" << "Fee");
    tablewidget->horizontalHeader()->setFixedHeight(35);
    tablewidget->horizontalHeader()->setFont(f);

    tablewidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    tablewidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    user.setStyleSheet("border: 0px solid white");
    user.setPlaceholderText("Enter user name to see transaction history");
    user.setAttribute(Qt::WA_MacShowFocusRect, 0);
    user.setMaximumHeight(40);
    user.setFixedHeight(40);
    user.setFrame(false);

    for (int i = 0; i < numTransactionCols; ++i)
    {
        tablewidget->setItem(0, i, new QTableWidgetItem(tr(firsItemNames[i])));
        tablewidget->item(0, i)->setFont(f);
        tablewidget->item(0, i)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        tablewidget->item(0, i)->setBackground(QColor(228,227,228));
        tablewidget->item(0, i)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        tablewidget->item(0, i)->setForeground(QColor::fromRgb(51,51,51));
    }

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
    std::vector<int> sizes = {10, 20, 20, 10, 10, 30};
    for (int i = 0; i < numTransactionCols; ++i) {
        tablewidget->setColumnWidth(i,(tqsTableSize.width()* sizes[i])/100);
    }
    
}

void Transactions_tab::resizeEvent(QResizeEvent *a_event)
{
  QWidget::resizeEvent(a_event);
  ArrangeSize();
}


Transactions_tab::~Transactions_tab()
{
    main_layout.removeWidget(tablewidget);
    delete tablewidget;
}

void Transactions_tab::doRowColor()
{
    if(green_row > 0)
    {
        for (int i = 0; i < numTransactionCols; ++i) {
            tablewidget->item(green_row,i)->setBackgroundColor(QColor(255,255,255));
            tablewidget->item(green_row,i)->setForeground(QColor::fromRgb(0,0,0));
        }
    }
    QPoint mouse_pos = tablewidget->mapFromGlobal(QCursor::pos());
    QTableWidgetItem *ite = tablewidget->itemAt(mouse_pos);
    
    if(ite != NULL)
    {
        
        int a = ite->row();
        if(a != 0) {
            for (int i = 0; i < numTransactionCols; ++i) {
                tablewidget->item(a, i)->setBackgroundColor(QColor(27,176,104));
                tablewidget->item(a, i)->setForeground(QColor::fromRgb(255,255,255));
            }
            
            green_row = a;
        }
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



std::string Transactions_tab::getAccountName(std::string accountId) {
    
    auto seach = _userIdCache.find(accountId);
    if (seach == _userIdCache.end()) {
        std::string accountInfo, accountName = "Unknown";
        RunTask("get_object \"" + accountId + "\"", accountInfo);
        
        auto accountInfoParsed = json::parse(accountInfo);
        if (accountInfoParsed.size() != 0) {
            accountName = accountInfoParsed[0]["name"].get<std::string>();
        }
        _userIdCache.insert(std::make_pair(accountId, accountName));
    }
    
    return _userIdCache[accountId];
}

void Transactions_tab::updateContents() {
    tablewidget->setRowCount(1); //Remove everything but header

    
    if (user.text().toStdString().empty()) {
        return;
    }
    
    std::string a_result;
    try {
        RunTask("get_account_history \"" + user.text().toStdString() +"\" 100", a_result);
    } catch (const std::exception& ex) {
        return;
    }
    
    
    try {
        auto contents = json::parse(a_result);
        
        tablewidget->setRowCount(contents.size() + 1);
        
        for (int i = 0; i < contents.size(); ++i) {
            auto content = contents[i];
            std::string from_account = getAccountName(content["from_account"].get<std::string>());
            std::string to_account = getAccountName(content["to_account"].get<std::string>());
            std::string operation_type = content["operation_type"].get<std::string>();
            std::string description = content["description"].get<std::string>();
            
            if (operation_type == "Buy" || operation_type == "Content submit") {
                std::string contentStr;
                RunTask("get_content \"" + description + "\"", contentStr);
                auto contentObject = json::parse(contentStr);
                
                std::string synopsis = contentObject["synopsis"].get<std::string>();
                from_account = getAccountName(contentObject["author"].get<std::string>());
                try {
                    auto synopsis_parsed = json::parse(synopsis);
                    synopsis = synopsis_parsed["title"].get<std::string>();
                } catch (...) {}
                
                description = synopsis;
            }
            
            auto transaction_amount_js = content["transaction_amount"]["amount"];
            auto transaction_fee_js = content["transaction_fee"]["amount"];
            
            QString transaction_amount, transaction_fee;
            
            if (transaction_fee_js.is_number()) {
                transaction_fee = QString::number(transaction_fee_js.get<double>() / GRAPHENE_BLOCKCHAIN_PRECISION) + tr(" DCT");
            } else {
                transaction_fee = QString::number(std::stod(transaction_fee_js.get<std::string>()) / GRAPHENE_BLOCKCHAIN_PRECISION) + tr(" DCT");
            }
            
            if (transaction_amount_js.is_number()) {
                transaction_amount = QString::number(transaction_amount_js.get<double>() / GRAPHENE_BLOCKCHAIN_PRECISION) + tr(" DCT");
            } else {
                transaction_amount = QString::number(std::stod(transaction_amount_js.get<std::string>()) / GRAPHENE_BLOCKCHAIN_PRECISION) + tr(" DCT");
            }
            
            std::vector<QString> values = { QString::fromStdString(operation_type),
                                            QString::fromStdString(from_account),
                                            QString::fromStdString(to_account),
                                            transaction_amount,
                                            transaction_fee,
                                            QString::fromStdString(description) };
            
                
            for (int col = 0; col < numTransactionCols; ++col) {
                
                tablewidget->setItem(i + 1, col, new QTableWidgetItem(values[col]));
                tablewidget->item(i + 1, col)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
                tablewidget->item(i + 1, col)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
                
            }
            

        }
    } catch (std::exception& ex) {
    }
    
    Connects();
}


