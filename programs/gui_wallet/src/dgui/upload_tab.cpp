#include "stdafx.h"

#include "gui_wallet_global.hpp"
#include "upload_tab.hpp"
#include "decent_wallet_ui_gui_contentdetailsgeneral.hpp"
#include "decent_button.hpp"

#ifndef _MSC_VER
#include <QMessageBox>
#include <QFileDialog>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QCalendarWidget>
#include <QDate>
#include <QDateEdit>
#include <QApplication>
#include <stdio.h>
#include <QStyleFactory>
#include <QInputMethod>
#include <QSignalMapper>
#include <QLocale>
#include <graphene/chain/config.hpp>
#include <graphene/chain/content_object.hpp>
#include <boost/filesystem.hpp>

#include <cryptopp/integer.h>
#include <cryptopp/aes.h>
#include <cryptopp/filters.h>
#include <cryptopp/files.h>
#include <cryptopp/ccm.h>
#include <cryptopp/md5.h>
#include <cryptopp/osrng.h>

#include <QIcon>

#include <QLayout>
#include <QCheckBox>
#include <stdio.h>
#include <QHeaderView>
#include <QLabel>
#include <QMouseEvent>
#include <stdio.h>
#include <stdarg.h>
#include "json.hpp"
#include "gui_design.hpp"

#include <ctime>
#include <limits>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <graphene/chain/config.hpp>


#include <QDateTime>
#include <QDate>
#include <QTime>
#endif

using string = std::string;
using namespace gui_wallet;
using namespace nlohmann;

CryptoPP::AutoSeededRandomPool rng;


