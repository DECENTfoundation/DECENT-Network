#include "purchased_tab.hpp"
#include <QHeaderView>
#include <QPushButton>
#include <QFileDialog>
#include <iostream>
#include <graphene/chain/config.hpp>
#include "json.hpp"
#include <QMessageBox>
#include "ui_wallet_functions.hpp"
#include "decent_wallet_ui_gui_contentdetailsgeneral.hpp"

using namespace gui_wallet;
using namespace nlohmann;


PurchasedTab::PurchasedTab() {
   
   m_pTableWidget.set_columns({
      {"Title", 30},
      {"Rating", 10},
      {"Size", 10},
      {"Price", 10},
      {"Created", 15},
      {"Status", 20},
      {"", 5},
      {" ", 5}
   });
   
   
   
   QHBoxLayout* search_lay = new QHBoxLayout();
   
   m_filterLineEditer.setPlaceholderText(QString("Enter the term to search in title and description"));
   m_filterLineEditer.setStyleSheet("border: 0");
   m_filterLineEditer.setFixedHeight(40);
   m_filterLineEditer.setAttribute(Qt::WA_MacShowFocusRect, 0);
   
   QPixmap image(":/icon/images/search.svg");
   
   QLabel* search_label = new QLabel();
   search_label->setSizeIncrement(100,40);
   search_label->setPixmap(image);
   
   
   
   search_lay->setContentsMargins(40, 0, 0, 0);
   search_lay->addWidget(search_label);
   search_lay->addWidget(&m_filterLineEditer);
   
   m_main_layout.setContentsMargins(0, 0, 0, 0);
   m_main_layout.addLayout(search_lay);
   m_main_layout.addWidget(&m_pTableWidget);
   
   connect(this, SIGNAL(showMessageBox(std::string)), this, SLOT(showMessageBoxSlot(std::string)));
   
   _isExtractingPackage = false;
   connect(&_fileDialog, SIGNAL(fileSelected(const QString&)), this, SLOT(extractionDirSelected(const QString&)));
   
   _fileDialog.setFileMode(QFileDialog::Directory);
   _fileDialog.setOptions(QFileDialog::ShowDirsOnly);
   //_fileDialog.setAttribute(Qt::WA_DeleteOnClose);
   
   _fileDialog.setOptions(QFileDialog::DontUseNativeDialog);
   
   _fileDialog.setLabelText(QFileDialog::Accept, "Extract");
   setLayout(&m_main_layout);
   
}



