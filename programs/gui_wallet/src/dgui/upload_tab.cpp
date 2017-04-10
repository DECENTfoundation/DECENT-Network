

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
#include <graphene/chain/config.hpp>


#include <QDateTime>
#include <QDate>
#include <QTime>


using namespace gui_wallet;
using namespace nlohmann;

CryptoPP::AutoSeededRandomPool rng;


Upload_popup::Upload_popup(Mainwindow_gui_wallet* pMainWindow)
:

m_getPublishersTimer(this)
{
    QPalette pltEdit;
    
    
    m_title_text.setPlaceholderText("Title:");
    m_title_text.setStyleSheet(d_lineEdit);
    m_title_text.setAttribute(Qt::WA_MacShowFocusRect, 0);
    m_title_text.setMinimumHeight(44);
    m_title_text.setContentsMargins(0, 0, 0, 0);
    
    m_synopsis_layout.setContentsMargins(0, 0, 0, 0);
    m_synopsis_layout.addWidget(&m_title_text);
    
    m_description_text.setContentsMargins(0, 0, 0, 0);
    m_description_text.setPlaceholderText("Description:");
    m_description_text.setStyleSheet(d_desc);
    m_description_text.setMinimumHeight(161);
    m_description_text.setContentsMargins(0, 0, 0, 0);
    
    m_synopsis_layout.setMargin(0);
    m_synopsis_layout.addWidget(&m_description_text);
    
    
    u_main_layout.addLayout(&m_synopsis_layout);
    
    ////////////////////////////////////////////////////////////////////////////
    /// Lifetime
    ////////////////////////////////////////////////////////////////////////////
    
    m_samplesPath = new QLineEdit("", this);
    m_samplesPath->setReadOnly(true);
    m_samplesPath->setHidden(true);
    
    de = new QDateEdit();
    de->setDate(QDate::currentDate());
    de->setDisplayFormat("yyyy-MM-dd");
    de->setCalendarPopup(true);
    de->setMinimumDate(QDate::currentDate());
    de->setStyle(QStyleFactory::create("fusion"));
    de->setMinimumHeight(44);
    de->setFixedWidth(220);
    
    QHBoxLayout* firstRow = new QHBoxLayout;
    
    //LIFETIME
    QLabel* lab = new QLabel("  LifeTime");
    lab->setStyleSheet(d_label);
    lab->setContentsMargins(0, 0, -5, 0);
    lab->setMinimumWidth(60);
    lab->setMinimumHeight(44);
    
    firstRow->setMargin(0);
    firstRow->addWidget(lab);
    firstRow->addWidget(de);
    u_main_layout.addLayout(firstRow);
    
    //SEEDERS
    QHBoxLayout* seedRow = new QHBoxLayout;

    seeders      = new QComboBox(this);
    QLabel* seed = new QLabel("  Seeders");
    seed->setStyleSheet(d_label);
    
    seed->setContentsMargins(0, 0, 0, 0);
    seed->setMinimumWidth(70);
    seed->setMinimumHeight(44);
    seeders->setStyle(QStyleFactory::create("fusion"));
    seeders->setStyleSheet("color : black;");
    seeders->setMinimumHeight(44);
    seeders->setFixedWidth(220);
    
    seedRow->addWidget(seed);
    seedRow->addWidget(seeders);
    seedRow->setMargin(0);
    u_main_layout.addLayout(seedRow);
    
    //KEYPARTICLES
    QHBoxLayout* keyRow = new QHBoxLayout;

    QLabel* key = new QLabel("  Key Particles");
    key->setStyleSheet(d_label);
    key->setContentsMargins(0, 0, 0, 0);
    key->setMinimumWidth(90);
    key->setMinimumHeight(44);
    
    keyparts = new QComboBox(this);
    keyparts->setStyle(QStyleFactory::create("fusion"));
    keyparts->setStyleSheet("color : black;");
    keyparts->setMinimumHeight(44);
   keyparts->setFixedWidth(220);
   
    for (int r = 2; r <= 7; ++r) {
        QString val = QString::fromStdString(std::to_string(r));
        keyparts->addItem(val, val);
    }
    
    keyRow->addWidget(key);
    keyRow->addWidget(keyparts);
    
    u_main_layout.addLayout(keyRow);
    
    //PRICE
    QHBoxLayout* priceRow = new QHBoxLayout;
    
    price = new QLineEdit;
    price->setValidator( new QDoubleValidator(0.001, 100000, 3, this) );
    price->setPlaceholderText("Price");
    price->setAttribute(Qt::WA_MacShowFocusRect, 0);
    price->setStyleSheet(d_price);
    price->setMinimumHeight(44);
    price->setContentsMargins(0, 0, 0, 0);
    
    priceRow->setContentsMargins(0, 0, 0, 0);
    priceRow->addWidget(price);
    
    u_main_layout.addLayout(priceRow);
    
    //CONTENT
    QFont fontBrowse( "Myriad Pro Regular", 13, QFont::Bold);
    QHBoxLayout* contRow = new QHBoxLayout;
    
    cont = new QLineEdit("  Path");
    cont->setReadOnly(true);
    cont->setStyleSheet(d_label);
    cont->setContentsMargins(0, 0, 0, 0);
    cont->setMinimumHeight(44);
    
    m_contentPath = new QLineEdit("", this);
    m_contentPath->setReadOnly(true);
    m_contentPath->setHidden(true);
    
    DecentButton* browse_content_button = new DecentButton();
    browse_content_button->setText("Browse");
    browse_content_button->setFont(fontBrowse);
    browse_content_button->setMinimumWidth(105);
    browse_content_button->setFixedHeight(43);
    connect(browse_content_button, SIGNAL(LabelClicked()),this, SLOT(browseContent()));
    
    contRow->addWidget(cont);
    contRow->addWidget(browse_content_button);

    u_main_layout.addLayout(contRow);
    
    //SIMPLES
    QHBoxLayout* simRow = new QHBoxLayout;
    
    sim = new QLineEdit("  Samples(Optional)");
    sim->setReadOnly(true);
    sim->setStyleSheet(d_label);
    sim->setContentsMargins(0, 0, 0, 0);
    sim->setMinimumHeight(44);
    
    DecentButton* browse_samples_button = new DecentButton();
    browse_samples_button->setText("Browse");
    browse_samples_button->setFont(fontBrowse);
    browse_samples_button->setMinimumWidth(105);
    browse_samples_button->setFixedHeight(43);
    connect(browse_samples_button, SIGNAL(LabelClicked()),this, SLOT(browseSamples()));
    
    simRow->addWidget(sim);
    simRow->addWidget(browse_samples_button);
    
    u_main_layout.addLayout(simRow);
    
    ////////////////////////////                             ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    ////////////////////////////////////////////////////////////////////////////
    /// Upload & Cancel
    ////////////////////////////////////////////////////////////////////////////
    QFont fontCanUplo( "Myriad Pro Regular", 15, QFont::Bold);
    
    QHBoxLayout* button = new QHBoxLayout;
   
    button->setSpacing(20);
    DecentButton* upload_label = new DecentButton();
    DecentButton* cancel_label = new DecentButton();

    cancel_label->setText("Cancel");
    cancel_label->setFont(fontCanUplo);
    cancel_label->setMinimumHeight(48);
    cancel_label->setMinimumWidth(137);
    cancel_label->setStyleSheet(d_cancel);
    
    upload_label->setText("Upload");
    upload_label->setFont(fontCanUplo);
    upload_label->setMinimumHeight(48);
    upload_label->setMinimumWidth(137);
    
    connect(upload_label, SIGNAL(LabelClicked()),this, SLOT(uploadContent()));
    connect(cancel_label, SIGNAL(LabelClicked()),this, SLOT( uploadCanceled() ));

    button->setContentsMargins(161, 30, 161, 20);
    button->addWidget(upload_label);
    button->addWidget(cancel_label);

    u_main_layout.addLayout(button);
    u_main_layout.setContentsMargins(0, 0, 0, 5 );
    u_main_layout.setSpacing(0);
    
    m_getPublishersTimer.setSingleShot(true);
    connect(&m_getPublishersTimer, SIGNAL(timeout()), SLOT(onGrabPublishers()));
    m_getPublishersTimer.start(1000);
}