Upload_popup::Upload_popup(QWidget* pParent)
: QDialog(pParent)
, m_getPublishersTimer(this)
{
   u_main_layout = new QVBoxLayout(this);

   _locale = ((QApplication*)QApplication::instance())->inputMethod()->locale();
   ////////////////////////////////////////////////////////////////////////////
   /// Title field
   ////////////////////////////////////////////////////////////////////////////
   
   _titleText = new QLineEdit();
   _titleText->setPlaceholderText(tr("Title"));
   _titleText->setAttribute(Qt::WA_MacShowFocusRect, 0);
   _titleText->setTextMargins(5, 5, 5, 5);
   _titleText->setMinimumHeight(40);
   u_main_layout->addWidget(_titleText);

   ////////////////////////////////////////////////////////////////////////////
   /// Description field
   ////////////////////////////////////////////////////////////////////////////
   _descriptionText = new QTextEdit();
   _descriptionText->setPlaceholderText(tr("Description"));
   _descriptionText->setStyleSheet(d_desc);
   _descriptionText->setMinimumHeight(160);
   _descriptionText->setMinimumWidth(480);
   _descriptionText->setTabChangesFocus(true);
   u_main_layout->addWidget(_descriptionText);
   
   ////////////////////////////////////////////////////////////////////////////
   /// Lifetime
   ////////////////////////////////////////////////////////////////////////////
   QHBoxLayout* lifeTimeRow = new QHBoxLayout();
   
   QLabel* lifeTimeLabel = new QLabel(tr("Expiration date"));
   lifeTimeLabel->setStyleSheet(d_label_v1);
   lifeTimeLabel->setMinimumWidth(60);
   lifeTimeLabel->setMinimumHeight(40);
   

   _lifeTime = new QDateEdit(this);
   _lifeTime->setDate(QDate::currentDate().addMonths(1));
   _lifeTime->setDisplayFormat("yyyy-MM-dd");
   _lifeTime->setCalendarPopup(true);
   _lifeTime->setMinimumDate(QDate::currentDate().addDays(1));
   _lifeTime->setStyle(QStyleFactory::create("fusion"));
   _lifeTime->setMinimumHeight(40);
   _lifeTime->setFixedWidth(320);
   
   lifeTimeRow->addWidget(lifeTimeLabel);
   lifeTimeRow->addWidget(_lifeTime);
   u_main_layout->addLayout(lifeTimeRow);
   
   
   
   
   ////////////////////////////////////////////////////////////////////////////
   /// Key particles
   ////////////////////////////////////////////////////////////////////////////
    QHBoxLayout* keyRow = new QHBoxLayout();

    QLabel* keypartsLabel = new QLabel(tr("Key particles"));
    keypartsLabel->setStyleSheet(d_label_v1);
    keypartsLabel->setMinimumWidth(60);
    keypartsLabel->setMinimumHeight(40);
    
    _keyparts = new QComboBox(this);
    _keyparts->setStyle(QStyleFactory::create("fusion"));
    _keyparts->setStyleSheet(c_keyparts);
    _keyparts->setMinimumHeight(40);
    _keyparts->setFixedWidth(320);
   
   
    for (int r = 2; r <= 7; ++r) {
        QString val = QString::fromStdString(std::to_string(r));
        _keyparts->addItem(val, val);
    }
    
    keyRow->addWidget(keypartsLabel);
    keyRow->addWidget(_keyparts);
    u_main_layout->addLayout(keyRow);

   
   
   ////////////////////////////////////////////////////////////////////////////
   /// Price
   ////////////////////////////////////////////////////////////////////////////
   QHBoxLayout* priceRow = new QHBoxLayout();
   
   QLabel* priceLabel = new QLabel(tr("Price"));
   priceLabel->setStyleSheet(d_label_v1);
   priceLabel->setMinimumWidth(60);
   priceLabel->setMinimumHeight(40);


   _price = new QLineEdit();
   _price->setValidator( new QDoubleValidator(0.0001, 100000, 4, this) );
   _price->setAttribute(Qt::WA_MacShowFocusRect, 0);
   _price->setStyleSheet(d_label_v2);
   _price->setTextMargins(5, 5, 5, 5);
   _price->setMinimumHeight(40);
   _price->setFixedWidth(320);

   priceRow->addWidget(priceLabel);
   priceRow->addWidget(_price);

   u_main_layout->addLayout(priceRow);
   
   ////////////////////////////////////////////////////////////////////////////
   /// Seeders
   ////////////////////////////////////////////////////////////////////////////
   QHBoxLayout* seedersRow = new QHBoxLayout();
   
   _seedersPath = new QLineEdit(tr("Seeders"));
   _seedersPath->setReadOnly(true);
   _seedersPath->setStyleSheet(d_label_v2);
   _seedersPath->setTextMargins(5, 5, 5, 5);
   _seedersPath->setMinimumWidth(100);
   _seedersPath->setMinimumHeight(40);
   
   DecentButton* seeders_button = new DecentButton();

   seeders_button->setText(tr("Select Seeders"));
   seeders_button->setFont(PopupButtonRegularFont());
#ifdef WINDOWS_HIGH_DPI
   seeders_button->setFixedWidth(240);
#else
   seeders_button->setFixedWidth(120);
#endif
   seeders_button->setFixedHeight(40);
   
   seedersRow->addWidget(_seedersPath);
   seedersRow->addWidget(seeders_button);
   u_main_layout->addLayout(seedersRow);
   
   
   ////////////////////////////////////////////////////////////////////////////
   /// Seeders Dialog
   ////////////////////////////////////////////////////////////////////////////
   _seeders_dialog = new QDialog(this, Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint);
   _seeders_dialog->setWindowTitle(tr("Seeders"));
   _seeders_dialog->setContentsMargins(0, 0, 0, 0);
   _seeders_dialog->resize(450, 250);

   connect(seeders_button, SIGNAL(clicked()), _seeders_dialog, SLOT(exec()) );
   


   ////////////////////////////////////////////////////////////////////////////
   /// Content path
   ////////////////////////////////////////////////////////////////////////////
   
   QHBoxLayout* contentRow = new QHBoxLayout();

   _contentPath = new QLineEdit(tr("Content path"));
   _contentPath->setReadOnly(true);
   _contentPath->setStyleSheet(d_label_v2);
   _contentPath->setMinimumHeight(40);
   _contentPath->setTextMargins(5, 5, 5, 5);

   DecentButton* browseContentButton = new DecentButton();
   browseContentButton->setText(tr("Browse"));
   browseContentButton->setFont(PopupButtonRegularFont());

   browseContentButton->setMinimumWidth(120);
   browseContentButton->setFixedHeight(40);
   connect(browseContentButton, SIGNAL(clicked()),this, SLOT(browseContent()));

   contentRow->addWidget(_contentPath);
   contentRow->addWidget(browseContentButton);
   u_main_layout->addLayout(contentRow);
   
   ////////////////////////////////////////////////////////////////////////////
   /// Samples path
   ////////////////////////////////////////////////////////////////////////////
   QHBoxLayout* samplesRow = new QHBoxLayout();

   _samplesPath = new QLineEdit(tr("Samples (optional)"));
   _samplesPath->setReadOnly(true);
   _samplesPath->setStyleSheet(d_label_v2);
   _samplesPath->setMinimumHeight(40);
   _samplesPath->setTextMargins(5, 5, 5, 5);
   

   DecentButton* browseSamplesButton = new DecentButton();
   browseSamplesButton->setText(tr("Browse"));
   browseSamplesButton->setFont(PopupButtonRegularFont());

   browseSamplesButton->setMinimumWidth(120);
   browseSamplesButton->setFixedHeight(40);
   connect(browseSamplesButton, SIGNAL(clicked()),this, SLOT(browseSamples()));

   samplesRow->addWidget(_samplesPath);
   samplesRow->addWidget(browseSamplesButton);

   u_main_layout->addLayout(samplesRow);
 
   ////////////////////////////////////////////////////////////////////////////
   /// Upload & Cancel
   ////////////////////////////////////////////////////////////////////////////
   

   QHBoxLayout* button = new QHBoxLayout;

   button->setSpacing(20);
   _upload_button = new DecentButton();
   _cancel_button = new DecentButton();

   _cancel_button->setText(tr("Cancel"));
   _cancel_button->setFont(PopupButtonBigFont());
   _cancel_button->setMinimumHeight(50);


   _upload_button->setText(tr("Publish"));
   _upload_button->setFont(PopupButtonBigFont());
   _upload_button->setMinimumHeight(50);

   connect(_upload_button, SIGNAL(clicked()),this, SLOT(uploadContent()));
   connect(_cancel_button, SIGNAL(clicked()),this, SLOT( uploadCanceled() ));

   button->setContentsMargins(20, 20, 20, 20);
   button->addWidget(_upload_button);
   button->addWidget(_cancel_button);

   u_main_layout->addLayout(button);
   u_main_layout->setContentsMargins(10, 10, 10, 10);
   u_main_layout->setSpacing(5);

   setWindowTitle(tr("Upload new content"));
   setStyleSheet(d_upload_popup);
   setLayout(u_main_layout);

   m_getPublishersTimer.setSingleShot(true);
   QObject::connect(&m_getPublishersTimer, &QTimer::timeout,
                    this, &Upload_popup::onGrabPublishers);

   m_getPublishersTimer.start(1000);
   
   updateUploadButtonStatus();
   
   _buttonStatusCheck = new QTimer(this);
   connect(_buttonStatusCheck, SIGNAL(timeout()), SLOT(updateUploadButtonStatus()));
   _buttonStatusCheck->start(500);
#ifdef _MSC_VER
   int height = style()->pixelMetric(QStyle::PM_TitleBarHeight);
   setWindowIcon(height > 32 ? QIcon(":/icon/images/windows_decent_icon_32x32.png")
      : QIcon(":/icon/images/windows_decent_icon_16x16.png"));
#endif
}

