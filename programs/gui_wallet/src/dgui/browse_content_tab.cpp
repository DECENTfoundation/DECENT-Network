#ifdef _MSC_VER
#include "stdafx.h"
#endif

#include "browse_content_tab.hpp" 
#include "gui_wallet_global.hpp"
#include "gui_wallet_mainwindow.hpp"

#ifndef _MSC_VER
#include <QLayout>
#include <QCheckBox>
#include <stdio.h>
#include <QHeaderView>
#include <QLabel>
#include <QMouseEvent>
#include <stdio.h>
#include <stdarg.h>
#include "json.hpp"

#include <ctime>
#include <limits>
#include <iostream>
#include <graphene/chain/config.hpp>
#include <graphene/chain/content_object.hpp>
#include <graphene/wallet/wallet.hpp>
#include "gui_design.hpp"

#include <QDateTime>
#include <QDate>
#include <QTime>
#endif

using namespace gui_wallet;
using namespace nlohmann;


BrowseContentTab::BrowseContentTab(Mainwindow_gui_wallet* parent)
: TabContentManager(parent)
, _content_popup(NULL)
, _parent(parent)
, m_pTableWidget(this)
{
    
    m_pTableWidget.set_columns({
        {"Title", 20},
        {"Author", 15, "author"},
        {"Rating", 5, "rating"},
        {"Size", 5, "size"},
        {"Price", 5, "price"},
        {"Uploaded", 7, "created"},
        {"Expiration", 7, "expiration"},
        {" ", -50},
    });
        
    QLabel* lab = new QLabel();
    QPixmap image(icon_search);
    lab->setPixmap(image);
    
    m_filterLineEdit.setPlaceholderText("Search Content");
    m_filterLineEdit.setFixedHeight(54);
    m_filterLineEdit.setStyleSheet(d_lineEdit);
    m_filterLineEdit.setAttribute(Qt::WA_MacShowFocusRect, 0);
    
    m_search_layout.setContentsMargins(42, 0, 0, 0);
    m_search_layout.addWidget(lab);
    m_search_layout.addWidget(&m_filterLineEdit);
    
    m_main_layout.setContentsMargins(0, 0, 0, 0);
    m_main_layout.setSpacing(0);
    m_main_layout.addLayout(&m_search_layout);
    m_main_layout.addWidget(&m_pTableWidget);
    setLayout(&m_main_layout);
    
    
}

void BrowseContentTab::timeToUpdate(const std::string& result) {
   _digital_contents.clear();
   
   if (result.empty()) {
      ShowDigitalContentsGUI();
      return;
   }
   
   auto contents = json::parse(result);
   
   _digital_contents.resize(contents.size());
   
   
   for (int i = 0; i < contents.size(); ++i) {
      SDigitalContent& cont = _digital_contents[i];
      
      cont.type = DCT::GENERAL;
      cont.author = contents[i]["author"].get<std::string>();
      cont.price.asset_id = contents[i]["price"]["asset_id"].get<std::string>();
      cont.synopsis = contents[i]["synopsis"].get<std::string>();
      cont.URI = contents[i]["URI"].get<std::string>();
      cont.created = contents[i]["created"].get<std::string>();
      cont.expiration = contents[i]["expiration"].get<std::string>();
      cont.size = contents[i]["size"].get<int>();
      
      if (contents[i]["times_bougth"].is_number()) {
         cont.times_bougth = contents[i]["times_bougth"].get<int>();
      } else {
         cont.times_bougth = 0;
      }
      
      
      if (contents[i]["price"]["amount"].is_number()){
         cont.price.amount =  contents[i]["price"]["amount"].get<double>();
      } else {
         cont.price.amount =  std::stod(contents[i]["price"]["amount"].get<std::string>());
      }
      
      cont.price.amount /= GRAPHENE_BLOCKCHAIN_PRECISION;
      cont.AVG_rating = contents[i]["AVG_rating"].get<double>()  / 1000;
      
   }
   
   ShowDigitalContentsGUI();
}



std::string BrowseContentTab::getUpdateCommand()
{
   std::string filterText = m_filterLineEdit.text().toStdString();
   return   string("search_content ") +
            "\"" + filterText + "\" " +
            "\"" + m_pTableWidget.getSortedColumn() + "\" " +
            "\"\" " +   // user
            "\"\" " +   // region code
            "100";
}


