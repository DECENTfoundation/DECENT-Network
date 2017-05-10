#include "stdafx.h"

#include "gui_wallet_global.hpp"
#include "upload_popup.hpp"
#include "decent_wallet_ui_gui_contentdetailsgeneral.hpp"
#include "decent_button.hpp"
#include "gui_design.hpp"

#ifndef _MSC_VER
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QFileDialog>
#include <QComboBox>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QCalendarWidget>
#include <QDateTime>
#include <QDate>
#include <QTime>
#include <QTimer>
#include <QDateEdit>
#include <QTextEdit>
#include <QApplication>
#include <QCheckBox>
#include <QStyleFactory>
#include <QInputMethod>
#include <QSignalMapper>
#include <QLocale>
#include <QLabel>
#include <QLineEdit>

#include <graphene/chain/config.hpp>
#include <graphene/chain/content_object.hpp>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>

#include <cryptopp/integer.h>
#include <cryptopp/aes.h>
#include <cryptopp/filters.h>
#include <cryptopp/files.h>
#include <cryptopp/ccm.h>
#include <cryptopp/md5.h>
#include <cryptopp/osrng.h>

#include <QIcon>





#include <QMouseEvent>




#include <ctime>
#include <limits>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <graphene/chain/config.hpp>




#include "json.hpp"
#endif

using string = std::string;
using namespace gui_wallet;
using namespace nlohmann;

