

#include "upload_tab.hpp"
#include "gui_wallet_global.hpp"

#include <QMessageBox>
#include <QFileDialog>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QCalendarWidget>
#include <QDate>
#include <QDateEdit>
#include <stdio.h>
#include <QStyleFactory>
#include "decent_button.hpp"

#include <graphene/chain/config.hpp>
#include <boost/filesystem.hpp>

#include <cryptopp/integer.h>
#include <cryptopp/aes.h>
#include <cryptopp/filters.h>
#include <cryptopp/files.h>
#include <cryptopp/ccm.h>
#include <cryptopp/md5.h>
#include <cryptopp/osrng.h>

#include <QIcon>

#include "gui_wallet_global.hpp"
#include "gui_wallet_mainwindow.hpp"

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


using namespace gui_wallet;
using namespace nlohmann;

CryptoPP::AutoSeededRandomPool rng;


Upload_popup::Upload_popup(Mainwindow_gui_wallet* pMainWindow) : m_getPublishersTimer(this) {
   
   QFont font( "Myriad Pro Regular", 13, QFont::Bold);

   u_main_layout = new QVBoxLayout(this);
   ////////////////////////////////////////////////////////////////////////////
   /// Title field
   ////////////////////////////////////////////////////////////////////////////
   
   _titleText = new QLineEdit();
   _titleText->setPlaceholderText("Title");
   _titleText->setAttribute(Qt::WA_MacShowFocusRect, 0);
   _titleText->setTextMargins(5, 5, 5, 5);
   _titleText->setMinimumHeight(40);
   u_main_layout->addWidget(_titleText);

   ////////////////////////////////////////////////////////////////////////////
   /// Description field
   ////////////////////////////////////////////////////////////////////////////
   _descriptionText = new QTextEdit();
   _descriptionText->setPlaceholderText("Description");
   _descriptionText->setStyleSheet(d_desc);
   _descriptionText->setMinimumHeight(160);
   _descriptionText->setMinimumWidth(420);
   u_main_layout->addWidget(_descriptionText);
   
   ////////////////////////////////////////////////////////////////////////////
   /// Lifetime
   ////////////////////////////////////////////////////////////////////////////
   QHBoxLayout* lifeTimeRow = new QHBoxLayout();
   
   QLabel* lifeTimeLabel = new QLabel("Expiration date");
   lifeTimeLabel->setStyleSheet(d_label_v1);
   lifeTimeLabel->setMinimumWidth(60);
   lifeTimeLabel->setMinimumHeight(40);
   

   _lifeTime = new QDateEdit(this);
   _lifeTime->setDate(QDate::currentDate());
   _lifeTime->setDisplayFormat("yyyy-MM-dd");
   _lifeTime->setCalendarPopup(true);
   _lifeTime->setMinimumDate(QDate::currentDate());
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

    QLabel* keypartsLabel = new QLabel("Key particles");
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
   
   QLabel* priceLabel = new QLabel("Price");
   priceLabel->setStyleSheet(d_label_v1);
   priceLabel->setMinimumWidth(60);
   priceLabel->setMinimumHeight(40);


   _price = new QLineEdit();
   _price->setValidator( new QDoubleValidator(0.001, 100000, 3, this) );
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
   
   QLabel* seedersLabel = new QLabel("Seeders");
   seedersLabel->setStyleSheet(d_label_v1);
   seedersLabel->setContentsMargins(0, 0, 0, 0);
   seedersLabel->setMinimumWidth(60);
   seedersLabel->setMinimumHeight(40);
   
   DecentButton* seeders_button = new DecentButton();
   seeders_button->setText("Select Seeders");
   seeders_button->setFont(font);
   seeders_button->setFixedWidth(100);
   seeders_button->setFixedHeight(40);
   
   seedersRow->addWidget(seedersLabel);
   seedersRow->addWidget(seeders_button);
   u_main_layout->addLayout(seedersRow);
   
   
   ////////////////////////////////////////////////////////////////////////////
   /// Seeders Dialog
   ////////////////////////////////////////////////////////////////////////////
   _seeders_dialog = new QDialog(this);
   _seeders_dialog->setWindowTitle("Seeders");
   
   dialog_layout = new QVBoxLayout(_seeders_dialog);

   _seeders_dialog->setLayout(dialog_layout);
   _seeders_dialog->resize(100, 200);

   connect(seeders_button, SIGNAL(LabelClicked()), _seeders_dialog, SLOT(exec()) );
   


   ////////////////////////////////////////////////////////////////////////////
   /// Content path
   ////////////////////////////////////////////////////////////////////////////
   
   QHBoxLayout* contentRow = new QHBoxLayout();

   _contentPath = new QLineEdit("Content path");
   _contentPath->setReadOnly(true);
   _contentPath->setStyleSheet(d_label_v2);
   _contentPath->setMinimumHeight(40);
   _contentPath->setTextMargins(5, 5, 5, 5);

   DecentButton* browseContentButton = new DecentButton();
   browseContentButton->setText("Browse");
   browseContentButton->setFont(font);
   browseContentButton->setMinimumWidth(100);
   browseContentButton->setFixedHeight(40);
   connect(browseContentButton, SIGNAL(LabelClicked()),this, SLOT(browseContent()));

   contentRow->addWidget(_contentPath);
   contentRow->addWidget(browseContentButton);
   u_main_layout->addLayout(contentRow);
   
   ////////////////////////////////////////////////////////////////////////////
   /// Content path
   ////////////////////////////////////////////////////////////////////////////
   QHBoxLayout* samplesRow = new QHBoxLayout();

   _samplesPath = new QLineEdit("Samples (optional)");
   _samplesPath->setReadOnly(true);
   _samplesPath->setStyleSheet(d_samples);
   _samplesPath->setMinimumHeight(40);
   _samplesPath->setTextMargins(5, 5, 5, 5);
   

   DecentButton* browseSamplesButton = new DecentButton();
   browseSamplesButton->setText("Browse");
   browseSamplesButton->setFont(font);
   browseSamplesButton->setMinimumWidth(100);
   browseSamplesButton->setFixedHeight(40);
   connect(browseSamplesButton, SIGNAL(LabelClicked()),this, SLOT(browseSamples()));

   samplesRow->addWidget(_samplesPath);
   samplesRow->addWidget(browseSamplesButton);

   u_main_layout->addLayout(samplesRow);
 
   ////////////////////////////////////////////////////////////////////////////
   /// Upload & Cancel
   ////////////////////////////////////////////////////////////////////////////
   QFont uploadButtonFont( "Myriad Pro Regular", 15, QFont::Bold);

   QHBoxLayout* button = new QHBoxLayout;

   button->setSpacing(20);
   _upload_button = new DecentButton();
   _cancel_button = new DecentButton();

   _cancel_button->setText("Cancel");
   _cancel_button->setFont(uploadButtonFont);
   _cancel_button->setMinimumHeight(50);
   //_cancel_button->setMinimumWidth(140);
   _cancel_button->setStyleSheet(d_cancel);

   _upload_button->setText("Publish");
   _upload_button->setFont(uploadButtonFont);
   _upload_button->setMinimumHeight(50);
   //_upload_button->setMinimumWidth(140);

   connect(_upload_button, SIGNAL(LabelClicked()),this, SLOT(uploadContent()));
   connect(_cancel_button, SIGNAL(LabelClicked()),this, SLOT( uploadCanceled() ));

   button->setContentsMargins(20, 20, 20, 20);
   button->addWidget(_upload_button);
   button->addWidget(_cancel_button);

   u_main_layout->addLayout(button);
   u_main_layout->setContentsMargins(10, 10, 10, 10);
   u_main_layout->setSpacing(5);
   
   
   setWindowTitle("Upload new content");
   setStyleSheet(d_upload_popup);
   setLayout(u_main_layout);
   
   
   m_getPublishersTimer.setSingleShot(true);
   connect(&m_getPublishersTimer, SIGNAL(timeout()), SLOT(onGrabPublishers()));
   m_getPublishersTimer.start(1000);
   
   updateUploadButtonStatus();
   
   _buttonStatusCheck = new QTimer(this);
   connect(_buttonStatusCheck, SIGNAL(timeout()), SLOT(updateUploadButtonStatus()));
   _buttonStatusCheck->start(500);
}

