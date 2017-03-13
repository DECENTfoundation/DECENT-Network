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

using namespace gui_wallet;
using namespace nlohmann;

CryptoPP::AutoSeededRandomPool rng;




Upload_tab::Upload_tab()
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

    m_main_layout.addLayout(&m_synopsis_layout);
    m_main_layout.addWidget(&m_infoLayoutHeader);
    //m_info_layout.addWidget(&m_infoLayoutHeader);

    ////////////////////////////////////////////////////////////////////////////
    /// Lifetime
    ////////////////////////////////////////////////////////////////////////////

    QLabel* lifetime = new QLabel("LifeTime");
    lifetime->setStyleSheet("border:0px");
//    m_info_widget.setCellWidget(0, 0, lifetime);
    
    m_samplesPath = new QLineEdit("", this);
    m_samplesPath->setReadOnly(true);
    m_samplesPath->setHidden(true);
    
    de = new QDateEdit();
    de->setDate(QDate::currentDate());
    de->setDisplayFormat("yyyy-MM-dd");
    de->setCalendarPopup(true);
    //de->setMinimumDate(QDate::currentDate());
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
    
    m_main_layout.addLayout(firstRow);
    
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
//    browse_samples_button->setContentsMargins(-1, 0, 10, 0);
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
    
    m_main_layout.addLayout(secondrow);
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

    //m_main_layout.addLayout(&m_info_layout);
    m_main_layout.addLayout(button);

    setLayout(&m_main_layout);

    m_getPublishersTimer.setSingleShot(true);
    connect(&m_getPublishersTimer, SIGNAL(timeout()), SLOT(onGrabPublishers()));
    m_getPublishersTimer.start(1000);
}

void Upload_tab::onGrabPublishers() {
    
    SetNewTask("list_publishers_by_price 100", this, NULL, +[](void* owner, void* a_clbkArg, int64_t a_err, const std::string& a_task, const std::string& a_result) {
        Upload_tab* obj = (Upload_tab*)owner;

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

void Upload_tab::browseContent() {
    QString contentPathSelected = QFileDialog::getOpenFileName(this, tr("Select content"), "~");
    m_contentPath->setText(contentPathSelected);
    cont->setText(contentPathSelected);
}

void Upload_tab::browseSamples() {
    QString sampleDir = QFileDialog::getExistingDirectory(this, tr("Select samples"), "~", QFileDialog::DontResolveSymlinks);
    m_samplesPath->setText(sampleDir);
    sim->setText(sampleDir);
}


void Upload_tab::uploadContent() {
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
        ((Upload_tab*)owner)->uploadDone(a_clbkArg, a_err, a_task, a_result);
    });
}

Upload_tab::~Upload_tab()
{
}

void Upload_tab::uploadDone(void* a_clbkArg, int64_t a_err, const std::string& a_task, const std::string& a_result) {
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

void Upload_tab::resizeEvent ( QResizeEvent * event )
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
