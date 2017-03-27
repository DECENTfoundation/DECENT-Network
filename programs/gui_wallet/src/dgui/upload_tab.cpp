/*
 *	File      : upload_tab.cpp
 *
 *	Created on: 21 Nov 2016
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */

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
#include "ui_wallet_functions.hpp"
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

#include <ctime>
#include <limits>
#include <iostream>
#include <graphene/chain/config.hpp>


#include <QDateTime>
#include <QDate>
#include <QTime>


using namespace gui_wallet;
using namespace nlohmann;

CryptoPP::AutoSeededRandomPool rng;


Upload_popup::Upload_popup(QWidget *parent)
:
QDialog(parent),
m_info_widget(3, 6),
m_title_label(tr("Title")),
m_description_label(tr("Description")),
m_infoLayoutHeader(tr("Content info:")),
m_getPublishersTimer(this)
{
    QFont m_font( "Open Sans Bold", 14, QFont::Bold);
    QPalette pltEdit;
    
    
    m_infoLayoutHeader.setFont(m_font);
    m_title_text.setPlaceholderText("  Title:");
    m_title_text.setAttribute(Qt::WA_MacShowFocusRect, 0);
    m_title_text.setFixedHeight(40);
    m_synopsis_layout.addWidget(&m_title_text);
    
    m_description_text.setPlaceholderText("  Description:");
    m_description_text.resize(138, 822);
    m_description_text.setFixedHeight(138);
    
    m_synopsis_layout.addWidget(&m_description_text);
    
    QFont font( "Open Sans Bold", 14, QFont::Bold);
    m_infoLayoutHeader.setFont(font);
    
    u_main_layout.addLayout(&m_synopsis_layout);
    u_main_layout.addWidget(&m_infoLayoutHeader);
    
    ////////////////////////////////////////////////////////////////////////////
    /// Lifetime
    ////////////////////////////////////////////////////////////////////////////
    
    QLabel* lifetime = new QLabel("LifeTime");
    lifetime->setStyleSheet("border:1px solid black");
    lifetime->setStyleSheet("border:0px");
    
    m_samplesPath = new QLineEdit("", this);
    m_samplesPath->setReadOnly(true);
    m_samplesPath->setHidden(true);
    
    de = new QDateEdit();
    de->setDate(QDate::currentDate());
    de->setDisplayFormat("yyyy-MM-dd");
    de->setCalendarPopup(true);
    de->setMinimumDate(QDate::currentDate());
    de->setStyle(QStyleFactory::create("fusion"));
    
    //////////////////////
    //////                             ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //////////////////////
    seeders = new QComboBox(this);
    
    
    QHBoxLayout* firstRow = new QHBoxLayout;
    
    //LIFETIME
    QLabel* lab = new QLabel("LifeTime");
    lab->setStyleSheet("border:0; color: Gray");
    lab->setContentsMargins(0, 0, -2, 0);
    lab->setMinimumWidth(60);
    lab->setFixedHeight(25);
    
    firstRow->addWidget(lab);
    firstRow->addWidget(de);
    
    //SEEDERS
    QLabel* seed = new QLabel("Seeders");
    seed->setStyleSheet("border:0; color: Gray");
    
    seed->setContentsMargins(20, 0, -2, 0);
    seed->setMinimumWidth(70);
    seed->setFixedHeight(25);
    seeders->setStyle(QStyleFactory::create("fusion"));
    firstRow->addWidget(seed);
    firstRow->addWidget(seeders);
    
    //KEYPARTICLES
    QLabel* key = new QLabel("Key Particles");
    key->setStyleSheet("border:0; color: Gray");
    key->setContentsMargins(20, 0, -2, 0);
    key->setMinimumWidth(90);
    key->setFixedHeight(25);
    
    keyparts = new QComboBox(this);
    keyparts->setStyle(QStyleFactory::create("fusion"));
    for (int r = 2; r <= 7; ++r) {
        QString val = QString::fromStdString(std::to_string(r));
        keyparts->addItem(val, val);
    }
    
    firstRow->addWidget(key);
    firstRow->addWidget(keyparts);
    
    u_main_layout.addLayout(firstRow);
    
    //\\//\\//\\\//\\\/SECOND ROW\//\\//\\//
    
    QHBoxLayout* secondrow = new QHBoxLayout;
    
    //PRICE
    price = new QLineEdit;
    price->setValidator( new QDoubleValidator(0.001, 100000, 3, this) );
    price->setPlaceholderText("Price");
    price->setAttribute(Qt::WA_MacShowFocusRect, 0);
    price->setStyleSheet("border:1px solid lightGray; color: Gray");
    price->setFixedHeight(30);
    price->setFixedWidth(150);
    price->setContentsMargins(0, 0, 10, 0);
    
    secondrow->setContentsMargins(0, 2, 0, 0);
    secondrow->addWidget(price);
    
    //SIMPLES
    sim = new QLineEdit("Samples");
    sim->setReadOnly(true);
    sim->setStyleSheet("border:1px solid lightGray; color: Gray");
    sim->setContentsMargins(10, 0, 0, 0);
    sim->setFixedWidth(270);
    sim->setFixedHeight(30);
    
    DecentButton* browse_samples_button = new DecentButton();
    browse_samples_button->setText("Browse");
    browse_samples_button->setFixedWidth(70);
    browse_samples_button->setFixedHeight(30);
    connect(browse_samples_button, SIGNAL(LabelClicked()),this, SLOT(browseSamples()));
    
    secondrow->addWidget(sim);
    secondrow->addWidget(browse_samples_button);
    
    //CONTENT
    cont = new QLineEdit("Path");
    cont->setReadOnly(true);
    cont->setStyleSheet("border:1px solid lightGray; color: Gray");
    cont->setContentsMargins(10, 0, 0, 0);
    cont->setFixedWidth(270);
    cont->setFixedHeight(30);
    
    m_contentPath = new QLineEdit("", this);
    m_contentPath->setReadOnly(true);
    m_contentPath->setHidden(true);
    
    DecentButton* browse_content_button = new DecentButton();
    browse_content_button->setText("Browse");
    browse_content_button->setFixedWidth(70);
    browse_content_button->setFixedHeight(30);
    connect(browse_content_button, SIGNAL(LabelClicked()),this, SLOT(browseContent()));
    
    secondrow->addWidget(cont);
    secondrow->addWidget(browse_content_button);
    
    u_main_layout.addLayout(secondrow);
    ////////////////////////////                             ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    ////////////////////////////////////////////////////////////////////////////
    /// Upload
    ////////////////////////////////////////////////////////////////////////////
    
    QHBoxLayout* button = new QHBoxLayout;
    
    DecentButton* upload_label = new DecentButton();
    upload_label->setText("UPLOAD");
    upload_label->setFixedHeight(30);
    upload_label->setFixedWidth(120);
    
    connect(upload_label, SIGNAL(LabelClicked()),this, SLOT(uploadContent()));
    button->setContentsMargins(250, 0, 250, 0);
    button->addWidget(upload_label);
    
    u_main_layout.addLayout(button);
    
    m_getPublishersTimer.setSingleShot(true);
    connect(&m_getPublishersTimer, SIGNAL(timeout()), SLOT(onGrabPublishers()));
    m_getPublishersTimer.start(1000);
}

