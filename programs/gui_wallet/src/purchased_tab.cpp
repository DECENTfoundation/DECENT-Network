/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */

#ifndef STDAFX_H
#include "../stdafx.h"
#endif

#include "purchased_tab.hpp"
#include "gui_wallet_global.hpp"
#include "decent_line_edit.hpp"
#include "decent_button.hpp"
#include "richdialog.hpp"

namespace gui_wallet
{
struct SDigitalContentPurchase : public SDigitalContent
{
   uint32_t total_key_parts = 0;
   uint32_t received_key_parts = 0;
   uint32_t total_download_bytes = 0;
   uint32_t received_download_bytes = 0;
   QString status_text;
};
   
PurchasedTab::PurchasedTab(QWidget* pParent,
                           DecentLineEdit* pFilterLineEdit)
: TabContentManager(pParent)
, m_pExtractSignalMapper(nullptr)
, m_pDetailsSignalMapper(nullptr)
, m_pTableWidget(new DecentTable(this))
, m_iActiveItemIndex(-1)
{
   m_pTableWidget->set_columns({
      {tr("Title"), 30},
      {tr("Size"), 15, "size"},
      {tr("Price"), 15, "price"},
      {tr("Purchased"), 15, "purchased"},
      {tr("Status"), 20},
      {" ", 5},
      {" ", 5}
   });

   // the main layout
   //
   QVBoxLayout* pMainLayout = new QVBoxLayout(this);
   pMainLayout->setContentsMargins(0, 0, 0, 0);
   pMainLayout->setSpacing(0);
   pMainLayout->addWidget(m_pTableWidget);

   setLayout(pMainLayout);

   QObject::connect(pFilterLineEdit, &QLineEdit::textChanged,
                    this, &PurchasedTab::slot_SearchTermChanged);
   setFilterWidget(pFilterLineEdit);

   QObject::connect(m_pTableWidget, &DecentTable::signal_SortingChanged,
                    this, &PurchasedTab::slot_SortingChanged);

   QObject::connect(m_pTableWidget, &DecentTable::cellClicked,
                    this, &PurchasedTab::slot_cellClicked);

   setRefreshTimer(5000);
}

//
// it is important to have a constructor/destructor body in cpp
// when class has forward declared members

PurchasedTab::~PurchasedTab() = default;

void PurchasedTab::timeToUpdate(const std::string& result)
{
   _current_content.clear();
   if (result.empty()) {
      return;
   }
   
   auto contents = nlohmann::json::parse(result);

   size_t iSize = contents.size();
   if (iSize > m_i_page_size)
      iSize = m_i_page_size;

   _current_content.reserve(iSize);
   
   for (int iIndex = 0; iIndex < iSize; ++iIndex)
   {
      auto content = contents[iIndex];

      std::string URI = content["URI"].get<std::string>();

      // Create SDigitalContent object
      
      _current_content.push_back(SDigitalContentPurchase());
      SDigitalContentPurchase& contentObject = _current_content.back();
      
      if (content["delivered"].get<bool>()) {
         contentObject.type = DCT::BOUGHT;
      } else {
         contentObject.type = DCT::WAITING_DELIVERY;
      }
      
      contentObject.author = content["author_account"].get<std::string>();
      uint64_t iPrice = json_to_int64(content["price"]["amount"]);
      std::string iSymbolId = content["price"]["asset_id"];
      contentObject.price = Globals::instance().asset(iPrice, iSymbolId);

      contentObject.synopsis = content["synopsis"].get<std::string>();
      contentObject.URI = content["URI"].get<std::string>();
      contentObject.created = content["created"].get<std::string>();
      contentObject.created = contentObject.created.substr(0, contentObject.created.find("T"));
      contentObject.purchased_time = content["expiration_or_delivery_time"].get<std::string>();

      contentObject.size = content["size"].get<int>();
      contentObject.id = content["id"].get<std::string>();
      contentObject.hash = content["hash"].get<std::string>();

      contentObject.total_key_parts = content["total_key_parts"].get<int>();
      contentObject.received_key_parts  = content["received_key_parts"].get<int>();
      contentObject.total_download_bytes  = content["total_download_bytes"].get<int>();
      contentObject.received_download_bytes  = content["received_download_bytes"].get<int>();

      QString status_text = tr("Keys") + ": " + QString::number(contentObject.received_key_parts) + "/" + QString::number(contentObject.total_key_parts);

      if (DCT::WAITING_DELIVERY == contentObject.type) {
         status_text = tr("Waiting for key delivery");
      } else {
         status_text = status_text + tr(" ") + QString::fromStdString(content["status_text"].get<std::string>());
      }
      contentObject.status_text = status_text;
      
      if (content["times_bought"].is_number()) {
         contentObject.times_bought = content["times_bought"].get<int>();
      } else {
         contentObject.times_bought = 0;
      }

      contentObject.AVG_rating = content["AVG_rating"].get<uint64_t>() / 1000;
   }


   if (contents.size() > m_i_page_size)
      set_next_page_iterator(contents[m_i_page_size]["id"].get<std::string>());
   else
      set_next_page_iterator(std::string());

   ShowDigitalContentsGUI();
}

void PurchasedTab::ShowDigitalContentsGUI()
{
   m_pTableWidget->setRowCount(_current_content.size());

   if (m_pExtractSignalMapper)
      delete m_pExtractSignalMapper;
   m_pExtractSignalMapper = new QSignalMapper(this);  // the last one will be deleted thanks to it's parent
   QObject::connect(m_pExtractSignalMapper, (void (QSignalMapper::*)(int))&QSignalMapper::mapped,
                    this, &PurchasedTab::slot_ExtractPackage);

   if (m_pDetailsSignalMapper)
      delete m_pDetailsSignalMapper;
   m_pDetailsSignalMapper = new QSignalMapper(this);  // similar to extract signal handler
   QObject::connect(m_pDetailsSignalMapper, (void (QSignalMapper::*)(int))&QSignalMapper::mapped,
                    this, &PurchasedTab::slot_Details);

   for (int iIndex = 0; iIndex < _current_content.size(); ++iIndex)
   {
      SDigitalContentPurchase& contentObject = _current_content[iIndex];

      std::string synopsis = unescape_string(contentObject.synopsis);
      std::replace(synopsis.begin(), synopsis.end(), '\t', ' '); // JSON does not like tabs :(
      std::replace(synopsis.begin(), synopsis.end(), '\n', ' '); // JSON does not like tabs :(

      graphene::chain::ContentObjectPropertyManager synopsis_parser(synopsis);
      std::string title = synopsis_parser.get<graphene::chain::ContentObjectTitle>();

      DecentButton* info_icon = new DecentButton(m_pTableWidget, DecentButton::TableIcon, DecentButton::Detail);
      info_icon->setToolTip(tr("Details"));
      info_icon->setEnabled(false);
      //info_icon->setAlignment(Qt::AlignCenter);
      m_pTableWidget->setCellWidget(iIndex, 6, info_icon);

      QObject::connect(info_icon, &DecentButton::clicked,
                       m_pDetailsSignalMapper, (void (QSignalMapper::*)())&QSignalMapper::map);
      m_pDetailsSignalMapper->setMapping(info_icon, iIndex);


      m_pTableWidget->setItem(iIndex, 0, new QTableWidgetItem(QString::fromStdString(title)));
      m_pTableWidget->setItem(iIndex, 1, new QTableWidgetItem(QString::number(contentObject.size) + tr(" MB")));
      m_pTableWidget->setItem(iIndex, 2, new QTableWidgetItem(contentObject.price.getString()));

      std::string purchase_date = contentObject.purchased_time.substr(0, contentObject.purchased_time.find("T"));
      m_pTableWidget->setItem(iIndex, 3, new QTableWidgetItem(convertDateToLocale(purchase_date)));

      uint32_t total_key_parts = contentObject.total_key_parts;
      uint32_t received_key_parts  = contentObject.received_key_parts;
      uint32_t total_download_bytes  = contentObject.total_download_bytes;
      uint32_t received_download_bytes  = contentObject.received_download_bytes;

      bool is_delivered = true;
      if (DCT::WAITING_DELIVERY == contentObject.type)
         is_delivered = false;
      
      m_pTableWidget->setItem(iIndex, 4, new QTableWidgetItem(contentObject.status_text));
      
      if (total_key_parts == 0) {
         total_key_parts = 1;
      }
      
      if (total_download_bytes == 0) {
         total_download_bytes = 1;
      }

      double progress = (0.1 * received_key_parts) / total_key_parts + (0.9 * received_download_bytes) / total_download_bytes;
      progress *= 100; // Percent

      if ((received_download_bytes < total_download_bytes) || !is_delivered) {
         m_pTableWidget->setItem(iIndex, 5, new QTableWidgetItem(QString::number(progress) + "%"));
         m_pTableWidget->item(iIndex, 5)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
         m_pTableWidget->item(iIndex, 5)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
      } else {

         DecentButton* extract_icon = new DecentButton(m_pTableWidget, DecentButton::TableIcon, DecentButton::Export);
         extract_icon->setToolTip(tr("Download content"));
         extract_icon->setEnabled(false);
         //extract_icon->setAlignment(Qt::AlignCenter);

         QObject::connect(extract_icon, &DecentButton::clicked,
                          m_pExtractSignalMapper, (void (QSignalMapper::*)())&QSignalMapper::map);
         m_pExtractSignalMapper->setMapping(extract_icon, iIndex);

         m_pTableWidget->setCellWidget(iIndex, 5, extract_icon);
      }

      for(int j = 0; j < m_pTableWidget->columnCount() - 2; ++j)
      {
         auto* item = m_pTableWidget->item(iIndex, j);
         if (item) {
            m_pTableWidget->item(iIndex, j)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            m_pTableWidget->item(iIndex, j)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
         }
      }
   }
}

std::string PurchasedTab::getUpdateCommand()
{
   auto& global_instance = gui_wallet::Globals::instance();
   const std::string& str_current_username = global_instance.getCurrentUser();

   if ( str_current_username.empty() )
   {
      return std::string();
   } // if key not imported

   return   "search_my_purchases "
            "\"" + str_current_username + "\" "
            "\"" + m_strSearchTerm.toStdString() + "\" "
            "\"" + m_pTableWidget->getSortedColumn() + "\" "
            "\"" + next_iterator() + "\" "
            + std::to_string(m_i_page_size + 1);
}

void PurchasedTab::slot_ExtractionDirSelected(QString const& path) {

   if (m_iActiveItemIndex < 0 || m_iActiveItemIndex >= _current_content.size())
      throw std::out_of_range("Content index is out of range");

   std::string key, dummy;
   std::string message;

   std::string strExtractID = _current_content[m_iActiveItemIndex].id;
   std::string strExtractHash = _current_content[m_iActiveItemIndex].hash;
   
   auto& global_instance = gui_wallet::Globals::instance();
   std::string str_current_username = global_instance.getCurrentUser();
   
   try {
      key = Globals::instance().runTask("restore_encryption_key \"" + str_current_username + "\" \"" + strExtractID + "\"");
      
      dummy = Globals::instance().runTask("extract_package \"" + strExtractHash + "\" \"" + path.toStdString() + "\" " + key);
      
      if (dummy.find("exception:") != std::string::npos) {
         message = dummy;
      }
   }
   catch (const std::exception& ex) {
      message = ex.what();
   }
   catch(const fc::exception& ex) {
      message = ex.what();
   }

   ShowMessageBox(message);
}

void PurchasedTab::slot_SearchTermChanged(const QString& strSearchTerm)
{
   m_strSearchTerm = strSearchTerm;
   reset(false);
}

void PurchasedTab::slot_ExtractPackage(int iIndex) {

   if (iIndex < 0 || iIndex >= _current_content.size())
      throw std::out_of_range("Content index is out of range");

   m_iActiveItemIndex = iIndex;

   QFileDialog* pFileDialog = new QFileDialog();
   QObject::connect(pFileDialog, &QFileDialog::fileSelected,
                    this, &PurchasedTab::slot_ExtractionDirSelected);

   pFileDialog->setFileMode(QFileDialog::Directory);
   pFileDialog->setOptions(QFileDialog::ShowDirsOnly);
   pFileDialog->setOptions(QFileDialog::DontUseNativeDialog);
   pFileDialog->setAttribute(Qt::WA_DeleteOnClose);
   pFileDialog->setLabelText(QFileDialog::Accept, tr("Extract"));

#ifdef _MSC_VER
   int height = pFileDialog->style()->pixelMetric(QStyle::PM_TitleBarHeight);
   pFileDialog->setWindowIcon(height > 32 ? QIcon(":/icon/images/windows_decent_icon_32x32.png")
      : QIcon(":/icon/images/windows_decent_icon_16x16.png"));
#endif

   pFileDialog->open();
}

void PurchasedTab::slot_Details(int iIndex)
{
   if (iIndex < 0 || iIndex >= _current_content.size())
      throw std::out_of_range("Content index is out of range");

   ContentReviewWidget* pDetailsDialog = new ContentReviewWidget(nullptr, _current_content[iIndex]);
   Globals::instance().signal_stackWidgetPush(pDetailsDialog);
}

void PurchasedTab::slot_cellClicked(int row, int /*col*/)
{
   if (row < 0 || row >= _current_content.size()) {
      throw std::out_of_range("Content index is out of range");
   }

   slot_Details(row);
}

void PurchasedTab::ShowMessageBox(const std::string& message)
{
   if (message.empty())
      gui_wallet::ShowMessageBox(tr("Success"),
                                 tr("Package was successfully extracted"));
   else
      gui_wallet::ShowMessageBox(tr("Error"),
                                 tr("Failed to extract package"),
                                 QObject::tr(message.c_str()));
}

void PurchasedTab::slot_SortingChanged(int index)
{
   reset();
}

}  // end namespace gui_wallet
