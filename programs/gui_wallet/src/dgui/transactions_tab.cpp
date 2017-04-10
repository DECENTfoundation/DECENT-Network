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
#include "gui_wallet_mainwindow.hpp"
#include "gui_design.hpp"

#include "json.hpp"

using namespace gui_wallet;
using namespace nlohmann;




TransactionsTab::TransactionsTab(Mainwindow_gui_wallet* pMainWindow)
: m_pMainWindow(pMainWindow)
{

   tablewidget.set_columns({
      {"Time", 20, "time"},
      {"Type", 10, "type"},
      {"From", 20, "from"},
      {"To"  , 20, "to"},
      {"Price", 10,"price"},
      {"Fee", 10,  "fee"},
      {"Description", 25, "description"}
   });
   
   
   user.setStyleSheet(d_lineEdit);
   user.setPlaceholderText("Enter user name to see transaction history");
   user.setAttribute(Qt::WA_MacShowFocusRect, 0);
   user.setFixedHeight(54);
   user.setFrame(false);
   
   
   QHBoxLayout* search_lay = new QHBoxLayout();
   QPixmap image(icon_search);
   search_label.setSizeIncrement(100,40);
   search_label.setPixmap(image);
   
   search_lay->setContentsMargins(42, 0, 0, 0);
   search_lay->addWidget(&search_label);
   search_lay->addWidget(&user);
   
   tablewidget.horizontalHeader()->setStretchLastSection(true);
   main_layout.setContentsMargins(0, 0, 0, 0);
   main_layout.setSpacing(0);
   main_layout.addLayout(search_lay);
   main_layout.addWidget(&tablewidget);
   setLayout(&main_layout);
   
   
   connect(&GlobalEvents::instance(), SIGNAL(currentUserChanged(std::string)), this, SLOT(currentUserChanged(std::string)));

}

void TransactionsTab::timeToUpdate(const std::string& result) {
   if (result.empty()) {
      tablewidget.setRowCount(0);
      return;
   }
   
   auto contents = json::parse(result);
   tablewidget.setRowCount(contents.size());
   
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
         transaction_fee = QString::number(transaction_fee_js.get<double>() / GRAPHENE_BLOCKCHAIN_PRECISION , 'f', 4) + tr(" DCT");
      } else {
         transaction_fee = QString::number(std::stod(transaction_fee_js.get<std::string>()) / GRAPHENE_BLOCKCHAIN_PRECISION, 'f', 4) + tr(" DCT");
      }
      
      if (transaction_amount_js.is_number()) {
         transaction_amount = QString::number(transaction_amount_js.get<double>() / GRAPHENE_BLOCKCHAIN_PRECISION, 'f', 4) + tr(" DCT");
      } else {
         transaction_amount = QString::number(std::stod(transaction_amount_js.get<std::string>()) / GRAPHENE_BLOCKCHAIN_PRECISION, 'f', 4) + tr(" DCT");
      }
      
      std::vector<QString> values = {  QString::fromStdString(timestamp),
         QString::fromStdString(operation_type),
         QString::fromStdString(from_account),
         QString::fromStdString(to_account),
         transaction_amount,
         transaction_fee,
         QString::fromStdString(description) };
      
      
      for (int col = 0; col < tablewidget.columnCount(); ++col) {
         
         tablewidget.setItem(i, col, new QTableWidgetItem(values[col]));
         tablewidget.item(i, col)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
         tablewidget.item(i, col)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
         
      }
      
      
   }

}


void TransactionsTab::currentUserChanged(std::string userName) {
   user.setText(QString::fromStdString(userName));
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


std::string TransactionsTab::getUpdateCommand() {
   if (user.text().toStdString().empty()) {
      return "";
   }

   return "get_account_history \"" + user.text().toStdString() + "\" \"" + tablewidget.getSortedColumn() + "\" 100";
    
}



void TransactionsTab::set_user_filter(const std::string& user_name) {
   user.setText(QString::fromStdString(user_name));
}