void Upload_popup::onGrabPublishers() {
   
   QVBoxLayout* dialog_layout = new QVBoxLayout(_seeders_dialog);
   
   _seeder_table = new DecentTable(this);
   _seeder_table->set_columns({
      {"", -25},
      {tr("Seeder"), 10, "rating"},
      {tr("Price"),  10, "price"},
      {tr("Size") ,  10, "size"}
   });
   

   std::string a_result = Globals::instance().runTask("list_publishers_by_price 100");
   
   //seeders popup ok button
   _seeder_ok = new DecentButton();
   _seeder_ok->setText(tr("OK"));
   _seeder_ok->setFixedHeight(50);
   _seeder_ok->setFixedWidth(100);
   _seeder_ok->setFont(TabButtonFont());

   auto publishers = json::parse(a_result);
   _seeder_table->setRowCount(publishers.size());
   
   _seeders_checkbox.clear();
   _seeders_checkbox.resize(publishers.size());
                             
   for (int r = 0; r < publishers.size(); ++r) {
      
      _seeders_checkbox[r] = new QCheckBox(_seeders_dialog);
      _seeder_table->setCellWidget(r, 0, _seeders_checkbox[r]);

      std::string pubIdStr = publishers[r]["seeder"].get<std::string>();
      _seeder_table->setItem(r, 1, new QTableWidgetItem(QString::fromStdString(pubIdStr)));
      _seeder_table->item(r, 1)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
      _seeder_table->item(r, 1)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
      
      double price = publishers[r]["price"]["amount"].get<double>() / GRAPHENE_BLOCKCHAIN_PRECISION;
      std::string pubPrice = QString::number(price).toStdString();
      _seeder_table->setItem(r, 2, new QTableWidgetItem(QString::number(price) + " DCT"));
      _seeder_table->item(r, 2)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
      _seeder_table->item(r, 2)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
      
      std::string pubAssetId = publishers[r]["price"]["asset_id"].get<std::string>();
      
      int free_space = publishers[r]["free_space"].get<int>();
      std::string pubFreeSpace = std::to_string(free_space) + "MB free";
      
      if (free_space > 800) {
         pubFreeSpace = QString::number(1.0 * free_space / 1024, 'f', 2).toStdString() + "GB free";
      }
      
      _seeder_table->setItem(r, 3, new QTableWidgetItem(QString::fromStdString(pubFreeSpace)));
      _seeder_table->item(r, 3)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
      _seeder_table->item(r, 3)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
      
      _publisherIdToPriceMap.insert(std::make_pair(pubIdStr, price));
      
      QObject::connect(_seeder_ok, SIGNAL(clicked()),this, SLOT(seederOkSlot()));
   }
   
   QHBoxLayout* button = new QHBoxLayout(_seeders_dialog);
   button->addWidget(_seeder_ok);
   button->setMargin(10);
   button->setAlignment(Qt::AlignHCenter);
   
   dialog_layout->addWidget(_seeder_table);
   dialog_layout->addLayout(button);
   dialog_layout->setContentsMargins(0, 0, 0, 0);
   dialog_layout->setMargin(0);
   dialog_layout->setSpacing(0);
   
   setLayout(dialog_layout);
}

