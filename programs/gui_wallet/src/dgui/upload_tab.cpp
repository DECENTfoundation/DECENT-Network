///*
// *	File      : upload_tab.cpp
// *
// *	Created on: 21 Nov 2016
// *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
// *
// *  This file implements ...
// *
// */
//
#include "upload_tab.hpp"
#include "ui_wallet_functions.hpp"
#include "gui_wallet_global.hpp"

#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QCalendarWidget>
#include <QDate>
#include <QDateEdit>
#include <QDateTime>
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

#include "json.hpp"

#include <QLayout>
#include <QCheckBox>
#include <stdio.h>
#include <QHeaderView>
#include <QLabel>
#include <QMouseEvent>
#include <stdarg.h>
#include "decent_button.hpp"

#include <ctime>
#include <limits>
#include <iostream>
#include <graphene/chain/config.hpp>

#include "gui_wallet_centralwidget.hpp"
#include "decent_wallet_ui_gui_contentdetailsbase.hpp"

#include <QTime>

using namespace gui_wallet;
using namespace nlohmann;

CryptoPP::AutoSeededRandomPool rng;




Upload_tab_pop_up::Upload_tab_pop_up()
        :
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
    //m_info_layout.addWidget(&m_infoLayoutHeader);

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

//    QPixmap image(":/icon/images/browse.svg");

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

    //setLayout(&m_main_layout);

    m_getPublishersTimer.setSingleShot(true);
    connect(&m_getPublishersTimer, SIGNAL(timeout()), SLOT(onGrabPublishers()));
    m_getPublishersTimer.start(1000);
}

void Upload_tab_pop_up::onGrabPublishers() {
    
    SetNewTask("list_publishers_by_price 100", this, NULL, +[](void* owner, void* a_clbkArg, int64_t a_err, const std::string& a_task, const std::string& a_result) {
        Upload_tab_pop_up* obj = (Upload_tab_pop_up*)owner;

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
        
        //obj->m_info_widget.setCellWidget(0, 3, obj->seeders);

    });
}

void Upload_tab_pop_up::browseContent() {
    QString contentPathSelected = QFileDialog::getOpenFileName(this, tr("Select content"), "~");
    m_contentPath->setText(contentPathSelected);
    cont->setText(contentPathSelected);
}

void Upload_tab_pop_up::browseSamples() {
    QString sampleDir = QFileDialog::getExistingDirectory(this, tr("Select samples"), "~", QFileDialog::DontResolveSymlinks);
    m_samplesPath->setText(sampleDir);
    sim->setText(sampleDir);
}


void Upload_tab_pop_up::uploadContent() {
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


    SetNewTask(submitCommand, this, NULL, +[](void* owner, void* a_clbkArg, int64_t a_err, const std::string& a_task, const std::string& a_result) {
        ((Upload_tab_pop_up*)owner)->uploadDone(a_clbkArg, a_err, a_task, a_result);
    });
}

Upload_tab_pop_up::~Upload_tab_pop_up()
{
}

void Upload_tab_pop_up::uploadDone(void* a_clbkArg, int64_t a_err, const std::string& a_task, const std::string& a_result) {
    if (a_err != 0) {
        ALERT("Failed to submit content");
        setEnabled(true);
        return;
    }

    // On success reset only these.
    m_title_text.setText("");
    m_description_text.setPlainText("");
    de->setDate(QDate::currentDate());
    price->setText("");
    m_contentPath->setText("");
    m_samplesPath->setText("");

    ALERT("Content is submitted!");
    setEnabled(true);
}

void Upload_tab_pop_up::resizeEvent ( QResizeEvent * event )
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




// =============[][\[][][][]]][][][][[][===Ubload tabs

//namespace DCF {enum DIG_CONT_FIELDS{TIME,SYNOPSIS,RATING,SIZE,PRICE,LEFT};}
static const char* s_vccpItemNames[]={"Title","Rating",
    "Size","Price","Time","Left",""};
static const int   s_cnNumberOfCols = sizeof(s_vccpItemNames)/sizeof(const char*);

static const int   s_cnNumberOfSearchFields(sizeof(gui_wallet::S_T::s_vcpcSearchTypeStrs)/sizeof(const char*));

using namespace gui_wallet;
using namespace nlohmann;


