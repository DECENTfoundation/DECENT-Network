#include "stdafx.h"

#include "purchased_tab.hpp"
#include "gui_wallet_mainwindow.hpp"
#include "decent_wallet_ui_gui_contentdetailsgeneral.hpp"
#include "gui_design.hpp"
#include "gui_wallet_global.hpp"
#include "json.hpp"

#ifndef _MSC_VER
#include <QHeaderView>
#include <QPushButton>
#include <QFileDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSignalMapper>
#include <graphene/chain/config.hpp>
#include <graphene/chain/content_object.hpp>
#endif

namespace gui_wallet
{
PurchasedTab::PurchasedTab(QWidget* pParent)
: TabContentManager(pParent)
, m_pExtractSignalMapper(nullptr)
, m_pDetailsSignalMapper(nullptr)
, m_pTableWidget(new DecentTable(this))
, m_iActiveItemIndex(-1)
{
   m_pTableWidget->set_columns({
      {"Title", 30},
      {"Size", 15, "size"},
      {"Price", 15, "price"},
      {"Created", 15, "created"},
      {"Status", 20},
      {"", 5},
      {" ", 5}
   });

   // search layout
   //
   QHBoxLayout* search_layout = new QHBoxLayout();

   QLineEdit* pfilterLineEditor = new QLineEdit(this);
   pfilterLineEditor->setPlaceholderText(QString("Search Content"));
   pfilterLineEditor->setStyleSheet(d_lineEdit);
   pfilterLineEditor->setFixedHeight(54);
   pfilterLineEditor->setAttribute(Qt::WA_MacShowFocusRect, 0);
   QObject::connect(pfilterLineEditor, &QLineEdit::textChanged,
                    this, &PurchasedTab::slot_SearchTermChanged);
   
   QPixmap image(icon_search);
   
   QLabel* search_label = new QLabel();
   search_label->setSizeIncrement(100,40);
   search_label->setPixmap(image);

   search_layout->setContentsMargins(40, 0, 0, 0);
   search_layout->addWidget(search_label);
   search_layout->addWidget(pfilterLineEditor);

   // the main layout
   //
   QVBoxLayout* pMainLayout = new QVBoxLayout(this);
   pMainLayout->setContentsMargins(0, 0, 0, 0);
   pMainLayout->setSpacing(0);
   pMainLayout->addLayout(search_layout);
   pMainLayout->addWidget(m_pTableWidget);

   setLayout(pMainLayout);
}

void PurchasedTab::timeToUpdate(const std::string& result) {
   m_pTableWidget->setRowCount(0);
   
   if (result.empty()) {
      return;
   }
   
   auto contents = nlohmann::json::parse(result);
   m_pTableWidget->setRowCount(contents.size());
   
   _current_content.clear();
   _current_content.reserve(contents.size());

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
   
   for (int iIndex = 0; iIndex < contents.size(); ++iIndex)
   {
      auto content = contents[iIndex];
      
      std::string time = content["created"].get<std::string>();
      
      std::string synopsis = unescape_string(content["synopsis"].get<std::string>());
      std::replace(synopsis.begin(), synopsis.end(), '\t', ' '); // JSON does not like tabs :(
      std::replace(synopsis.begin(), synopsis.end(), '\n', ' '); // JSON does not like tabs :(

      graphene::chain::ContentObjectPropertyManager synopsis_parser(synopsis);
      std::string title = synopsis_parser.get<graphene::chain::ContentObjectTitle>();
      
      //double rating = content["average_rating"].get<double>() / 1000;
      uint64_t size = content["size"].get<int>();
      
      
      double price = 0;
      if (content["price"]["amount"].is_number()){
         price =  content["price"]["amount"].get<double>();
      } else {
         price =  std::stod(content["price"]["amount"].get<std::string>());
      }
      price /= GRAPHENE_BLOCKCHAIN_PRECISION;
      
      std::string expiration_or_delivery_time = content["expiration_or_delivery_time"].get<std::string>();
      std::string URI = content["URI"].get<std::string>();
      

      // Create SDigitalContent object
      
      _current_content.push_back(SDigitalContent());
      SDigitalContent& contentObject = _current_content.back();
      
      if (content["delivered"].get<bool>()) {
         contentObject.type = DCT::BOUGHT;
      } else {
         contentObject.type = DCT::WAITING_DELIVERY;
      }
      
      contentObject.author = content["author_account"].get<std::string>();
      contentObject.price.asset_id = content["price"]["asset_id"].get<std::string>();
      contentObject.synopsis = content["synopsis"].get<std::string>();
      contentObject.URI = content["URI"].get<std::string>();
      contentObject.created = content["created"].get<std::string>();
      contentObject.expiration = content["expiration"].get<std::string>();
      contentObject.size = content["size"].get<int>();
      contentObject.id = content["id"].get<std::string>();
      contentObject.hash = content["hash"].get<std::string>();
      
      if (content["times_bougth"].is_number()) {
         contentObject.times_bougth = content["times_bougth"].get<int>();
      } else {
         contentObject.times_bougth = 0;
      }

      if (content["price"]["amount"].is_number()){
         contentObject.price.amount =  content["price"]["amount"].get<double>();
      } else {
         contentObject.price.amount =  std::stod(content["price"]["amount"].get<std::string>());
      }
      
      contentObject.price.amount /= GRAPHENE_BLOCKCHAIN_PRECISION;
      contentObject.AVG_rating = content["average_rating"].get<double>() / 1000;

      EventPassthrough<DecentSmallButton>* info_icon = new EventPassthrough<DecentSmallButton>(icon_popup, icon_popup_white, m_pTableWidget);
      info_icon->setAlignment(Qt::AlignCenter);
      m_pTableWidget->setCellWidget(iIndex, 6, info_icon);

      QObject::connect(info_icon, &DecentSmallButton::clicked,
                       m_pDetailsSignalMapper, (void (QSignalMapper::*)())&QSignalMapper::map);
      m_pDetailsSignalMapper->setMapping(info_icon, iIndex);


      m_pTableWidget->setItem(iIndex, 0, new QTableWidgetItem(QString::fromStdString(title)));
      m_pTableWidget->setItem(iIndex, 1, new QTableWidgetItem(QString::number(size) + tr(" MB")));
      if(price)
      {
         m_pTableWidget->setItem(iIndex, 2, new QTableWidgetItem(QString::number(price, 'f', 4) + " DCT"));
      }
      else
      {
         m_pTableWidget->setItem(iIndex, 2, new QTableWidgetItem("Free"));
      }

      std::string s_time = time.substr(0, time.find("T"));
      
      m_pTableWidget->setItem(iIndex, 3, new QTableWidgetItem(QString::fromStdString(s_time)));
      

      int total_key_parts = content["total_key_parts"].get<int>();
      int received_key_parts  = content["received_key_parts"].get<int>();
      int total_download_bytes  = content["total_download_bytes"].get<int>();
      int received_download_bytes  = content["received_download_bytes"].get<int>();
      
      
      QString status_text = tr("Keys: ") + QString::number(received_key_parts) + "/" + QString::number(total_key_parts);
      
      bool is_delivered = content["delivered"].get<bool>();
      if (!is_delivered) {
         status_text = "Waiting for key delivery";
      } else {
         status_text = status_text + tr(" ") + QString::fromStdString(content["status_text"].get<std::string>());
      }
      
      m_pTableWidget->setItem(iIndex, 4, new QTableWidgetItem(status_text));
      
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

         EventPassthrough<DecentSmallButton>* extract_icon = new EventPassthrough<DecentSmallButton>(icon_export, icon_export_white, m_pTableWidget);
         extract_icon->setAlignment(Qt::AlignCenter);


         QObject::connect(extract_icon, &DecentSmallButton::clicked,
                          m_pExtractSignalMapper, (void (QSignalMapper::*)())&QSignalMapper::map);
         m_pExtractSignalMapper->setMapping(extract_icon, iIndex);

         m_pTableWidget->setCellWidget(iIndex, 5, extract_icon);
      }

      for(int j = 0; j < m_pTableWidget->columnCount() - 2; ++j) {
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
   std::string str_current_username = global_instance.getCurrentUser();

   if ( str_current_username == "" )
   {
      return "";
   } // if key not imported

   return   "search_my_purchases "
            "\"" + str_current_username + "\" "
            "\"" + m_strSearchTerm.toStdString() + "\" "
            "\"" + m_pTableWidget->getSortedColumn() + "\"";
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
      RunTask("restore_encryption_key \"" + str_current_username + "\" \"" + strExtractID + "\"", key);
      
      RunTask("extract_package \"" + strExtractHash + "\" \"" + path.toStdString() + "\" " + key, dummy);
      
      if (dummy.find("exception:") != std::string::npos) {
         message = dummy;
      }
   } catch (const std::exception& ex) {
      message = ex.what();
   }

   ShowMessageBox(message);
}

void PurchasedTab::slot_SearchTermChanged(QString const& strSearchTerm)
{
   m_strSearchTerm = strSearchTerm;
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
   pFileDialog->setLabelText(QFileDialog::Accept, "Extract");

   pFileDialog->open();
}

void PurchasedTab::slot_Details(int iIndex)
{
   if (iIndex < 0 || iIndex >= _current_content.size())
      throw std::out_of_range("Content index is out of range");

   // content details dialog is ugly, needs to be rewritten
   ContentDetailsBase* pDetailsDialog = new ContentDetailsBase(nullptr);
   pDetailsDialog->execCDB(_current_content[iIndex], true);
   pDetailsDialog->setAttribute(Qt::WA_DeleteOnClose);
   pDetailsDialog->open();
}

void PurchasedTab::ShowMessageBox(std::string const& message)
{
   if (message.empty())
      gui_wallet::ShowMessageBox("Success",
                                 "Package was successfully extracted");
   else
      gui_wallet::ShowMessageBox("Error",
                                 "Failed to extract package",
                                 QObject::tr(message.c_str()));
}
}  // end namespace gui_wallet


