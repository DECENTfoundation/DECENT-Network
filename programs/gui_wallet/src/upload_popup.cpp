/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */

#ifndef STDAFX_H
#include "../stdafx.h"
#endif

#include "upload_popup.hpp"
#include "gui_wallet_global.hpp"
#include "decent_button.hpp"
#include "decent_text_edit.hpp"
#include "decent_line_edit.hpp"
#include "decent_label.hpp"

namespace gui_wallet
{

Upload_popup::Upload_popup(QWidget* pParent, const std::string& id_modify/* = string()*/) : StackLayerWidget(pParent)
, m_pStatusCheckTimer(new QTimer(this))
, m_pDescriptionText(new DecentTextEdit(this, DecentTextEdit::Editor))
, m_pLifeTime(new QDateEdit(this))
, m_dPrice(-1)
, m_id_modify(id_modify)
{
   std::vector<Publisher> publishers = Globals::instance().getPublishers();
   m_arrPublishers.resize(publishers.size());
   for (size_t iIndex = 0; iIndex < publishers.size(); ++iIndex)
   {
      m_arrPublishers[iIndex].first = publishers[iIndex];
      m_arrPublishers[iIndex].second = false;
   }
   
   //
   DecentLabel* pTitleLabel = new DecentLabel(this);
   pTitleLabel->setText(tr("Upload new content"));

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
   m_pLifeTime->setDisplayFormat(Globals::instance().locale().dateFormat(QLocale::ShortFormat));  //  "yyyy-MM-dd"
   m_pLifeTime->setCalendarPopup(true);
   m_pLifeTime->setMinimumDate(QDate::currentDate().addDays(1));
   m_pLifeTime->setStyle(QStyleFactory::create("fusion"));
   //
   // Price
   //
   QLabel* pPriceLabel = new QLabel(this);
   pPriceLabel->setEnabled(false);
   pPriceLabel->setText(tr("Price"));

   Asset min_price_asset = Globals::instance().asset(1);
   double min_price = 0; //min_price_asset.to_value();

   Asset max_price_asset = Globals::instance().asset(100000 * pow(10, g_max_number_of_decimal_places));
   double max_price = max_price_asset.to_value();

   QDoubleValidator* dblValidator = new QDoubleValidator(min_price, max_price, g_max_number_of_decimal_places, this);
   dblValidator->setLocale(Globals::instance().locale());

   m_pPriceEditor = new DecentLineEdit(this, DecentLineEdit::DialogLineEdit);
   m_pPriceEditor->setPlaceholderText(QString(tr("Price in %1")).arg(QString::fromStdString(Globals::instance().asset(0).m_str_symbol)));
   m_pPriceEditor->setValidator(dblValidator);
   m_pPriceEditor->setAttribute(Qt::WA_MacShowFocusRect, 0);
   m_pPriceEditor->setTextMargins(5, 5, 5, 5);
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

   m_pTotalPriceLabel = new QLabel(this);
   m_pTotalPriceLabel->setVisible(false);
   m_pTotalPriceLabel->setText(tr("Price"));

   //
   // Upload & Cancel
   //
   DecentButton* pUploadButton = new DecentButton(this, DecentButton::DialogAction);
   DecentButton* pCancelButton = new DecentButton(this, DecentButton::DialogCancel);

   pCancelButton->setText(tr("Back"));
   pCancelButton->setFont(PopupButtonBigFont());

   pUploadButton->setText(tr("Publish"));
   pUploadButton->setFont(PopupButtonBigFont());
   pUploadButton->setEnabled(false);
   
   //resubmit layout type
   if (!m_id_modify.empty())
   {
      m_pLifeTime->setReadOnly(true);
      pBrowseContentButton->setDisabled(true);
      pBrowseSamplesButton->setDisabled(true);
      pSeedersButton->setDisabled(true);
   }
   //
   // Layouts
   //
   QVBoxLayout* pFirstRow = new QVBoxLayout();
   pFirstRow->addWidget(pTitleLabel);
   pFirstRow->addWidget(pTitleText);
   pFirstRow->addWidget(m_pDescriptionText);

   QHBoxLayout* pLifeTimeRow = new QHBoxLayout();
   pLifeTimeRow->addWidget(pLifeTimeLabel);
   pLifeTimeRow->addWidget(m_pLifeTime);

   QHBoxLayout* pPriceRow = new QHBoxLayout();
   pPriceRow->addWidget(pPriceLabel);
   pPriceRow->addWidget(m_pPriceEditor);

   QHBoxLayout* pSeedersRow = new QHBoxLayout();
   pSeedersRow->addWidget(pSeedersPath);
   pSeedersRow->addWidget(pSeedersButton);

   QHBoxLayout* pContentRow = new QHBoxLayout();
   pContentRow->addWidget(pContentPath);
   pContentRow->addWidget(pBrowseContentButton);

   QHBoxLayout* pSamplesRow = new QHBoxLayout();
   pSamplesRow->addWidget(pSamplesPath);
   pSamplesRow->addWidget(pBrowseSamplesButton);

   QHBoxLayout* pPublishTextRow = new QHBoxLayout();
   pPublishTextRow->addWidget(m_pTotalPriceLabel);

   QHBoxLayout* pButtonsLayout = new QHBoxLayout;
   pButtonsLayout->setContentsMargins(20, 20, 20, 20);
   pButtonsLayout->addWidget(pUploadButton);
   pButtonsLayout->addWidget(pCancelButton);

   QVBoxLayout* pMainLayout = new QVBoxLayout;
   pMainLayout->addLayout(pFirstRow);
   pMainLayout->addLayout(pLifeTimeRow);
   pMainLayout->addLayout(pPriceRow);
   pMainLayout->addLayout(pSeedersRow);
   pMainLayout->addLayout(pContentRow);
   pMainLayout->addLayout(pSamplesRow);
   pMainLayout->addLayout(pPublishTextRow);
   pMainLayout->addLayout(pButtonsLayout);
   pMainLayout->setContentsMargins(10, 10, 10, 10);
   setLayout(pMainLayout);

   QWidget::setTabOrder(pTitleText, m_pDescriptionText);
   QWidget::setTabOrder(m_pDescriptionText, m_pLifeTime);
   QWidget::setTabOrder(m_pLifeTime, m_pPriceEditor);



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
   QObject::connect(m_pPriceEditor, &QLineEdit::textChanged,
                    this, &Upload_popup::slot_PriceChanged);

   QObject::connect(pContentPath, &QLineEdit::textChanged,
                    this, &Upload_popup::slot_ContentPathChanged);
   QObject::connect(this, &Upload_popup::signal_ContentPathChange,
                    pContentPath, &QLineEdit::setText);

   QObject::connect(pSamplesPath, &QLineEdit::textChanged,
                    this, &Upload_popup::slot_SamplesPathChanged);
   QObject::connect(this, &Upload_popup::signal_SamplesPathChange,
                    pSamplesPath, &QLineEdit::setText);

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
                    this, &StackLayerWidget::closed);

