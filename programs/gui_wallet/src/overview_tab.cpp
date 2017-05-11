#include "stdafx.h"

#include "gui_wallet_global.hpp"
#include "overview_tab.hpp"
#include "decent_line_edit.hpp"
#include "decent_button.hpp"
#include "richdialog.hpp"

#ifndef _MSC_VER
#include <QPixmap>
#include <QRect>
#include <QSignalMapper>
#include <QLabel>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <graphene/chain/config.hpp>
#include "json.hpp"
#endif

#include "gui_design.hpp"

// these were included in hpp, let's have these around when needed
//#include <QtSvg/QSvgRenderer>
//#include <QPainter>
//#include <QSvgWidget>

using string = std::string;

namespace gui_wallet
{

Overview_tab::Overview_tab(QWidget* pParent)
: TabContentManager(pParent)
, m_pAccountSignalMapper(nullptr)
, m_pTableWidget(new DecentTable(this))
{
   m_pTableWidget->set_columns({
      {tr("Account ID"), 20, "id"},
      {tr("Account"), 50, "name"},
      {"", 10},
      {"", 10},
      {"", 10}
   });

   DecentLineEdit* pfilterLineEditor = new DecentLineEdit(this, DecentLineEdit::TableSearch);
   pfilterLineEditor->setPlaceholderText(QString(tr("Search")));
   pfilterLineEditor->setAttribute(Qt::WA_MacShowFocusRect, 0);
   pfilterLineEditor->setFixedHeight(54);

   QPixmap image(icon_search);

   QLabel* pSearchLabel = new QLabel(this);
   pSearchLabel->setSizeIncrement(100,40);
   pSearchLabel->setPixmap(image);

   QHBoxLayout* pSearchLayout = new QHBoxLayout();
   pSearchLayout->setMargin(0);
   pSearchLayout->setContentsMargins(0,0,0,0);
   pSearchLayout->setContentsMargins(42, 0, 0, 0);
   pSearchLayout->addWidget(pSearchLabel);
   pSearchLayout->addWidget(pfilterLineEditor);

   QVBoxLayout* pMainLayout = new QVBoxLayout();
   pMainLayout->setContentsMargins(0, 5, 0, 0);
   pMainLayout->setMargin(0);
   pMainLayout->addLayout(pSearchLayout);
   pMainLayout->addWidget(m_pTableWidget);
   pMainLayout->setSpacing(0);
    
   setLayout(pMainLayout);

   QObject::connect(pfilterLineEditor, &QLineEdit::textChanged,
                    this, &Overview_tab::slot_SearchTermChanged);

   QObject::connect(m_pTableWidget, &DecentTable::signal_SortingChanged,
                    this, &Overview_tab::slot_SortingChanged);
}

void Overview_tab::timeToUpdate(const std::string& result) {
   if (result.empty()) {
      m_pTableWidget->setRowCount(0);
      return;
   }
   
   auto contents = nlohmann::json::parse(result);

   size_t iSize = contents.size();
   if (iSize > m_i_page_size)
      iSize = m_i_page_size;
   
   m_pTableWidget->setRowCount(iSize);

   if (m_pAccountSignalMapper)
      delete m_pAccountSignalMapper;
   m_pAccountSignalMapper = new QSignalMapper(this);  // the last one will be deleted thanks to it's parent
   QObject::connect(m_pAccountSignalMapper, (void (QSignalMapper::*)(QString const&))&QSignalMapper::mapped,
                    this, &Overview_tab::slot_AccountChanged);
   
   for (size_t iIndex = 0; iIndex < iSize; ++iIndex)
   {
      auto const& content = contents[iIndex];
      
      std::string name = content["name"].get<std::string>();
      std::string id = content["id"].get<std::string>();
      
      m_pTableWidget->setItem(iIndex, 1, new QTableWidgetItem(QString::fromStdString(name)));
      m_pTableWidget->setItem(iIndex, 0, new QTableWidgetItem(QString::fromStdString(id)));

      // Transaction Button
      //
      DecentButton* pTransactionButton = new DecentButton(m_pTableWidget, DecentButton::TableIcon, DecentButton::Transaction);
      pTransactionButton->setIconSize(QSize(40,40));
      pTransactionButton->setEnabled(false);
      
      QObject::connect(pTransactionButton, &DecentButton::clicked,
                       m_pAccountSignalMapper, (void (QSignalMapper::*)())&QSignalMapper::map);
      QObject::connect(pTransactionButton, &DecentButton::clicked,
                       this, &Overview_tab::slot_Transactions);
      
      m_pAccountSignalMapper->setMapping(pTransactionButton, name.c_str());
      m_pTableWidget->setCellWidget(iIndex, 2, pTransactionButton);

      // Details Button
      //
      DecentButton* pDetailsButton = new DecentButton(m_pTableWidget, DecentButton::TableIcon, DecentButton::Detail);
      pDetailsButton->setIconSize(QSize(40,40));
      pDetailsButton->setEnabled(false);
      m_pTableWidget->setCellWidget(iIndex, 4, pDetailsButton);

      m_pAccountSignalMapper->setMapping(pDetailsButton, name.c_str());
      QObject::connect(pDetailsButton, &DecentButton::clicked,
                       m_pAccountSignalMapper, (void (QSignalMapper::*)())&QSignalMapper::map);
      QObject::connect(pDetailsButton, &DecentButton::clicked,
                       this, &Overview_tab::slot_Details);

      // Transfer Button
      //
      DecentButton* pTransferButton = new DecentButton(m_pTableWidget, DecentButton::TableIcon, DecentButton::Transfer);
      pTransferButton->setIconSize(QSize(40,40));
      pTransferButton->setEnabled(false);
      m_pTableWidget->setCellWidget(iIndex, 3, pTransferButton);
            

      m_pAccountSignalMapper->setMapping(pTransferButton, name.c_str());
      QObject::connect(pTransferButton, &DecentButton::clicked,
                       m_pAccountSignalMapper, (void (QSignalMapper::*)())&QSignalMapper::map);
      QObject::connect(pTransferButton, &DecentButton::clicked,
                       this, &Overview_tab::slot_Transfer);

      m_pTableWidget->item(iIndex,0)->setBackground(Qt::white);
      m_pTableWidget->item(iIndex,1)->setBackground(Qt::white);
      
      m_pTableWidget->item(iIndex,0)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
      m_pTableWidget->item(iIndex,1)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
      
      m_pTableWidget->item(iIndex,0)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
      m_pTableWidget->item(iIndex,1)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
      
//      m_pTableWidget->item(iIndex,0)->setForeground(QColor::fromRgb(88,88,88));
//      m_pTableWidget->item(iIndex,1)->setForeground(QColor::fromRgb(88,88,88));
   }

   if (contents.size() > m_i_page_size)
      set_next_page_iterator(contents[m_i_page_size]["id"].get<std::string>());
   else
      set_next_page_iterator(string());
}

std::string Overview_tab::getUpdateCommand() {
   return   "search_accounts "
            "\"" + m_strSearchTerm.toStdString() + "\" "
            "\"" + m_pTableWidget->getSortedColumn() + "\" "
            "\"" + next_iterator() + "\" "
            + std::to_string(m_i_page_size + 1);
}

void Overview_tab::slot_Transactions()
{
   Globals::instance().signal_showTransactionsTab(m_strSelectedAccount.toStdString());
}

void Overview_tab::slot_Details()
{
   try {
      auto accountInfo = Globals::instance().runTaskParse("get_account " + m_strSelectedAccount.toStdString());
      
      std::string id = accountInfo["id"].get<std::string>();
      std::string registrar = accountInfo["registrar"].get<std::string>();
      std::string referrer = accountInfo["referrer"].get<std::string>();
      std::string lifetime_referrer = accountInfo["lifetime_referrer"].get<std::string>();
      std::string network_fee_percentage = std::to_string(accountInfo["network_fee_percentage"].get<int>());
      std::string lifetime_referrer_fee_percentage = std::to_string(accountInfo["lifetime_referrer_fee_percentage"].get<int>());
      std::string referrer_rewards_percentage = std::to_string(accountInfo["referrer_rewards_percentage"].get<int>());
      std::string name = accountInfo["name"].get<std::string>();
      
      UserInfoDialog* dialog = new UserInfoDialog(this,
                                                  QString::fromStdString(registrar),
                                                  QString::fromStdString(referrer),
                                                  QString::fromStdString(lifetime_referrer),
                                                  QString::fromStdString(network_fee_percentage),
                                                  QString::fromStdString(lifetime_referrer_fee_percentage),
                                                  QString::fromStdString(referrer_rewards_percentage),
                                                  QString::fromStdString(name),
                                                  QString::fromStdString(id));
      dialog->setAttribute(Qt::WA_DeleteOnClose);
      dialog->open();
   } catch(...) {
      // Ignore for now
   }
}

void Overview_tab::slot_Transfer()
{
   Globals::instance().showTransferDialog(m_strSelectedAccount.toStdString());
}

void Overview_tab::slot_SearchTermChanged(QString const& strSearchTerm)
{
   m_strSearchTerm = strSearchTerm;
   reset(false);
}
void Overview_tab::slot_AccountChanged(QString const& strAccountName)
{
   m_strSelectedAccount = strAccountName;
}

void Overview_tab::slot_SortingChanged(int index)
{
   reset();
}
}//   end namespace gui_wallet