void BrowseContentTab::show_content_popup() {
    QLabel* btn = (QLabel*)sender();
    int id = btn->property("id").toInt();
    if (id < 0 || id >= _digital_contents.size()) {
        throw std::out_of_range("Content index is out of range");
    }
    
   if (_content_popup) {
      delete _content_popup;
      _content_popup = NULL;
   }
   
   _content_popup = new ContentDetailsGeneral(_parent);
    
   connect(_content_popup, SIGNAL(ContentWasBought()), this, SLOT(content_was_bought()));
   _content_popup->execCDD(_digital_contents[id]);
}

void BrowseContentTab::content_was_bought() {
   if (_content_popup) {
      delete _content_popup;
      _content_popup = NULL;
   }
   _parent->GoToThisTab(4, "");
   _parent->UpdateAccountBalances(Globals::instance().getCurrentUser());
   

}

void BrowseContentTab::ShowDigitalContentsGUI() {
   
   m_pTableWidget.setRowCount(_digital_contents.size());
   
   int index = 0;
   for(SDigitalContent& aTemporar: _digital_contents) {

      std::string synopsis = unescape_string(aTemporar.synopsis);
      std::replace(synopsis.begin(), synopsis.end(), '\t', ' '); // JSON does not like tabs
      std::replace(synopsis.begin(), synopsis.end(), '\n', ' '); // JSON does not like newlines either
      graphene::chain::ContentObjectPropertyManager synopsis_parser(synopsis);
      std::string title = synopsis_parser.get<graphene::chain::ContentObjectTitle>();

      // Title
      int colIndex = 0;
      m_pTableWidget.setItem(index, colIndex,new QTableWidgetItem(QString::fromStdString(title)));
      m_pTableWidget.item(index, colIndex)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
      m_pTableWidget.item(index, colIndex)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
     
      // Author
      colIndex++;
      m_pTableWidget.setItem(index, colIndex,new QTableWidgetItem(QString::fromStdString(aTemporar.author)));
      m_pTableWidget.item(index, colIndex)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
      m_pTableWidget.item(index, colIndex)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
      
      // Rating
      colIndex++;
      QString rating = QString::number(aTemporar.AVG_rating, 'g', 2);
      m_pTableWidget.setItem(index,colIndex,new QTableWidgetItem(rating));
      m_pTableWidget.item(index, colIndex)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
      m_pTableWidget.item(index, colIndex)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
      
      
      // Size
      colIndex++;
      QString unit = " MB";
      double sizeAdjusted = aTemporar.size;
      
      if(aTemporar.size > 1024) {
         unit = " GB";
         sizeAdjusted = aTemporar.size / 1024.0;
      }
      
      m_pTableWidget.setItem(index, colIndex,new QTableWidgetItem(QString::number(sizeAdjusted, 'g', 2) + unit));
      m_pTableWidget.item(index, colIndex)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
      m_pTableWidget.item(index, colIndex)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
      
      // Price
      colIndex++;
      if(aTemporar.price.amount)
      {
         m_pTableWidget.setItem(index, colIndex, new QTableWidgetItem(QString::number(aTemporar.price.amount , 'f' , 4) + " DCT"));
      }
      else
      {
         m_pTableWidget.setItem(index, colIndex, new QTableWidgetItem("Free"));
      }
      m_pTableWidget.item(index, colIndex)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
      m_pTableWidget.item(index, colIndex)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
      
      
      
      // Uploaded
      colIndex++;
      std::string created_str = aTemporar.created.substr(0, 10);
      m_pTableWidget.setItem(index, colIndex, new QTableWidgetItem(QString::fromStdString(created_str)));
      m_pTableWidget.item(index, colIndex)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
      m_pTableWidget.item(index, colIndex)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
      
      // Expiration
      colIndex++;
      QDateTime time = QDateTime::fromString(QString::fromStdString(aTemporar.expiration), "yyyy-MM-ddTHH:mm:ss");
      std::string e_str = CalculateRemainingTime(QDateTime::currentDateTime(), time);
      
      m_pTableWidget.setItem(index, colIndex, new QTableWidgetItem(QString::fromStdString(e_str)));
      m_pTableWidget.item(index, colIndex)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
      m_pTableWidget.item(index, colIndex)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
      
      
      // Button
      colIndex++;
      EventPassthrough<DecentSmallButton>* info_icon = new EventPassthrough<DecentSmallButton>(icon_popup, icon_popup_white);
      info_icon->setProperty("id", QVariant::fromValue(index));
      info_icon->setAlignment(Qt::AlignCenter);
      connect(info_icon, SIGNAL(clicked()), this, SLOT(show_content_popup()));
      m_pTableWidget.setCellWidget(index, colIndex, info_icon);
      
      
      ++index;
   }

}