Upload_tab::Upload_tab() : m_pTableWidget(new UTableWidget(0,s_cnNumberOfCols)), green_row(-1)
{
    
    PrepareTableWidgetHeaderGUI();
    green_row = 0;
    for(int i(0); i<s_cnNumberOfSearchFields;++i){m_searchTypeCombo.addItem(tr(S_T::s_vcpcSearchTypeStrs[i]));}
    m_searchTypeCombo.setCurrentIndex(0);
    
    m_filterLineEdit.setStyleSheet( "{"
                                   "background: #f3f3f3;"
                                   "background-image: url(:Images/search.svg); /* actual size, e.g. 16x16 */"
                                   "background-repeat: no-repeat;"
                                   "background-position: left;"
                                   "color: #252424;"
                                   "font-family: SegoeUI;"
                                   "font-size: 12px;"
                                   "padding: 2 2 2 20; /* left padding (last number) must be more than the icon's width */"
                                   "}");
    QLabel* lab = new QLabel();
    QPixmap image(":/icon/images/search.svg");
    lab->setPixmap(image);
    
    DecentButton* upload_button = new DecentButton();
    upload_button->setFixedWidth(100);
    upload_button->setFixedHeight(40);
    upload_button->setText("Upload");
    upload_button->setContentsMargins(0, 0, 0, 0);
    
    m_filterLineEdit.setPlaceholderText("Enter search term");
    m_filterLineEdit.setFixedHeight(40);
    m_filterLineEdit.setStyleSheet("border: 1px solid white");
    m_filterLineEdit.setAttribute(Qt::WA_MacShowFocusRect, 0);
    
    m_search_layout.setContentsMargins(42, 0, 0, 0);
    m_search_layout.addWidget(lab);
    m_search_layout.addWidget(&m_filterLineEdit);
    m_search_layout.addWidget(upload_button);
    m_main_layout.addLayout(&m_search_layout);
    m_main_layout.addWidget(m_pTableWidget);
    setLayout(&m_main_layout);
    
    connect(&m_filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(onTextChanged(QString)));
    
    
    m_contentUpdateTimer.connect(&m_contentUpdateTimer, SIGNAL(timeout()), this, SLOT(maybeUpdateContent()));
    m_contentUpdateTimer.setInterval(1000);
    m_contentUpdateTimer.start();
    connect(m_pTableWidget,SIGNAL(mouseMoveEventDid()),this,SLOT(doRowColor()));
    connect(upload_button, SIGNAL(LabelClicked()), this, SLOT( UploadPopUp() ));
    ArrangeSize();
}


Upload_tab::~Upload_tab()
{
    m_main_layout.removeWidget(m_pTableWidget);
    delete m_pTableWidget;
}

void Upload_tab::DigContCallback(_NEEDED_ARGS2_)
{
    
    emit ShowDetailsOnDigContentSig(*a_pDigContent);
}

void Upload_tab::maybeUpdateContent() {
    if (!m_doUpdate) {
        return;
    }
    
    m_doUpdate = false;
    updateContents();
    ArrangeSize();
}

void Upload_tab::onTextChanged(const QString& text) {
    
    m_doUpdate = true;
    ArrangeSize();
}

void Upload_tab::PrepareTableWidgetHeaderGUI()
 {
    UTableWidget& m_TableWidget = *m_pTableWidget;
    
    QFont font( "Open Sans Bold", 14, QFont::Bold);
    
    //    m_TableWidget.setStyleSheet("QTableWidget{border : 1px solid red}");
    
    m_TableWidget.horizontalHeader()->setDefaultSectionSize(300);
    m_TableWidget.setRowHeight(0,35);
    m_TableWidget.verticalHeader()->hide();
    
    m_TableWidget.setHorizontalHeaderLabels(QStringList() << "Title" << "Rating" << "Size" << "Price" << "Time" << "Left"  << "" );
    m_TableWidget.horizontalHeader()->setFixedHeight(35);
    m_TableWidget.horizontalHeader()->setFont(font);
    //    m_TableWidget.horizontalHeader()->setStyleSheet("color:rgb(228,227,228)");
    
    m_main_layout.setContentsMargins(0, 0, 0, 0);
    
    m_TableWidget.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_TableWidget.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_TableWidget.setSelectionMode(QAbstractItemView::NoSelection);
    
    m_TableWidget.horizontalHeader()->setStyleSheet("QHeaderView::section {"
                                                    "border-right: 1px solid rgb(193,192,193);"
                                                    "border-bottom: 0px;"
                                                    "border-top: 0px;}");
    
    Connects();
    ArrangeSize();
}




