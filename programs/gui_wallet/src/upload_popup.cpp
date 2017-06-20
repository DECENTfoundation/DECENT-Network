#include "stdafx.h"

#include "gui_wallet_global.hpp"
#include "upload_popup.hpp"
#include "decent_button.hpp"
#include "decent_text_edit.hpp"
#include "decent_line_edit.hpp"
#include "gui_design.hpp"

#ifndef _MSC_VER
#include <QLocale>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QStringList>
#include <QComboBox>
#include <QDateTime>
#include <QDate>
#include <QTime>
#include <QTimer>
#include <QDateEdit>
#include <QCheckBox>
#include <QStyleFactory>
#include <QSignalMapper>
#include <QLocale>
#include <QFileInfo>

#include <graphene/chain/config.hpp>
#include <graphene/chain/content_object.hpp>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#endif

using std::string;

namespace gui_wallet
{

Upload_popup::Upload_popup(QWidget* pParent, const std::string content)
: QDialog(pParent)
, m_pStatusCheckTimer(new QTimer(this))
, m_pDescriptionText(new DecentTextEdit(this, DecentTextEdit::Editor))
, m_pLifeTime(new QDateEdit(this))
, m_iKeyParticles(2)
, m_dPrice(-1)
, m_resubmit_content(content)
{
   std::vector<Publisher> publishers = Globals::instance().getPublishers();
   m_arrPublishers.resize(publishers.size());
   for (size_t iIndex = 0; iIndex < publishers.size(); ++iIndex)
   {
      m_arrPublishers[iIndex].first = publishers[iIndex];
      m_arrPublishers[iIndex].second = false;
   }
   
   //
   setWindowTitle(tr("Upload new content"));
   //
   // Title field
   //
   DecentLineEdit* pTitleText = new DecentLineEdit(this, DecentLineEdit::DialogLineEdit);
   pTitleText->setPlaceholderText(tr("Title"));
   pTitleText->setAttribute(Qt::WA_MacShowFocusRect, 0);
   pTitleText->setTextMargins(5, 5, 5, 5);
   //
   // Description field
   //
   m_pDescriptionText->setPlaceholderText(tr("Description"));
   m_pDescriptionText->setTabChangesFocus(true);

   //
   // Lifetime
   //
   QLabel* pLifeTimeLabel = new QLabel(this);
   pLifeTimeLabel->setEnabled(false);
   pLifeTimeLabel->setText(tr("Expiration date"));

   m_pLifeTime->setDate(QDate::currentDate().addMonths(1));
   m_pLifeTime->setDisplayFormat("yyyy-MM-dd");
   m_pLifeTime->setCalendarPopup(true);
   m_pLifeTime->setMinimumDate(QDate::currentDate().addDays(1));
   m_pLifeTime->setStyle(QStyleFactory::create("fusion"));
   //
   // Key particles
   //
   QLabel* pKeypartsLabel = new QLabel(this);
   pKeypartsLabel->setEnabled(false);
   pKeypartsLabel->setText(tr("Key particles"));

   QComboBox* pKeypartsCombo = new QComboBox(this);
   pKeypartsCombo->setStyle(QStyleFactory::create("fusion"));

   for (int r = 2; r <= 7; ++r) {
      QString val = QString::fromStdString(std::to_string(r));
      pKeypartsCombo->addItem(val, val);
   }
   //
   // Price
   //
   QLabel* pPriceLabel = new QLabel(this);
   pPriceLabel->setEnabled(false);
   pPriceLabel->setText(tr("Price"));

   QDoubleValidator* dblValidator = new QDoubleValidator(0.0001, 100000, 4, this);
   dblValidator->setLocale(Globals::instance().locale());

   DecentLineEdit* pPriceEditor = new DecentLineEdit(this, DecentLineEdit::DialogLineEdit);
   pPriceEditor->setPlaceholderText(tr("Price"));
   pPriceEditor->setValidator(dblValidator);
   pPriceEditor->setAttribute(Qt::WA_MacShowFocusRect, 0);
   pPriceEditor->setTextMargins(5, 5, 5, 5);
   //
   // Seeders
   //
   DecentLineEdit* pSeedersPath = new DecentLineEdit(this, DecentLineEdit::DialogLineEdit);
   pSeedersPath->setEnabled(false);
   pSeedersPath->setPlaceholderText(tr("Seeders"));
   pSeedersPath->setReadOnly(true);
   pSeedersPath->setTextMargins(5, 5, 5, 5);

   DecentButton* pSeedersButton = new DecentButton(this, DecentButton::DialogAction);

   pSeedersButton->setText(tr("Select Seeders"));
   pSeedersButton->setFont(PopupButtonRegularFont());
   //
   // Content path
   //
   DecentLineEdit* pContentPath = new DecentLineEdit(this, DecentLineEdit::DialogLineEdit);
   pContentPath->setEnabled(false);
   pContentPath->setPlaceholderText(tr("Content path"));
   pContentPath->setReadOnly(true);
   pContentPath->setTextMargins(5, 5, 5, 5);

   DecentButton* pBrowseContentButton = new DecentButton(this, DecentButton::DialogAction);
   pBrowseContentButton->setText(tr("Browse"));
   pBrowseContentButton->setFont(PopupButtonRegularFont());
   //
   // Samples path
   //
   DecentLineEdit* pSamplesPath = new DecentLineEdit(this, DecentLineEdit::DialogLineEdit);
   pSamplesPath->setEnabled(false);
   pSamplesPath->setPlaceholderText(tr("Samples (optional)"));
   pSamplesPath->setReadOnly(true);
   pSamplesPath->setTextMargins(5, 5, 5, 5);

   DecentButton* pBrowseSamplesButton = new DecentButton(this, DecentButton::DialogAction);
   pBrowseSamplesButton->setText(tr("Browse"));
   pBrowseSamplesButton->setFont(PopupButtonRegularFont());
   //
   // Upload & Cancel
   //
   DecentButton* pUploadButton = new DecentButton(this, DecentButton::DialogAction);
   DecentButton* pCancelButton = new DecentButton(this, DecentButton::DialogCancel);

   pCancelButton->setText(tr("Cancel"));
   pCancelButton->setFont(PopupButtonBigFont());

   if(content.empty())
      pUploadButton->setText(tr("Publish"));
   else
      pUploadButton->setText(tr("Resubmit"));
   pUploadButton->setFont(PopupButtonBigFont());
   
   //resubmit layout type
   if ( !content.empty() ){
      m_pLifeTime->setReadOnly(true);
      pKeypartsCombo->setDisabled(true);
//      pSeedersButton->setDisabled(true);
      pBrowseContentButton->setDisabled(true);
      pBrowseSamplesButton->setDisabled(true);
   }
   //
   // Layouts
   //
   QHBoxLayout* pLifeTimeRow = new QHBoxLayout();
   pLifeTimeRow->addWidget(pLifeTimeLabel);
   pLifeTimeRow->addWidget(m_pLifeTime);

   QHBoxLayout* pKeyRow = new QHBoxLayout();
   pKeyRow->addWidget(pKeypartsLabel);
   pKeyRow->addWidget(pKeypartsCombo);

   QHBoxLayout* pPriceRow = new QHBoxLayout();
   pPriceRow->addWidget(pPriceLabel);
   pPriceRow->addWidget(pPriceEditor);

   QHBoxLayout* pSeedersRow = new QHBoxLayout();
   pSeedersRow->addWidget(pSeedersPath);
   pSeedersRow->addWidget(pSeedersButton);

   QHBoxLayout* pContentRow = new QHBoxLayout();
   pContentRow->addWidget(pContentPath);
   pContentRow->addWidget(pBrowseContentButton);

   QHBoxLayout* pSamplesRow = new QHBoxLayout();
   pSamplesRow->addWidget(pSamplesPath);
   pSamplesRow->addWidget(pBrowseSamplesButton);

   QHBoxLayout* pButtonsLayout = new QHBoxLayout;
   pButtonsLayout->setContentsMargins(20, 20, 20, 20);
   pButtonsLayout->addWidget(pUploadButton);
   pButtonsLayout->addWidget(pCancelButton);

   QVBoxLayout* pMainLayout = new QVBoxLayout;
   pMainLayout->addWidget(pTitleText);
   pMainLayout->addWidget(m_pDescriptionText);
   pMainLayout->addLayout(pLifeTimeRow);
   pMainLayout->addLayout(pKeyRow);
   pMainLayout->addLayout(pPriceRow);
   pMainLayout->addLayout(pSeedersRow);
   pMainLayout->addLayout(pContentRow);
   pMainLayout->addLayout(pSamplesRow);
   pMainLayout->addLayout(pButtonsLayout);
   pMainLayout->setContentsMargins(10, 10, 10, 10);
   setLayout(pMainLayout);

   slot_UpdateStatus();
   //
   // signal/slot conntections
   //
   QObject::connect(pTitleText, &QLineEdit::textChanged,
                    this, &Upload_popup::slot_TitleChanged);
   QObject::connect(pTitleText, &QLineEdit::textChanged,
                    this, &Upload_popup::slot_UpdateStatus);
   QObject::connect(m_pDescriptionText, &QTextEdit::textChanged,
                    this, &Upload_popup::slot_UpdateStatus);
   QObject::connect(pPriceEditor, &QLineEdit::textChanged,
                    this, &Upload_popup::slot_PriceChanged);

   QObject::connect(pContentPath, &QLineEdit::textChanged,
                    this, &Upload_popup::slot_ContentPathChanged);
   QObject::connect(this, &Upload_popup::signal_ContentPathChange,
                    pContentPath, &QLineEdit::setText);

   QObject::connect(pSamplesPath, &QLineEdit::textChanged,
                    this, &Upload_popup::slot_SamplesPathChanged);
   QObject::connect(this, &Upload_popup::signal_SamplesPathChange,
                    pSamplesPath, &QLineEdit::setText);

   QObject::connect(pKeypartsCombo, (void (QComboBox::*)(QString const&))&QComboBox::activated,
                    this, &Upload_popup::slot_KeyParticlesChanged);

   QObject::connect(pSeedersButton, &QPushButton::clicked,
                    this, &Upload_popup::slot_ChooseSeeders);
   QObject::connect(this, &Upload_popup::signal_SetSeedersText,
                    pSeedersPath, &QLineEdit::setText);

   QObject::connect(pBrowseContentButton, &QPushButton::clicked,
                    this, &Upload_popup::slot_BrowseContent);
   QObject::connect(pBrowseSamplesButton, &QPushButton::clicked,
                    this, &Upload_popup::slot_BrowseSamples);

   QObject::connect(pUploadButton, &QPushButton::clicked,
                    this, &Upload_popup::slot_UploadContent);
   QObject::connect(pCancelButton, &QPushButton::clicked,
                    this, &QDialog::close);

   QObject::connect(this, &Upload_popup::signal_UploadButtonEnabled,
                    pUploadButton, &QWidget::setEnabled);
   QObject::connect(this, &Upload_popup::signal_UploadButtonSetText,
                    pUploadButton, &QPushButton::setText);

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

   DecentTable* pSeedersTable = new DecentTable(pDialog);
   pSeedersTable->set_columns({
      {"", -25},
      {tr("Seeder"), 10, "rating"},
      {tr("Price"),  10, "price"},
      {tr("Size") ,  10, "size"}
   });

   DecentButton* pOKButton = new DecentButton(pDialog, DecentButton::DialogAction);
   pOKButton->setText(tr("OK"));
   pOKButton->setFont(TabButtonFont());

   QHBoxLayout* pButtonsLayout = new QHBoxLayout;
   pButtonsLayout->setMargin(10);
   pButtonsLayout->setAlignment(Qt::AlignHCenter);
   pButtonsLayout->addWidget(pOKButton);

   QVBoxLayout* pMainLayout = new QVBoxLayout;
   pMainLayout->setContentsMargins(0, 0, 0, 0);
   pMainLayout->setMargin(0);
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
#ifdef _MSC_VER
   int height = pDialog->style()->pixelMetric(QStyle::PM_TitleBarHeight);
   pDialog->setWindowIcon(height > 32 ? QIcon(":/icon/images/windows_decent_icon_32x32.png")
      : QIcon(":/icon/images/windows_decent_icon_16x16.png"));
#endif
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
      m_dPrice = Globals::instance().locale().toDouble(strPrice, &bPriceIsOK);

      if (false == bPriceIsOK)
         m_dPrice = -1;
   }
}

