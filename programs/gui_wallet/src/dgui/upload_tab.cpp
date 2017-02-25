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



#include <cryptopp/integer.h>
#include <cryptopp/aes.h>
#include <cryptopp/filters.h>
#include <cryptopp/files.h>
#include <cryptopp/ccm.h>
#include <cryptopp/md5.h>
#include <cryptopp/osrng.h>



#include "json.hpp"



using namespace gui_wallet;
using namespace nlohmann;

extern int g_nDebugApplication;

CryptoPP::AutoSeededRandomPool rng;


#define ALERT(message)                                  \
{                                                       \
    QMessageBox msgBox;                                 \
    msgBox.setWindowTitle("Error");                     \
    msgBox.setText(QString::fromStdString(message));    \
    msgBox.exec();                                      \
}                                                       \



Upload_tab::Upload_tab()
        :
        m_info_widget(FieldsRows::NUM_FIELDS, 2),
        m_title_label(tr("Title")),
        m_description_label(tr("Description")),
        m_infoLayoutHeader(tr("Information about content")),
        m_getPublishersTimer(this)
{
    QPalette pltEdit;

    m_synopsis_layout.addWidget(&m_title_label);
    m_synopsis_layout.addWidget(&m_title_text);

    m_synopsis_layout.addWidget(&m_description_label);
    m_synopsis_layout.addWidget(&m_description_text);

    m_main_layout.addLayout(&m_synopsis_layout);
    m_info_layout.addWidget(&m_infoLayoutHeader);

    ////////////////////////////////////////////////////////////////////////////
    /// Lifetime
    ////////////////////////////////////////////////////////////////////////////

    m_info_widget.setCellWidget(FieldsRows::LIFETIME, 0, new QLabel("Lifetime"));

    QDateEdit *de = new QDateEdit();
    de->setDate(QDate::currentDate());
    de->setDisplayFormat("yyyy-MM-dd");
    de->setCalendarPopup(true);
    de->setMinimumDate(QDate::currentDate());

    m_info_widget.setCellWidget(FieldsRows::LIFETIME, 1, de);

    ////////////////////////////////////////////////////////////////////////////
    /// Key particles
    ////////////////////////////////////////////////////////////////////////////

    m_info_widget.setCellWidget(FieldsRows::KEYPARTS, 0, new QLabel("Key particles"));

    QComboBox* keyParts = new QComboBox(this);
    for (int r = 2; r <= 7; ++r) {
        QString val = QString::fromStdString(std::to_string(r));
        keyParts->addItem(val, val);
    }
    m_info_widget.setCellWidget(FieldsRows::KEYPARTS, 1, keyParts);

    ////////////////////////////////////////////////////////////////////////////
    /// Tags
    ////////////////////////////////////////////////////////////////////////////
    QLineEdit* tagsEdit = new QLineEdit("", this);
    m_info_widget.setCellWidget(FieldsRows::TAGS, 0, new QLabel("Tags"));
    m_info_widget.setCellWidget(FieldsRows::TAGS, 1, tagsEdit);

    ////////////////////////////////////////////////////////////////////////////
    /// Price
    ////////////////////////////////////////////////////////////////////////////


    QLineEdit* priceEdit = new QLineEdit("", this);
    priceEdit->setValidator( new QDoubleValidator(0.001, 100000, 3, this) );

    m_info_widget.setCellWidget(FieldsRows::PRICE, 0, new QLabel("Price"));
    m_info_widget.setCellWidget(FieldsRows::PRICE, 1, priceEdit);

    ////////////////////////////////////////////////////////////////////////////
    /// Asset ID
    ////////////////////////////////////////////////////////////////////////////

    m_info_widget.setCellWidget(FieldsRows::ASSETID, 0, new QLabel("Asset Type"));
    //Dropdown will be added later


    ////////////////////////////////////////////////////////////////////////////
    /// Seeders
    ////////////////////////////////////////////////////////////////////////////

    m_info_widget.setCellWidget(FieldsRows::SEEDERS, 0, new QLabel("Seeders"));
    //Dropdown will be added later



    ////////////////////////////////////////////////////////////////////////////
    /// content button
    ////////////////////////////////////////////////////////////////////////////

    QLineEdit* contentPath = new QLineEdit("", this);
    contentPath->setReadOnly(true);

    m_info_widget.setCellWidget(FieldsRows::CONTENTPATH, 0, new QLabel("Content"));
    m_info_widget.setCellWidget(FieldsRows::CONTENTPATH, 1, contentPath);


    QPushButton* browse_content_button = new QPushButton("Browse...");
    m_info_widget.setCellWidget(FieldsRows::SELECTSAMPLES, 0, new QLabel(""));
    m_info_widget.setCellWidget(FieldsRows::SELECTSAMPLES, 1, browse_content_button);
    connect(browse_content_button, SIGNAL(clicked()),this, SLOT(browseSamples()));




    ////////////////////////////////////////////////////////////////////////////
    /// samples button
    ////////////////////////////////////////////////////////////////////////////

    QLineEdit* samplesPath = new QLineEdit("", this);
    samplesPath->setReadOnly(true);

    m_info_widget.setCellWidget(FieldsRows::SAMPLESPATH, 0, new QLabel("Samples"));
    m_info_widget.setCellWidget(FieldsRows::SAMPLESPATH, 1, samplesPath);


    QPushButton* browse_samples_button = new QPushButton("Browse...");
    m_info_widget.setCellWidget(FieldsRows::SELECTPATH, 0, new QLabel(""));
    m_info_widget.setCellWidget(FieldsRows::SELECTPATH, 1, browse_samples_button);
    connect(browse_samples_button, SIGNAL(clicked()),this, SLOT(browseContent()));





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
    connect(uploadButton, SIGNAL(clicked()),this, SLOT(uploadContent()));
    m_info_layout.addWidget(uploadButton);

    m_main_layout.addLayout(&m_info_layout);

    setLayout(&m_main_layout);


    m_getPublishersTimer.setSingleShot(true);
    connect(&m_getPublishersTimer, SIGNAL(timeout()), SLOT(onGrabPublishers()));
    m_getPublishersTimer.start(1000);
}