void Upload_popup::onGrabPublishers() {
   std::string a_result;
   RunTask("list_publishers_by_price 100", a_result);
   
   //seeders popup ok button
   _seeder_ok = new DecentButton();
   _seeder_ok->setText("OK");
   _seeder_ok->setFixedHeight(30);
   _seeder_ok->setContentsMargins(0, 0, 0, 0);
   
   auto publishers = json::parse(a_result);

   for (int r = 0; r < publishers.size(); ++r) {
      std::string pubIdStr = publishers[r]["seeder"].get<std::string>();
      
      double price = publishers[r]["price"]["amount"].get<double>() / GRAPHENE_BLOCKCHAIN_PRECISION;
      
      std::string pubPrice = QString::number(price).toStdString();
      std::string pubAssetId = publishers[r]["price"]["asset_id"].get<std::string>();
      
      int free_space = publishers[r]["free_space"].get<int>();
      std::string pubFreeSpace = std::to_string(free_space) + "MB free";
      
      if (free_space > 800) {
          pubFreeSpace = QString::number(1.0 * free_space / 1024, 'f', 2).toStdString() + "GB free";
      }
      
      _publisherIdToPriceMap.insert(std::make_pair(pubIdStr, price));
      
      _seeders_checkbox[r] = new QCheckBox(_seeders_dialog);
      _seeders_checkbox[r]->setText(QString("%0 @%1 %2 [%3]").arg(QString::fromStdString(pubIdStr),
                                                          QString::fromStdString(pubPrice),
                                                          QString::fromStdString("DCT"),
                                                          QString::fromStdString(pubFreeSpace)) /*, QString::fromStdString(pubIdStr))*/);
      
      dialog_layout->addWidget(_seeders_checkbox[r]);
      dialog_layout->addWidget(_seeder_ok);
      
      QObject::connect(_seeders_checkbox[r], SIGNAL(stateChanged(int)), SLOT(stateChanged(const int)));
      QObject::connect(_seeder_ok, SIGNAL(LabelClicked()),_seeders_dialog, SLOT(close()));
   }
   
   
}

