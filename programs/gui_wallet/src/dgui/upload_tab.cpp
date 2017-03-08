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
        //m_info_widget(FieldsRows::NUM_FIELDS, 2),
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
    //m_title_text.setFixedWidth(200);
    m_synopsis_layout.addWidget(&m_title_text);

    m_description_text.setPlaceholderText("  Description");
    m_description_text.resize(138, 822);
    m_description_text.setFixedHeight(138);

    m_synopsis_layout.addWidget(&m_description_text);
    m_synopsis_layout.addWidget(new QLabel());


    QFont font( "Open Sans Bold", 14, QFont::Bold);
    m_infoLayoutHeader.setFont(font);


    m_main_layout.addLayout(&m_synopsis_layout);
    m_info_layout.addWidget(&m_infoLayoutHeader);

    m_info_widget.setFrameStyle(QFrame::NoFrame);

    ////////////////////////////////////////////////////////////////////////////
    /// Lifetime
    ////////////////////////////////////////////////////////////////////////////


    m_info_widget.setCellWidget(0, 0, new QLabel("        Lifetime"));

    QDateEdit *de = new QDateEdit();
    de->setDate(QDate::currentDate());
    de->setDisplayFormat("yyyy-MM-dd");
    de->setCalendarPopup(true);
    de->setMinimumDate(QDate::currentDate());

    m_info_widget.setCellWidget(0, 1, de);

    ////////////////////////////////////////////////////////////////////////////
    /// Seeders
    ////////////////////////////////////////////////////////////////////////////

    m_info_widget.setCellWidget(0, 2, new QLabel("        Seeders"));
    //Dropdown will be added later


    ////////////////////////////////////////////////////////////////////////////
    /// Key particles
    ////////////////////////////////////////////////////////////////////////////

    m_info_widget.setCellWidget(0, 4, new QLabel("        Key particles"));

    QComboBox* keyParts = new QComboBox(this);
    for (int r = 2; r <= 7; ++r) {
        QString val = QString::fromStdString(std::to_string(r));
        keyParts->addItem(val, val);
    }

    m_info_widget.setCellWidget(0, 5, keyParts);


    ////////////////////////////////////////////////////////////////////////////
    /// Price
    ////////////////////////////////////////////////////////////////////////////


    QLineEdit* priceEdit = new QLineEdit("", this);
    priceEdit->setValidator( new QDoubleValidator(0.001, 100000, 3, this) );

    m_info_widget.setCellWidget(2, 0, new QLabel("        Price"));
    m_info_widget.setCellWidget(2, 1, priceEdit);


    ////////////////////////////////////////////////////////////////////////////
    /// samples button
    ////////////////////////////////////////////////////////////////////////////

    m_samplesPath = new QLineEdit("", this);
    m_samplesPath->setReadOnly(true);
    m_samplesPath->setHidden(true);
    
    QPixmap image(":/icon/images/browse.svg");
    QIcon button_icon(image);
    m_info_widget.setCellWidget(2, 2, new QLabel("        Samples"));
    QPushButton* browse_samples_button = new QPushButton();
    browse_samples_button->setIcon(button_icon);
    
    m_info_widget.setCellWidget(2, 5, browse_samples_button);
    connect(browse_samples_button, SIGNAL(clicked()),this, SLOT(browseContent()));



    ////////////////////////////////////////////////////////////////////////////
    /// content button
    ////////////////////////////////////////////////////////////////////////////

    m_contentPath = new QLineEdit("", this);
    m_contentPath->setReadOnly(true);
    m_contentPath->setHidden(true);

    m_info_widget.setCellWidget(2, 4, new QLabel("        Content"));

    QPixmap image2(":/icon/images/browse.svg");
    QIcon button_icon2(image2);
    
    QPushButton* browse_content_button = new QPushButton();
    browse_content_button->setIcon(button_icon2);
    m_info_widget.setCellWidget(2, 3, browse_content_button);
    connect(browse_content_button, SIGNAL(clicked()),this, SLOT(browseSamples()));




    m_info_widget.horizontalHeader()->hide();
    m_info_widget.verticalHeader()->hide();
    m_info_layout.addWidget(&m_info_widget);
    m_info_widget.setShowGrid(false);
    QPalette plt_tbl = m_info_widget.palette();
    plt_tbl.setColor(QPalette::Base, palette().color(QPalette::Window));
    m_info_widget.setPalette(plt_tbl);



    ////////////////////////////////////////////////////////////////////////////
    /// Upload
    ////////////////////////////////////////////////////////////////////////////

    QPushButton* uploadButton = new QPushButton("Upload");
    uploadButton->setFixedHeight(40);
    uploadButton->setStyleSheet("background-color: rgb(27,176,104); color: rgb(255, 255, 255)");
    uploadButton->setFixedHeight(30);


    
    
    connect(uploadButton, SIGNAL(clicked()),this, SLOT(uploadContent()));
    m_info_layout.addWidget(uploadButton);
    m_main_layout.addLayout(&m_info_layout);

    setLayout(&m_main_layout);

    m_getPublishersTimer.setSingleShot(true);
    connect(&m_getPublishersTimer, SIGNAL(timeout()), SLOT(onGrabPublishers()));
    m_getPublishersTimer.start(1000);
}

