#include "stdafx.h"

#include "gui_wallet_global.hpp"
#include "upload_tab.hpp"
#include "upload_popup.hpp"
#include "decent_wallet_ui_gui_contentdetailsgeneral.hpp"
#include "decent_button.hpp"
#include "gui_design.hpp"

#ifndef _MSC_VER
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDateTime>
#include <QDate>
#include <QTime>
#include <QTimer>
#include <QSignalMapper>
#include <QLabel>
#include <QLineEdit>

#include <graphene/chain/config.hpp>
#include <graphene/chain/content_object.hpp>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>

#include "json.hpp"
#endif

using string = std::string;

namespace gui_wallet
{

Upload_tab::Upload_tab(QWidget* pParent)
: TabContentManager(pParent)
, m_pTableWidget(new DecentTable(this))
, m_pDetailsSignalMapper(nullptr)
{
   m_pTableWidget->set_columns({
      {tr("Title"), 20},
      {tr("Rating"), 10, "rating"},
      {tr("Size"), 10, "size"},
      {tr("Price"), 10, "price"},
      {tr("Published"), 10, "created"},
      {tr("Expiration"), 10, "expiration"},
      {tr("Status"), 10},
#ifdef WINDOWS_HIGH_DPI
      { " ", -80 }
#else
      {" ", -50}
#endif
   });

   DecentButton* pUploadButton = new DecentButton(this);
   pUploadButton->setFont(TabButtonFont());
   pUploadButton->setText(tr("Publish"));
   pUploadButton->setMinimumWidth(102);
   pUploadButton->setMinimumHeight(54);

   QLabel* pSearchLabel = new QLabel(this);
   QPixmap image(icon_search);
   pSearchLabel->setPixmap(image);

   QLineEdit* pfilterLineEditor = new QLineEdit(this);
   pfilterLineEditor->setPlaceholderText(tr("Search Content"));
   pfilterLineEditor->setFixedHeight(54);
   pfilterLineEditor->setStyleSheet(d_lineEdit);
   pfilterLineEditor->setAttribute(Qt::WA_MacShowFocusRect, 0);

   QHBoxLayout* pSearchLayout = new QHBoxLayout;
   pSearchLayout->setContentsMargins(42, 0, 0, 0);
   pSearchLayout->addWidget(pSearchLabel);
   pSearchLayout->addWidget(pfilterLineEditor);
   pSearchLayout->addWidget(pUploadButton, 0 , Qt::AlignCenter);
   //pSearchLayout->addWidget(pUploadButton, 0 , Qt::AlignBottom);

   QVBoxLayout* pMainLayout = new QVBoxLayout;
   pMainLayout->setContentsMargins(0, 0, 0, 0);
   pMainLayout->setSpacing(0);
   pMainLayout->addLayout(pSearchLayout);
   pMainLayout->addWidget(m_pTableWidget);
   setLayout(pMainLayout);

   QObject::connect(&Globals::instance(), &Globals::currentUserChanged,
                    this, &Upload_tab::slot_UpdateContents);
   QObject::connect(this, &Upload_tab::signal_setEnabledUpload,
                    pUploadButton, &QWidget::setEnabled);
   QObject::connect(m_pTableWidget, &DecentTable::signal_SortingChanged,
                    this, &Upload_tab::slot_SortingChanged);
   QObject::connect(pfilterLineEditor, &QLineEdit::textChanged,
                    this, &Upload_tab::slot_SearchTermChanged);
   QObject::connect(pUploadButton, &QPushButton::clicked,
                    this, &Upload_tab::slot_UploadPopup);

   slot_UpdateContents();
}

// when class has forward declared members
// this becomes necessary
Upload_tab::~Upload_tab() = default;

void Upload_tab::slot_UpdateContents()
{
   string currentUserName = Globals::instance().getCurrentUser();
   if(currentUserName.empty())
      emit signal_setEnabledUpload(false);
   else
      emit signal_setEnabledUpload(true);
}

void Upload_tab::timeToUpdate(const string& result)
{
   _digital_contents.clear();

   if (result.empty())
      return;

   auto contents = nlohmann::json::parse(result);
   size_t iSize = contents.size();
   if (iSize > m_i_page_size)
      iSize = m_i_page_size;
   
   _digital_contents.resize(contents.size());
   
   for (size_t iIndex = 0; iIndex < iSize; ++iIndex)
   {
      SDigitalContent& content = _digital_contents[iIndex];
      auto const& json_content = contents[iIndex];
      
      content.type = DCT::GENERAL;
      content.author = json_content["author"].get<string>();
      uint64_t iPrice = json_to_int64(json_content["price"]["amount"]);
      content.price = Globals::instance().asset(iPrice);
      content.synopsis = json_content["synopsis"].get<string>();
      content.URI = json_content["URI"].get<string>();
      content.created = json_content["created"].get<string>();
      content.created = content.created.substr(0, content.created.find("T"));
      content.expiration = json_content["expiration"].get<string>();
      content.size = json_content["size"].get<int>();
      content.status = json_content["status"].get<string>();

      QDateTime time = QDateTime::fromString(QString::fromStdString(content.expiration), "yyyy-MM-ddTHH:mm:ss");

      if (content.status.empty())
      {
         if (time < QDateTime::currentDateTime())
            content.status = "Expired";
         else
            content.status = "Published";
      }
      
      if (json_content["times_bought"].is_number())
         content.times_bought = json_content["times_bought"].get<int>();
      else
         content.times_bought = 0;

      content.AVG_rating = json_content["AVG_rating"].get<double>() / 1000.0;
   }

   if (contents.size() > m_i_page_size)
      set_next_page_iterator(contents[m_i_page_size]["id"].get<string>());
   else
      set_next_page_iterator(string());
   
   ShowDigitalContentsGUI();
}

string Upload_tab::getUpdateCommand()
{
   string currentUserName = Globals::instance().getCurrentUser();
   if (currentUserName.empty())
      return string();
   
   return   "search_user_content "
            "\"" + currentUserName + "\" "
            "\"" + m_strSearchTerm.toStdString() + "\" "
            "\"" + m_pTableWidget->getSortedColumn() + "\" "
            "\"\" "   // region_code
            "\"" + next_iterator() + "\" "
            + std::to_string(m_i_page_size + 1);
}

void Upload_tab::ShowDigitalContentsGUI()
{
   m_pTableWidget->setRowCount(_digital_contents.size());

   enum {eTitle, eRating, eSize, ePrice, eCreated, eRemaining, eStatus, eIcon};

   if (m_pDetailsSignalMapper)
      delete m_pDetailsSignalMapper;
   m_pDetailsSignalMapper = new QSignalMapper(this);  // similar to extract signal handler
   QObject::connect(m_pDetailsSignalMapper, (void (QSignalMapper::*)(int))&QSignalMapper::mapped,
                    this, &Upload_tab::slot_ShowContentPopup);

   for (size_t iIndex = 0; iIndex < _digital_contents.size(); ++iIndex)
   {
      SDigitalContent const& content = _digital_contents[iIndex];

      std::string synopsis = unescape_string(content.synopsis);
      std::replace(synopsis.begin(), synopsis.end(), '\t', ' '); // JSON does not like tabs :(
      std::replace(synopsis.begin(), synopsis.end(), '\n', ' '); // JSON does not like tabs :(

      graphene::chain::ContentObjectPropertyManager synopsis_parser(synopsis);
      std::string title = synopsis_parser.get<graphene::chain::ContentObjectTitle>();

      // Title
      m_pTableWidget->setItem(iIndex, eTitle, new QTableWidgetItem(QString::fromStdString(title)));

      // Rating
      QString rating = QString::number(content.AVG_rating, 'f', 2);
      m_pTableWidget->setItem(iIndex, eRating, new QTableWidgetItem(rating));

      // Size
      QString unit = " MB";
      double sizeAdjusted = content.size;

      if(content.size > 1024)
      {
         unit = " GB";
         sizeAdjusted = content.size / 1024.0;
      }
      m_pTableWidget->setItem(iIndex, eSize, new QTableWidgetItem(QString::number(sizeAdjusted, 'f', 2) + unit));

      // Price
      m_pTableWidget->setItem(iIndex, ePrice, new QTableWidgetItem(content.price.getString().c_str()));

      // Created
      m_pTableWidget->setItem(iIndex, eCreated, new QTableWidgetItem(QString::fromStdString(content.created)));

      QDateTime time = QDateTime::fromString(QString::fromStdString(content.expiration), "yyyy-MM-ddTHH:mm:ss");

      std::string e_str = CalculateRemainingTime(QDateTime::currentDateTime(), time);

      // Remaining
      m_pTableWidget->setItem(iIndex, eRemaining, new QTableWidgetItem(QString::fromStdString(e_str)));

      // Status
      m_pTableWidget->setItem(iIndex, eStatus, new QTableWidgetItem(QString::fromStdString(content.status)));

      for (size_t iColIndex = eTitle; iColIndex < eIcon; ++iColIndex)
      {
         m_pTableWidget->item(iIndex, iColIndex)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
         m_pTableWidget->item(iIndex, iColIndex)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
      }
      // Icon
      DecentButton* info_icon = new DecentButton(m_pTableWidget, DecentButton::TableIcon, DecentButton::Detail);
      info_icon->setEnabled(false);
      info_icon->setIconSize(QSize(40,40));
      m_pTableWidget->setCellWidget(iIndex, eIcon, info_icon);

      QObject::connect(info_icon, &DecentButton::clicked,
                       m_pDetailsSignalMapper, (void (QSignalMapper::*)())&QSignalMapper::map);
      m_pDetailsSignalMapper->setMapping(info_icon, iIndex);
   }
}

void Upload_tab::slot_UploadPopup()
{
   if (false == Globals::instance().getCurrentUser().empty())
   {
      Upload_popup popup(this);
      popup.exec();
   }
}

void Upload_tab::slot_ShowContentPopup(int iIndex)
{
   if (iIndex < 0 || iIndex >= _digital_contents.size())
      throw std::out_of_range("Content index is out of range");

   ContentDetailsGeneral* pDetailsDialog = new ContentDetailsGeneral(nullptr);
   QObject::connect(pDetailsDialog, &ContentDetailsGeneral::ContentWasBought,
                    this, &Upload_tab::slot_Bought);
   pDetailsDialog->execCDD(_digital_contents[iIndex], true);
   pDetailsDialog->setAttribute(Qt::WA_DeleteOnClose);
   pDetailsDialog->open();
}

void Upload_tab::slot_Bought()
{
   Globals::instance().signal_showPurchasedTab();
   Globals::instance().updateAccountBalance();
}

void Upload_tab::slot_SortingChanged(int index)
{
   reset();
}

void Upload_tab::slot_SearchTermChanged(QString const& strSearchTerm)
{
   m_strSearchTerm = strSearchTerm;
   reset(false);
}

}  // end namespace gui_wallet

