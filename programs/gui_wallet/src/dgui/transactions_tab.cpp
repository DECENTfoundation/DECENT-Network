//*
// *	File      : TransactionsTab.cpp
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
#include <QResource>
#include <QStringList>

#include <iostream>

#include <boost/algorithm/string/replace.hpp>

#include <graphene/chain/config.hpp>

#include "gui_wallet_global.hpp"
#include "qt_commonheader.hpp"
#include "ui_wallet_functions.hpp"

#include "json.hpp"

using namespace gui_wallet;
using namespace nlohmann;




TransactionsTab::TransactionsTab() : green_row(0) {
   
   QFont f( "Open Sans Bold", 14, QFont::Bold);
   
   tablewidget = new DecentTable();
   
   tablewidget->setColumnCount(_table_columns.size());
   
   tablewidget->verticalHeader()->setDefaultSectionSize(35);
   tablewidget->horizontalHeader()->setDefaultSectionSize(230);
   tablewidget->verticalHeader()->hide();
   
   tablewidget->setStyleSheet("QTableView{border : 0px}");
   tablewidget->setSelectionMode(QAbstractItemView::NoSelection);
   
   QStringList headerList;
   for (int i = 0; i < _table_columns.size(); i++) {
      headerList << QString::fromStdString(_table_columns[i]);
   }
   tablewidget->setHorizontalHeaderLabels(headerList);
   
   tablewidget->horizontalHeader()->setFixedHeight(35);
   tablewidget->horizontalHeader()->setFont(f);
   
   tablewidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
   tablewidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
   tablewidget->horizontalHeader()->setStyleSheet("QHeaderView::section {"
                                                  "border-right: 1px solid rgb(193,192,193);"
                                                  "border-bottom: 0px;"
                                                  "border-top: 0px;}");
  
   user.setStyleSheet("border: 0px solid white");
   user.setPlaceholderText("Enter user name to see transaction history");
   user.setAttribute(Qt::WA_MacShowFocusRect, 0);
   user.setMaximumHeight(40);
   user.setFixedHeight(40);
   user.setFrame(false);
   
   
   QHBoxLayout* search_lay = new QHBoxLayout();
   QPixmap image(":/icon/images/search.svg");
   search_label.setSizeIncrement(100,40);
   search_label.setPixmap(image);
   
   search_lay->setContentsMargins(42, 0, 0, 0);
   search_lay->addWidget(&search_label);
   search_lay->addWidget(&user);
   
   tablewidget->horizontalHeader()->setStretchLastSection(true);
   main_layout.setContentsMargins(0, 0, 0, 0);
   main_layout.addLayout(search_lay);
   main_layout.addWidget(tablewidget);
   setLayout(&main_layout);
   
   
   connect(&user, SIGNAL(textChanged(QString)), this, SLOT(onTextChanged(QString)));
   connect(&GlobalEvents::instance(), SIGNAL(currentUserChanged(std::string)), this, SLOT(currentUserChanged(std::string)));
   
   m_contentUpdateTimer.connect(&m_contentUpdateTimer, SIGNAL(timeout()), this, SLOT(maybeUpdateContent()));
   m_contentUpdateTimer.setInterval(1000);
   m_contentUpdateTimer.start();

   connect(tablewidget, SIGNAL(mouseMoveEventDid(QPoint)), this, SLOT(hightlight_row(QPoint)));
}




void TransactionsTab::currentUserChanged(std::string userName) {
   user.setText(QString::fromStdString(userName));
}

void TransactionsTab::maybeUpdateContent() {
   if (!m_doUpdate) {
      return;
   }
   
   m_doUpdate = false;
   updateContents();
}

void TransactionsTab::onTextChanged(const QString& text) {
   
   m_doUpdate = true;
}


void TransactionsTab::createNewRow()
{
   tablewidget->setRowCount(100);
}

void TransactionsTab::ArrangeSize()
{
   QSize tqsTableSize = tablewidget->size();
   std::vector<int> sizes = {18, 10, 17, 17, 8, 8, 22};
   for (int i = 0; i < _table_columns.size(); ++i) {
      tablewidget->setColumnWidth(i, (tqsTableSize.width() * sizes[i]) / 100);
   }
   
}

void TransactionsTab::resizeEvent(QResizeEvent *a_event)
{
   QWidget::resizeEvent(a_event);
   ArrangeSize();
}


TransactionsTab::~TransactionsTab()
{
   main_layout.removeWidget(tablewidget);
   delete tablewidget;
}

void TransactionsTab::hightlight_row(QPoint point)
{
   if(tablewidget->rowCount() < 0) {
      return;
   }
   
   for (int i = 0; i < _table_columns.size(); ++i)
   {
      if(tablewidget->item(green_row,i) != NULL)
      {
         tablewidget->item(green_row,i)->setBackgroundColor(QColor(255,255,255));
         tablewidget->item(green_row,i)->setForeground(QColor::fromRgb(0,0,0));
      }
   }
   
   
   int row = tablewidget->rowAt(point.y());
   if(row < 0)
      return;
   
   for (int i = 0; i < _table_columns.size(); ++i)
   {
      if(tablewidget->item(row, i) != NULL)
      {
         tablewidget->item(row, i)->setBackgroundColor(QColor(27,176,104));
         tablewidget->item(row, i)->setForeground(QColor::fromRgb(255,255,255));
      }
   }
   
   green_row = row;
   
}



std::string TransactionsTab::getAccountName(std::string accountId) {
   
   auto seach = _user_id_cache.find(accountId);
   if (seach == _user_id_cache.end()) {
      std::string accountInfo, accountName = "Unknown";
      RunTask("get_object \"" + accountId + "\"", accountInfo);
      
      auto accountInfoParsed = json::parse(accountInfo);
      if (accountInfoParsed.size() != 0) {
         accountName = accountInfoParsed[0]["name"].get<std::string>();
      }
      _user_id_cache.insert(std::make_pair(accountId, accountName));
   }
   
   return _user_id_cache[accountId];
}

void TransactionsTab::updateContents() {
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
      
      tablewidget->setRowCount(contents.size());
      
      for (int i = 0; i < contents.size(); ++i) {
         auto content = contents[i];
         std::string from_account = getAccountName(content["from_account"].get<std::string>());
         std::string to_account = getAccountName(content["to_account"].get<std::string>());
         std::string operation_type = content["operation_type"].get<std::string>();
         std::string description = content["description"].get<std::string>();
         std::string timestamp = boost::replace_all_copy(content["timestamp"].get<std::string>(), "T", " ");
         

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
         
         std::vector<QString> values = {  QString::fromStdString(timestamp),
                                          QString::fromStdString(operation_type),
                                          QString::fromStdString(from_account),
                                          QString::fromStdString(to_account),
                                          transaction_amount,
                                          transaction_fee,
                                          QString::fromStdString(description) };
         
         
         for (int col = 0; col < _table_columns.size(); ++col) {
            
            tablewidget->setItem(i, col, new QTableWidgetItem(values[col]));
            tablewidget->item(i, col)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            tablewidget->item(i, col)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
            
         }
         
         
      }
   } catch (std::exception& ex) {
      std::cout << ex.what() << std::endl;
   }
   
}

void TransactionsTab::SetInfo(std::string info_from_overview)
{
   user.setText(QString::fromStdString(info_from_overview));
}


