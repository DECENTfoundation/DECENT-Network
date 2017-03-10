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
        m_infoLayoutHeader(tr("Information About Content")),
        m_getPublishersTimer(this)
{
    QFont m_font( "Open Sans Bold", 14, QFont::Bold);
    QPalette pltEdit;
    

    m_infoLayoutHeader.setFont(m_font);
    m_title_text.setPlaceholderText("  Title");
    m_title_text.setAttribute(Qt::WA_MacShowFocusRect, 0);
    m_title_text.setFixedHeight(40);
    m_synopsis_layout.addWidget(&m_title_text);

    m_description_text.setPlaceholderText("  Description");
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
    lifetime->setStyleSheet("border:1px solid black");
//    m_info_widget.setCellWidget(0, 0, lifetime);
    
    m_samplesPath = new QLineEdit("", this);
    m_samplesPath->setReadOnly(true);
    m_samplesPath->setHidden(true);
    
    de = new QDateEdit();
    de->setDate(QDate::currentDate());
    de->setDisplayFormat("yyyy-MM-dd");
    de->setCalendarPopup(true);
    de->setMinimumDate(QDate::currentDate());
    
//    //m_info_widget.setCellWidget(0, 1, de);
//
//    ////////////////////////////////////////////////////////////////////////////
//    /// Seeders
//    ////////////////////////////////////////////////////////////////////////////
//
//    m_info_widget.setCellWidget(0, 2, new QLabel("        Seeders"));
//    //Dropdown will be added later
//
//
//    ////////////////////////////////////////////////////////////////////////////
//    /// Key particles
//    ////////////////////////////////////////////////////////////////////////////
//
//    m_info_widget.setCellWidget(0, 4, new QLabel("        Key particles"));
//
//    QComboBox* keyParts = new QComboBox(this);
//    for (int r = 2; r <= 7; ++r) {
//        QString val = QString::fromStdString(std::to_string(r));
//        keyParts->addItem(val, val);
//    }
//
//    m_info_widget.setCellWidget(0, 5, keyParts);
//
//
//    ////////////////////////////////////////////////////////////////////////////
//    /// Price
//    ////////////////////////////////////////////////////////////////////////////
//
//
//    QLineEdit* priceEdit = new QLineEdit("", this);
//    priceEdit->setValidator( new QDoubleValidator(0.001, 100000, 3, this) );
//
//    m_info_widget.setCellWidget(2, 0, new QLabel("        Price"));
//    m_info_widget.setCellWidget(2, 1, priceEdit);
//
//
//    ////////////////////////////////////////////////////////////////////////////
//    /// samples button
//    ////////////////////////////////////////////////////////////////////////////


    
//    QPixmap image(":/icon/images/browse.svg");
//    QIcon button_icon(image);
//    m_info_widget.setCellWidget(2, 2, new QLabel("        Samples"));
//    QPushButton* browse_samples_button = new QPushButton();
//    browse_samples_button->setIcon(button_icon);
//    
//    m_info_widget.setCellWidget(2, 5, browse_samples_button);
//    connect(browse_samples_button, SIGNAL(clicked()),this, SLOT(browseContent()));



    ////////////////////////////////////////////////////////////////////////////
    /// content button
    ////////////////////////////////////////////////////////////////////////////

//    m_contentPath = new QLineEdit("", this);
//    m_contentPath->setReadOnly(true);
//    m_contentPath->setHidden(true);
//
//    m_info_widget.setCellWidget(2, 4, new QLabel("        Content"));
//
//    QPixmap image2(":/icon/images/browse.svg");
//    QIcon button_icon2(image2);
//    
//    QPushButton* browse_content_button = new QPushButton();
//    browse_content_button->setIcon(button_icon2);
//    m_info_widget.setCellWidget(2, 3, browse_content_button);
//    connect(browse_content_button, SIGNAL(clicked()),this, SLOT(browseSamples()));
//
//    m_info_widget.setFrameStyle(QFrame::NoFrame);
//    m_info_widget.setShowGrid(false);
//    m_info_widget.horizontalHeader()->hide();
//    m_info_widget.verticalHeader()->hide();
//    m_info_layout.addWidget(&m_info_widget);
//    QPalette plt_tbl = m_info_widget.palette();
//    plt_tbl.setColor(QPalette::Base, palette().color(QPalette::Window));
//    m_info_widget.setPalette(plt_tbl);

//    m_info_widget.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
//    m_info_widget.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    
    //////////////////////
    //////                             ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //////////////////////
    seeders = new QComboBox(this);

    QHBoxLayout* firstRow = new QHBoxLayout;
    
    //LIFETIME
    QLabel* lab = new QLabel("LifeTime");
    lab->setStyleSheet("border:1px solid lightGray; color: Gray");
    lab->setContentsMargins(0, 0, -2, 0);
    lab->setMinimumWidth(60);
    lab->setFixedHeight(23);

    firstRow->addWidget(lab);
    firstRow->addWidget(de);
    
    //SEEDERS
    QLabel* seed = new QLabel("Seeders");
    seed->setStyleSheet("border:1px solid lightGray; color: Gray");
    seed->setContentsMargins(15, 0, -2, 0);
    seed->setMinimumWidth(70);
    seed->setFixedHeight(18);

    firstRow->addWidget(seed);
    firstRow->addWidget(seeders);
    
    //KEYPARTICLES
    QLabel* key = new QLabel("Key Particles");
    key->setStyleSheet("border:1px solid lightGray; color: Gray");
    key->setContentsMargins(15, 0, -2, 0);
    key->setMinimumWidth(90);
    key->setFixedHeight(18);

    keyparts = new QComboBox(this);
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
    price->setFixedWidth(130);
    price->setContentsMargins(0, 0, 10, 0);

    secondrow->addWidget(price);
    
    //SIMPLES
    sim = new QLineEdit("Samples");
    sim->setReadOnly(true);
    sim->setStyleSheet("border:1px solid lightGray; color: Gray");
    sim->setContentsMargins(10, 0, 0, 0);
    sim->setFixedHeight(30);
    
    QPixmap image(":/icon/images/browse.svg");
    QIcon button_icon(image);
    
    QPushButton* browse_samples_button = new QPushButton();
    browse_samples_button->setIcon(button_icon);
    browse_samples_button->setFixedWidth(50);
    connect(browse_samples_button, SIGNAL(clicked()),this, SLOT(browseSamples()));

    secondrow->addWidget(sim);
    secondrow->addWidget(browse_samples_button);
    
    
    //CONTENT
    cont = new QLineEdit("Content");
    cont->setReadOnly(true);
    cont->setStyleSheet("border:1px solid lightGray; color: Gray");
    cont->setContentsMargins(10, 0, 0, 0);
    cont->setFixedHeight(30);

    m_contentPath = new QLineEdit("", this);
    m_contentPath->setReadOnly(true);
    m_contentPath->setHidden(true);

    QPixmap image2(":/icon/images/browse.svg");
    QIcon button_icon2(image2);
    
    QPushButton* browse_content_button = new QPushButton();
    browse_content_button->setIcon(button_icon2);
    browse_content_button->setFixedWidth(50);
    connect(browse_content_button, SIGNAL(clicked()),this, SLOT(browseContent()));
    
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
    upload_label->setMinimumHeight(26);
    
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
            std::string pubPrice = std::to_string(publishers[r]["price"]["amount"].get<int>() / GRAPHENE_BLOCKCHAIN_PRECISION);
            std::string pubAssetId = publishers[r]["price"]["asset_id"].get<std::string>();
            std::string pubFreeSpace = std::to_string(publishers[r]["free_space"].get<int>()) + "MB free";

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
    std::string m_life_time = de->date().toString().toStdString();
    std::string m_seeders   = seeders->currentText().toStdString();
    std::string m_keyparts  = keyparts->currentText().toStdString();
    std::string m_price     = price->text().toStdString();
    
//    std::string lifetime = ((QDateEdit*)m_info_widget.cellWidget(0, 1))->text().toStdString();
//    std::string seeders = ((QComboBox*)m_info_widget.cellWidget(0, 3))->currentData().toString().toStdString();
//    std::string keyparts = ((QComboBox*)m_info_widget.cellWidget(0, 5))->currentData().toString().toStdString();
//    std::string price = ((QLineEdit*)m_info_widget.cellWidget(2, 1))->text().toStdString();
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
    
    if (boost::filesystem::file_size(path) > 100 * 1024 * 1024) {
        ALERT("Content size is limited in Testnet 0.1 to 100MB");
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
    submitCommand += " \"magnet\"";                                     //Protocol
    submitCommand += " " + assetName;                                   //price_asset_name
    submitCommand += " " + m_price;                                       //price_amount
    submitCommand += " [" + m_seeders + "]";                              //seeders
    submitCommand += " \"" + m_life_time + "T23:59:59\"";                  //expiration
    submitCommand += " DCT";                                            //publishing_fee_asset
    submitCommand += " 300";                                            //publishing_fee_amount
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
    ((QDateEdit*)m_info_widget.cellWidget(0, 1))->setDate(QDate::currentDate());
    ((QLineEdit*)m_info_widget.cellWidget(2, 1))->setText("");
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
