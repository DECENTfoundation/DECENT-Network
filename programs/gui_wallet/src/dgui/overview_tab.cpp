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
    table_widget.setColumnCount(3);
    table_widget.setRowCount(1);

    QSize tqsTableSize = table_widget.size();
    table_widget.setColumnWidth(0,(tqsTableSize.width()*12)/100);
    table_widget.setColumnWidth(1,(tqsTableSize.width()*32)/100);
    table_widget.setColumnWidth(2,(tqsTableSize.width()*56)/100);

    table_widget.setRowHeight(0,35);

    table_widget.setStyleSheet("QTableView{border : 1px solid lightGray}");

    table_widget.setItem(0,0,new QTableWidgetItem(tr("Info")));
    table_widget.setItem(0,1,new QTableWidgetItem(tr("Account ID")));
    table_widget.setItem(0,2,new QTableWidgetItem(tr("Author")));


    QFont f( "Open Sans Bold", 14, QFont::Bold);

    table_widget.item(0,0)->setFont(f);
    table_widget.item(0,1)->setFont(f);
    table_widget.item(0,2)->setFont(f);

    table_widget.item(0,0)->setText("Info");

    table_widget.item(0,0)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
    table_widget.item(0,1)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
    table_widget.item(0,2)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);

    table_widget.item(0,0)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    table_widget.item(0,1)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    table_widget.item(0,2)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);

    table_widget.item(0,0)->setBackground(QColor(228,227,228));
    table_widget.item(0,1)->setBackground(QColor(228,227,228));
    table_widget.item(0,2)->setBackground(QColor(228,227,228));

    table_widget.item(0,0)->setForeground(QColor::fromRgb(51,51,51));
    table_widget.item(0,1)->setForeground(QColor::fromRgb(51,51,51));
    table_widget.item(0,2)->setForeground(QColor::fromRgb(51,51,51));


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
    RunTask("list_accounts \"" + search.text().toStdString() +"\" 100", result);
    
    auto contents = json::parse(result);
    
    table_widget.setRowCount(contents.size() + 1);
    
    for (int i = 0; i < contents.size(); ++i) {
        auto content = contents[i];
        
        NewButton* btn = new NewButton(content[0].get<std::string>());
        btn->setAlignment(Qt::AlignCenter);
        
        QPixmap image1(":/icon/images/info1_white.svg");
        btn->setPixmap(image1);
        //btn->setMouseTracking(true);

        connect(btn, SIGNAL(ButtonPushedSignal(std::string)), this , SLOT(buttonPressed(std::string)));
        
        table_widget.setCellWidget(i + 1, 0, btn);
        table_widget.setItem(i+1, 1, new QTableWidgetItem(QString::fromStdString(content[0].get<std::string>())));
        table_widget.setItem(i+1, 2, new QTableWidgetItem(QString::fromStdString(content[1].get<std::string>())));
        
        
        table_widget.setRowHeight(i + 1,40);
        table_widget.cellWidget(i + 1 , 0)->setStyleSheet("* { background-color: rgb(255,255,255); }");
        table_widget.item(i+1,1)->setBackground(Qt::white);
        table_widget.item(i+1,2)->setBackground(Qt::white);
        
        table_widget.item(i+1,1)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        table_widget.item(i+1,2)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        
        table_widget.item(i+1,1)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        table_widget.item(i+1,2)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        
        table_widget.item(i+1,1)->setForeground(QColor::fromRgb(88,88,88));
        table_widget.item(i+1,2)->setForeground(QColor::fromRgb(88,88,88));
        
        
        
    }
    
    
    
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
    table_widget.setColumnWidth(0,(tqsTableSize.width()*12)/100);
    table_widget.setColumnWidth(1,(tqsTableSize.width()*32)/100);
    table_widget.setColumnWidth(2,(tqsTableSize.width()*56)/100);
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
        NewButton* btn = (NewButton*)table_widget.cellWidget(i,0);
        btn->setStyleSheet("* { background-color: rgb(255,255,255); }");
        table_widget.item(i,1)->setBackground(QColor(255,255,255));
        table_widget.item(i,2)->setBackground(QColor(255,255,255));

        //table_widget.item(i+1,0)->setForeground(QColor::fromRgb(0,0,0));
        table_widget.item(i,1)->setForeground(QColor::fromRgb(88,88,88));
        table_widget.item(i,2)->setForeground(QColor::fromRgb(88,88,88));


        QPixmap image(":/icon/images/info1.svg");

        btn->setPixmap(image);

    }
    QPoint mouse_pos = table_widget.mapFromGlobal(QCursor::pos());
    if(mouse_pos.x() > 0 && mouse_pos.x() < 110)
    {
        mouse_pos.setX(mouse_pos.x() + 300);
    }
    QTableWidgetItem *ite = table_widget.itemAt(mouse_pos);

    if(ite != NULL)
    {
        int a = ite->row();
        if(a != 0)
        {
            QPixmap image(":/icon/images/info1_white.svg");
            table_widget.cellWidget(a , 0)->setStyleSheet("* { background-color: rgb(27,176,104); color : white; }");
            ((NewButton*)table_widget.cellWidget(a,0))->setPixmap(image);

            table_widget.item(a,1)->setBackgroundColor(QColor(27,176,104));
            table_widget.item(a,2)->setBackgroundColor(QColor(27,176,104));
            table_widget.item(a,1)->setForeground(QColor::fromRgb(255,255,255));
            table_widget.item(a,2)->setForeground(QColor::fromRgb(255,255,255));



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

