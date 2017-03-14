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
#include "gui_wallet_centralwidget.hpp"
#include <QPixmap>
#include <QStackedWidget>
#include <QRect>
#include <QFont>
#include <graphene/chain/config.hpp>
#include "json.hpp"

using namespace gui_wallet;
using namespace nlohmann;


Overview_tab::Overview_tab(class Mainwindow_gui_wallet* a_pPar)
    : m_pPar(a_pPar)
{
    table_widget.setColumnCount(4);
    table_widget.setRowCount(1);

    QSize tqsTableSize = table_widget.size();
    table_widget.setColumnWidth(0,(tqsTableSize.width()*18)/100);
    table_widget.setColumnWidth(1,(tqsTableSize.width()*50)/100);
    table_widget.setColumnWidth(2,(tqsTableSize.width()*17)/100);
    table_widget.setColumnWidth(3,(tqsTableSize.width()*15)/100);

    table_widget.setRowHeight(0,35);

    table_widget.setStyleSheet("QTableView{border : 1px solid lightGray}");

    table_widget.setItem(0,0,new QTableWidgetItem(tr("Account ID")));
    table_widget.setItem(0,1,new QTableWidgetItem(tr("Author")));
    table_widget.setItem(0,2,new QTableWidgetItem(tr("")));
    table_widget.setItem(0,3,new QTableWidgetItem(tr("")));


    QFont f( "Open Sans Bold", 14, QFont::Bold);

    table_widget.item(0,0)->setFont(f);
    table_widget.item(0,1)->setFont(f);
    table_widget.item(0,2)->setFont(f);
    table_widget.item(0,3)->setFont(f);


    table_widget.item(0,0)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
    table_widget.item(0,1)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
    table_widget.item(0,2)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
    table_widget.item(0,3)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);

    table_widget.item(0,0)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    table_widget.item(0,1)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    table_widget.item(0,2)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    table_widget.item(0,3)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    
    table_widget.item(0,0)->setBackground(QColor(228,227,228));
    table_widget.item(0,1)->setBackground(QColor(228,227,228));
    table_widget.item(0,2)->setBackground(QColor(228,227,228));
    table_widget.item(0,3)->setBackground(QColor(228,227,228));

    table_widget.item(0,0)->setForeground(QColor::fromRgb(51,51,51));
    table_widget.item(0,1)->setForeground(QColor::fromRgb(51,51,51));
    table_widget.item(0,2)->setForeground(QColor::fromRgb(51,51,51));
    table_widget.item(0,3)->setForeground(QColor::fromRgb(51,51,51));


    table_widget.horizontalHeader()->hide();
    table_widget.verticalHeader()->hide();

    table_widget.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    table_widget.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);


    table_widget.setStyleSheet(("gridline-color: rgb(228,227,228));"));
    table_widget.setStyleSheet("QTableView{border : 0px}");




    QVBoxLayout* main = new QVBoxLayout();
    QHBoxLayout* search_lay = new QHBoxLayout();

    main->setContentsMargins(0, 5, 0, 0);
    main->setMargin(0);

    search_lay->setMargin(0);
    search_lay->setContentsMargins(0,0,0,0);

    QPixmap image(":/icon/images/search.svg");

    search_label.setSizeIncrement(100,40);
    search_label.setPixmap(image);
    search.setPlaceholderText(QString("Search"));
    search.setStyleSheet("border: 0px solid white");
    search.setAttribute(Qt::WA_MacShowFocusRect, 0);
    search.setFixedHeight(40);


    search_lay->addWidget(new QLabel());
    search_lay->addWidget(new QLabel());
    search_lay->addWidget(new QLabel());
    search_lay->addWidget(&search_label);
    search_lay->addWidget(&search);



    main->addLayout(search_lay);
    //main->addWidget(&search);
    main->addWidget(&table_widget);

    setLayout(main);
    
    
    connect(&search, SIGNAL(textChanged(QString)), this, SLOT(onTextChanged(QString)));
    
    
    m_contentUpdateTimer.connect(&m_contentUpdateTimer, SIGNAL(timeout()), this, SLOT(maybeUpdateContent()));
    m_contentUpdateTimer.setInterval(1000);
    m_contentUpdateTimer.start();
    
    table_widget.setMouseTracking(true);
    connect(&table_widget, SIGNAL(mouseMoveEventDid()), this, SLOT(doRowColor()));
}