void Upload_popup::seederOkSlot()
{
   _checkedSeeders.clear();
   for (int i = 0; i < _seeders_checkbox.size(); ++i){
      if (_seeders_checkbox[i]->isChecked()){
         _checkedSeeders.push_back(_seeder_table->item(i, 1)->text().toStdString());
      }
   }
   
   std::string str;
   if (_checkedSeeders.size() < 1){
      str += "Seeders";
   }

   if( _checkedSeeders.size() > 4){
      str = std::to_string(_checkedSeeders.size());
      _seedersPath->setText(QString::fromStdString(str));
      _seeders_dialog->close();
      
      return;
   }
   
   for ( int i = 0; i < _checkedSeeders.size(); ++i){
      str += _checkedSeeders[i];
      str += (i + 1) == _checkedSeeders.size()? "": ", ";
   }

   _seedersPath->setText(QString::fromStdString(str));
   _seeders_dialog->close();
}

void Upload_popup::updateUploadButtonStatus() {
   bool isValid = true;


   std::string lifeTime    = _lifeTime->text().toStdString();
   //std::string seeders     = _seeders->currentData().toString().toStdString();
   //seeders push_back in stateChanged slot on _checkedSeeders member
   std::string keyparts    = _keyparts->currentData().toString().toStdString();

   bool isOK = false;
   std::string price = QString::number(_locale.toDouble(_price->text(), &isOK)).toStdString();
   if (!isOK)
       isValid = false;

   

   std::string path        = _contentPath->text().toStdString();
   std::string samplesPath = _samplesPath->text().toStdString();
   
   std::string title = _titleText->text().toStdString();
   std::string desc = _descriptionText->toPlainText().toStdString();

   
   if (price.empty())
      isValid = false;
   
   if (path.empty())
      isValid = false;
   
   boost::system::error_code ec;
   uint64_t fileSize = boost::filesystem::file_size(path, ec);
   
   if (fileSize > 100 * 1024 * 1024)
      isValid = false;
   
   if (ec)
      isValid = false;
   
   
   if (title.empty())
      isValid = false;
   
   if (desc.empty())
      isValid = false;
   
   if (Globals::instance().getCurrentUser().empty())
      isValid = false;
   

   auto it = _publisherIdToPriceMap.find("b"); //So it was not empty
   double publisherPrice = 0;
   if (_checkedSeeders.empty()) {
      isValid = false;
   }
   else{

      for (std::vector<std::string>::iterator iter = _checkedSeeders.begin(); iter != _checkedSeeders.end(); ++iter){
         it = _publisherIdToPriceMap.find(*iter);

         if (it == _publisherIdToPriceMap.end()) {
            isValid = false;
         }else{
            publisherPrice += it->second;
         }
      }
   }
   double publishingPrice = 0;
   if( isValid )
      publishingPrice = publisherPrice;
   uint64_t size = std::max( (uint64_t)1, ( fileSize + (1024 * 1024) -1 ) / (1024 * 1024));
   double totalPricePerDay = size * publishingPrice;
   int days = QDate::currentDate().daysTo(_lifeTime->date());
   
   
   if (isValid) {
      _upload_button->setText(tr("Publish for") + " " + QString::number(days * totalPricePerDay) + " DCT");
      _upload_button->setEnabled(true);
   } else {
      _upload_button->setText(tr("Publish"));
      _upload_button->setEnabled(false);
   }
}


