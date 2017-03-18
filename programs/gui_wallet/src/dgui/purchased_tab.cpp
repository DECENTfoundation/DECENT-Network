/*
 *	File: decent_wallet_ui_gui_purchasedtab.hpp
 *
 *	Created on: 11 Feb 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */

#include "purchased_tab.hpp"
#include <QHeaderView>
#include <QPushButton>
#include <QFileDialog>
#include <iostream>
#include <graphene/chain/config.hpp>
#include "json.hpp"
#include <QMessageBox>


using namespace gui_wallet;
using namespace nlohmann;




PurchasedTab::PurchasedTab() : m_pTableWidget(0, _column_names.size()) {
   
   PrepareTableWidgetHeaderGUI();
   
   QHBoxLayout* search_lay = new QHBoxLayout();
   
   m_filterLineEditer.setPlaceholderText(QString("Enter the term to search in title and description"));
   m_filterLineEditer.setStyleSheet("border: 1px solid white");
   m_filterLineEditer.setFixedHeight(40);
   m_filterLineEditer.setAttribute(Qt::WA_MacShowFocusRect, 0);
   
   QPixmap image(":/icon/images/search.svg");
   
   QLabel* search_label = new QLabel();
   search_label->setSizeIncrement(100,40);
   search_label->setPixmap(image);
   
   
   
   search_lay->setContentsMargins(40, 0, 0, 0);
   search_lay->addWidget(search_label);
   search_lay->addWidget(&m_filterLineEditer);
   
   m_pTableWidget.horizontalHeader()->setStretchLastSection(true);
   m_pTableWidget.setSelectionMode(QAbstractItemView::NoSelection);
   m_main_layout.setContentsMargins(0, 0, 0, 0);
   
   m_main_layout.addLayout(search_lay);
   m_main_layout.addWidget(&m_pTableWidget);
   
   setLayout(&m_main_layout);
   
   
   connect(&m_filterLineEditer, SIGNAL(textChanged(QString)), this, SLOT(onTextChanged(QString)));
   
   m_contentUpdateTimer.connect(&m_contentUpdateTimer, SIGNAL(timeout()), this, SLOT(maybeUpdateContent()));
   m_contentUpdateTimer.setInterval(1000);
}



void PurchasedTab::maybeUpdateContent() {
   m_contentUpdateTimer.stop();
   updateContents();
   m_contentUpdateTimer.start();
}

void PurchasedTab::onTextChanged(const QString& text) {
   
   m_doUpdate = true;
}