Upload_popup::Upload_popup(QWidget* pParent)
: QDialog(pParent)
, m_pStatusCheckTimer(new QTimer(this))
, m_pDescriptionText(new QTextEdit(this))
, m_pLifeTime(new QDateEdit(this))
, m_iKeyParticles(2)
, m_dPrice(-1)
{
   std::vector<Publisher> publishers = Globals::instance().getPublishers();
   m_arrPublishers.resize(publishers.size());
   for (size_t iIndex = 0; iIndex < publishers.size(); ++iIndex)
   {
      m_arrPublishers[iIndex].first = publishers[iIndex];
      m_arrPublishers[iIndex].second = false;
   }

   _locale = ((QApplication*)QApplication::instance())->inputMethod()->locale();
   //
   // Title field
   //
   QLineEdit* pTitleText = new QLineEdit();
   pTitleText->setPlaceholderText(tr("Title"));
   pTitleText->setAttribute(Qt::WA_MacShowFocusRect, 0);
   pTitleText->setTextMargins(5, 5, 5, 5);
   pTitleText->setMinimumHeight(40);
   //
   // Description field
   //
   m_pDescriptionText->setPlaceholderText(tr("Description"));
   m_pDescriptionText->setStyleSheet(d_desc);
   m_pDescriptionText->setMinimumHeight(160);
   m_pDescriptionText->setMinimumWidth(480);
   m_pDescriptionText->setTabChangesFocus(true);
   //
   // Lifetime
   //
   QLabel* pLifeTimeLabel = new QLabel(this);
   pLifeTimeLabel->setText(tr("Expiration date"));
   pLifeTimeLabel->setStyleSheet(d_label_v1);
   pLifeTimeLabel->setMinimumWidth(60);
   pLifeTimeLabel->setMinimumHeight(40);

   m_pLifeTime->setDate(QDate::currentDate().addMonths(1));
   m_pLifeTime->setDisplayFormat("yyyy-MM-dd");
   m_pLifeTime->setCalendarPopup(true);
   m_pLifeTime->setMinimumDate(QDate::currentDate().addDays(1));
   m_pLifeTime->setStyle(QStyleFactory::create("fusion"));
   m_pLifeTime->setMinimumHeight(40);
   m_pLifeTime->setFixedWidth(320);
   //
   // Key particles
   //
   QLabel* pKeypartsLabel = new QLabel(this);
   pKeypartsLabel->setText(tr("Key particles"));
   pKeypartsLabel->setStyleSheet(d_label_v1);
   pKeypartsLabel->setMinimumWidth(60);
   pKeypartsLabel->setMinimumHeight(40);

   QComboBox* pKeypartsCombo = new QComboBox(this);
   pKeypartsCombo->setStyle(QStyleFactory::create("fusion"));
   pKeypartsCombo->setStyleSheet(c_keyparts);
   pKeypartsCombo->setMinimumHeight(40);
   pKeypartsCombo->setFixedWidth(320);

   for (int r = 2; r <= 7; ++r) {
      QString val = QString::fromStdString(std::to_string(r));
      pKeypartsCombo->addItem(val, val);
   }
   //
   // Price
   //
   QLabel* priceLabel = new QLabel(this);
   priceLabel->setText(tr("Price"));
   priceLabel->setStyleSheet(d_label_v1);
   priceLabel->setMinimumWidth(60);
   priceLabel->setMinimumHeight(40);

   QLineEdit* pPriceEditor = new QLineEdit();
   
   QDoubleValidator* dblValidator = new QDoubleValidator(0.0001, 100000, 4, this);
   dblValidator->setLocale(Globals::instance().m_locale);
   pPriceEditor->setValidator(dblValidator);

   pPriceEditor->setAttribute(Qt::WA_MacShowFocusRect, 0);
   pPriceEditor->setStyleSheet(d_label_v2);
   pPriceEditor->setTextMargins(5, 5, 5, 5);
   pPriceEditor->setMinimumHeight(40);
   pPriceEditor->setFixedWidth(320);
   //
   // Seeders
   //
   QLineEdit* pSeedersPath = new QLineEdit(tr("Seeders"));
   pSeedersPath->setPlaceholderText(tr("Seeders"));
   pSeedersPath->setReadOnly(true);
   pSeedersPath->setStyleSheet(d_label_v2);
   pSeedersPath->setTextMargins(5, 5, 5, 5);
   pSeedersPath->setMinimumWidth(100);
   pSeedersPath->setMinimumHeight(40);

   DecentButton* pSeedersButton = new DecentButton(this);

   pSeedersButton->setText(tr("Select Seeders"));
   pSeedersButton->setFont(PopupButtonRegularFont());
#ifdef WINDOWS_HIGH_DPI
   pSeedersButton->setFixedWidth(240);
#else
   pSeedersButton->setFixedWidth(120);
#endif
   pSeedersButton->setFixedHeight(40);




   ////////////////////////////////////////////////////////////////////////////
   /// Content path
   ////////////////////////////////////////////////////////////////////////////

   QHBoxLayout* contentRow = new QHBoxLayout();

   _contentPath = new QLineEdit(tr("Content path"));
   _contentPath->setReadOnly(true);
   _contentPath->setStyleSheet(d_label_v2);
   _contentPath->setMinimumHeight(40);
   _contentPath->setTextMargins(5, 5, 5, 5);

   DecentButton* browseContentButton = new DecentButton(this);
   browseContentButton->setText(tr("Browse"));
   browseContentButton->setFont(PopupButtonRegularFont());

   browseContentButton->setMinimumWidth(120);
   browseContentButton->setFixedHeight(40);
   connect(browseContentButton, SIGNAL(clicked()),this, SLOT(browseContent()));

   contentRow->addWidget(_contentPath);
   contentRow->addWidget(browseContentButton);



   ////////////////////////////////////////////////////////////////////////////
   /// Samples path
   ////////////////////////////////////////////////////////////////////////////
   QHBoxLayout* samplesRow = new QHBoxLayout();

   _samplesPath = new QLineEdit(tr("Samples (optional)"));
   _samplesPath->setReadOnly(true);
   _samplesPath->setStyleSheet(d_label_v2);
   _samplesPath->setMinimumHeight(40);
   _samplesPath->setTextMargins(5, 5, 5, 5);


   DecentButton* browseSamplesButton = new DecentButton(this);
   browseSamplesButton->setText(tr("Browse"));
   browseSamplesButton->setFont(PopupButtonRegularFont());

   browseSamplesButton->setMinimumWidth(120);
   browseSamplesButton->setFixedHeight(40);
   connect(browseSamplesButton, SIGNAL(clicked()),this, SLOT(browseSamples()));

   samplesRow->addWidget(_samplesPath);
   samplesRow->addWidget(browseSamplesButton);



   ////////////////////////////////////////////////////////////////////////////
   /// Upload & Cancel
   ////////////////////////////////////////////////////////////////////////////


   QHBoxLayout* button = new QHBoxLayout;

   button->setSpacing(20);
   _upload_button = new DecentButton(this);
   DecentButton* pCancelButton = new DecentButton(this);

   pCancelButton->setText(tr("Cancel"));
   pCancelButton->setFont(PopupButtonBigFont());
   pCancelButton->setMinimumHeight(50);


   _upload_button->setText(tr("Publish"));
   _upload_button->setFont(PopupButtonBigFont());
   _upload_button->setMinimumHeight(50);

   connect(_upload_button, SIGNAL(clicked()),this, SLOT(uploadContent()));
   QObject::connect(pCancelButton, &QPushButton::clicked,
                    this, &QDialog::close);

   button->setContentsMargins(20, 20, 20, 20);
   button->addWidget(_upload_button);
   button->addWidget(pCancelButton);



   setWindowTitle(tr("Upload new content"));


   QHBoxLayout* pLifeTimeRow = new QHBoxLayout();
   pLifeTimeRow->addWidget(pLifeTimeLabel);
   pLifeTimeRow->addWidget(m_pLifeTime);

   QHBoxLayout* pKeyRow = new QHBoxLayout();
   pKeyRow->addWidget(pKeypartsLabel);
   pKeyRow->addWidget(pKeypartsCombo);

   QHBoxLayout* pPriceRow = new QHBoxLayout();
   pPriceRow->addWidget(priceLabel);
   pPriceRow->addWidget(pPriceEditor);

   QHBoxLayout* pSeedersRow = new QHBoxLayout();
   pSeedersRow->addWidget(pSeedersPath);
   pSeedersRow->addWidget(pSeedersButton);

   QVBoxLayout* pMainLayout = new QVBoxLayout;
   pMainLayout->addWidget(pTitleText);
   pMainLayout->addWidget(m_pDescriptionText);
   pMainLayout->addLayout(pLifeTimeRow);
   pMainLayout->addLayout(pKeyRow);
   pMainLayout->addLayout(pPriceRow);
   pMainLayout->addLayout(pSeedersRow);
   pMainLayout->addLayout(contentRow);
   pMainLayout->addLayout(samplesRow);
   pMainLayout->addLayout(button);
   pMainLayout->setContentsMargins(10, 10, 10, 10);
   pMainLayout->setSpacing(5);
   setLayout(pMainLayout);

   slot_UpdateStatus();

   QObject::connect(pTitleText, &QLineEdit::textChanged,
                    this, &Upload_popup::slot_TitleChanged);
   QObject::connect(pTitleText, &QLineEdit::textChanged,
                    this, &Upload_popup::slot_UpdateStatus);
   QObject::connect(m_pDescriptionText, &QTextEdit::textChanged,
                    this, &Upload_popup::slot_UpdateStatus);
   QObject::connect(pKeypartsCombo, (void (QComboBox::*)(QString const&))&QComboBox::activated,
                    this, &Upload_popup::slot_KeyParticlesChanged);
   QObject::connect(pPriceEditor, &QLineEdit::textChanged,
                    this, &Upload_popup::slot_PriceChanged);
   QObject::connect(pSeedersButton, &QPushButton::clicked,
                    this, &Upload_popup::slot_ChooseSeeders);
   QObject::connect(this, &Upload_popup::signal_SetSeedersText,
                    pSeedersPath, &QLineEdit::setText);


   QObject::connect(m_pStatusCheckTimer, &QTimer::timeout,
                    this, &Upload_popup::slot_UpdateStatus);
   m_pStatusCheckTimer->start(500);

#ifdef _MSC_VER
   int height = style()->pixelMetric(QStyle::PM_TitleBarHeight);
   setWindowIcon(height > 32 ? QIcon(":/icon/images/windows_decent_icon_32x32.png")
                 : QIcon(":/icon/images/windows_decent_icon_16x16.png"));
#endif
}