void Upload_popup::browseContent() {
    QString contentPathSelected = QFileDialog::getOpenFileName(this, tr("Select content"), "~");
   
   if (contentPathSelected.size() == 0) {
       return;
   }
   boost::system::error_code ec;
   if (boost::filesystem::file_size(contentPathSelected.toStdString(), ec) > 100 * 1024 * 1024) {
      ALERT("Content size is limited in Testnet 0.1 to 100MB");
      return;
   }
   
    _contentPath->setText(contentPathSelected);
   _contentPath->setProperty("path", contentPathSelected);
}

void Upload_popup::browseSamples() {
    QString sampleDir = QFileDialog::getExistingDirectory(this, tr("Select samples"), "~", QFileDialog::DontResolveSymlinks);
    _samplesPath->setText(sampleDir);
   _samplesPath->setProperty("path", sampleDir);
}


void Upload_popup::uploadContent()
{
   std::string m_seeder = "";
   if ( _checkedSeeders.size() > 0 ){
      for (int i = 0; i < _checkedSeeders.size(); ++i){
         m_seeder += _checkedSeeders[i];
         if( i + 1 != _checkedSeeders.size() ) {
            m_seeder += ", ";
         }
      }
   }
   
   std::string m_life_time = _lifeTime->text().toStdString();
   std::string m_keyparts  = _keyparts->currentData().toString().toStdString();


   
   double price = _locale.toDouble(_price->text(), NULL);
   std::string m_price     = QString::number(price).toStdString();

   std::string assetName = "DCT";
   
   std::string path = "";
   if (_contentPath->property("path").isValid())
      path = _contentPath->property("path").toString().toStdString();

   
   std::string samples_path = "";
   if (_samplesPath->property("path").isValid())
      samples_path = _samplesPath->property("path").toString().toStdString();

   std::string title = _titleText->text().toStdString();
   std::string desc = _descriptionText->toPlainText().toStdString();

   setEnabled(false);

   json synopsis_obj;
   synopsis_obj["title"] = title;
   synopsis_obj["description"] = desc;

   std::string synopsis = synopsis_obj.dump(4);

   CryptoPP::Integer randomKey (rng, 512);

   std::ostringstream oss;
   oss << randomKey;
   std::string randomKeyString(oss.str());



   std::string submitCommand = "submit_content_new";
   submitCommand += " " + Globals::instance().getCurrentUser();    // author
   submitCommand += " \"" + path + "\"";                                // URI
   submitCommand += " \"" + samples_path + "\"";                        // Samples
   submitCommand += " \"ipfs\"";                                        // Protocol
   submitCommand += " " + assetName;                                    // price_asset_name
   submitCommand += " [[\"\", \"" + m_price + "\"]]";                   // price_amount
   submitCommand += " [" + m_seeder + "]";                  // seeders
   submitCommand += " \"" + m_life_time + "T23:59:59\"";                // expiration
   submitCommand += " \"" + escape_string(synopsis) + "\"";             // synopsis
   submitCommand += " true";                                            // broadcast

   // this is an example how price per regions will be used
   // submitCommand += " [[\"default\", \"0\"], [\"US\", \"10\"]]";

   std::string a_result;
   std::string message;

   try
   {
      a_result = Globals::instance().runTask(submitCommand);
      if (a_result.find("exception:") != std::string::npos)
      {
         message = a_result;
      }
   }
   catch (const std::exception& ex)
   {
      message = ex.what();
      setEnabled(true);
   }

   

   if (message.empty())
   {
      _titleText->setText("");
      _descriptionText->setPlainText("");
      _lifeTime->setDate(QDate::currentDate());
      _price->setText("");
      _contentPath->setText(tr("Content path"));
      _samplesPath->setText(tr("Samples (optional)"));

      ShowMessageBox(tr("Content is being processed...") , tr("Success"));

      setEnabled(true);

      this->close();
   }
   else
   {
      ShowMessageBox(tr("Error"), tr("Failed to submit content"), message.c_str());
   }
}