void PurchasedTab::updateContents() {
   
   
   auto& global_instance = gui_wallet::GlobalEvents::instance();
   std::string str_current_username = global_instance.getCurrentUser();
   
   if ( str_current_username == "" ) {
      return;
   } // if key not imported
   
   std::string a_result;
   RunTask("get_buying_history_objects_by_consumer_term "
           "\"" + str_current_username +"\" "
           "\"" + m_filterLineEditer.text().toStdString() +"\"", a_result);
   
   
   if (last_contents == a_result) {
      return;
   }
   
   
   try {
      auto contents = json::parse(a_result);
      last_contents = a_result;
      if (contents.size() + 1 != m_pTableWidget.rowCount()) {
         m_pTableWidget.setRowCount(0); //Remove everything but header
         m_pTableWidget.setRowCount(contents.size());
         
      }
      
      QPixmap info_image(":/icon/images/info1.svg");
      QPixmap extract_image(":/icon/images/extract.svg");
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
         
         
         m_pTableWidget.horizontalHeader()->setStretchLastSection(true);
         
         
         EventPassthrough<ClickableLabel>* info_icon = new EventPassthrough<ClickableLabel>();
         info_icon->setProperty("id", QVariant::fromValue(i));
         info_icon->setPixmap(info_image);
         info_icon->setAlignment(Qt::AlignCenter);
         connect(info_icon, SIGNAL(clicked()), this, SLOT(show_content_popup()));
         m_pTableWidget.setCellWidget(i, 0, info_icon);
         
         
         m_pTableWidget.setItem(i, 1, new QTableWidgetItem(QString::fromStdString(synopsis)));
         m_pTableWidget.setItem(i, 2, new QTableWidgetItem(QString::number(rating)));
         m_pTableWidget.setItem(i, 3, new QTableWidgetItem(QString::number(size) + tr(" MB")));
         m_pTableWidget.setItem(i, 4, new QTableWidgetItem(QString::number(price) + " DCT"));
         
         
         std::string s_time = time.substr(0, time.find("T"));
         
         m_pTableWidget.setItem(i, 5, new QTableWidgetItem(QString::fromStdString(s_time)));
         
         
         std::string download_status_str;
         RunTask("get_download_status \"" + gui_wallet::GlobalEvents::instance().getCurrentUser() + "\" \"" + URI + "\"", download_status_str);
         
         auto download_status = json::parse(download_status_str);
         
         
         int total_key_parts = download_status["total_key_parts"].get<int>();
         int received_key_parts  = download_status["received_key_parts"].get<int>();
         int total_download_bytes  = download_status["total_download_bytes"].get<int>();
         int received_download_bytes  = download_status["received_download_bytes"].get<int>();
         
         
         QString status_text = tr("Keys: ") + QString::number(received_key_parts) + "/" + QString::number(total_key_parts);
         
         if (!content["delivered"].get<bool>()) {
            status_text = "Waiting for delivery";
         } else {
            status_text = status_text + tr(" ") + QString::fromStdString(download_status["status_text"].get<std::string>());
         }
         
         m_pTableWidget.setItem(i, 6, new QTableWidgetItem(status_text));
         
         if (total_key_parts == 0) {
            total_key_parts = 1;
         }
         
         if (total_download_bytes == 0) {
            total_download_bytes = 1;
         }
         
         
         double progress = (0.1 * received_key_parts) / total_key_parts + (0.9 * received_download_bytes) / total_download_bytes;
         progress *= 100; // Percent
         
         if (received_download_bytes < total_download_bytes) {
            m_pTableWidget.setItem(i, 7, new QTableWidgetItem(QString::number(progress) + "%"));
            m_pTableWidget.item(i, 7)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            m_pTableWidget.item(i, 7)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
         } else {
            
            EventPassthrough<ClickableLabel>* extract_icon = new EventPassthrough<ClickableLabel>();
            
            
            extract_icon->setProperty("id", QVariant::fromValue(QString::fromStdString(content["id"].get<std::string>())));
            extract_icon->setProperty("hash", QVariant::fromValue(QString::fromStdString(dcontent_json["_hash"].get<std::string>())));
            extract_icon->setProperty("URI", QVariant::fromValue(QString::fromStdString(content["URI"].get<std::string>())));
            
            extract_icon->setPixmap(extract_image.scaled(20, 20, Qt::KeepAspectRatio));
            
            extract_icon->setAlignment(Qt::AlignCenter);

            connect(extract_icon, SIGNAL(clicked()), this, SLOT(extractPackage()));
            m_pTableWidget.setCellWidget(i, 7, extract_icon);
         }
         
         
         for(int j = 1; j < _column_names.size() - 1; ++j)
         {
            m_pTableWidget.item(i, j)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            m_pTableWidget.item(i, j)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
         }
         
         
      }
      
      
   } catch (std::exception& ex) {
      std::cout << ex.what() << std::endl;
   }
   
   
   
}

void PurchasedTab::extractPackage() {
   QPushButton* btn = (QPushButton*)sender();
   std::string id = btn->property("id").toString().toStdString();
   std::string URI = btn->property("URI").toString().toStdString();
   std::string hash = btn->property("hash").toString().toStdString();
   
   QString extract_dir = QFileDialog::getExistingDirectory(this, tr("Select directory to extract"), "~", QFileDialog::DontResolveSymlinks);
   if (extract_dir.isEmpty()) {
      return;
   }
   
   std::string key, dummy;
   
   try {
      RunTask("restore_encryption_key \"" + id + "\"", key);
      
      RunTask("extract_package \"" + hash + "\" \"" + extract_dir.toStdString() + "\" " + key, dummy);
      MESSAGE("Package was successfully extracted");
      
   } catch (const std::exception& ex) {
      ALERT_DETAILS("Failed to extract package", ex.what());
   }
   
}


void PurchasedTab::show_content_popup() {
   QPushButton* btn = (QPushButton*)sender();
   int id = btn->property("id").toInt();
   if (id < 0 || id >= _current_content.size()) {
      throw std::out_of_range("Content index is our of range");
   }
   
   emit ShowDetailsOnDigContentSig(_current_content[id]);
}