void Upload_tab::onGrabPublishers() {
    
    SetNewTask("list_publishers_by_price 100", this, NULL, +[](void* owner, void* a_clbkArg, int64_t a_err, const std::string& a_task, const std::string& a_result) {
        Upload_tab* obj = (Upload_tab*)owner;

        auto publishers = json::parse(a_result);
        QComboBox* seeders = new QComboBox(obj);

        for (int r = 0; r < publishers.size(); ++r) {
            std::string pubIdStr = publishers[r]["seeder"].get<std::string>();
            std::string pubPrice = std::to_string(publishers[r]["price"]["amount"].get<int>() / GRAPHENE_BLOCKCHAIN_PRECISION);
            std::string pubAssetId = publishers[r]["price"]["asset_id"].get<std::string>();
            std::string pubFreeSpace = std::to_string(publishers[r]["free_space"].get<int>()) + "MB free";

            seeders->addItem(QString("%0 @%1 %2 [%3]").arg(QString::fromStdString(pubIdStr),
                                                            QString::fromStdString(pubPrice),
                                                            QString::fromStdString("DCT"),
                                                            QString::fromStdString(pubFreeSpace)), QString::fromStdString(pubIdStr));
        }

        obj->m_info_widget.setCellWidget(0, 3, seeders);

    });
}

void Upload_tab::browseContent() {
    QString contentPathSelected = QFileDialog::getOpenFileName(this, tr("Select content"), "~");
    m_contentPath->setText(contentPathSelected);
}

void Upload_tab::browseSamples() {
    QString sampleDir = QFileDialog::getExistingDirectory(this, tr("Select samples"), "~", QFileDialog::DontResolveSymlinks);
    m_samplesPath->setText(sampleDir);
}


void Upload_tab::uploadContent() {
    std::string lifetime = ((QDateEdit*)m_info_widget.cellWidget(0, 1))->text().toStdString();
    std::string seeders = ((QComboBox*)m_info_widget.cellWidget(0, 3))->currentData().toString().toStdString();
    std::string keyparts = ((QComboBox*)m_info_widget.cellWidget(0, 5))->currentData().toString().toStdString();
    std::string price = ((QLineEdit*)m_info_widget.cellWidget(2, 1))->text().toStdString();
    std::string assetName = "DECENT";
    std::string path = m_contentPath->text().toStdString();
    std::string samples_path = m_samplesPath->text().toStdString();

    std::string title = m_title_text.text().toStdString();
    std::string desc = m_description_text.toPlainText().toStdString();


    if (price.empty()) {
        ALERT("Please specify price");
        return;
    }

    if (path.empty()) {
        ALERT("Please specify path");
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
    submitCommand += " \"magnet\"";                                    //Protocol
    submitCommand += " " + assetName;                                   //price_asset_name
    submitCommand += " " + price;                                       //price_amount
    submitCommand += " [" + seeders + "]";                              //seeders
    submitCommand += " \"" + lifetime + "T23:59:59\"";                  //expiration
    submitCommand += " DECENT";                                         //publishing_fee_asset
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

    m_info_widget.setColumnWidth(0,20*aInfWidgSize.width()/100);
    m_info_widget.setColumnWidth(1,15*aInfWidgSize.width()/100);
    m_info_widget.setColumnWidth(2,20*aInfWidgSize.width()/100);
    m_info_widget.setColumnWidth(3,15*aInfWidgSize.width()/100);
    m_info_widget.setColumnWidth(4,20*aInfWidgSize.width()/100);
    m_info_widget.setColumnWidth(5,10*aInfWidgSize.width()/100);
}
