//*
//*	File      : overview_tab.cpp
//*
//*	Created on: 21 Nov 2016
//*	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
//*
//*  This file implements ...
//*
//*/
#ifdef _MSC_VER
#include "stdafx.h"
#endif

#include "overview_tab.hpp"
#include "gui_wallet_mainwindow.hpp"
#include "gui_wallet_centralwidget.hpp"

#ifndef _MSC_VER
#include <QPixmap>
#include <QStackedWidget>
#include <QRect>
#include <QFont>
#include <graphene/chain/config.hpp>
#include "json.hpp"
#endif

#include "gui_design.hpp"


using namespace gui_wallet;
using namespace nlohmann;


Overview_tab::Overview_tab(class Mainwindow_gui_wallet* a_pPar)
: m_pPar(a_pPar)
, table_widget(this)
{
   table_widget.set_columns({
      {"Account ID", 40, "id"},
      {"Account", 40, "name"},
      {"", 10},
      {"", 10}
   });
   
   QVBoxLayout* main = new QVBoxLayout();
   QHBoxLayout* search_lay = new QHBoxLayout();
   
   main->setContentsMargins(0, 5, 0, 0);
   main->setMargin(0);
   
   search_lay->setMargin(0);
   search_lay->setContentsMargins(0,0,0,0);
   
   QPixmap image(icon_search);
   
   search_label.setSizeIncrement(100,40);
   search_label.setPixmap(image);
   search.setPlaceholderText(QString("Search"));
   search.setStyleSheet(d_lineEdit);
   search.setAttribute(Qt::WA_MacShowFocusRect, 0);
   search.setFixedHeight(54);
   
   
   search_lay->setContentsMargins(42, 0, 0, 0);
   search_lay->addWidget(&search_label);
   search_lay->addWidget(&search);
   
   
   
   main->addLayout(search_lay);
   main->addWidget(&table_widget);
   main->setSpacing(0);
    
   setLayout(main);
 
   table_widget.setMouseTracking(true);
}



void Overview_tab::timeToUpdate(const std::string& result) {
   if (result.empty()) {
      table_widget.setRowCount(0);
      return;
   }
   
   auto contents = json::parse(result);
   
   table_widget.setRowCount(contents.size());
   
   for (int i = 0; i < contents.size() + 1; ++i) {
      auto content = contents[i];
      
      std::string name = content["name"].get<std::string>();
      std::string id = content["id"].get<std::string>();
      
      table_widget.setItem(i, 1, new QTableWidgetItem(QString::fromStdString(name)));
      table_widget.setItem(i, 0, new QTableWidgetItem(QString::fromStdString(id)));
      
      EventPassthrough<DecentSmallButton>* trans = new EventPassthrough<DecentSmallButton>(icon_transaction, icon_transaction_white);
      trans->setProperty("accountName", QVariant::fromValue(QString::fromStdString(name)));

      trans->setAlignment(Qt::AlignCenter);
      connect(trans, SIGNAL(clicked()), this, SLOT(transactionButtonPressed()));
      table_widget.setCellWidget(i, 2, trans);
      
      EventPassthrough<DecentSmallButton>* transf = new EventPassthrough<DecentSmallButton>(icon_transfer, icon_transfer_white);

      transf->setProperty("accountName", QVariant::fromValue(QString::fromStdString(name)));
      transf->setAlignment(Qt::AlignCenter);
      connect(transf, SIGNAL(clicked()), this, SLOT(buttonPressed()));
      table_widget.setCellWidget(i, 3, transf);
      
      table_widget.setRowHeight(i,40);
      table_widget.cellWidget(i, 2)->setStyleSheet(d_table);
      table_widget.cellWidget(i, 3)->setStyleSheet(d_table);
      
      
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


std::string Overview_tab::getUpdateCommand() {
   return "search_accounts \"" + search.text().toStdString() + "\" \"" + table_widget.getSortedColumn() + "\" 100";
}


void Overview_tab::transactionButtonPressed()
{
    DecentSmallButton* button = (DecentSmallButton*)sender();
    QString accountName = button->property("accountName").toString();
    m_pPar->GoToThisTab(1 , accountName.toStdString());
}

void Overview_tab::buttonPressed()
{
    DecentSmallButton* button = (DecentSmallButton*)sender();
    QString accountName = button->property("accountName").toString();

   try {
      std::string result;
      RunTask("get_account " + accountName.toStdString(), result);
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