void Overview_tab::maybeUpdateContent() {
    if (!m_doUpdate) {
        return;
    }
    
    m_doUpdate = false;
    try {
        updateContents();
    } catch (...) {
        // Ignore for now;
    }
}

void Overview_tab::onTextChanged(const QString& text) {
    
    m_doUpdate = true;
}


void Overview_tab::updateContents() {
    table_widget.setRowCount(1); //Remove everything but header
    
    
    if (search.text().toStdString().empty()) {
        return;
    }
    
    std::string result;
    RunTask("search_accounts \"" + search.text().toStdString() +"\" 100", result);
    
    auto contents = json::parse(result);
    
    table_widget.setRowCount(contents.size() + 1);
    
    for (int i = 0; i < contents.size(); ++i) {
        auto content = contents[i];
        
        NewButton* transaction = new NewButton(content[0].get<std::string>());
        NewButton* transfer = new NewButton(content[0].get<std::string>());
        transaction->setAlignment(Qt::AlignCenter);
        transfer->setAlignment(Qt::AlignCenter);
        
        transaction->setText("Transaction");
        transfer->setText("Transfer");
        QFont f( "Open Sans Bold", 14, QFont::Bold);
        transaction->setFont(f);
        transaction->setStyleSheet("* { background-color: rgb(255,255,255); color : rgb(27,176,104); }");
        transfer->setFont(f);
        transfer->setStyleSheet("* { background-color: rgb(255,255,255); color : rgb(27,176,104); }");
        
        transaction->setMouseTracking(true);
        transfer->setMouseTracking(true);

        connect(transfer, SIGNAL(ButtonPushedSignal(std::string)), this , SLOT(buttonPressed(std::string)));
        connect(transaction, SIGNAL(ButtonPushedSignal(std::string)), this , SLOT(TransactionButtonPressed(std::string)));
        connect(transfer, SIGNAL(mouseWasMoved()), this , SLOT(doRowColor()));
        connect(transaction, SIGNAL(mouseWasMoved()), this , SLOT(doRowColor()));
        
        
        table_widget.setItem(i+1, 1, new QTableWidgetItem(QString::fromStdString(content[0].get<std::string>())));
        table_widget.setItem(i+1, 0, new QTableWidgetItem(QString::fromStdString(content[1].get<std::string>())));
        table_widget.setCellWidget(i+1, 2, transaction);
        table_widget.setCellWidget(i+1, 3, transfer);
        
        
        
        table_widget.setRowHeight(i+1,40);
        table_widget.cellWidget(i+1 , 2)->setStyleSheet("* { background-color: rgb(255,255,255); color : rgb(27,176,104); }");
        table_widget.cellWidget(i+1 , 3)->setStyleSheet("* { background-color: rgb(255,255,255); color : rgb(27,176,104); }");
        
        table_widget.item(i+1,0)->setBackground(Qt::white);
        table_widget.item(i+1,1)->setBackground(Qt::white);
        
        table_widget.item(i+1,0)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        table_widget.item(i+1,1)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        
        table_widget.item(i+1,0)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        table_widget.item(i+1,1)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        
        table_widget.item(i+1,0)->setForeground(QColor::fromRgb(88,88,88));
        table_widget.item(i+1,1)->setForeground(QColor::fromRgb(88,88,88));
        
        
        
    }
    
    
    
}



void Overview_tab::TransactionButtonPressed(std::string accountName)
{
    m_pPar->GoToThisTab(1 , accountName);
}