void PurchasedTab::timeToUpdate(const std::string& result) {
   m_pTableWidget.setRowCount(0);
   
   if (result.empty()) {
      return;
   }
   
   auto contents = json::parse(result);
   m_pTableWidget.setRowCount(contents.size());
   
   QPixmap info_image(":/icon/images/pop_up.png");
   QPixmap extract_image(":/icon/images/export.png");
   QFont bold_font( "Open Sans Bold", 14, QFont::Bold);
   
   _current_content.clear();
   _current_content.reserve(contents.size());
   
   for (int i = 0; i < contents.size(); ++i) {
      
      auto content = contents[i];
      
      
      std::string time = contents[i]["expiration_time"].get<std::string>();
      
      std::string synopsis = unescape_string(contents[i]["synopsis"].get<std::string>());
      std::replace(synopsis.begin(), synopsis.end(), '\t', ' '); // JSON does not like tabs :(
      std::replace(synopsis.begin(), synopsis.end(), '\n', ' '); // JSON does not like tabs :(
      
      try {
         auto synopsis_parsed = json::parse(synopsis);
         synopsis = synopsis_parsed["title"].get<std::string>();
      } catch (...) {}
      
      double rating = contents[i]["rating"].get<double>() / 1000;
      uint64_t size = contents[i]["size"].get<int>();
      
      
      double price = 0;
      if (contents[i]["price"]["amount"].is_number()){
         price =  contents[i]["price"]["amount"].get<double>();
      } else {
         price =  std::stod(contents[i]["price"]["amount"].get<std::string>());
      }
      price /= GRAPHENE_BLOCKCHAIN_PRECISION;
      
      std::string expiration_or_delivery_time = contents[i]["expiration_or_delivery_time"].get<std::string>();
      std::string URI = contents[i]["URI"].get<std::string>();
      
      _current_content.push_back(SDigitalContent());
      SDigitalContent& contentObject = _current_content.back();
      
      std::string dcresult;
      RunTask("get_content \"" + URI + "\"", dcresult);
      
      auto dcontent_json = json::parse(dcresult);
      
      if (content["delivered"].get<bool>()) {
         contentObject.type = DCT::BOUGHT;
      } else {
         contentObject.type = DCT::WAITING_DELIVERY;
      }
      
      contentObject.author = dcontent_json["author"].get<std::string>();
      contentObject.price.asset_id = dcontent_json["price"]["asset_id"].get<std::string>();
      contentObject.synopsis = dcontent_json["synopsis"].get<std::string>();
      contentObject.URI = dcontent_json["URI"].get<std::string>();
      contentObject.created = dcontent_json["created"].get<std::string>();
      contentObject.expiration = dcontent_json["expiration"].get<std::string>();
      contentObject.size = dcontent_json["size"].get<int>();
      
      if (dcontent_json["times_bougth"].is_number()) {
         contentObject.times_bougth = dcontent_json["times_bougth"].get<int>();
      } else {
         contentObject.times_bougth = 0;
      }
      
      
      if (dcontent_json["price"]["amount"].is_number()){
         contentObject.price.amount =  dcontent_json["price"]["amount"].get<double>();
      } else {
         contentObject.price.amount =  std::stod(dcontent_json["price"]["amount"].get<std::string>());
      }
      
      contentObject.price.amount /= GRAPHENE_BLOCKCHAIN_PRECISION;
      contentObject.AVG_rating = dcontent_json["AVG_rating"].get<double>() / 1000;
      
      
      
      EventPassthrough<DecentSmallButton>* info_icon = new EventPassthrough<DecentSmallButton>(":/icon/images/pop_up.png", ":/icon/images/pop_up1.png");
      info_icon->setProperty("id", QVariant::fromValue(i));
      //info_icon->setPixmap(info_image);
      info_icon->setAlignment(Qt::AlignCenter);
      connect(info_icon, SIGNAL(clicked()), this, SLOT(show_content_popup()));
      m_pTableWidget.setCellWidget(i, 7, info_icon);
      
      
      
      m_pTableWidget.setItem(i, 0, new QTableWidgetItem(QString::fromStdString(synopsis)));
      m_pTableWidget.setItem(i, 1, new QTableWidgetItem(QString::number(rating)));
      m_pTableWidget.setItem(i, 2, new QTableWidgetItem(QString::number(size) + tr(" MB")));
      m_pTableWidget.setItem(i, 3, new QTableWidgetItem(QString::number(price) + " DCT"));
      
      
      std::string s_time = time.substr(0, time.find("T"));
      
      m_pTableWidget.setItem(i, 4, new QTableWidgetItem(QString::fromStdString(s_time)));
      
      
      std::string download_status_str;
      RunTask("get_download_status \"" + gui_wallet::GlobalEvents::instance().getCurrentUser() + "\" \"" + URI + "\"", download_status_str);
      
      auto download_status = json::parse(download_status_str);
      
      
      int total_key_parts = download_status["total_key_parts"].get<int>();
      int received_key_parts  = download_status["received_key_parts"].get<int>();
      int total_download_bytes  = download_status["total_download_bytes"].get<int>();
      int received_download_bytes  = download_status["received_download_bytes"].get<int>();
      
      
      QString status_text = tr("Keys: ") + QString::number(received_key_parts) + "/" + QString::number(total_key_parts);
      
      bool is_delivered = content["delivered"].get<bool>();
      if (!is_delivered) {
         status_text = "Waiting for key delivery";
      } else {
         status_text = status_text + tr(" ") + QString::fromStdString(download_status["status_text"].get<std::string>());
      }
      
      m_pTableWidget.setItem(i, 5, new QTableWidgetItem(status_text));
      
      if (total_key_parts == 0) {
         total_key_parts = 1;
      }
      
      if (total_download_bytes == 0) {
         total_download_bytes = 1;
      }
      
      
      double progress = (0.1 * received_key_parts) / total_key_parts + (0.9 * received_download_bytes) / total_download_bytes;
      progress *= 100; // Percent
      
      if ((received_download_bytes < total_download_bytes) || !is_delivered) {
         m_pTableWidget.setItem(i, 6, new QTableWidgetItem(QString::number(progress) + "%"));
         m_pTableWidget.item(i, 6)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
         m_pTableWidget.item(i, 6)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
      } else {
         
         EventPassthrough<DecentSmallButton>* extract_icon = new EventPassthrough<DecentSmallButton>(":/icon/images/export.png", ":/icon/images/export1.png");
         
         
         extract_icon->setProperty("id", QVariant::fromValue(QString::fromStdString(content["id"].get<std::string>())));
         extract_icon->setProperty("hash", QVariant::fromValue(QString::fromStdString(dcontent_json["_hash"].get<std::string>())));
         extract_icon->setProperty("URI", QVariant::fromValue(QString::fromStdString(content["URI"].get<std::string>())));
         
         
         extract_icon->setAlignment(Qt::AlignCenter);
         
         connect(extract_icon, SIGNAL(clicked()), this, SLOT(extractPackage()));
         m_pTableWidget.setCellWidget(i, 6, extract_icon);
      }
      
      
      for(int j = 0; j < m_pTableWidget.columnCount() - 2; ++j) {
         auto* item = m_pTableWidget.item(i, j);
         if (item) {
            m_pTableWidget.item(i, j)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            m_pTableWidget.item(i, j)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
         }
      }
      
      
   }
}