void Upload_tab::onGrabPublishers() {

    SetNewTask("list_assets \"\" 100", this, NULL, +[](void* owner, void* a_clbkArg, int64_t a_err, const std::string& a_task, const std::string& a_result) {
        Upload_tab* obj = (Upload_tab*)owner;
        
        QComboBox* assetCombo = new QComboBox(obj);

        auto assets = json::parse(a_result);
        for (int r = 0; r < assets.size(); ++r) {
            const std::string& id = assets[r]["id"].get<std::string>();
            const std::string& symbol = assets[r]["symbol"].get<std::string>();
            
            obj->m_assetMap.insert(std::make_pair(id, symbol));

            assetCombo->addItem(QString::fromStdString(symbol), QString::fromStdString(id));
        }
        
        obj->m_info_widget.setCellWidget(FieldsRows::ASSETID, 1, assetCombo);

        SetNewTask("list_publishers_by_price 100", obj, NULL, +[](void* owner, void* a_clbkArg, int64_t a_err, const std::string& a_task, const std::string& a_result) {
            Upload_tab* obj = (Upload_tab*)owner;

            auto publishers = json::parse(a_result);
            QComboBox* seeders = new QComboBox(obj);

            for (int r = 0; r < publishers.size(); ++r) {
                std::string pubIdStr = publishers[r]["seeder"].get<std::string>();
                std::string pubPrice = std::to_string(publishers[r]["price"]["amount"].get<int>());
                std::string pubAssetId = publishers[r]["price"]["asset_id"].get<std::string>();
                std::string pubFreeSpace = std::to_string(publishers[r]["free_space"].get<int>()) + "MB free";

                AssetMap::iterator it = obj->m_assetMap.find(pubAssetId);
                if (it == obj->m_assetMap.end()) {
                    std::cout << "Invalid asset id " << pubAssetId << std::endl;
                    continue;
                }

                std::string assetSymbol = it->second;

                seeders->addItem(QString("%0 @%1 %2 [%3]").arg(QString::fromStdString(pubIdStr),
                                                                QString::fromStdString(pubPrice), 
                                                                QString::fromStdString(assetSymbol), 
                                                                QString::fromStdString(pubFreeSpace)), QString::fromStdString(pubIdStr));
            }

            obj->m_info_widget.setCellWidget(FieldsRows::SEEDERS, 1, seeders);

        });


    });
}

