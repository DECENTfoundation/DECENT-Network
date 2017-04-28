#include "stdafx.h"

#include "gui_wallet_global.hpp"
#include "browse_content_tab.hpp"
#include "decent_wallet_ui_gui_contentdetailsgeneral.hpp"

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
, m_pDetailsSignalMapper(nullptr)
{
   m_pTableWidget->set_columns({
      {tr("Title"), 20},
      {tr("Author"), 15, "author"},
      {tr("Rating"), 5, "rating"},
      {tr("Size"), 5, "size"},
      {tr("Price"), 5, "price"},
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

   QLabel* pLabelSearchIcon = new QLabel(this);
   QPixmap px_search_icon(icon_search);
   pLabelSearchIcon->setPixmap(px_search_icon);

   QLineEdit* pSearchTerm = new QLineEdit(this);
   pSearchTerm->setPlaceholderText(tr("Search Content"));
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

   QObject::connect(m_pTableWidget, &DecentTable::signal_SortingChanged,
                    this, &BrowseContentTab::slot_SortingChanged);
}

void BrowseContentTab::timeToUpdate(const std::string& result) {
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
      cont.expiration = json_item["expiration"].get<std::string>();
      cont.size = json_item["size"].get<int>();
      
      if (json_item["times_bought"].is_number()) {
         cont.times_bought = json_item["times_bought"].get<int>();
      } else {
         cont.times_bought = 0;
      }
      
      uint64_t iPrice = json_to_int64(json_item["price"]["amount"]);
      cont.price = Globals::instance().asset(iPrice);

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
   return   string("search_content ") +
            "\"" + m_strSearchTerm.toStdString() + "\" " +
            "\"" + m_pTableWidget->getSortedColumn() + "\" " +
            "\"\" " +   // user
            "\"\" " +   // region code
            "\"" + next_iterator() + "\" " +
            std::to_string(m_i_page_size + 1);
}

void BrowseContentTab::slot_Details(int iIndex)
{
    if (iIndex < 0 || iIndex >= _digital_contents.size()) {
        throw std::out_of_range("Content index is out of range");
    }

   // content details dialog is ugly, needs to be rewritten
   ContentDetailsGeneral* pDetailsDialog = new ContentDetailsGeneral(nullptr);
   QObject::connect(pDetailsDialog, &ContentDetailsGeneral::ContentWasBought,
                    this, &BrowseContentTab::slot_Bought);
   pDetailsDialog->execCDD(_digital_contents[iIndex], true);
   pDetailsDialog->setAttribute(Qt::WA_DeleteOnClose);
   pDetailsDialog->open();
}

void BrowseContentTab::slot_Bought()
{
   Globals::instance().signal_showPurchasedTab();
   Globals::instance().updateAccountBalance();
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
      QString rating = QString::number(item.AVG_rating, 'g', 2);
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
      
      m_pTableWidget->setItem(index, colIndex,new QTableWidgetItem(QString::number(sizeAdjusted, 'g', 2) + unit));
      m_pTableWidget->item(index, colIndex)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
      m_pTableWidget->item(index, colIndex)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
      
      // Price
      colIndex++;
      m_pTableWidget->setItem(index, colIndex, new QTableWidgetItem(item.price.getString().c_str()));

      m_pTableWidget->item(index, colIndex)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
      m_pTableWidget->item(index, colIndex)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);

      // Uploaded
      colIndex++;
      std::string created_str = item.created.substr(0, 10);
      m_pTableWidget->setItem(index, colIndex, new QTableWidgetItem(QString::fromStdString(created_str)));
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
      DecentButton* info_icon = new DecentButton(m_pTableWidget, icon_popup, icon_popup_white);

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
}
void BrowseContentTab::slot_SortingChanged(int index)
{
   reset();
}
} // end namespace gui_wallet