void Upload_tab::updateContents() {
    std::string filterText = m_filterLineEdit.text().toStdString();

    if(filterText.empty())
    {
        filterText = GlobalEvents::instance().getCurrentUser();
    }
    
        SetNewTask("search_content \"" + filterText + "\" 100", this, NULL, +[](void* owner, void* a_clbkArg, int64_t a_err, const std::string& a_task, const std::string& a_result) {
        Upload_tab* obj = (Upload_tab*)owner;
        
        try {
            auto contents = json::parse(a_result);
            
            std::vector<SDigitalContent> dContents;
            dContents.clear();
            dContents.resize(contents.size());
            
            
            for (int i = 0; i < contents.size(); ++i) {
                dContents[i].type = DCT::GENERAL;
                
                dContents[i].author = contents[i]["author"].get<std::string>();
                
                
                
                dContents[i].price.asset_id = contents[i]["price"]["asset_id"].get<std::string>();
                dContents[i].synopsis = contents[i]["synopsis"].get<std::string>();
                dContents[i].URI = contents[i]["URI"].get<std::string>();
                dContents[i].created = contents[i]["created"].get<std::string>();
                dContents[i].expiration = contents[i]["expiration"].get<std::string>();
                dContents[i].size = contents[i]["size"].get<int>();
                
                if (contents[i]["times_bougth"].is_number()) {
                    dContents[i].times_bougth = contents[i]["times_bougth"].get<int>();
                } else {
                    dContents[i].times_bougth = 0;
                }
                
                
                if (contents[i]["price"]["amount"].is_number()){
                    dContents[i].price.amount =  contents[i]["price"]["amount"].get<double>();
                } else {
                    dContents[i].price.amount =  std::stod(contents[i]["price"]["amount"].get<std::string>());
                }
                
                dContents[i].price.amount /= GRAPHENE_BLOCKCHAIN_PRECISION;
                
                dContents[i].AVG_rating = contents[i]["AVG_rating"].get<double>()  / 1000;
            }
            
            
            obj->ShowDigitalContentsGUI(dContents);
            obj->ArrangeSize();
        } catch (std::exception& ex) {
        }
    });
    Connects();
    ArrangeSize();
}



void Upload_tab::ShowDigitalContentsGUI(std::vector<SDigitalContent>& contents)
{
    
    m_main_layout.removeWidget(m_pTableWidget);
    delete m_pTableWidget;
    
    
    m_pTableWidget = new UTableWidget(contents.size(), s_cnNumberOfCols);
    
    
    
    UTableWidget& m_TableWidget = *m_pTableWidget;
    
    PrepareTableWidgetHeaderGUI();
    
    int index = 0;
    for(SDigitalContent& aTemporar: contents)
    {
        std::string created_str;
        for(int i = 0; i < 10; ++i)
        {
            created_str.push_back(aTemporar.created[i]);
        }
        
        m_TableWidget.setItem(index,4,new QTableWidgetItem(QString::fromStdString(created_str)));
        m_TableWidget.item(index, 4)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        m_TableWidget.item(index, 4)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        
        
        std::string synopsis = unescape_string(aTemporar.synopsis);
        std::replace(synopsis.begin(), synopsis.end(), '\t', ' '); // JSON does not like tabs :(
        std::replace(synopsis.begin(), synopsis.end(), '\n', ' '); // JSON does not like newlines either :(
        //massageBox_title.push_back(	)
        
        try {
            auto synopsis_parsed = json::parse(synopsis);
            synopsis = synopsis_parsed["title"].get<std::string>();
            
        } catch (...) {}
        
        
        
        m_TableWidget.setItem(index, 0, new QTableWidgetItem(QString::fromStdString(synopsis)));
        m_TableWidget.item(index, 0)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        m_TableWidget.item(index, 0)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        
        std::string rating;
        for(int i = 0; i < std::to_string(aTemporar.AVG_rating).find(".") + 2; ++i)
        {
            rating.push_back(std::to_string(aTemporar.AVG_rating)[i]);
        }
        m_TableWidget.setItem(index,1,new QTableWidgetItem(QString::fromStdString(rating)));
        m_TableWidget.item(index, 1)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        m_TableWidget.item(index, 1)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        
        
        
        if(aTemporar.size < 1024)
        {
            m_TableWidget.setItem(index, 2,new QTableWidgetItem(QString::fromStdString(std::to_string(aTemporar.size) + " MB")));
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
            m_TableWidget.setItem(index, 2,new QTableWidgetItem(QString::fromStdString(size_s + " GB")));
        }
        m_TableWidget.item(index, 2)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        m_TableWidget.item(index, 2)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        
        
        m_TableWidget.setItem(index, 3, new QTableWidgetItem(QString::number(aTemporar.price.amount) + " DCT"));
        m_TableWidget.item(index, 3)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        m_TableWidget.item(index, 3)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        
        
        QDateTime time = QDateTime::fromString(QString::fromStdString(aTemporar.expiration), "yyyy-MM-ddTHH:mm:ss");
        
        e_str = CalculateRemainingTime(QDateTime::currentDateTime(), time);
        
        m_TableWidget.setItem(index, 5,new QTableWidgetItem(QString::fromStdString(e_str)));
        m_TableWidget.item(index, 5)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        m_TableWidget.item(index, 5)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        
        QPixmap image1(":/icon/images/info1_white.svg");
        m_TableWidget.setCellWidget(index, 6, new TableWidgetItemW<UButton>(aTemporar,this,NULL,&Upload_tab::DigContCallback,
                                                                            tr("")));
        ((UButton*)m_TableWidget.cellWidget(index, 6))->setPixmap(image1);
        ((UButton*)m_TableWidget.cellWidget(index, 6))->setAlignment(Qt::AlignCenter);
        
        ++index;
    }
    
    m_main_layout.addWidget(&m_TableWidget);
    
    Connects();
    
    m_pTableWidget->setStyleSheet("QTableView{border : 1px solid white}");
    m_pTableWidget->horizontalHeader()->setStretchLastSection(true);
    ArrangeSize();
}