void Upload_popup::stateChanged(const int state)
{
   _checkedSeeders.clear();
   for (int i = 0; i < 3; ++i){
      if (_seeders_checkbox[i]->isChecked()){
         char buf[10];
         std::size_t _size = _seeders_checkbox[i]->text().toStdString().copy(buf, 6, 0);
         buf[_size] = '\0';
         
         _checkedSeeders.push_back(buf);
      }
   }
}

void Upload_popup::updateUploadButtonStatus() {
   std::string lifeTime    = _lifeTime->text().toStdString();
   //std::string seeders     = _seeders->currentData().toString().toStdString();
   //seeders push_back in stateChanged slot on _checkedSeeders member
   std::string keyparts    = _keyparts->currentData().toString().toStdString();
   std::string price       = _price->text().toStdString();
   std::string path        = _contentPath->text().toStdString();
   std::string samplesPath = _samplesPath->text().toStdString();
   
   std::string title = _titleText->text().toStdString();
   std::string desc = _descriptionText->toPlainText().toStdString();

   bool isValid = true;
   
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
   
   if (GlobalEvents::instance().getCurrentUser().empty())
      isValid = false;
   

   auto it = _publisherIdToPriceMap.find("b"); //So it was not empty
   if (_checkedSeeders.empty()) {
      isValid = false;
   }
   else{
      for (std::vector<std::string>::iterator iter = _checkedSeeders.begin(); iter != _checkedSeeders.end(); ++iter){
         it = _publisherIdToPriceMap.find(*iter);
         if (it == _publisherIdToPriceMap.end()) {
            isValid = false;
         }
      }
   }
   
   double publishingPrice = it->second;
   uint64_t size = std::max( (uint64_t)1, ( fileSize + (1024 * 1024) -1 ) / (1024 * 1024));
   double totalPricePerDay = size * publishingPrice;
   int days = QDate::currentDate().daysTo(_lifeTime->date());
   
   
   if (isValid) {
      _upload_button->setText(tr("Publish for ") + QString::number(days * totalPricePerDay) + tr(" DCT"));
      _upload_button->setEnabled(true);
   } else {
      _upload_button->setText("Publish");
      _upload_button->setEnabled(false);
   }
}


void Upload_popup::browseContent() {
    QString contentPathSelected = QFileDialog::getOpenFileName(this, tr("Select content"), "~");
   
   boost::system::error_code ec;
   if (boost::filesystem::file_size(contentPathSelected.toStdString(), ec) > 100 * 1024 * 1024) {
      ALERT("Content size is limited in Testnet 0.1 to 100MB");
      return;
   }
   
    _contentPath->setText(contentPathSelected);
}

