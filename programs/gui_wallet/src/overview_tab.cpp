/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */

#ifndef STDAFX_H
#include <QBoxLayout>
#include <QSignalMapper>
#endif

#include "overview_tab.hpp"
#include "gui_wallet_global.hpp"
#include "decent_line_edit.hpp"
#include "decent_button.hpp"
#include "richdialog.hpp"

namespace gui_wallet
{

Overview_tab::Overview_tab(QWidget* pParent,
                           DecentLineEdit* pFilterLineEdit)
: TabContentManager(pParent)
, m_pAccountSignalMapper(nullptr)
, m_pTableWidget(new DecentTable(this))
{
   m_pTableWidget->set_columns({
      {tr("Account ID"), 20, "id"},
      {tr("Account"), 50, "name"},
      {"", 5},
      {"", 5},
      {"", 5}
   });

   QVBoxLayout* pMainLayout = new QVBoxLayout();
   pMainLayout->setContentsMargins(0, 5, 0, 0);
   pMainLayout->setMargin(0);
   pMainLayout->addWidget(m_pTableWidget);
   pMainLayout->setSpacing(0);
    
   setLayout(pMainLayout);

   if (pFilterLineEdit) {
      QObject::connect(pFilterLineEdit, &QLineEdit::textChanged,
                       this, &Overview_tab::slot_SearchTermChanged);
      setFilterWidget(pFilterLineEdit);
   }

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
      pTransactionButton->setToolTip(tr("Transactions"));
      pTransactionButton->setEnabled(false);
      
      QObject::connect(pTransactionButton, &DecentButton::clicked,
                       m_pAccountSignalMapper, (void (QSignalMapper::*)())&QSignalMapper::map);
      QObject::connect(pTransactionButton, &DecentButton::clicked,
                       this, &Overview_tab::slot_Transactions);
      
      m_pAccountSignalMapper->setMapping(pTransactionButton, QString::fromStdString(name));
      m_pTableWidget->setCellWidget(iIndex, 2, pTransactionButton);

      // Details Button
      //
      DecentButton* pDetailsButton = new DecentButton(m_pTableWidget, DecentButton::TableIcon, DecentButton::Detail);
      pDetailsButton->setToolTip(tr("Details"));
      pDetailsButton->setEnabled(false);
      m_pTableWidget->setCellWidget(iIndex, 4, pDetailsButton);

      m_pAccountSignalMapper->setMapping(pDetailsButton, QString::fromStdString(name));
      QObject::connect(pDetailsButton, &DecentButton::clicked,
                       m_pAccountSignalMapper, (void (QSignalMapper::*)())&QSignalMapper::map);
      QObject::connect(pDetailsButton, &DecentButton::clicked,
                       this, &Overview_tab::slot_Details);

      // Transfer Button
      //
      DecentButton* pTransferButton = new DecentButton(m_pTableWidget, DecentButton::TableIcon, DecentButton::Transfer);
      pTransferButton->setToolTip(tr("Transfer"));
      pTransferButton->setEnabled(false);
      m_pTableWidget->setCellWidget(iIndex, 3, pTransferButton);
            

      m_pAccountSignalMapper->setMapping(pTransferButton, QString::fromStdString(name));
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
      set_next_page_iterator(std::string());
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
      bool is_publishing_manager = accountInfo["rights_to_publish"]["is_publishing_manager"].get<bool>();
      std::string name = accountInfo["name"].get<std::string>();
      int size = accountInfo["rights_to_publish"]["publishing_rights_received"].size();
      bool is_publishing_rights_received = size;

      UserInfoWidget *userInfoWidget = new UserInfoWidget(nullptr,
                                                          is_publishing_manager,
                                                          is_publishing_rights_received,
                                                          QString::fromStdString(registrar),
                                                          QString::fromStdString(name),
                                                          QString::fromStdString(id));

      Globals::instance().signal_stackWidgetPush(userInfoWidget);
   }
   catch(const std::exception& ex) {
      GUI_ELOG("Overview_tab::slot_Details: ${e}", ("e", ex.what()));
   }
   catch(const fc::exception& ex) {
      GUI_ELOG("Overview_tab::slot_Details: ${e}", ("e", ex.what()));
   }
}

void Overview_tab::slot_Transfer()
{
   Globals::instance().slot_showTransferDialog(m_strSelectedAccount);
}

void Overview_tab::slot_SearchTermChanged(const QString& strSearchTerm)
{
   m_strSearchTerm = strSearchTerm;
   reset(true);
}

void Overview_tab::slot_AccountChanged(const QString& strAccountName)
{
   m_strSelectedAccount = strAccountName;
}

void Overview_tab::slot_SortingChanged(int index)
{
   reset();
}

}//   end namespace gui_wallet