void Upload_popup::slot_ContentPathChanged(QString const& strContentPath)
{
   m_strContentPath = strContentPath;
}

void Upload_popup::slot_SamplesPathChanged(QString const& strSamplesPath)
{
   m_strSamplesPath = strSamplesPath;
}

void Upload_popup::slot_UpdateStatus()
{
   std::string path = m_strContentPath.toStdString();
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
      emit signal_UploadButtonEnabled(false);
      emit signal_UploadButtonSetText(tr("Publish"));
   }
   else
   {
      std::string lifeTime = m_pLifeTime->text().toStdString();
      int days = QDate::currentDate().daysTo(m_pLifeTime->date());

      uint64_t effectiveMB = std::max(1.0, *fileSizeMBytes + 1 - 1.0 / 1024 / 1024);
      uint64_t totalPricePerDay = effectiveMB * publishingPrice;
      uint64_t totalPrice = days * totalPricePerDay;
      
      emit signal_UploadButtonEnabled(true);
      emit signal_UploadButtonSetText(tr("Publish for") + " " + Globals::instance().asset(totalPrice).getString().c_str());
   };
   
   //for resubmit
   if ( !m_resubmit_content.empty() ){
      emit signal_UploadButtonSetText(tr("Resubmit"));

      if( -1 != m_dPrice && !strDescription.isEmpty() && !m_strTitle.isEmpty() ){
         emit signal_UploadButtonEnabled(true);
   
      }
   }
}