void Upload_popup::onGrabPublishers() {
   
    AsyncTask("list_publishers_by_price 100", this, NULL, +[](void* owner, void* a_clbkArg, int64_t a_err, const std::string& a_task, const std::string& a_result) {
       Upload_popup* obj = (Upload_popup*)owner;
       
       auto publishers = json::parse(a_result);
        
        for (int r = 0; r < publishers.size(); ++r) {
            std::string pubIdStr = publishers[r]["seeder"].get<std::string>();
            std::string pubPrice = QString::number(publishers[r]["price"]["amount"].get<double>() / GRAPHENE_BLOCKCHAIN_PRECISION).toStdString();
            std::string pubAssetId = publishers[r]["price"]["asset_id"].get<std::string>();
            
            int free_space = publishers[r]["free_space"].get<int>();
            std::string pubFreeSpace = std::to_string(free_space) + "MB free";
            
            if (free_space > 800) {
                pubFreeSpace = QString::number(1.0 * free_space / 1024, 'f', 2).toStdString() + "GB free";
            }
            
            obj->seeders->addItem(QString("%0 @%1 %2 [%3]").arg(QString::fromStdString(pubIdStr),
                                                                QString::fromStdString(pubPrice),
                                                                QString::fromStdString("DCT"),
                                                                QString::fromStdString(pubFreeSpace)), QString::fromStdString(pubIdStr));
        }
        
        
    });
}