void Upload_popup::uploadCanceled()
{
   this->close();
}


//////////////////////////////////////////////////
// UPLOAD TAB
//////////////////////////////////////////////////

Upload_tab::Upload_tab(QWidget* pParent)
: TabContentManager(pParent)
, m_pTableWidget(new DecentTable(this))
, m_pDetailsSignalMapper(nullptr)
{
   m_pTableWidget->set_columns({
      {tr("Title"), 20},
      {tr("Rating"), 10, "rating"},
      {tr("Size"), 10, "size"},
      {tr("Price"), 10, "price"},
      {tr("Published"), 10, "created"},
      {tr("Expiration"), 10, "expiration"},
      {tr("Status"), 10},
#ifdef WINDOWS_HIGH_DPI
      { " ", -80 }
#else
      {" ", -50}
#endif
   });

   DecentButton* pUploadButton = new DecentButton(this);
   pUploadButton->setFont(TabButtonFont());
   pUploadButton->setText(tr("Publish"));
   pUploadButton->setMinimumWidth(102);
   pUploadButton->setMinimumHeight(54);

   QLabel* pSearchLabel = new QLabel(this);
   QPixmap image(icon_search);
   pSearchLabel->setPixmap(image);

   QLineEdit* pfilterLineEditor = new QLineEdit(this);
   pfilterLineEditor->setPlaceholderText(tr("Search Content"));
   pfilterLineEditor->setFixedHeight(54);
   pfilterLineEditor->setStyleSheet(d_lineEdit);
   pfilterLineEditor->setAttribute(Qt::WA_MacShowFocusRect, 0);

   QHBoxLayout* pSearchLayout = new QHBoxLayout;
   pSearchLayout->setContentsMargins(42, 0, 0, 0);
   pSearchLayout->addWidget(pSearchLabel);
   pSearchLayout->addWidget(pfilterLineEditor);
   pSearchLayout->addWidget(pUploadButton, 0 , Qt::AlignBottom);

   QVBoxLayout* pMainLayout = new QVBoxLayout;
   pMainLayout->setContentsMargins(0, 0, 0, 0);
   pMainLayout->setSpacing(0);
   pMainLayout->addLayout(pSearchLayout);
   pMainLayout->addWidget(m_pTableWidget);
   setLayout(pMainLayout);

   QObject::connect(&Globals::instance(), &Globals::currentUserChanged,
                    this, &Upload_tab::slot_UpdateContents);
   QObject::connect(this, &Upload_tab::signal_setEnabledUpload,
                    pUploadButton, &QWidget::setEnabled);
   QObject::connect(m_pTableWidget, &DecentTable::signal_SortingChanged,
                    this, &Upload_tab::slot_SortingChanged);
   QObject::connect(pfilterLineEditor, &QLineEdit::textChanged,
                    this, &Upload_tab::slot_SearchTermChanged);
   QObject::connect(pUploadButton, &QPushButton::clicked,
                    this, &Upload_tab::slot_UploadPopup);

   slot_UpdateContents();
}