std::string PurchasedTab::getUpdateCommand() {
   auto& global_instance = gui_wallet::GlobalEvents::instance();
   std::string str_current_username = global_instance.getCurrentUser();
   
   if ( str_current_username == "" ) {
      return "";
   } // if key not imported
   
   return "get_buying_history_objects_by_consumer_term "
           "\"" + str_current_username +"\" "
           "\"" + m_filterLineEditer.text().toStdString() +"\"";

}

void PurchasedTab::extractionDirSelected(const QString& path) {
   
   std::string id = _fileDialog.property("id").toString().toStdString();
   std::string URI = _fileDialog.property("URI").toString().toStdString();
   std::string hash = _fileDialog.property("hash").toString().toStdString();

   std::string key, dummy;
   
   std::string message;
   
   try {
      RunTask("restore_encryption_key \"" + id + "\"", key);
      
      RunTask("extract_package \"" + hash + "\" \"" + path.toStdString() + "\" " + key, dummy);
      
      if (dummy.find("exception:") != std::string::npos) {
         message = dummy;
      }
   } catch (const std::exception& ex) {
      message = ex.what();
   }
   
   emit showMessageBox(message);
   _isExtractingPackage = false;
}

void PurchasedTab::extractPackage() {
   std::cout << "isExtractingPackage = " << _isExtractingPackage << std::endl;
   if (_isExtractingPackage) {
      return;
   }
   
   _isExtractingPackage = true;
   
   
   QPushButton* btn = (QPushButton*)sender();
   
   _fileDialog.setProperty("id", btn->property("id"));
   _fileDialog.setProperty("hash", btn->property("hash"));
   _fileDialog.setProperty("URI", btn->property("URI"));
   
   _fileDialog.open();
   /*
   if (!) {
      _isExtractingPackage = false;
      return;
   }
    */
   
}


void PurchasedTab::showMessageBoxSlot(std::string message) {
   if (message.empty()) {
      _msgBox.setWindowTitle("Success");
      _msgBox.setText(tr("Package was successfully extracted"));
      _msgBox.open();
      
   } else {
      _msgBox.setWindowTitle("Error");
      _msgBox.setText(tr("Failed to extract package"));
      _msgBox.setDetailedText(QObject::tr(message.c_str()));
      _msgBox.open();
   }
}
           
void PurchasedTab::show_content_popup() {
   if (_isExtractingPackage) {
      return;
   }
   
   QPushButton* btn = (QPushButton*)sender();
   int id = btn->property("id").toInt();
   if (id < 0 || id >= _current_content.size()) {
      throw std::out_of_range("Content index is our of range");
   }
   
   if (nullptr == _details_dialog)
      delete _details_dialog;
   _details_dialog = new ContentDetailsBase();
   _details_dialog->execCDB(_current_content[id]);
}

