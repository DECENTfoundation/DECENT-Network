/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
#include "stdafx.h"

#include "gui_wallet_global.hpp"
#include "upload_tab.hpp"
#include "upload_popup.hpp"
#include "decent_button.hpp"
#include "decent_line_edit.hpp"
#include "richdialog.hpp"

#include <boost/algorithm/string/replace.hpp>

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

#include <graphene/chain/content_object.hpp>

#include "json.hpp"
#endif

using std::string;

namespace gui_wallet
{

Upload_tab::Upload_tab(QWidget* pParent,
                       DecentLineEdit* pFilterLineEdit,
                       DecentButton* pUploadButton)
: TabContentManager(pParent)
, m_pTableWidget(new DecentTable(this))
, m_pDetailsSignalMapper(nullptr)
, m_pResubmitSignalMapper(nullptr)
{
   m_pTableWidget->set_columns({
      {tr("Title"), 20},
      {tr("Rating"), 10, "rating"},
      {tr("Size"), 10, "size"},
      {tr("Price"), 10, "price"},
      {tr("Published"), 10, "created"},
      {tr("Expiration"), 10, "expiration"},
      {tr("Status"), 10},
      {" ", 4},
      {" ", 4}
   });

   QVBoxLayout* pMainLayout = new QVBoxLayout;
   pMainLayout->setContentsMargins(0, 0, 0, 0);
   pMainLayout->setSpacing(0);
   pMainLayout->addWidget(m_pTableWidget);
   setLayout(pMainLayout);

   QObject::connect(m_pTableWidget, &DecentTable::signal_SortingChanged,
                    this, &Upload_tab::slot_SortingChanged);

   QObject::connect(pFilterLineEdit, &QLineEdit::textChanged,
                    this, &Upload_tab::slot_SearchTermChanged);

   QObject::connect(pUploadButton, &QPushButton::clicked,
                    this, &Upload_tab::slot_UploadPopup);

}

// when class has forward declared members
// this becomes necessary
Upload_tab::~Upload_tab() = default;

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
      std::string iSymbolId = json_content["price"]["asset_id"];
      content.price = Globals::instance().asset(iPrice, iSymbolId);
      content.synopsis = json_content["synopsis"].get<string>();
      content.URI = json_content["URI"].get<string>();
      content.created = json_content["created"].get<string>();
      content.expiration = json_content["expiration"].get<string>();
      content.size = json_content["size"].get<int>();
      content.status = json_content["status"].get<string>();
      content.id = json_content["id"].get<string>();

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

   graphene::chain::ContentObjectPropertyManager type_composer;
   graphene::chain::ContentObjectTypeValue type(graphene::chain::EContentObjectApplication::DecentCore);
   string str_type;
   type.to_string(str_type);

   return   "search_user_content "
            "\"" + currentUserName + "\" "
            "\"" + m_strSearchTerm.toStdString() + "\" "
            "\"" + m_pTableWidget->getSortedColumn() + "\" "
            "\"\" "   // region_code
            "\"" + next_iterator() + "\" "
            "\"" + str_type + "\" "
            + std::to_string(m_i_page_size + 1);
}

void Upload_tab::ShowDigitalContentsGUI()
{
   m_pTableWidget->setRowCount(_digital_contents.size());

   enum {eTitle, eRating, eSize, ePrice, eCreated, eRemaining, eStatus, eIcon, eResubmit};

   if (m_pDetailsSignalMapper)
      delete m_pDetailsSignalMapper;
   m_pDetailsSignalMapper = new QSignalMapper(this);
   QObject::connect(m_pDetailsSignalMapper, (void (QSignalMapper::*)(int))&QSignalMapper::mapped,
                    this, &Upload_tab::slot_ShowContentPopup);

   if (m_pResubmitSignalMapper)
      delete m_pResubmitSignalMapper;
   m_pResubmitSignalMapper = new QSignalMapper(this);
   QObject::connect(m_pResubmitSignalMapper, (void (QSignalMapper::*)(int))&QSignalMapper::mapped,
                    this, &Upload_tab::slot_UploadPopupResubmit);

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
      m_pTableWidget->setItem(iIndex, ePrice, new QTableWidgetItem(content.price.getString()));

      // Created
      std::string created_date;
      if (content.created != "1970-01-01") {
         created_date = content.created.substr(0, content.created.find("T"));
      }

      m_pTableWidget->setItem(iIndex, eCreated, new QTableWidgetItem(convertDateToLocale(created_date)));

      QDateTime time = convertStringToDateTime(content.expiration);
      QString e_str = CalculateRemainingTime(QDateTime::currentDateTime(), time);

      // Remaining
      m_pTableWidget->setItem(iIndex, eRemaining, new QTableWidgetItem(e_str));

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
      info_icon->setToolTip(tr("Details"));
      m_pTableWidget->setCellWidget(iIndex, eIcon, info_icon);
      
      // Resubmit
      DecentButton* resubmit_button = new DecentButton(m_pTableWidget, DecentButton::TableIcon, DecentButton::Resubmit);
      resubmit_button->setEnabled(false);
      resubmit_button->setToolTip(tr("Publish"));
//      resubmit_button->setText("res");
      m_pTableWidget->setCellWidget(iIndex, 8, resubmit_button);

      QObject::connect(info_icon, &DecentButton::clicked,
                       m_pDetailsSignalMapper, (void (QSignalMapper::*)())&QSignalMapper::map);
      m_pDetailsSignalMapper->setMapping(info_icon, iIndex);
      
      QObject::connect(resubmit_button, &DecentButton::clicked,
                       m_pResubmitSignalMapper, (void (QSignalMapper::*)())&QSignalMapper::map);
      m_pResubmitSignalMapper->setMapping(resubmit_button, iIndex);
   }
}

void Upload_tab::slot_UploadPopupResubmit(int iIndex)
{
   if (iIndex < 0 || iIndex >= _digital_contents.size())
      throw std::out_of_range("Content index is out of range");

   if (!Globals::instance().getCurrentUser().empty())
   {
      if (_digital_contents[iIndex].status == "Uploading")
         return;

      Upload_popup* pUploadWidget = new Upload_popup(nullptr, _digital_contents[iIndex].id);
      Globals::instance().signal_stackWidgetPush(pUploadWidget);
   }
}

void Upload_tab::slot_UploadPopup()
{
   if (!Globals::instance().getCurrentUser().empty())
   {
      Upload_popup* pUploadWidget = new Upload_popup(nullptr);
      Globals::instance().signal_stackWidgetPush(pUploadWidget);
   }
}
   
void Upload_tab::slot_ShowContentPopup(int iIndex)
{
   if (iIndex < 0 || iIndex >= _digital_contents.size())
      throw std::out_of_range("Content index is out of range");

   ContentInfoWidget* pDetailsDialog = new ContentInfoWidget(nullptr, _digital_contents[iIndex]);
   Globals::instance().signal_stackWidgetPush(pDetailsDialog);

   QObject::connect(pDetailsDialog, &ContentInfoWidget::accepted,
                    this, &Upload_tab::slot_Bought);
}
   
void Upload_tab::slot_Bought()
{
   Globals::instance().signal_showPurchasedTab();
   Globals::instance().slot_updateAccountBalance();
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