void Upload_popup::browseContent() {
    QString contentPathSelected = QFileDialog::getOpenFileName(this, tr("Select content"), "~");
    m_contentPath->setText(contentPathSelected);
    cont->setText(contentPathSelected);
}

void Upload_popup::browseSamples() {
    QString sampleDir = QFileDialog::getExistingDirectory(this, tr("Select samples"), "~", QFileDialog::DontResolveSymlinks);
    m_samplesPath->setText(sampleDir);
    sim->setText(sampleDir);
}


void Upload_popup::uploadContent() {
    std::string m_life_time = de->text().toStdString();
    std::string m_seeders   = seeders->currentData().toString().toStdString();
    std::string m_keyparts  = keyparts->currentData().toString().toStdString();
    std::string m_price     = price->text().toStdString();
    
    
    std::string assetName = "DCT";
    std::string path = m_contentPath->text().toStdString();
    std::string samples_path = m_samplesPath->text().toStdString();
    
    std::string title = m_title_text.text().toStdString();
    std::string desc = m_description_text.toPlainText().toStdString();
    
    if (m_price.empty()) {
        ALERT("Please specify price");
        return;
    }
    
    if (path.empty()) {
        ALERT("Please specify path");
        return;
    }
    
    
    boost::system::error_code ec;
    if (boost::filesystem::file_size(path, ec) > 100 * 1024 * 1024) {
        ALERT("Content size is limited in Testnet 0.1 to 100MB");
        return;
    }
    
    if (ec) {
        ALERT("Please select valid file for upload.");
        return;
    }
    
    
    if (title.empty()) {
        ALERT("Please specify title");
        return;
    }
    
    if (desc.empty()) {
        ALERT("Please specify description");
        return;
    }
    
    if (GlobalEvents::instance().getCurrentUser().empty()) {
        ALERT("Please select user to upload");
        return;
    }
    
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
    submitCommand += " " + GlobalEvents::instance().getCurrentUser();   //author
    submitCommand += " \"" + path + "\"";                               //URI
    submitCommand += " \"" + samples_path + "\"";                       //Samples
    submitCommand += " \"ipfs\"";                                     //Protocol
    submitCommand += " " + assetName;                                   //price_asset_name
    submitCommand += " " + m_price;                                       //price_amount
    submitCommand += " [" + m_seeders + "]";                              //seeders
    submitCommand += " \"" + m_life_time + "T23:59:59\"";                  //expiration
    submitCommand += " \"" + escape_string(synopsis) + "\"";            //synopsis
    submitCommand += " true";                                           //broadcast

   
   std::string a_result;
   std::string message;
   
   try {
      RunTask(submitCommand, a_result);
      if (a_result.find("exception:") != std::string::npos) {
         message = a_result;
      }
   } catch (const std::exception& ex) {
      message = ex.what();
      setEnabled(true);
   }
   
   QMessageBox* msgBox = new QMessageBox();
   msgBox->setAttribute(Qt::WA_DeleteOnClose);
   
   if (message.empty()) {
      m_title_text.setText("");
      m_description_text.setPlainText("");
      de->setDate(QDate::currentDate());
      price->setText("");
      m_contentPath->setText("");
      m_samplesPath->setText("");
      
      msgBox->setWindowTitle("Success");
      msgBox->setText(tr("Content is submitted"));

      setEnabled(true);

      emit uploadFinished();

   } else {
      msgBox->setWindowTitle("Error");
      msgBox->setText(tr("Failed to submit content"));
      msgBox->setDetailedText(message.c_str());
   }
   
   msgBox->open();

}

void Upload_popup::uploadDone(void* a_clbkArg, int64_t a_err, const std::string& a_task, const std::string& a_result) {

    if (a_err != 0) {
        //ALERT("Failed to submit content");
        setEnabled(true);
        return;
    }
    

}