// when class has forward declared members
// this becomes necessary
Upload_tab::~Upload_tab() = default;

void Upload_tab::slot_UpdateContents()
{
   string currentUserName = Globals::instance().getCurrentUser();
   if(currentUserName.empty())
      emit signal_setEnabledUpload(false);
   else
      emit signal_setEnabledUpload(true);
}

void Upload_tab::timeToUpdate(const string& result)
{
   _digital_contents.clear();

   if (result.empty())
      return;

   auto contents = nlohmann::json::parse(result);
   size_t iSize = contents.size();
   if (iSize > m_i_page_size)
      iSize = m_i_page_size;
   
   _digital_contents.resize(contents.size());
   
   for (size_t iIndex = 0; iIndex < iSize; ++iIndex)
   {
      SDigitalContent& content = _digital_contents[iIndex];
      auto const& json_content = contents[iIndex];
      
      content.type = DCT::GENERAL;
      content.author = json_content["author"].get<string>();
      uint64_t iPrice = json_to_int64(json_content["price"]["amount"]);
      content.price = Globals::instance().asset(iPrice);
      content.synopsis = json_content["synopsis"].get<string>();
      content.URI = json_content["URI"].get<string>();
      content.created = json_content["created"].get<string>();
      content.created = content.created.substr(0, content.created.find("T"));
      content.expiration = json_content["expiration"].get<string>();
      content.size = json_content["size"].get<int>();
      content.status = json_content["status"].get<string>();

      QDateTime time = QDateTime::fromString(QString::fromStdString(content.expiration), "yyyy-MM-ddTHH:mm:ss");

      if (content.status.empty())
      {
         if (time < QDateTime::currentDateTime())
            content.status = "Expired";
         else
            content.status = "Published";
      }
      
      if (json_content["times_bought"].is_number())
         content.times_bought = json_content["times_bought"].get<int>();
      else
         content.times_bought = 0;

      content.AVG_rating = json_content["AVG_rating"].get<double>() / 1000.0;
   }

   if (contents.size() > m_i_page_size)
      set_next_page_iterator(contents[m_i_page_size]["id"].get<string>());
   else
      set_next_page_iterator(string());
   
   ShowDigitalContentsGUI();
}

string Upload_tab::getUpdateCommand()
{
   string currentUserName = Globals::instance().getCurrentUser();
   if (currentUserName.empty())
      return string();
   
   return   "search_user_content "
            "\"" + currentUserName + "\" "
            "\"" + m_strSearchTerm.toStdString() + "\" "
            "\"" + m_pTableWidget->getSortedColumn() + "\" "
            "\"\" "   // region_code
            "\"" + next_iterator() + "\" "
            + std::to_string(m_i_page_size + 1);
}