void Upload_popup::browseSamples() {
    QString sampleDir = QFileDialog::getExistingDirectory(this, tr("Select samples"), "~", QFileDialog::DontResolveSymlinks);
    _samplesPath->setText(sampleDir);
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
   std::string m_price     = _price->text().toStdString();

   std::string assetName = "DCT";
   std::string path = _contentPath->text().toStdString();
   std::string samples_path = _samplesPath->text().toStdString();

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
   submitCommand += " " + GlobalEvents::instance().getCurrentUser();    // author
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
      RunTask(submitCommand, a_result);
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

   QMessageBox* msgBox = new QMessageBox();
   msgBox->setAttribute(Qt::WA_DeleteOnClose);

   if (message.empty())
   {
      _titleText->setText("");
      _descriptionText->setPlainText("");
      _lifeTime->setDate(QDate::currentDate());
      _price->setText("");
      _contentPath->setText("");
      _samplesPath->setText("");

      msgBox->setWindowTitle("Success");
      msgBox->setText(tr("Content is submitted"));

      setEnabled(true);

      emit uploadFinished();
   }
   else
   {
      msgBox->setWindowTitle("Error");
      msgBox->setText(tr("Failed to submit content"));
      msgBox->setDetailedText(message.c_str());
   }

   msgBox->open();
}


// UPLOAD TAB


Upload_tab::Upload_tab(Mainwindow_gui_wallet* parent) :  popup(0), _content_popup(NULL), _parent(parent) {

    m_pTableWidget.set_columns({
        {"Title", 20},
        {"Rating", 10, "rating"},
        {"Size", 10, "size"},
        {"Price", 10, "price"},
        {"Published", 10, "created"},
        {"Expiration", 10, "expiration"},
        {" ", -50}

    });
    
    QFont fontUpload( "Myriad Pro Regular", 14, QFont::Bold);

    upload_button = new DecentButton();
    upload_button->setFont(fontUpload);
    upload_button->setText("Publish");
    upload_button->setMinimumWidth(102);
    upload_button->setMinimumHeight(54);
    
    QLabel* lab = new QLabel();
    QPixmap image(icon_search);
    lab->setPixmap(image);
    
    m_filterLineEdit.setPlaceholderText("Search Content");
    m_filterLineEdit.setFixedHeight(54);
    m_filterLineEdit.setStyleSheet(d_lineEdit);
    m_filterLineEdit.setAttribute(Qt::WA_MacShowFocusRect, 0);
    
    m_search_layout.setContentsMargins(42, 0, 0, 0);
    m_search_layout.addWidget(lab);
    m_search_layout.addWidget(&m_filterLineEdit);
    m_search_layout.addWidget(upload_button, 0 , Qt::AlignBottom);
    
    m_main_layout.setContentsMargins(0, 0, 0, 0);
    m_main_layout.setSpacing(0);
    m_main_layout.addLayout(&m_search_layout);
    m_main_layout.addWidget(&m_pTableWidget);
    setLayout(&m_main_layout);
    
    connect(&m_filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(onTextChanged(QString)));
    connect(upload_button, SIGNAL(LabelClicked()), this, SLOT(uploadPopup()));
    connect(&GlobalEvents::instance(), SIGNAL(currentUserChanged(std::string)), this, SLOT(updateContents()));

    
}

void Upload_tab::timeToUpdate(const std::string& result) {
   _digital_contents.clear();

   if (result.empty()) {
      return;
   }
   
   auto contents = json::parse(result);
   
   _digital_contents.resize(contents.size());
   
   
   for (int i = 0; i < contents.size(); ++i) {
      SDigitalContent& cont = _digital_contents[i];
      
      cont.type = DCT::GENERAL;
      cont.author = contents[i]["author"].get<std::string>();
      cont.price.asset_id = contents[i]["price"]["asset_id"].get<std::string>();
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
      
      
      if (contents[i]["price"]["amount"].is_number()){
         cont.price.amount =  contents[i]["price"]["amount"].get<double>();
      } else {
         cont.price.amount =  std::stod(contents[i]["price"]["amount"].get<std::string>());
      }
      
      cont.price.amount /= GRAPHENE_BLOCKCHAIN_PRECISION;
      cont.AVG_rating = contents[i]["AVG_rating"].get<double>()  / 1000;
   }
   
   ShowDigitalContentsGUI();
   
}


std::string Upload_tab::getUpdateCommand() {
   std::string filterText = m_filterLineEdit.text().toStdString();
   
   std::string currentUserName = GlobalEvents::instance().getCurrentUser();
   
   if (currentUserName.empty()) {
      return "";
   }
   
   return   std::string("search_user_content ") +
            "\"" + currentUserName + "\" " +
            "\"" + filterText + "\" " +
            "\"" + m_pTableWidget.getSortedColumn() + "\" " +
            "\"\" " +   // region_code
            "100";
   
}