Upload_popup::~Upload_popup() = default;

QStringList Upload_popup::getChosenPublishers() const
{
   QStringList lstSummary;

   for (size_t iIndex = 0; iIndex < m_arrPublishers.size(); ++iIndex)
   {
      auto const& seederItem = m_arrPublishers[iIndex];
      Publisher const& seeder = seederItem.first;

      if (seederItem.second)
         lstSummary << seeder.m_str_name.c_str();
   }

   return lstSummary;
}

void Upload_popup::slot_ChooseSeeders()
{
   QDialog* pDialog = new QDialog(nullptr, Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint);
   pDialog->setAttribute(Qt::WA_DeleteOnClose);
   pDialog->setWindowTitle(tr("Seeders"));
   pDialog->setContentsMargins(0, 0, 0, 0);
   pDialog->resize(450, 250);

   DecentTable* pSeedersTable = new DecentTable(pDialog);
   pSeedersTable->set_columns({
      {"", -25},
      {tr("Seeder"), 10, "rating"},
      {tr("Price"),  10, "price"},
      {tr("Size") ,  10, "size"}
   });

   DecentButton* pOKButton = new DecentButton(pDialog);
   pOKButton->setText(tr("OK"));
   pOKButton->setFixedHeight(50);
   pOKButton->setFixedWidth(100);
   pOKButton->setFont(TabButtonFont());

   QHBoxLayout* pButtonsLayout = new QHBoxLayout;
   pButtonsLayout->setMargin(10);
   pButtonsLayout->setAlignment(Qt::AlignHCenter);
   pButtonsLayout->addWidget(pOKButton);

   QVBoxLayout* pMainLayout = new QVBoxLayout;
   pMainLayout->setContentsMargins(0, 0, 0, 0);
   pMainLayout->setMargin(0);
   pMainLayout->setSpacing(0);
   pMainLayout->addWidget(pSeedersTable);
   pMainLayout->addLayout(pButtonsLayout);

   pDialog->setLayout(pMainLayout);

   enum {eCheckBox, eName, ePrice, eSpace};

   QSignalMapper* pSeederCheckSignalMapper = new QSignalMapper(pDialog);
   QObject::connect(pSeederCheckSignalMapper, (void (QSignalMapper::*)(int))&QSignalMapper::mapped,
                    this, &Upload_popup::slot_SeederChanged);
   QObject::connect(pSeederCheckSignalMapper, (void (QSignalMapper::*)(int))&QSignalMapper::mapped,
                    this, &Upload_popup::slot_UpdateStatus);

   pSeedersTable->setRowCount(m_arrPublishers.size());
   for (size_t iIndex = 0; iIndex < m_arrPublishers.size(); ++iIndex)
   {
      auto const& seederItem = m_arrPublishers[iIndex];
      Publisher const& seeder = seederItem.first;

      int free_space = seeder.m_storage_size;
      QString pubFreeSpace = std::to_string(free_space).c_str();
      pubFreeSpace += " MB free";
      if (free_space > 800)
         pubFreeSpace = QString::number(free_space / 1024.0, 'f', 2) + " GB free";

      EventPassthrough<QCheckBox>* pCheckBox = new EventPassthrough<QCheckBox>(pSeedersTable);
      pSeedersTable->setCellWidget(iIndex, eCheckBox, pCheckBox);
      pSeedersTable->setItem(iIndex, eName, new QTableWidgetItem(QString::fromStdString(seeder.m_str_name)));
      pSeedersTable->setItem(iIndex, ePrice, new QTableWidgetItem(seeder.m_price.getString().c_str()));
      pSeedersTable->setItem(iIndex, eSpace, new QTableWidgetItem(pubFreeSpace));

      for (size_t iColIndex = eName; iColIndex < eSpace; ++iColIndex)
      {
         pSeedersTable->item(iIndex, iColIndex)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
         pSeedersTable->item(iIndex, iColIndex)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
      }

      if (seederItem.second)
         pCheckBox->setCheckState(Qt::Checked);

      QObject::connect(pCheckBox, &QCheckBox::stateChanged,
                       pSeederCheckSignalMapper, (void (QSignalMapper::*)())&QSignalMapper::map);
      pSeederCheckSignalMapper->setMapping(pCheckBox, iIndex);
   }

   QObject::connect(pOKButton, &QPushButton::clicked,
                    pDialog, &QDialog::close);

   pDialog->open();
}