void Upload_popup::onGrabPublishers() {
   std::string a_result;
   RunTask("list_publishers_by_price 100", a_result);
    //AsyncTask("list_publishers_by_price 100", this, NULL, +[](void* owner, void* a_clbkArg, int64_t a_err, const std::string& a_task, const std::string& a_result) {
       //Upload_popup* obj = (Upload_popup*)owner;
       
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
            
            seeders->addItem(QString("%0 @%1 %2 [%3]").arg(QString::fromStdString(pubIdStr),
                                                                QString::fromStdString(pubPrice),
                                                                QString::fromStdString("DCT"),
                                                                QString::fromStdString(pubFreeSpace)), QString::fromStdString(pubIdStr));
        }
        
        
    //});
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


Upload_tab::Upload_tab(Mainwindow_gui_wallet* parent) :  popup(0), _content_popup(NULL), _parent(parent) {

    m_pTableWidget.set_columns({
        {"Title", 20},
        {"Rating", 10, "rating"},
        {"Size", 10, "size"},
        {"Price", 10, "price"},
        {"Uploaded", 10, "created"},
        {"Expiration", 10, "expiration"},
        {" ", -50}

    });
    
    QFont fontUpload( "Myriad Pro Regular", 14, QFont::Bold);

    upload_button = new DecentButton();
    upload_button->setFont(fontUpload);
    upload_button->setText("Upload");
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

    m_contentUpdateTimer.connect(&m_contentUpdateTimer, SIGNAL(timeout()), this, SLOT(maybeUpdateContent()));
    m_contentUpdateTimer.connect(&GlobalEvents::instance(), SIGNAL(walletUnlocked()), this, SLOT(requestContentUpdate()));
    
    m_contentUpdateTimer.setInterval(1000);
    m_contentUpdateTimer.start();
    
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
   
   return "search_user_content \"" + currentUserName + "\" \"" + filterText + "\" \"" + m_pTableWidget.getSortedColumn() + "\" 100";
   
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


void Upload_tab::uploadPopup()
{

    popup.setWindowTitle("Upload");
    popup.setLayout(&popup.u_main_layout);
    popup.setStyleSheet(d_upload_popup);
    popup.show();
}

void Upload_popup::uploadCanceled()
{
    this->close();
}