   QObject::connect(this, &Upload_popup::signal_UploadButtonEnabled,
                    pUploadButton, &QWidget::setEnabled);

   QObject::connect(m_pStatusCheckTimer, &QTimer::timeout,
                    this, &Upload_popup::slot_UpdateStatus);
   m_pStatusCheckTimer->start(500);

   pTitleText->show();
   pTitleText->setFocus();



   if (!m_id_modify.empty())
   {
      std::string title, description, price, expiration;
      getContents(m_id_modify, title, description, price, expiration);

      QDateTime time = QDateTime::fromString(QString::fromStdString(expiration), "yyyy-MM-ddTHH:mm:ss");

      m_pLifeTime->setDate(time.date());
      pTitleText->setText(QString::fromStdString(title));
      m_pDescriptionText->setText(QString::fromStdString(description));
      m_pPriceEditor->setText(QString::fromStdString(price));
      slot_SeederChanged(-1);
   }

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
         lstSummary << QString::fromStdString(seeder.m_str_name);
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
      QString pubFreeSpace = QString::number(free_space);
      pubFreeSpace += " MB free";
      if (free_space > 800)
         pubFreeSpace = QString::number(free_space / 1024.0, 'f', 2) + " GB free";

      EventPassthrough<QCheckBox>* pCheckBox = new EventPassthrough<QCheckBox>(pSeedersTable);
      pSeedersTable->setCellWidget(iIndex, eCheckBox, pCheckBox);
      pSeedersTable->setItem(iIndex, eName, new QTableWidgetItem(QString::fromStdString(seeder.m_str_name)));
      pSeedersTable->setItem(iIndex, ePrice, new QTableWidgetItem(seeder.m_price.getString()));
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
   if (iIndex >= 0 && iIndex < m_arrPublishers.size())
   {
      bool& bState = m_arrPublishers[iIndex].second;
      bState = !bState;
   }

   QStringList lstSummary = getChosenPublishers();

   QString strSummary;
   if (lstSummary.size() > 4)
      strSummary = QString::number(lstSummary.size());
   else
      strSummary = lstSummary.join(", ");

   emit signal_SetSeedersText(strSummary);
}

void Upload_popup::slot_TitleChanged(const QString& strTitle)
{
   m_strTitle = strTitle;
}