void Upload_tab::ShowDigitalContentsGUI()
{
   m_pTableWidget->setRowCount(_digital_contents.size());

   enum {eTitle, eRating, eSize, ePrice, eCreated, eRemaining, eStatus, eIcon};

   if (m_pDetailsSignalMapper)
      delete m_pDetailsSignalMapper;
   m_pDetailsSignalMapper = new QSignalMapper(this);  // similar to extract signal handler
   QObject::connect(m_pDetailsSignalMapper, (void (QSignalMapper::*)(int))&QSignalMapper::mapped,
                    this, &Upload_tab::slot_ShowContentPopup);

   for (size_t iIndex = 0; iIndex < _digital_contents.size(); ++iIndex)
   {
      SDigitalContent const& content = _digital_contents[iIndex];

      std::string synopsis = unescape_string(content.synopsis);
      std::replace(synopsis.begin(), synopsis.end(), '\t', ' '); // JSON does not like tabs :(
      std::replace(synopsis.begin(), synopsis.end(), '\n', ' '); // JSON does not like tabs :(

      graphene::chain::ContentObjectPropertyManager synopsis_parser(synopsis);
      std::string title = synopsis_parser.get<graphene::chain::ContentObjectTitle>();

      // Title
      //
      m_pTableWidget->setItem(iIndex, eTitle, new QTableWidgetItem(QString::fromStdString(title)));

      // Rating
      //
      QString rating = QString::number(content.AVG_rating, 'f', 2);
      m_pTableWidget->setItem(iIndex, eRating, new QTableWidgetItem(rating));

      // Size
      //
      QString unit = " MB";
      double sizeAdjusted = content.size;

      if(content.size > 1024)
      {
         unit = " GB";
         sizeAdjusted = content.size / 1024.0;
      }
      m_pTableWidget->setItem(iIndex, eSize, new QTableWidgetItem(QString::number(sizeAdjusted, 'f', 2) + unit));

      // Price
      m_pTableWidget->setItem(iIndex, ePrice, new QTableWidgetItem(content.price.getString().c_str()));

      // Created
      m_pTableWidget->setItem(iIndex, eCreated, new QTableWidgetItem(QString::fromStdString(content.created)));

      QDateTime time = QDateTime::fromString(QString::fromStdString(content.expiration), "yyyy-MM-ddTHH:mm:ss");

      std::string e_str = CalculateRemainingTime(QDateTime::currentDateTime(), time);

      // Remaining
      m_pTableWidget->setItem(iIndex, eRemaining, new QTableWidgetItem(QString::fromStdString(e_str)));

      // Status
      m_pTableWidget->setItem(iIndex, eStatus, new QTableWidgetItem(QString::fromStdString(content.status)));


      for (size_t iColIndex = eTitle; iColIndex < eIcon; ++iColIndex)
      {
         m_pTableWidget->item(iIndex, iColIndex)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
         m_pTableWidget->item(iIndex, iColIndex)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
      }
      // Icon
      //
      DecentSmallButton* info_icon = new DecentSmallButton(icon_popup, icon_popup_white, this);
      info_icon->setAlignment(Qt::AlignCenter);
      m_pTableWidget->setCellWidget(iIndex, eIcon, info_icon);

      QObject::connect(info_icon, &DecentSmallButton::clicked,
                       m_pDetailsSignalMapper, (void (QSignalMapper::*)())&QSignalMapper::map);
      m_pDetailsSignalMapper->setMapping(info_icon, iIndex);
   }
}

void Upload_tab::slot_UploadPopup()
{
   if (false == Globals::instance().getCurrentUser().empty())
   {
      Upload_popup popup(this);
      popup.exec();
   }
}

void Upload_tab::slot_ShowContentPopup(int iIndex)
{
   if (iIndex < 0 || iIndex >= _digital_contents.size())
      throw std::out_of_range("Content index is out of range");

   ContentDetailsGeneral* pDetailsDialog = new ContentDetailsGeneral(nullptr);
   QObject::connect(pDetailsDialog, &ContentDetailsGeneral::ContentWasBought,
                    this, &Upload_tab::slot_Bought);
   pDetailsDialog->execCDD(_digital_contents[iIndex], true);
   pDetailsDialog->setAttribute(Qt::WA_DeleteOnClose);
   pDetailsDialog->open();
}

void Upload_tab::slot_Bought()
{
   Globals::instance().signal_showPurchasedTab();
   Globals::instance().updateAccountBalance();
}

void Upload_tab::slot_SortingChanged(int index)
{
   reset();
}

void Upload_tab::slot_SearchTermChanged(QString const& strSearchTerm)
{
   m_strSearchTerm = strSearchTerm;
}