void PurchasedTab::PrepareTableWidgetHeaderGUI()
{
   QFont font("Open Sans Bold", 14, QFont::Bold);
   
   m_pTableWidget.horizontalHeader()->setDefaultSectionSize(300);
   m_pTableWidget.setRowHeight(0,35);
   m_pTableWidget.verticalHeader()->hide();
   
   QStringList columns;
   for (std::string title: _column_names) {
      columns << QString::fromStdString(title);
   }
   m_pTableWidget.setHorizontalHeaderLabels(columns);
   
   m_pTableWidget.horizontalHeader()->setFixedHeight(35);
   m_pTableWidget.horizontalHeader()->setFont(font);
   m_pTableWidget.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
   m_pTableWidget.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
   
   m_pTableWidget.horizontalHeader()->setStyleSheet("QHeaderView::section {"
                                                     "border-right: 1px solid rgb(193,192,193);"
                                                     "border-bottom: 0px;"
                                                     "border-top: 0px;}");
}



void PurchasedTab::ArrangeSize()
{
   QSize tqsTableSize = m_pTableWidget.size();
   
   m_pTableWidget.setStyleSheet("QTableView{border : 0px}");
   m_pTableWidget.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
   m_pTableWidget.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
   
   m_pTableWidget.setColumnWidth(0, tqsTableSize.width() * 0.1);
   
   for(int i = 1; i < _column_names.size(); ++i) {
      m_pTableWidget.setColumnWidth(i, (0.9 * tqsTableSize.width()) / (_column_names.size() - 1) );
   }
}


void PurchasedTab::resizeEvent ( QResizeEvent * a_event )
{
   QWidget::resizeEvent(a_event);
   ArrangeSize();
}


/*

void PurchasedTab::hightlight_row(QPoint point)
{
   for(int i = 0; i < m_pTableWidget.rowCount(); ++i) {
      for(int j = 1; j < _column_names.size(); ++j) {
         
         if (m_pTableWidget.item(i,j) != NULL) {
            m_pTableWidget.item(i,j)->setBackground(QColor(255,255,255));
            m_pTableWidget.item(i,j)->setForeground(QColor::fromRgb(88,88,88));
         }
      }
      
      QPushButton* button_type = qobject_cast<QPushButton*>(m_pTableWidget.cellWidget(i, _column_names.size()));
      
      if ( NULL == button_type ) {
         if(m_pTableWidget.item(i,_column_names.size()) != NULL) {
            m_pTableWidget.item(i,_column_names.size())->setBackground(QColor(255,255,255));
            m_pTableWidget.item(i,_column_names.size())->setForeground(QColor::fromRgb(88,88,88));
         }
      }
      
      if(m_pTableWidget.cellWidget(i, 0) != NULL) {
         m_pTableWidget.cellWidget(i, 0)->setStyleSheet("* { background-color: rgb(255,255,255); color : white; }");
      }
   }
   
   
   int row = m_pTableWidget.rowAt(point.y());
   
   if (row < 0) {
      return;
   }
   
   if(m_pTableWidget.cellWidget(row , 0) != NULL) {
      m_pTableWidget.cellWidget(row , 0)->setStyleSheet("* { background-color: rgb(27,176,104); color : white; }");
   }
   
   for(int i = 1 ; i < _column_names.size(); ++i) {
      
      if(m_pTableWidget.item(row,i) != NULL) {
         m_pTableWidget.item(row,i)->setBackgroundColor(QColor(27,176,104));
         m_pTableWidget.item(row,i)->setForeground(QColor::fromRgb(255,255,255));
      }
   }
   
   
   QPushButton* button_type = qobject_cast<QPushButton*>(m_pTableWidget.cellWidget(row, _column_names.size()));
   if ( NULL == button_type ) {
      
      if(m_pTableWidget.item(row, _column_names.size()) != NULL)  {
         m_pTableWidget.item(row, _column_names.size())->setBackgroundColor(QColor(27,176,104));
         m_pTableWidget.item(row, _column_names.size())->setForeground(QColor::fromRgb(255,255,255));
      }
   }
   
}
*/