void Upload_tab::ArrangeSize()
{
    QSize tqs_TableSize = m_pTableWidget->size();
    
    m_pTableWidget->setColumnWidth(0,(tqs_TableSize.width()*24)/100);
    m_pTableWidget->setColumnWidth(1,(tqs_TableSize.width()*8)/100);
    
    
    for(int i = 2; i < 6; ++i)
    {
        m_pTableWidget->setColumnWidth(i,(tqs_TableSize.width()*15)/100);
    }
    m_pTableWidget->setColumnWidth(6,(tqs_TableSize.width()*8)/100);
}


void Upload_tab::resizeEvent ( QResizeEvent * a_event )
{
    QWidget::resizeEvent(a_event);
    ArrangeSize();
}


void Upload_tab::Connects()
{
    connect(m_pTableWidget,SIGNAL(mouseMoveEventDid()),this,SLOT(doRowColor()));
    for(int i = 0; i < m_pTableWidget->rowCount(); ++i)
        connect((UButton*)m_pTableWidget->cellWidget(i, 6),SIGNAL(mouseWasMoved()),this,SLOT(doRowColor()));
}

void Upload_tab::doRowColor()
{
    if(m_pTableWidget->rowCount() < 1)  { return; }
    if(green_row >= 0)
    {
        for(int i = 0; i < 6; ++i)
        {
            m_pTableWidget->item(green_row,i)->setBackgroundColor(QColor(255,255,255));
            m_pTableWidget->item(green_row,i)->setForeground(QColor::fromRgb(0,0,0));
        }
        m_pTableWidget->cellWidget(green_row , 6)->setStyleSheet("* { background-color: rgb(255,255,255); color : white; }");
    }
    
    QPoint mouse_pos = m_pTableWidget->mapFromGlobal(QCursor::pos());
    if(mouse_pos.x() > m_pTableWidget->size().width() - 400 && mouse_pos.x() < m_pTableWidget->size().width())
    {
        mouse_pos.setX(mouse_pos.x() - 300);
    }
    mouse_pos.setY(mouse_pos.y() - 41);
    QTableWidgetItem *ite = m_pTableWidget->itemAt(mouse_pos);
    if(ite != NULL)
    {
        int row = ite->row();
        if(row < 0) {return;}
        
        for(int i = 0; i < 6; ++i)
        {
            m_pTableWidget->item(row,i)->setBackgroundColor(QColor(27,176,104));
            m_pTableWidget->item(row,i)->setForeground(QColor::fromRgb(255,255,255));
        }
        m_pTableWidget->cellWidget(row , 6)->setStyleSheet("* { background-color: rgb(27,176,104); color : white; }");
        
        green_row = row;
    }
    else
    {
        green_row = 0;
    }
}

void UTableWidget::mouseMoveEvent(QMouseEvent *event)
{
    mouseMoveEventDid();
}

void Upload_tab::UploadPopUp()
{
    upload_up* popup = new upload_up();
    Upload_tab_pop_up* up = new Upload_tab_pop_up();
    
    popup->setWindowTitle("New Upload");
    popup->setLayout(&up->u_main_layout);
    popup->show();
    
}

upload_up::upload_up(QWidget *parent)
    :QDialog(parent)
{}