void Upload_tab::show_content_popup() {
   if (_isUploading) {
      return;
   }
   
   _isUploading = true;
   
    QLabel* btn = (QLabel*)sender();
    int id = btn->property("id").toInt();
    if (id < 0 || id >= _digital_contents.size()) {
        throw std::out_of_range("Content index is out of range");
    }
    
   if (_content_popup) {
        delete _content_popup;
      _content_popup = NULL;
   }
    _content_popup = new ContentDetailsGeneral(_parent);
    
    connect(_content_popup, SIGNAL(ContentWasBought()), this, SLOT(content_was_bought()));
    _content_popup->execCDD(_digital_contents[id]);
    
   _isUploading = false;
}

void Upload_tab::content_was_bought() {
   if (_content_popup) {
      delete _content_popup;
      _content_popup = NULL;
   }
   
    _parent->GoToThisTab(4, "");
    _parent->UpdateAccountBalances(GlobalEvents::instance().getCurrentUser());
}

void Upload_tab::ShowDigitalContentsGUI() {
    
    m_pTableWidget.setRowCount(_digital_contents.size());
    
    int index = 0;
    for(SDigitalContent& aTemporar: _digital_contents) {
        
        EventPassthrough<DecentSmallButton>* info_icon = new EventPassthrough<DecentSmallButton>(icon_popup, icon_popup_white);
        info_icon->setProperty("id", QVariant::fromValue(index));
        info_icon->setAlignment(Qt::AlignCenter);
        connect(info_icon, SIGNAL(clicked()), this, SLOT(show_content_popup()));
        m_pTableWidget.setCellWidget(index, 6, info_icon);
        
        // Need to rewrite this
        std::string created_str = aTemporar.created.substr(0, 10);
        
        m_pTableWidget.setItem(index,4,new QTableWidgetItem(QString::fromStdString(created_str)));
        m_pTableWidget.item(index, 4)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        m_pTableWidget.item(index, 4)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        
        
        std::string synopsis = unescape_string(aTemporar.synopsis);
        std::replace(synopsis.begin(), synopsis.end(), '\t', ' '); // JSON does not like tabs
        std::replace(synopsis.begin(), synopsis.end(), '\n', ' '); // JSON does not like newlines either
        //massageBox_title.push_back(	)
        
        try {
            auto synopsis_parsed = json::parse(synopsis);
            synopsis = synopsis_parsed["title"].get<std::string>();
            
        } catch (...) {}
        
        
        
        m_pTableWidget.setItem(index,0,new QTableWidgetItem(QString::fromStdString(synopsis)));
        m_pTableWidget.item(index, 0)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        m_pTableWidget.item(index, 0)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        
        std::string rating;
        for(int i = 0; i < std::to_string(aTemporar.AVG_rating).find(".") + 4; ++i)
        {
            rating.push_back(std::to_string(aTemporar.AVG_rating)[i]);
        }
        m_pTableWidget.setItem(index,1,new QTableWidgetItem(QString::fromStdString(rating)));
        m_pTableWidget.item(index, 1)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        m_pTableWidget.item(index, 1)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        
        
        
        if(aTemporar.size < 1024)
        {
            m_pTableWidget.setItem(index,2,new QTableWidgetItem(QString::fromStdString(std::to_string(aTemporar.size) + ".000 MB")));
        }
        else
        {
            float size = (float)aTemporar.size/1024;
            std::string size_s;
            std::string s = std::to_string(size);
            for(int i = 0; i < s.find(".") + 4; ++i)
            {
                size_s.push_back(s[i]);
            }
            m_pTableWidget.setItem(index,2,new QTableWidgetItem(QString::fromStdString(size_s + " GB")));
        }
        m_pTableWidget.item(index, 2)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        m_pTableWidget.item(index, 2)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        
        
        m_pTableWidget.setItem(index,3,new QTableWidgetItem(QString::number(aTemporar.price.amount,'f', 4) + " DCT"));
        m_pTableWidget.item(index, 3)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        m_pTableWidget.item(index, 3)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        
        
        QDateTime time = QDateTime::fromString(QString::fromStdString(aTemporar.expiration), "yyyy-MM-ddTHH:mm:ss");
        
        std::string e_str = CalculateRemainingTime(QDateTime::currentDateTime(), time);
        
        m_pTableWidget.setItem(index, 5, new QTableWidgetItem(QString::fromStdString(e_str)));
        m_pTableWidget.item(index, 5)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        m_pTableWidget.item(index, 5)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        
        ++index;
    }
}

void Upload_tab::uploadPopup() {
    popup.exec();
}

void Upload_popup::uploadCanceled()
{
    this->close();
}