void Upload_popup::slot_PriceChanged(const QString& strPrice)
{
   if (strPrice.isEmpty())
      m_dPrice = -1;
   else
   {
      bool bPriceIsOK = m_pPriceEditor->hasAcceptableInput();
      if (bPriceIsOK) {
         m_dPrice = Globals::instance().locale().toDouble(strPrice, &bPriceIsOK);
      }

      if (!bPriceIsOK)
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
   boost::optional<double> fileSizeGBytes;
   if (!path.empty())
      fileSizeGBytes = boost::filesystem::file_size(path, ec) / 1024.0 / 1024.0 / 1024.0;

   bool isPublishersValid = false;
   uint64_t publishingPrice = 0;

   for(size_t iSeederIndex = 0; iSeederIndex < m_arrPublishers.size(); ++iSeederIndex)
   {
      auto const& seederItem = m_arrPublishers[iSeederIndex];
      Publisher const& seeder = seederItem.first;

      if (!seederItem.second)
         continue;

      isPublishersValid = true;
      publishingPrice += seeder.m_price.m_amount;
   }

   if (m_id_modify.empty())
   {
      if (-1 == m_dPrice ||
          ec ||
          !fileSizeGBytes ||
          *fileSizeGBytes > 10 ||
          m_strTitle.isEmpty() ||
          strDescription.isEmpty() ||
          Globals::instance().getCurrentUser().empty() ||
          !isPublishersValid)
      {
         m_pTotalPriceLabel->setVisible(false);
         emit signal_UploadButtonEnabled(false);
      }
      else
      {
         std::string lifeTime = m_pLifeTime->text().toStdString();
         int64_t days = QDate::currentDate().daysTo(m_pLifeTime->date());

         uint64_t effectiveMB = std::max(1.0, (*fileSizeGBytes) * 1024.0 + 1 - 1.0 / 1024 / 1024);
         uint64_t totalPricePerDay = effectiveMB * publishingPrice;
         uint64_t totalPrice = days * totalPricePerDay;

         m_pTotalPriceLabel->setText(tr("Publish for") + " " + Globals::instance().asset(totalPrice).getString());
         m_pTotalPriceLabel->setVisible(true);

         emit signal_UploadButtonEnabled(true);
      }
   }
   else
   {
      if (-1 == m_dPrice ||
          strDescription.isEmpty() ||
          m_strTitle.isEmpty() ||
          Globals::instance().getCurrentUser().empty() ||
          !isPublishersValid)
      {
         m_pTotalPriceLabel->setVisible(false);
         emit signal_UploadButtonEnabled(false);
      }
      else
      {
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
   if (boost::filesystem::file_size(contentPathSelected.toStdString(), ec) > (uint64_t) DECENT_MAX_FILE_SIZE * 1024 * 1024)
   {
      ShowMessageBox("Error", "Content size is limited to 10GB");
      return;
   }

   emit signal_ContentPathChange(contentPathSelected);
}

void Upload_popup::slot_BrowseSamples()
{
   QString sampleDir = QFileDialog::getExistingDirectory(this, tr("Select samples"), "~", QFileDialog::DontResolveSymlinks);

   emit signal_SamplesPathChange(sampleDir);
}
   
void Upload_popup::getContents(std::string const& id,
                               std::string& hash,
                               std::string& str_expiration,
                               std::string& str_size,
                               std::string& str_quorum,
                               std::string& str_fee,
                               std::string& str_cd,
                               std::string& uri) const
{
   try
   {
      std::string command =  "search_content "
                        "\"\" "
                        "\"\" "
                        "\"\" "
                        "\"\" "
                        "\"" + id + "\" "
                        "\"\" "
                        "1";

      nlohmann::json contents = Globals::instance().runTaskParse(command);
      nlohmann::json content_summary = contents[0];
      hash = content_summary["_hash"];
      str_expiration = content_summary["expiration"].get<std::string>();
      str_size = std::to_string( content_summary["size"].get<uint64_t>() );
      uri = content_summary["URI"];

      nlohmann::json const content_object = Globals::instance().runTaskParse("get_content \"" + uri + "\"");
      nlohmann::json const& quorum = content_object["quorum"];
      nlohmann::json const& cd = content_object["cd"];

      str_quorum = std::to_string( quorum.get<uint32_t>() );

      str_cd = std::string();
      str_cd += " {\"n\": " + std::to_string( cd["n"].get<uint>() ) + ",";
      str_cd += " \"u_seed\": \"" + cd["u_seed"].get<std::string>() + "\",";
      str_cd += " \"pubKey\": \"" + cd["pubKey"].get<std::string>() + "\"}";
   }
   catch(const std::exception& ex) {
      std::cout << "Upload_popup::getContents " << ex.what() << std::endl;
   }
   catch(const fc::exception& ex) {
      std::cout << "Upload_popup::getContents " << ex.what() << std::endl;
   }

}
void Upload_popup::getContents(std::string const& id,
                               std::string& title,
                               std::string& description,
                               std::string& price,
                               std::string& str_expiration)
{
   try
   {
      std::string command =  "search_content "
                        "\"\" "
                        "\"\" "
                        "\"\" "
                        "\"\" "
                        "\"" + id + "\" "
                        "\"\" "
                        "1";

      nlohmann::json contents = Globals::instance().runTaskParse(command);
      nlohmann::json content_summary = contents[0];

      std::string synopsis = content_summary["synopsis"].get<std::string>();
      uint64_t iPrice = json_to_int64(content_summary["price"]["amount"]);
      std::string iSymbolId = content_summary["price"]["asset_id"];
      Asset asset_price = Globals::instance().asset(iPrice, iSymbolId);
      price = std::to_string(asset_price.to_value());
      str_expiration = content_summary["expiration"].get<std::string>();
      std::string uri = content_summary["URI"];

      graphene::chain::ContentObjectPropertyManager synopsis_parser(synopsis);
      title = synopsis_parser.get<graphene::chain::ContentObjectTitle>();
      description = synopsis_parser.get<graphene::chain::ContentObjectDescription>();

      nlohmann::json const content_object = Globals::instance().runTaskParse("get_content \"" + uri + "\"");
      nlohmann::json const& key_parts = content_object["key_parts"];

      std::map<std::string, size_t> indexPublishers;
      for (size_t index = 0; index < m_arrPublishers.size(); ++index)
         indexPublishers.insert(std::make_pair(m_arrPublishers[index].first.m_str_name, index));

      for (size_t index = 0; index < key_parts.size(); ++index)
      {
         std::string seeder_name = key_parts[index][0].get<std::string>();
         auto it = indexPublishers.find(seeder_name);
         if (it != indexPublishers.end())
         {
            m_arrPublishers[it->second].second = true;
         }
      }
   }
   catch(const std::exception& ex) {
      std::cout << "Upload_popup::getContents " << ex.what() << std::endl;
   }
   catch(const fc::exception& ex) {
      std::cout << "Upload_popup::getContents " << ex.what() << std::endl;
   }

}

void Upload_popup::slot_UploadContent()
{
   std::string m_life_time =  m_pLifeTime->date().toString("yyyy-MM-dd").toStdString();

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

   std::string str_seeders = getChosenPublishers().join(", ").toStdString();

   std::string submitCommand;
   
   if (!m_id_modify.empty())
   {
      std::string hash;
      std::string str_expiration;
      std::string str_size;
      std::string str_quorum;
      std::string str_fee;
      std::string cd;
      std::string uri;

      getContents(m_id_modify, hash, str_expiration, str_size, str_quorum, str_fee, cd, uri);

      std::string str_AES_key = Globals::instance().runTask("generate_encryption_key");
      
      //submit content
      submitCommand = "submit_content";
      submitCommand += " " + Globals::instance().getCurrentUser() + " []";//author
      submitCommand += " \"" + uri + "\"";                                //URI
      submitCommand += " [{\"region\" : \"\", \"amount\" : \"" + m_price + "\", \"asset_symbol\" : \"" + assetName + "\" }]";//price
      submitCommand += " " + str_size + "";                               //file size
      submitCommand += " \"" + hash + "\"";                               //hash of package
      submitCommand += " [" + str_seeders + "]";                          //seeders
      submitCommand += " " + str_quorum + "";                             //quorum
      submitCommand += " \"" + str_expiration + "\"";                     //expiration
      submitCommand += " \"DCT\"";                                        //fee asset
      submitCommand += " \"" + str_fee + "\"";                            //fee price
      submitCommand += " \"" + escape_string(synopsis) + "\"";            //synopsis
      submitCommand += " " + str_AES_key  + "";                           //AES key
      submitCommand += cd;                                                //cd
      submitCommand += " true";
   }
   else
   {
      submitCommand = "submit_content_async";
      submitCommand += " " + Globals::instance().getCurrentUser() + " []"; // author
      submitCommand += " \"" + path + "\"";                                // URI
      submitCommand += " \"" + samples_path + "\"";                        // Samples
      submitCommand += " \"ipfs\"";                                        // Protocol
      submitCommand += " [{\"region\" : \"\", \"amount\" : \"" + m_price + "\", \"asset_symbol\" : \"" + assetName + "\" }]";// price_amount
      submitCommand += " [" + str_seeders + "]";                           // seeders
      submitCommand += " \"" + m_life_time + "T23:59:59\"";                // expiration
      submitCommand += " \"" + escape_string(synopsis) + "\"";             // synopsis
      
      // this is an example how price per regions will be used
      // submitCommand += " [[\"default\", \"0\"], [\"US\", \"10\"]]";
   }//if-else end

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
      emit accepted();
   }
   else
   {
      ShowMessageBox(tr("Error"), tr("Failed to submit content"), message.c_str());
   }
}

} // end namespace gui_wallet