void Upload_popup::slot_BrowseContent()
{
   QString contentPathSelected = QFileDialog::getOpenFileName(this, tr("Select content"), "~");

   if (contentPathSelected.isEmpty())
      return;

   boost::system::error_code ec;
   if (boost::filesystem::file_size(contentPathSelected.toStdString(), ec) > 100 * 1024 * 1024)
   {
      ShowMessageBox("Error", "Content size is limited in Testnet 0.1 to 100MB");
      return;
   }

   emit signal_ContentPathChange(contentPathSelected);
}

void Upload_popup::slot_BrowseSamples()
{
   QString sampleDir = QFileDialog::getExistingDirectory(this, tr("Select samples"), "~", QFileDialog::DontResolveSymlinks);

   emit signal_SamplesPathChange(sampleDir);
}

void Upload_popup::slot_UploadContent()
{
   std::string m_life_time = m_pLifeTime->text().toStdString();
   std::string str_keyparts = std::to_string(m_iKeyParticles);

   std::string m_price = QString::number(m_dPrice).toStdString();

   std::string assetName = Globals::instance().asset(0).m_str_symbol;

   std::string path = m_strContentPath.toStdString();

   std::string samples_path = m_strSamplesPath.toStdString();

   std::string title = m_strTitle.toStdString();
   std::string desc = m_pDescriptionText->toPlainText().toStdString();

   graphene::chain::ContentObjectTypeValue content_type(graphene::chain::EContentObjectApplication::DecentCore);
   graphene::chain::ContentObjectPropertyManager synopsis_construct;
   synopsis_construct.set<graphene::chain::ContentObjectType>(content_type);
   synopsis_construct.set<graphene::chain::ContentObjectTitle>(title);
   synopsis_construct.set<graphene::chain::ContentObjectDescription>(desc);
   std::string synopsis = synopsis_construct.m_str_synopsis;

   string str_seeders = getChosenPublishers().join(", ").toStdString();
   std::string _submitCommand;
   std::string submitCommand;
   std::string res;
   
   if( !m_resubmit_content.empty() )
      try
      {
         //QFileInfo file(QString::fromStdString(path));
         //std::string str_file_size = std::to_string(file.size());
         
         string search_command = "search_user_content";
         search_command += " \"" + Globals::instance().getCurrentUser() + "\"";
         search_command += " \"" + m_resubmit_content + "\"";
         search_command += " \"\" \"\" \"\" \"0\" 1";
         
         std::string contents = Globals::instance().runTask(search_command);
        
         std::cout << contents << std::endl;
         
         nlohmann::json parse = nlohmann::json::parse(contents);
         std::string str_URI = parse[0]["URI"].get<std::string>();
         
         //content info
         std::string info = Globals::instance().runTask("get_content \"" + str_URI + "\"");
         nlohmann::json jsp = nlohmann::json::parse(info);
         
         std::cout << info << std::endl;
         
         std::string _hash = jsp["_hash"].get<std::string>();
         std::string str_expiration = jsp["expiration"].get<std::string>();
         std::string str_size = std::to_string( jsp["size"].get<uint64_t>() );
         std::string str_quorum = std::to_string( jsp["quorum"].get<uint32_t>() );
         std::string str_fee = std::to_string( jsp["publishing_fee_escrow"]["amount"].get<uint16_t>() );
         
         //cd
         std::string cd;
         cd += " {\"n\": "        + std::to_string( jsp["cd"]["n"].get<uint>() ) + ",";
         cd += " \"u_seed\": \"" + jsp["cd"]["u_seed"].get<std::string>() + "\",";
         cd += " \"pubKey\": \"" + jsp["cd"]["pubKey"].get<std::string>() + "\"}";

         std::string str_AES_key    = Globals::instance().runTask("generate_encryption_key");
         
         //submit content
         _submitCommand = "submit_content";
         _submitCommand += " " + Globals::instance().getCurrentUser() + " []"; //author
         _submitCommand += " \"" + str_URI + "\"";                            //URI
         _submitCommand += " [{\"region\" : \"\", \"amount\" : \"" + m_price + "\", \"asset_symbol\" : \"" + assetName + "\" }]";//price
         _submitCommand += " " + str_size + "";                               //file size
         _submitCommand += " {\"_hash\": \"" + _hash + "\"}";                                //hash of package
         _submitCommand += " [" + str_seeders + "]";                          //seeders
         _submitCommand += " " + str_quorum + "";                             //quorum
         _submitCommand += " \"" + str_expiration + "\"";                     //expiration
         _submitCommand += " \"DCT\"";                                        //fee asset
         _submitCommand += " \"" + str_fee + "\"";                            //fee price
         _submitCommand += " \"" + escape_string(synopsis) + "\"";            //synopsis
         _submitCommand += " {" + str_AES_key  + "}";                         //AES key
         _submitCommand += cd;                                                //cd
         _submitCommand += " true";
         
         res = Globals::instance().runTask(_submitCommand);
      }catch(...)
   {
      std::cout << res << std::endl;
   }
   else
   {
      submitCommand = "submit_content_async";
      submitCommand += " " + Globals::instance().getCurrentUser() + " []";  // author
      submitCommand += " \"" + path + "\"";                                // URI
      submitCommand += " \"" + samples_path + "\"";                        // Samples
      submitCommand += " \"ipfs\"";                                        // Protocol
      submitCommand += " [{\"region\" : \"\", \"amount\" : \"" + m_price + "\", \"asset_symbol\" : \"" + assetName + "\" }]";// price_amount
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
         a_result = Globals::instance().runTask(submitCommand);//submit
         if (a_result.find("exception:") != std::string::npos)
         {
            message = a_result;
         }
      }
      catch (const std::exception& ex)
      {
         message = ex.what();
      }

      if (message.empty())
      {
         close();
      }
      else
      {
         ShowMessageBox(tr("Error"), tr("Failed to submit content"), message.c_str());
      }
   }//else case

}

} // end namespace gui_wallet