void Upload_tab::browseContent() {
    QString contentDir = QFileDialog::getExistingDirectory(this, tr("Select content"), "~", QFileDialog::DontResolveSymlinks);
    //QString contentDir = QFileDialog::getOpenFileName(this, tr("Select content"), "~");
    QLineEdit* contentPath = (QLineEdit*)m_info_widget.cellWidget(FieldsRows::CONTENTPATH, 1);
    contentPath->setText(contentDir);
}

void Upload_tab::browseSamples() {
    QString sampleDir = QFileDialog::getExistingDirectory(this, tr("Select samples"), "~", QFileDialog::DontResolveSymlinks);
    //QString sampleDir = QFileDialog::getOpenFileName(this, tr("Select content"), "~");
    QLineEdit* samplePath = (QLineEdit*)m_info_widget.cellWidget(FieldsRows::SAMPLESPATH, 1);
    samplePath->setText(sampleDir);
}
/*

void Upload_tab::uploadContent() {
    std::string lifetime = ((QDateEdit*)m_info_widget.cellWidget(FieldsRows::LIFETIME, 1))->text().toStdString();
    std::string keyparts = ((QComboBox*)m_info_widget.cellWidget(FieldsRows::KEYPARTS, 1))->currentData().toString().toStdString();
    std::string tags = ((QLineEdit*)m_info_widget.cellWidget(FieldsRows::TAGS, 1))->text().toStdString();
    std::string price = ((QLineEdit*)m_info_widget.cellWidget(FieldsRows::PRICE, 1))->text().toStdString();
    std::string assetType = ((QComboBox*)m_info_widget.cellWidget(FieldsRows::ASSETID, 1))->currentData().toString().toStdString();
    std::string assetName = ((QComboBox*)m_info_widget.cellWidget(FieldsRows::ASSETID, 1))->currentText().toStdString();
    std::string seeders = ((QComboBox*)m_info_widget.cellWidget(FieldsRows::SEEDERS, 1))->currentData().toString().toStdString();
    std::string path = ((QLineEdit*)m_info_widget.cellWidget(FieldsRows::CONTENTPATH, 1))->text().toStdString();
    std::string samples_path = ((QLineEdit*)m_info_widget.cellWidget(FieldsRows::SAMPLESPATH, 1))->text().toStdString();


    std::string title = m_title_text.text().toStdString();
    std::string desc = m_description_text.toPlainText().toStdString();


    if (tags.empty()) {
        ALERT("Please specify tags");
        return;
    }

    if (price.empty()) {
        ALERT("Please specify price");
        return;
    }

    if (path.empty()) {
        ALERT("Please specify path");
        return;
    }

    if (title.empty()) {
        ALERT("Please specify synopsis");
        return;
    }
    
    if (desc.empty()) {
        ALERT("Please specify synopsis");
        return;
    }

    if (GlobalEvents::instance().getCurrentUser().empty()) {
        ALERT("Please select user to upload");
        return;   
    }


    json synopsis_obj;
    synopsis_obj["title"] = title;
    synopsis_obj["description"] = desc;

    std::string synopsis = synopsis_obj.dump(4);

    CryptoPP::Integer randomKey (rng, 512);

    std::ostringstream oss;
    oss << randomKey;
    std::string randomKeyString(oss.str());



    std::string* submitCommand = new std::string("submit_content");
    *submitCommand += " " + GlobalEvents::instance().getCurrentUser();   //author
    *submitCommand += " \"%0\"";                                         //URI
    *submitCommand += " " + assetName;                                   //price_asset_name
    *submitCommand += " " + price;                                       //price_amount
    *submitCommand += " %1";                                             //size
    *submitCommand += " \"%2\"";                                         //hash
    *submitCommand += " [" + seeders + "]";                              //seeders
    *submitCommand += " 1";                                              //quorum
    *submitCommand += " \"" + lifetime + "T23:59:59\"";                  //expiration
    *submitCommand += " DECENT";                                         //publishing_fee_asset
    *submitCommand += " 300";                                            //publishing_fee_amount
    *submitCommand += " \"" + synopsis + "\"";                           //synopsis
    *submitCommand += " \"" + randomKeyString + "\"";                    //secret
    *submitCommand += " %3";                                             //cd
    *submitCommand += " true";                                           //broadcast

    

    QString createPackage = QString("create_package \"%0\" \"%1\" %2").arg(QString::fromStdString(path),
                                                                           QString::fromStdString(samples_path),
                                                                           QString::fromStdString(randomKeyString));

    SetNewTask(createPackage.toStdString(), this, submitCommand, +[](void* owner, void* a_clbkArg, int64_t a_err, const std::string& a_task, const std::string& a_result) {
        if (a_err != 0) {
            ALERT("Failed to upload content");
            return;
        }

        Upload_tab* obj = (Upload_tab*)owner;
        std::string submitCommandTemplate = *(std::string*)a_clbkArg;

        auto packageResult = json::parse(a_result);

        std::string packageHash = packageResult[0].get<std::string>();
        int n = packageResult[1]["n"].get<int>();
        std::string u_seed = packageResult[1]["u_seed"].get<std::string>();
        std::string pubKey = packageResult[1]["pubKey"].get<std::string>();

        QString cdJson = QString("{ \"n\": %0, \"u_seed\": \"%1\",\"pubKey\": \"%2\" }").arg(QString::fromStdString(std::to_string(n)),
                                                                                             QString::fromStdString(u_seed),
                                                                                             QString::fromStdString(pubKey));

        //TODO: fix package size
        QString submitCmd = QString(QString::fromStdString(submitCommandTemplate)).arg("%0", "1", QString::fromStdString(packageHash), cdJson);

        *(std::string*)a_clbkArg = submitCmd.toStdString();

        QString uploadCommand = QString("upload_package %0 ipfs").arg(QString::fromStdString(packageHash));

        SetNewTask(uploadCommand.toStdString(), obj, a_clbkArg, +[](void* owner, void* a_clbkArg, int64_t a_err, const std::string& a_task, const std::string& a_result) {
            
            Upload_tab* obj = (Upload_tab*)owner;
            auto uploadResult = json::parse(a_result);


            std::string submitCommandTemplate = *(std::string*)a_clbkArg;
            QString submitCmd = QString(QString::fromStdString(submitCommandTemplate)).arg(QString::fromStdString(uploadResult.get<std::string>()));

            SetNewTask(submitCmd.toStdString(), obj, NULL, +[](void* owner, void* a_clbkArg, int64_t a_err, const std::string& a_task, const std::string& a_result) {
                if (a_err != 0) {
                    ALERT("Failed to submit content");
                    return;
                }

                ALERT("Content is submitted!");
            });

        });

     });
}

*/