void Overview_tab::buttonPressed(std::string accountName)
{
    try {
        std::string result;
        RunTask("get_account " + accountName, result);
        auto accountInfo = json::parse(result);
        
        std::string id = accountInfo["id"].get<std::string>();
        std::string registrar = accountInfo["registrar"].get<std::string>();
        std::string referrer = accountInfo["referrer"].get<std::string>();
        std::string lifetime_referrer = accountInfo["lifetime_referrer"].get<std::string>();
        std::string network_fee_percentage = std::to_string(accountInfo["network_fee_percentage"].get<int>());
        std::string lifetime_referrer_fee_percentage = std::to_string(accountInfo["lifetime_referrer_fee_percentage"].get<int>());
        std::string referrer_rewards_percentage = std::to_string(accountInfo["referrer_rewards_percentage"].get<int>());
        
        std::string name = accountInfo["name"].get<std::string>();
        

        
        QZebraWidget* info_window = new QZebraWidget();
        
        info_window->AddInfo("Registrar", registrar);
        info_window->AddInfo("Referrer", referrer);
        info_window->AddInfo("Lifetime Referrer", lifetime_referrer);
        info_window->AddInfo("Network Fee", network_fee_percentage);
        info_window->AddInfo("Lifetime Referrer Fee", lifetime_referrer_fee_percentage);
        info_window->AddInfo("Referrer Rewards Percentage", referrer_rewards_percentage);
        
        
        info_window->setWindowTitle(QString::fromStdString(name) + tr(" (") + QString::fromStdString(id) + tr(")"));
        info_window->setFixedSize(620,420);
        info_window->show();
    } catch(...) {
        // Ignore for now
    }

}

void Overview_tab::ArrangeSize()
{
    QSize tqsTableSize = table_widget.size();
    table_widget.setColumnWidth(0,(tqsTableSize.width()*18)/100);
    table_widget.setColumnWidth(1,(tqsTableSize.width()*50)/100);
    table_widget.setColumnWidth(2,(tqsTableSize.width()*17)/100);
    table_widget.setColumnWidth(3,(tqsTableSize.width()*15)/100);
}

void Overview_tab::resizeEvent(QResizeEvent *a_event)
{
    QWidget::resizeEvent(a_event);
    ArrangeSize();
}

void Overview_tab::doRowColor()
{
    for(int i = 1; i < table_widget.rowCount(); ++i)
    {
        table_widget.cellWidget(i, 2)->setStyleSheet("* { background-color: rgb(255,255,255); color : rgb(27,176,104); }");
        table_widget.cellWidget(i, 3)->setStyleSheet("* { background-color: rgb(255,255,255); color : rgb(27,176,104); }");
        table_widget.item(i,0)->setBackground(QColor(255,255,255));
        table_widget.item(i,1)->setBackground(QColor(255,255,255));

        //table_widget.item(i+1,0)->setForeground(QColor::fromRgb(0,0,0));
        table_widget.item(i,0)->setForeground(QColor::fromRgb(88,88,88));
        table_widget.item(i,1)->setForeground(QColor::fromRgb(88,88,88));
    }
    QPoint mouse_pos = table_widget.mapFromGlobal(QCursor::pos());
    if(mouse_pos.x() > 400)
    {
        mouse_pos.setX(mouse_pos.x() - mouse_pos.x() + 50);
    }
    QTableWidgetItem *ite = table_widget.itemAt(mouse_pos);

    if(ite != NULL)
    {
        int a = ite->row();
        if(a != 0)
        {
            table_widget.cellWidget(a , 2)->setStyleSheet("* { background-color: rgb(27,176,104); color : white; }");
            table_widget.cellWidget(a , 3)->setStyleSheet("* { background-color: rgb(27,176,104); color : white; }");

            table_widget.item(a,0)->setBackgroundColor(QColor(27,176,104));
            table_widget.item(a,1)->setBackgroundColor(QColor(27,176,104));
            table_widget.item(a,0)->setForeground(QColor::fromRgb(255,255,255));
            table_widget.item(a,1)->setForeground(QColor::fromRgb(255,255,255));



        }
    }
}

Overview_tab::~Overview_tab()
{

}

TableWidget::TableWidget() : QTableWidget()
{
    this->setMouseTracking(true);
}


void TableWidget::mouseMoveEvent(QMouseEvent *event)
{
    mouseMoveEventDid();
}

