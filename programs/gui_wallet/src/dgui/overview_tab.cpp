//*
//*	File      : overview_tab.cpp
//*
//*	Created on: 21 Nov 2016
//*	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
//*
//*  This file implements ...
//*
//*/
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
   table_widget.set_columns({
      {"Account ID", 40},
      {"Author", 40},
      {"", 10},
      {"", 10}
   });
   
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
   
   
   search_lay->setContentsMargins(42, 0, 0, 0);
   search_lay->addWidget(&search_label);
   search_lay->addWidget(&search);
   
   
   
   main->addLayout(search_lay);
   main->addWidget(&table_widget);
   
   setLayout(main);
   
   connect(&search, SIGNAL(textChanged(QString)), this, SLOT(onTextChanged(QString)));
   
   
   m_contentUpdateTimer.connect(&m_contentUpdateTimer, SIGNAL(timeout()), this, SLOT(maybeUpdateContent()));
   m_contentUpdateTimer.setInterval(1000);
   m_contentUpdateTimer.start();
   
   table_widget.setMouseTracking(true);
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
   table_widget.setRowCount(0); //Remove everything but header
   
   
   if (search.text().toStdString().empty()) {
      return;
   }
   
   std::string result;
   RunTask("search_accounts \"" + search.text().toStdString() +"\" 100", result);
   
   auto contents = json::parse(result);
   
   table_widget.setRowCount(contents.size());
   
   for (int i = 0; i < contents.size() + 1; ++i) {
      auto content = contents[i];
      
      NewButton* transaction = new NewButton(content[0].get<std::string>());
       //transaction->load(QString(":/icons/transaction5.svg"));
      NewButton* transfer = new NewButton(content[0].get<std::string>());
       //transfer->load(QString(":/icons/transaction5.svg"));
      transaction->setAlignment(Qt::AlignCenter);
      transfer->setAlignment(Qt::AlignCenter);
       


      
      QPixmap image1(":/icon/images/transaction.png");
      QPixmap image(":/icon/images/transfer.png");
      transaction->setPixmap(image1);
      transfer->setPixmap(image);
      
      transaction->setMouseTracking(true);
      transfer->setMouseTracking(true);
      
      connect(transfer, SIGNAL(ButtonPushedSignal(std::string)), this , SLOT(buttonPressed(std::string)));
      connect(transaction, SIGNAL(ButtonPushedSignal(std::string)), this , SLOT(transaction_button_pressed(std::string)));
      
      table_widget.setItem(i, 1, new QTableWidgetItem(QString::fromStdString(content[0].get<std::string>())));
      table_widget.setItem(i, 0, new QTableWidgetItem(QString::fromStdString(content[1].get<std::string>())));
      table_widget.setCellWidget(i, 2, transaction);
      table_widget.setCellWidget(i, 3, transfer);
      
      table_widget.setRowHeight(i,40);
      table_widget.cellWidget(i, 2)->setStyleSheet("* { background-color: rgb(255,255,255); color : rgb(27,176,104); }");
      table_widget.cellWidget(i, 3)->setStyleSheet("* { background-color: rgb(255,255,255); color : rgb(27,176,104); }");
      
      
      table_widget.item(i,0)->setBackground(Qt::white);
      table_widget.item(i,1)->setBackground(Qt::white);
      
      table_widget.item(i,0)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
      table_widget.item(i,1)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
      
      table_widget.item(i,0)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
      table_widget.item(i,1)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
      
      table_widget.item(i,0)->setForeground(QColor::fromRgb(88,88,88));
      table_widget.item(i,1)->setForeground(QColor::fromRgb(88,88,88));
      
   }
}



void Overview_tab::transaction_button_pressed(std::string accountName)
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
