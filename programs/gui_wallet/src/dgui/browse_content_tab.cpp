#include "stdafx.h"

#include "browse_content_tab.hpp" 
#include "gui_wallet_global.hpp"
#include "decent_wallet_ui_gui_contentdetailsgeneral.hpp"

#ifndef _MSC_VER
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
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



namespace gui_wallet
{
BrowseContentTab::BrowseContentTab(QWidget* pParent)
: TabContentManager(pParent)
, m_pTableWidget(new DecentTable(this))
{
   m_pTableWidget->set_columns({
      {"Title", 20},
      {"Author", 15, "author"},
      {"Rating", 5, "rating"},
      {"Size", 5, "size"},
      {"Price", 5, "price"},
      {"Uploaded", 7, "created"},
      {"Expiration", 7, "expiration"},
      {" ", -50},
   });

   QLabel* pLabelSearchIcon = new QLabel(this);
   QPixmap px_search_icon(icon_search);
   pLabelSearchIcon->setPixmap(px_search_icon);

   QLineEdit* pSearchTerm = new QLineEdit(this);
   pSearchTerm->setPlaceholderText("Search Content");
   pSearchTerm->setFixedHeight(54);
   pSearchTerm->setStyleSheet(d_lineEdit);
   pSearchTerm->setAttribute(Qt::WA_MacShowFocusRect, 0);

   QHBoxLayout* pSearchLayout = new QHBoxLayout();

   pSearchLayout->setContentsMargins(42, 0, 0, 0);
   pSearchLayout->addWidget(pLabelSearchIcon);
   pSearchLayout->addWidget(pSearchTerm);

   QVBoxLayout* pMainLayout = new QVBoxLayout();
   pMainLayout->setContentsMargins(0, 0, 0, 0);
   pMainLayout->setSpacing(0);
   pMainLayout->addLayout(pSearchLayout);
   pMainLayout->addWidget(m_pTableWidget);
   setLayout(pMainLayout);

   QObject::connect(pSearchTerm, &QLineEdit::textChanged,
                    this, &BrowseContentTab::slot_SearchTermChanged);
}

void BrowseContentTab::timeToUpdate(const std::string& result) {
   _digital_contents.clear();
   
   if (result.empty()) {
      ShowDigitalContentsGUI();
      return;
   }
   
   auto contents = nlohmann::json::parse(result);
   
   _digital_contents.resize(contents.size());
   
   
   for (int i = 0; i < contents.size(); ++i) {
      SDigitalContent& cont = _digital_contents[i];
      
      cont.type = DCT::GENERAL;
      cont.author = contents[i]["author"].get<std::string>();

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
      
      uint64_t iPrice = json_to_int64(contents[i]["price"]["amount"]);
      cont.price = Globals::instance().asset(iPrice);

      cont.AVG_rating = contents[i]["AVG_rating"].get<double>() / 1000;
      
   }
   
   ShowDigitalContentsGUI();
}



std::string BrowseContentTab::getUpdateCommand()
{
   return   string("search_content ") +
            "\"" + m_strSearchTerm.toStdString() + "\" " +
            "\"" + m_pTableWidget->getSortedColumn() + "\" " +
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

   // content details dialog is ugly, needs to be rewritten
   ContentDetailsGeneral* pDetailsDialog = new ContentDetailsGeneral(nullptr);
   QObject::connect(pDetailsDialog, &ContentDetailsGeneral::ContentWasBought,
                    this, &BrowseContentTab::content_was_bought);
   pDetailsDialog->execCDD(_digital_contents[id], true);
   pDetailsDialog->setAttribute(Qt::WA_DeleteOnClose);
   pDetailsDialog->open();
}

void BrowseContentTab::content_was_bought()
{
   Globals::instance().signal_showPurchasedTab();
   Globals::instance().updateAccountBalance();
}

void BrowseContentTab::ShowDigitalContentsGUI() {
   
   m_pTableWidget->setRowCount(_digital_contents.size());
   
   int index = 0;
   for(SDigitalContent& aTemporar: _digital_contents) {

      std::string synopsis = unescape_string(aTemporar.synopsis);
      std::replace(synopsis.begin(), synopsis.end(), '\t', ' '); // JSON does not like tabs
      std::replace(synopsis.begin(), synopsis.end(), '\n', ' '); // JSON does not like newlines either
      graphene::chain::ContentObjectPropertyManager synopsis_parser(synopsis);
      std::string title = synopsis_parser.get<graphene::chain::ContentObjectTitle>();

      // Title
      int colIndex = 0;
      m_pTableWidget->setItem(index, colIndex,new QTableWidgetItem(QString::fromStdString(title)));
      m_pTableWidget->item(index, colIndex)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
      m_pTableWidget->item(index, colIndex)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
     
      // Author
      colIndex++;
      m_pTableWidget->setItem(index, colIndex,new QTableWidgetItem(QString::fromStdString(aTemporar.author)));
      m_pTableWidget->item(index, colIndex)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
      m_pTableWidget->item(index, colIndex)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
      
      // Rating
      colIndex++;
      QString rating = QString::number(aTemporar.AVG_rating, 'g', 2);
      m_pTableWidget->setItem(index,colIndex,new QTableWidgetItem(rating));
      m_pTableWidget->item(index, colIndex)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
      m_pTableWidget->item(index, colIndex)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
      
      
      // Size
      colIndex++;
      QString unit = " MB";
      double sizeAdjusted = aTemporar.size;
      
      if(aTemporar.size > 1024) {
         unit = " GB";
         sizeAdjusted = aTemporar.size / 1024.0;
      }
      
      m_pTableWidget->setItem(index, colIndex,new QTableWidgetItem(QString::number(sizeAdjusted, 'g', 2) + unit));
      m_pTableWidget->item(index, colIndex)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
      m_pTableWidget->item(index, colIndex)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
      
      // Price
      colIndex++;
      m_pTableWidget->setItem(index, colIndex, new QTableWidgetItem(aTemporar.price.getString().c_str()));

      m_pTableWidget->item(index, colIndex)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
      m_pTableWidget->item(index, colIndex)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
      
      
      
      // Uploaded
      colIndex++;
      std::string created_str = aTemporar.created.substr(0, 10);
      m_pTableWidget->setItem(index, colIndex, new QTableWidgetItem(QString::fromStdString(created_str)));
      m_pTableWidget->item(index, colIndex)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
      m_pTableWidget->item(index, colIndex)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
      
      // Expiration
      colIndex++;
      QDateTime time = QDateTime::fromString(QString::fromStdString(aTemporar.expiration), "yyyy-MM-ddTHH:mm:ss");
      std::string e_str = CalculateRemainingTime(QDateTime::currentDateTime(), time);
      
      m_pTableWidget->setItem(index, colIndex, new QTableWidgetItem(QString::fromStdString(e_str)));
      m_pTableWidget->item(index, colIndex)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
      m_pTableWidget->item(index, colIndex)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
      
      
      // Button
      colIndex++;
      DecentSmallButton* info_icon = new DecentSmallButton(icon_popup, icon_popup_white);
      info_icon->setProperty("id", QVariant::fromValue(index));
      info_icon->setAlignment(Qt::AlignCenter);
      connect(info_icon, SIGNAL(clicked()), this, SLOT(show_content_popup()));
      m_pTableWidget->setCellWidget(index, colIndex, info_icon);
      
      
      ++index;
   }

}

void BrowseContentTab::slot_SearchTermChanged(QString const& strSearchTerm)
{
   m_strSearchTerm = strSearchTerm;
}
} // end namespace gui_wallet


