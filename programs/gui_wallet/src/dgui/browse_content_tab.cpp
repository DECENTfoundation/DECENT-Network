/*
 *	File: BrowseContentTab.cpp
 *
 *	Cted on: 11 Nov 2016
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
#include "browse_content_tab.hpp"
#include "gui_wallet_global.hpp"
#include "ui_wallet_functions.hpp"
#include "gui_wallet_mainwindow.hpp"

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
#include <graphene/wallet/wallet.hpp>


#include <QDateTime>
#include <QDate>
#include <QTime>

using namespace gui_wallet;
using namespace nlohmann;


BrowseContentTab::BrowseContentTab(Mainwindow_gui_wallet* parent) : _content_popup(NULL), _parent(parent) {
    
    m_pTableWidget.set_columns({
        {"Title", 20},
        {"Rating", 10},
        {"Size", 10},
        {"Price", 10},
        {"Uploaded", 10},
        {"Expiration", 10},
        {" ", -50},
    });
    
    
 
    QLabel* lab = new QLabel();
    QPixmap image(":/icon/images/search.svg");
    lab->setPixmap(image);
    
    m_filterLineEdit.setPlaceholderText("Enter search term");
    m_filterLineEdit.setFixedHeight(40);
    m_filterLineEdit.setStyleSheet("border: 0");
    m_filterLineEdit.setAttribute(Qt::WA_MacShowFocusRect, 0);
    
    m_search_layout.setContentsMargins(42, 0, 0, 0);
    m_search_layout.addWidget(lab);
    m_search_layout.addWidget(&m_filterLineEdit);
    
    m_main_layout.setContentsMargins(0, 0, 0, 0);
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



std::string BrowseContentTab::getUpdateCommand() {
   std::string filterText = m_filterLineEdit.text().toStdString();
   return "search_content \"" + filterText + "\" 100";
}


void BrowseContentTab::show_content_popup() {
    QLabel* btn = (QLabel*)sender();
    int id = btn->property("id").toInt();
    if (id < 0 || id >= _digital_contents.size()) {
        throw std::out_of_range("Content index is our of range");
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
   _parent->UpdateAccountBalances(GlobalEvents::instance().getCurrentUser());
   

}

void BrowseContentTab::ShowDigitalContentsGUI() {
    
    m_pTableWidget.setRowCount(_digital_contents.size());
    
    int index = 0;
    for(SDigitalContent& aTemporar: _digital_contents) {
        
        EventPassthrough<DecentSmallButton>* info_icon = new EventPassthrough<DecentSmallButton>(":/icon/images/pop_up.png", ":/icon/images/pop_up1.png");
        info_icon->setProperty("id", QVariant::fromValue(index));
        info_icon->setAlignment(Qt::AlignCenter);
        connect(info_icon, SIGNAL(clicked()), this, SLOT(show_content_popup()));
        m_pTableWidget.setCellWidget(index, 6, info_icon);
        
        
        
        
        // Need to rewrite this
        std::string created_str = aTemporar.created.substr(0, 10);
        
        m_pTableWidget.setItem(index, 4, new QTableWidgetItem(QString::fromStdString(created_str)));
        m_pTableWidget.item(index, 4)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        m_pTableWidget.item(index, 4)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        
        
        std::string synopsis = unescape_string(aTemporar.synopsis);
        std::replace(synopsis.begin(), synopsis.end(), '\t', ' '); // JSON does not like tabs
        std::replace(synopsis.begin(), synopsis.end(), '\n', ' '); // JSON does not like newlines either
        //massageBox_title.push_back(	)
        
        try {
            auto synopsis_parsed = json::parse(synopsis);
            synopsis = synopsis_parsed["title"].get<std::string>();
            
        } catch (...) {}
        
        
        
        m_pTableWidget.setItem(index,0,new QTableWidgetItem(QString::fromStdString(synopsis)));
        m_pTableWidget.item(index, 0)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        m_pTableWidget.item(index, 0)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        
        std::string rating;
        for(int i = 0; i < std::to_string(aTemporar.AVG_rating).find(".") + 2; ++i)
        {
            rating.push_back(std::to_string(aTemporar.AVG_rating)[i]);
        }
        m_pTableWidget.setItem(index,1,new QTableWidgetItem(QString::fromStdString(rating)));
        m_pTableWidget.item(index, 1)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        m_pTableWidget.item(index, 1)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        
        
        
        if(aTemporar.size < 1024)
        {
            m_pTableWidget.setItem(index,2,new QTableWidgetItem(QString::fromStdString(std::to_string(aTemporar.size) + " MB")));
        }
        else
        {
            float size = (float)aTemporar.size/1024;
            std::string size_s;
            std::string s = std::to_string(size);
            for(int i = 0; i < s.find(".") + 3; ++i)
            {
                size_s.push_back(s[i]);
            }
            m_pTableWidget.setItem(index,2,new QTableWidgetItem(QString::fromStdString(size_s + " GB")));
        }
        m_pTableWidget.item(index, 2)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        m_pTableWidget.item(index, 2)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        
        
        m_pTableWidget.setItem(index,3,new QTableWidgetItem(QString::number(aTemporar.price.amount) + " DCT"));
        m_pTableWidget.item(index, 3)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        m_pTableWidget.item(index, 3)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        
        
        QDateTime time = QDateTime::fromString(QString::fromStdString(aTemporar.expiration), "yyyy-MM-ddTHH:mm:ss");
        
        std::string e_str = CalculateRemainingTime(QDateTime::currentDateTime(), time);
        
        m_pTableWidget.setItem(index,5,new QTableWidgetItem(QString::fromStdString(e_str)));
        m_pTableWidget.item(index, 5)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        m_pTableWidget.item(index, 5)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        
        ++index;
    }
}

void BrowseContentTab::RunTask(std::string const& str_command, std::string& str_result)
{
   _parent->RunTask(str_command, str_result);
}