void Upload_popup::resizeEvent ( QResizeEvent * event )
{
    QWidget::resizeEvent(event);
    
    QSize aInfWidgSize = m_info_widget.size();
    
    m_info_widget.setColumnWidth(0,15*aInfWidgSize.width()/100);
    m_info_widget.setColumnWidth(1,20*aInfWidgSize.width()/100);
    m_info_widget.setColumnWidth(2,15*aInfWidgSize.width()/100);
    m_info_widget.setColumnWidth(3,20*aInfWidgSize.width()/100);
    m_info_widget.setColumnWidth(4,15*aInfWidgSize.width()/100);
    m_info_widget.setColumnWidth(5,15*aInfWidgSize.width()/100);
}


// UPLOAD TAB

Upload_tab::Upload_tab(Mainwindow_gui_wallet* parent) :  _content_popup(NULL), _parent(parent) {
    
    m_pTableWidget.set_columns({
        {"Title", 20},
        {"Rating", 10},
        {"Size", 10},
        {"Price", 10},
        {"Uploaded", 10},
        {"Expiration", 10},
        {" ", -50}

    });
    
    
 
    upload_button = new DecentButton();
    upload_button->setText("UPLOAD");
    upload_button->setFixedWidth(150);
    QLabel* lab = new QLabel();
    QPixmap image(":/icon/images/search.svg");
    lab->setPixmap(image);
    
    m_filterLineEdit.setPlaceholderText("Enter search term");
    m_filterLineEdit.setFixedHeight(40);
   m_filterLineEdit.setStyleSheet("border: 0");
    m_filterLineEdit.setAttribute(Qt::WA_MacShowFocusRect, 0);
    
    m_search_layout.setContentsMargins(42, 0, 0, -50);
    m_search_layout.addWidget(lab);
    m_search_layout.addWidget(&m_filterLineEdit);
    m_search_layout.addWidget(upload_button);
    
    m_main_layout.setContentsMargins(0, 0, 0, 0);
    m_main_layout.addLayout(&m_search_layout);
    
    m_main_layout.addWidget(&m_pTableWidget);
    setLayout(&m_main_layout);
    
    connect(upload_button, SIGNAL(LabelClicked()), this, SLOT(upload_popup()));
   _isUploading = false;
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
   
   return "search_user_content \"" + currentUserName + "\" \"" + filterText + "\" 100";
   
}




void Upload_tab::show_content_popup() {
   if (_isUploading) {
      return;
   }
   
   _isUploading = true;
   
    QLabel* btn = (QLabel*)sender();
    int id = btn->property("id").toInt();
    if (id < 0 || id >= _digital_contents.size()) {
        throw std::out_of_range("Content index is our of range");
    }
    
   if (_content_popup) {
        delete _content_popup;
      _content_popup = NULL;
   }
    _content_popup = new ContentDetailsGeneral();
    
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
        
        EventPassthrough<DecentSmallButton>* info_icon = new EventPassthrough<DecentSmallButton>(":/icon/images/pop_up.png",":/icon/images/pop_up1.png");
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
        for(int i = 0; i < std::to_string(aTemporar.AVG_rating).find(".") + 2; ++i)
        {
            rating.push_back(std::to_string(aTemporar.AVG_rating)[i]);
        }
        m_pTableWidget.setItem(index,1,new QTableWidgetItem(QString::fromStdString(rating)));
        m_pTableWidget.item(index, 1)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        m_pTableWidget.item(index, 1)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        
        
        
        if(aTemporar.size < 1024)
        {
            m_pTableWidget.setItem(index,2,new QTableWidgetItem(QString::fromStdString(std::to_string(aTemporar.size) + " MB")));
        }
        else
        {
            float size = (float)aTemporar.size/1024;
            std::string size_s;
            std::string s = std::to_string(size);
            for(int i = 0; i < s.find(".") + 3; ++i)
            {
                size_s.push_back(s[i]);
            }
            m_pTableWidget.setItem(index,2,new QTableWidgetItem(QString::fromStdString(size_s + " GB")));
        }
        m_pTableWidget.item(index, 2)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        m_pTableWidget.item(index, 2)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        
        
        m_pTableWidget.setItem(index,3,new QTableWidgetItem(QString::number(aTemporar.price.amount) + " DCT"));
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


void Upload_tab::upload_popup()
{
    Upload_popup* popup = new Upload_popup();
    connect(popup,SIGNAL(uploadFinished()),popup,SLOT(close()));
    popup->setLayout(&popup->u_main_layout);
    popup->show();
}