void Upload_popup::slot_SeederChanged(int iIndex)
{
   bool& bState = m_arrPublishers[iIndex].second;
   bState = !bState;

   QStringList lstSummary = getChosenPublishers();

   QString strSummary;
   if (lstSummary.size() > 4)
      strSummary = QString::number(lstSummary.size());
   else
      strSummary = lstSummary.join(", ");

   emit signal_SetSeedersText(strSummary);
}

void Upload_popup::slot_TitleChanged(QString const& strTitle)
{
   m_strTitle = strTitle;
}

void Upload_popup::slot_KeyParticlesChanged(QString const& strKeyParticles)
{
   m_iKeyParticles = strKeyParticles.toLongLong();
}

void Upload_popup::slot_PriceChanged(QString const& strPrice)
{
   if (strPrice.isEmpty())
      m_dPrice = -1;
   else
   {
      bool bPriceIsOK = false;
      m_dPrice = _locale.toDouble(strPrice, &bPriceIsOK);

      if (false == bPriceIsOK)
         m_dPrice = -1;
   }
}

void Upload_popup::slot_UpdateStatus()
{
   std::string path = _contentPath->text().toStdString();
   //std::string samplesPath = _samplesPath->text().toStdString();

   QString strDescription = m_pDescriptionText->toPlainText();

   boost::system::error_code ec;
   boost::optional<double> fileSizeMBytes;
   if (false == path.empty())
      fileSizeMBytes = boost::filesystem::file_size(path, ec) / 1024.0 / 1024.0;

   bool isPublishersValid = false;
   uint64_t publishingPrice = 0;

   for (size_t iSeederIndex = 0; iSeederIndex < m_arrPublishers.size(); ++iSeederIndex)
   {
      auto const& seederItem = m_arrPublishers[iSeederIndex];
      Publisher const& seeder = seederItem.first;

      if (false == seederItem.second)
         continue;

      isPublishersValid = true;
      publishingPrice += seeder.m_price.m_amount;
   }

   if (-1 == m_dPrice ||
       ec ||
       !fileSizeMBytes ||
       *fileSizeMBytes > 100 ||
       m_strTitle.isEmpty() ||
       strDescription.isEmpty() ||
       Globals::instance().getCurrentUser().empty() ||
       false == isPublishersValid)
   {
      _upload_button->setText(tr("Publish"));
      _upload_button->setEnabled(false);
   }
   else
   {
      std::string lifeTime = m_pLifeTime->text().toStdString();
      int days = QDate::currentDate().daysTo(m_pLifeTime->date());

      uint64_t effectiveMB = std::max(1.0, *fileSizeMBytes + 1 - 1.0 / 1024 / 1024);
      uint64_t totalPricePerDay = effectiveMB * publishingPrice;
      uint64_t totalPrice = days * totalPricePerDay;

      _upload_button->setText(tr("Publish for") + " " + Globals::instance().asset(totalPrice).getString().c_str());
      _upload_button->setEnabled(true);
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
   std::string m_life_time = m_pLifeTime->text().toStdString();
   std::string str_keyparts = std::to_string(m_iKeyParticles);

   std::string m_price = QString::number(m_dPrice).toStdString();

   std::string assetName = Globals::instance().asset(0).m_str_symbol;

   std::string path = "";
   if (_contentPath->property("path").isValid())
      path = _contentPath->property("path").toString().toStdString();


   std::string samples_path = "";
   if (_samplesPath->property("path").isValid())
      samples_path = _samplesPath->property("path").toString().toStdString();

   std::string title = m_strTitle.toStdString();
   std::string desc = m_pDescriptionText->toPlainText().toStdString();

   setEnabled(false);

   json synopsis_obj;
   synopsis_obj["title"] = title;
   synopsis_obj["description"] = desc;

   std::string synopsis = synopsis_obj.dump(4);




   string str_seeders = getChosenPublishers().join(", ").toStdString();

   std::string submitCommand = "submit_content_new";
   submitCommand += " " + Globals::instance().getCurrentUser();         // author
   submitCommand += " \"" + path + "\"";                                // URI
   submitCommand += " \"" + samples_path + "\"";                        // Samples
   submitCommand += " \"ipfs\"";                                        // Protocol
   submitCommand += " " + assetName;                                    // price_asset_name
   submitCommand += " [[\"\", \"" + m_price + "\"]]";                   // price_amount
   submitCommand += " [" + str_seeders + "]";                           // seeders
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
      ShowMessageBox(tr("Content is being processed...") , tr("Success"));
      
      setEnabled(true);
      
      this->close();
   }
   else
   {
      ShowMessageBox(tr("Error"), tr("Failed to submit content"), message.c_str());
   }
}

