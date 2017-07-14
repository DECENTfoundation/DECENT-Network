/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
#include "stdafx.h"

#include "gui_wallet_global.hpp"
#include "browse_content_tab.hpp"
#include "decent_line_edit.hpp"
#include "decent_button.hpp"
#include "richdialog.hpp"

#include <boost/algorithm/string/replace.hpp>

#ifndef _MSC_VER
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSignalMapper>
#include "json.hpp"

#include <graphene/chain/config.hpp>
#include <graphene/chain/content_object.hpp>
#include <graphene/wallet/wallet.hpp>

#include <QDateTime>
#include <QDate>
#include <QTime>
#endif

namespace gui_wallet
{
BrowseContentTab::BrowseContentTab(QWidget* pParent,
                                   DecentLineEdit* pFilterLineEdit)
: TabContentManager(pParent)
, m_pTableWidget(new DecentTable(this))
, m_pDetailsSignalMapper(nullptr)
{
   m_pTableWidget->set_columns({
      {tr("Title"), 20},
      {tr("Author"), 15, "author"},
      {tr("Rating"), 5, "rating"},
      {tr("Size"), 5, "size"},
      {tr("Price"), 6, "price"},
      {tr("Uploaded"), 7, "created"},
      {tr("Expiration"), 7, "expiration"},
      { " ",
#ifdef WINDOWS_HIGH_DPI
           -90
#else
           -50
#endif
      },
   });

   QVBoxLayout* pMainLayout = new QVBoxLayout();
   pMainLayout->setContentsMargins(0, 0, 0, 0);
   pMainLayout->setSpacing(0);
   pMainLayout->addWidget(m_pTableWidget);
   setLayout(pMainLayout);

   QObject::connect(pFilterLineEdit, &QLineEdit::textChanged,
                    this, &BrowseContentTab::slot_SearchTermChanged);

   QObject::connect(m_pTableWidget, &DecentTable::signal_SortingChanged,
                    this, &BrowseContentTab::slot_SortingChanged);
}

void BrowseContentTab::timeToUpdate(const std::string& result)
{
   _digital_contents.clear();
   
   if (result.empty()) {
      ShowDigitalContentsGUI();
      return;
   }
   
   auto contents = nlohmann::json::parse(result);
   size_t iSize = contents.size();
   if (iSize > m_i_page_size)
      iSize = m_i_page_size;
   
   _digital_contents.resize(iSize);
   
   for (size_t iIndex = 0; iIndex < iSize; ++iIndex)
   {
      SDigitalContent& cont = _digital_contents[iIndex];
      auto const& json_item = contents[iIndex];
      
      cont.type = DCT::GENERAL;
      cont.author = json_item["author"].get<std::string>();

      cont.synopsis = json_item["synopsis"].get<std::string>();
      cont.URI = json_item["URI"].get<std::string>();
      cont.created = json_item["created"].get<std::string>();
      cont.created = cont.created.substr(0, cont.created.find("T"));
      cont.expiration = json_item["expiration"].get<std::string>();
      cont.size = json_item["size"].get<int>();
      
      if (json_item["times_bought"].is_number()) {
         cont.times_bought = json_item["times_bought"].get<int>();
      } else {
         cont.times_bought = 0;
      }
      
      uint64_t iPrice = json_to_int64(json_item["price"]["amount"]);
      std::string iSymbolId = json_item["price"]["asset_id"];
      cont.price = Globals::instance().asset(iPrice, iSymbolId);

      cont.AVG_rating = json_item["AVG_rating"].get<double>() / 1000;
   }

   if (contents.size() > m_i_page_size)
      set_next_page_iterator(contents[m_i_page_size]["id"].get<std::string>());
   else
      set_next_page_iterator(string());
   
   ShowDigitalContentsGUI();
}

std::string BrowseContentTab::getUpdateCommand()
{
   graphene::chain::ContentObjectPropertyManager type_composer;
   graphene::chain::ContentObjectTypeValue type(graphene::chain::EContentObjectApplication::DecentCore);
   string str_type;
   type.to_string(str_type);

   return   string("search_content ") +
            "\"" + m_strSearchTerm.toStdString() + "\" " +
            "\"" + m_pTableWidget->getSortedColumn() + "\" " +
            "\"\" " +   // user
            "\"\" " +   // region code
            "\"" + next_iterator() + "\" "
            "\"" + str_type + "\" " +
            std::to_string(m_i_page_size + 1);
}

void BrowseContentTab::slot_Details(int iIndex)
{
    if (iIndex < 0 || iIndex >= _digital_contents.size()) {
        throw std::out_of_range("Content index is out of range");
    }

   ContentInfoWidget* pDetailsDialog = new ContentInfoWidget(nullptr, _digital_contents[iIndex]);
   Globals::instance().signal_stackWidgetPush(pDetailsDialog);

   QObject::connect(pDetailsDialog, &ContentInfoWidget::accepted,
                    this, &BrowseContentTab::slot_Bought);
}

void BrowseContentTab::slot_Bought()
{
   Globals::instance().signal_showPurchasedTab();
   Globals::instance().slot_updateAccountBalance();
}

void BrowseContentTab::ShowDigitalContentsGUI() {
   
   m_pTableWidget->setRowCount(_digital_contents.size());

   if (m_pDetailsSignalMapper)
      delete m_pDetailsSignalMapper;
   m_pDetailsSignalMapper = new QSignalMapper(this);
   QObject::connect(m_pDetailsSignalMapper, (void (QSignalMapper::*)(int))&QSignalMapper::mapped,
                    this, &BrowseContentTab::slot_Details);
   
   int index = 0;
   for(SDigitalContent& item: _digital_contents)
   {
      std::string synopsis = unescape_string(item.synopsis);
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
      m_pTableWidget->setItem(index, colIndex,new QTableWidgetItem(QString::fromStdString(item.author)));
      m_pTableWidget->item(index, colIndex)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
      m_pTableWidget->item(index, colIndex)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
      
      // Rating
      colIndex++;
      QString rating = QString::number(item.AVG_rating, 'f', 2);
      m_pTableWidget->setItem(index,colIndex,new QTableWidgetItem(rating));
      m_pTableWidget->item(index, colIndex)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
      m_pTableWidget->item(index, colIndex)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
      
      
      // Size
      colIndex++;
      QString unit = " MB";
      double sizeAdjusted = item.size;
      
      if(item.size > 1024) {
         unit = " GB";
         sizeAdjusted = item.size / 1024.0;
      }
      
      m_pTableWidget->setItem(index, colIndex,new QTableWidgetItem(QString::number(sizeAdjusted, 'f', 2) + unit));
      m_pTableWidget->item(index, colIndex)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
      m_pTableWidget->item(index, colIndex)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
      
      // Price
      colIndex++;
      m_pTableWidget->setItem(index, colIndex, new QTableWidgetItem(item.price.getString().c_str()));

      m_pTableWidget->item(index, colIndex)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
      m_pTableWidget->item(index, colIndex)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);

      // Uploaded
      colIndex++;
      m_pTableWidget->setItem(index, colIndex, new QTableWidgetItem(QString::fromStdString(item.created)));
      m_pTableWidget->item(index, colIndex)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
      m_pTableWidget->item(index, colIndex)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
      
      // Expiration
      colIndex++;
      QDateTime time = QDateTime::fromString(QString::fromStdString(item.expiration), "yyyy-MM-ddTHH:mm:ss");
      std::string e_str = CalculateRemainingTime(QDateTime::currentDateTime(), time);
      
      m_pTableWidget->setItem(index, colIndex, new QTableWidgetItem(QString::fromStdString(e_str)));
      m_pTableWidget->item(index, colIndex)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
      m_pTableWidget->item(index, colIndex)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);

      // Button
      colIndex++;
      DecentButton* info_icon = new DecentButton(m_pTableWidget, DecentButton::TableIcon, DecentButton::Detail);
      info_icon->setEnabled(false);

      QObject::connect(info_icon, &DecentButton::clicked,
                       m_pDetailsSignalMapper, (void (QSignalMapper::*)())&QSignalMapper::map);
      m_pDetailsSignalMapper->setMapping(info_icon, index);

      //info_icon->setAlignment(Qt::AlignCenter);
      m_pTableWidget->setCellWidget(index, colIndex, info_icon);

      ++index;
   }
}

void BrowseContentTab::slot_SearchTermChanged(QString const& strSearchTerm)
{
   m_strSearchTerm = strSearchTerm;
   reset(false);
}
void BrowseContentTab::slot_SortingChanged(int index)
{
   reset();
}
} // end namespace gui_wallet