void Upload_tab::uploadContent() {
    std::string lifetime = ((QDateEdit*)m_info_widget.cellWidget(FieldsRows::LIFETIME, 1))->text().toStdString();
    std::string keyparts = ((QComboBox*)m_info_widget.cellWidget(FieldsRows::KEYPARTS, 1))->currentData().toString().toStdString();
    std::string tags = ((QLineEdit*)m_info_widget.cellWidget(FieldsRows::TAGS, 1))->text().toStdString();
    std::string price = ((QLineEdit*)m_info_widget.cellWidget(FieldsRows::PRICE, 1))->text().toStdString();
    std::string assetType = ((QComboBox*)m_info_widget.cellWidget(FieldsRows::ASSETID, 1))->currentData().toString().toStdString();
    std::string assetName = ((QComboBox*)m_info_widget.cellWidget(FieldsRows::ASSETID, 1))->currentText().toStdString();
    std::string seeders = ((QComboBox*)m_info_widget.cellWidget(FieldsRows::SEEDERS, 1))->currentData().toString().toStdString();
    std::string path = ((QLineEdit*)m_info_widget.cellWidget(FieldsRows::CONTENTPATH, 1))->text().toStdString();
    std::string samples_path = ((QLineEdit*)m_info_widget.cellWidget(FieldsRows::SAMPLESPATH, 1))->text().toStdString();


    std::string title = m_title_text.text().toStdString();
    std::string desc = m_description_text.toPlainText().toStdString();


    if (tags.empty()) {
        ALERT("Please specify tags");
        return;
    }

    if (price.empty()) {
        ALERT("Please specify price");
        return;
    }

    if (path.empty()) {
        ALERT("Please specify path");
        return;
    }

    if (title.empty()) {
        ALERT("Please specify synopsis");
        return;
    }
    
    if (desc.empty()) {
        ALERT("Please specify synopsis");
        return;
    }

    if (GlobalEvents::instance().getCurrentUser().empty()) {
        ALERT("Please select user to upload");
        return;   
    }


    json synopsis_obj;
    synopsis_obj["title"] = title;
    synopsis_obj["description"] = desc;

    std::string synopsis = synopsis_obj.dump(4);

    CryptoPP::Integer randomKey (rng, 512);

    std::ostringstream oss;
    oss << randomKey;
    std::string randomKeyString(oss.str());



    std::string* submitCommand = new std::string("submit_content_new");
    *submitCommand += " " + GlobalEvents::instance().getCurrentUser();   //author
    *submitCommand += " \"" + path + "\"";                               //URI
    *submitCommand += " \"" + samples_path + "\"";                       //URI
    *submitCommand += " \"ipfs\"";                                       //URI
    *submitCommand += " " + assetName;                                   //price_asset_name
    *submitCommand += " " + price;                                       //price_amount
    *submitCommand += " [" + seeders + "]";                              //seeders
    *submitCommand += " \"" + lifetime + "T23:59:59\"";                  //expiration
    *submitCommand += " DECENT";                                         //publishing_fee_asset
    *submitCommand += " 300";                                            //publishing_fee_amount
    *submitCommand += " \"" + json::escape_string(synopsis) + "\"";                           //synopsis
    *submitCommand += " true";                                           //broadcast

    

    QString createPackage = QString("create_package \"%0\" \"%1\" %2").arg(QString::fromStdString(path),
                                                                           QString::fromStdString(samples_path),
                                                                           QString::fromStdString(randomKeyString));

    SetNewTask(createPackage.toStdString(), this, submitCommand, +[](void* owner, void* a_clbkArg, int64_t a_err, const std::string& a_task, const std::string& a_result) {
        if (a_err != 0) {
            ALERT("Failed to upload content");
            return;
        }

        Upload_tab* obj = (Upload_tab*)owner;
        std::string submitCommandTemplate = *(std::string*)a_clbkArg;

        auto packageResult = json::parse(a_result);

        std::string packageHash = packageResult[0].get<std::string>();
        int n = packageResult[1]["n"].get<int>();
        std::string u_seed = packageResult[1]["u_seed"].get<std::string>();
        std::string pubKey = packageResult[1]["pubKey"].get<std::string>();

        QString cdJson = QString("{ \"n\": %0, \"u_seed\": \"%1\",\"pubKey\": \"%2\" }").arg(QString::fromStdString(std::to_string(n)),
                                                                                             QString::fromStdString(u_seed),
                                                                                             QString::fromStdString(pubKey));

        //TODO: fix package size
        QString submitCmd = QString(QString::fromStdString(submitCommandTemplate)).arg("%0", "1", QString::fromStdString(packageHash), cdJson);

        *(std::string*)a_clbkArg = submitCmd.toStdString();

        QString uploadCommand = QString("upload_package %0 ipfs").arg(QString::fromStdString(packageHash));

        SetNewTask(uploadCommand.toStdString(), obj, a_clbkArg, +[](void* owner, void* a_clbkArg, int64_t a_err, const std::string& a_task, const std::string& a_result) {
            
            Upload_tab* obj = (Upload_tab*)owner;
            auto uploadResult = json::parse(a_result);


            std::string submitCommandTemplate = *(std::string*)a_clbkArg;
            QString submitCmd = QString(QString::fromStdString(submitCommandTemplate)).arg(QString::fromStdString(uploadResult.get<std::string>()));

            SetNewTask(submitCmd.toStdString(), obj, NULL, +[](void* owner, void* a_clbkArg, int64_t a_err, const std::string& a_task, const std::string& a_result) {
                if (a_err != 0) {
                    ALERT("Failed to submit content");
                    return;
                }

                ALERT("Content is submitted!");
            });

        });

     });
}







Upload_tab::~Upload_tab()
{
}



void Upload_tab::uploadDone(void* a_clbkArg, int64_t a_err, const std::string& a_task, const std::string& a_result) {

}

void Upload_tab::resizeEvent ( QResizeEvent * event )
{
    QWidget::resizeEvent(event);

    QSize aInfWidgSize = m_info_widget.size();

    m_info_widget.setColumnWidth(0,39*aInfWidgSize.width()/100);
    m_info_widget.setColumnWidth(1,59*aInfWidgSize.width()/100);

}
