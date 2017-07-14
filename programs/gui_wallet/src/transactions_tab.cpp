/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
#include "stdafx.h"

#include "gui_wallet_global.hpp"
#include "transactions_tab.hpp"
#include "decent_line_edit.hpp"

#ifndef _MSC_VER
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QStringList>
#include <QVBoxLayout>
#include <QLineEdit>

#include <boost/algorithm/string/replace.hpp>
#include <graphene/chain/config.hpp>
#include <graphene/chain/content_object.hpp>
#include <graphene/chain/transaction_detail_object.hpp>
#include <string>

#include "json.hpp"
#endif

using std::string;

namespace gui_wallet
{
TransactionsTab::TransactionsTab(QWidget* pParent,
                                 DecentLineEdit* pFilterLineEdit)
: TabContentManager(pParent)
, m_pTableWidget(new DecentTable(this))
{
   m_pTableWidget->set_columns({
      {tr("Time"), 20, "time"},
      {tr("Type"), 10, "type"},
      {tr("From"), 20, "from"},
      {tr("To")  , 20, "to"},
      {tr("Price"), 10,"price"},
      {tr("Fee"), 10,  "fee"},
      {tr("Description"), 25, "description"}
   });
   
   m_pTableWidget->horizontalHeader()->setStretchLastSection(true);

   QVBoxLayout* pMainLayout = new QVBoxLayout();
   pMainLayout->setContentsMargins(0, 0, 0, 0);
   pMainLayout->setSpacing(0);
   pMainLayout->addWidget(m_pTableWidget);
   setLayout(pMainLayout);

   QObject::connect(&Globals::instance(), &Globals::currentUserChanged,
                    pFilterLineEdit, &QLineEdit::setText);

   QObject::connect(this, &TransactionsTab::signal_setUserFilter,
                    pFilterLineEdit, &QLineEdit::setText);

   QObject::connect(pFilterLineEdit, &QLineEdit::textChanged,
                    this, &TransactionsTab::slot_SearchTermChanged);

   QObject::connect(m_pTableWidget, &DecentTable::signal_SortingChanged,
                    this, &TransactionsTab::slot_SortingChanged);
}

void TransactionsTab::timeToUpdate(const string& result)
{
   m_pTableWidget->setRowCount(0);
   if (result.empty())
      return;

   auto contents = nlohmann::json::parse(result);
   size_t iSize = contents.size();
   if (iSize > m_i_page_size)
      iSize = m_i_page_size;

   m_pTableWidget->setRowCount(iSize);

   for (size_t iIndex = 0; iIndex < iSize; ++iIndex)
   {
      auto const& content = contents[iIndex];
      string from_account = Globals::instance().getAccountName(content["m_from_account"].get<string>());
      string to_account = Globals::instance().getAccountName(content["m_to_account"].get<string>());
      graphene::chain::transaction_detail_object::eOperationType en_operation_type =
         (graphene::chain::transaction_detail_object::eOperationType)content["m_operation_type"].get<uint8_t>();
      string description = content["m_str_description"].get<string>();
      string timestamp = boost::replace_all_copy(content["m_timestamp"].get<string>(), "T", " ");

      Asset transaction_amount_ast = Globals::instance().asset(json_to_int64(content["m_transaction_amount"]["amount"]));
      Asset transaction_fee_ast = Globals::instance().asset(json_to_int64(content["m_transaction_fee"]["amount"]));

      QString str_operation_type;
      switch (en_operation_type)
      {
      case graphene::chain::transaction_detail_object::account_create:
         str_operation_type = tr("Create account");
         break;
      case graphene::chain::transaction_detail_object::content_submit:
         str_operation_type = tr("Content submit");
         break;
      case graphene::chain::transaction_detail_object::content_buy:
         str_operation_type = tr("Buy");
            break;
      case graphene::chain::transaction_detail_object::content_rate:
         str_operation_type = tr("Rate");
         break;
      case graphene::chain::transaction_detail_object::transfer:
         str_operation_type = tr("Transfer");
         break;
      case graphene::chain::transaction_detail_object::subscription:
         str_operation_type = tr("Subscription");
         break;
      }

      std::vector<QString> values =
      {
         QString::fromStdString(timestamp),
         str_operation_type,
         QString::fromStdString(from_account),
         QString::fromStdString(to_account),
         transaction_amount_ast.getString().c_str(),
         transaction_fee_ast.getString().c_str(),
         QString::fromStdString(description)
      };

      for (int col = 0; col < m_pTableWidget->columnCount(); ++col)
      {
         m_pTableWidget->setItem(iIndex, col, new QTableWidgetItem(values[col]));
         m_pTableWidget->item(iIndex, col)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
         m_pTableWidget->item(iIndex, col)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
      }
   }
   
   if (contents.size() > m_i_page_size)
      set_next_page_iterator(contents[m_i_page_size]["id"].get<string>());
   else
      set_next_page_iterator(string());
}

string TransactionsTab::getUpdateCommand()
{
   if (m_strSearchTerm.toStdString().empty())
      return string();

   return   "search_account_history "
            "\"" + m_strSearchTerm.toStdString() + "\" "
            "\"" + m_pTableWidget->getSortedColumn() + "\" "
            "\"" + next_iterator() + "\" "
            + std::to_string(m_i_page_size + 1);
}

void TransactionsTab::slot_SortingChanged(int index)
{
   reset();
}

void TransactionsTab::slot_SearchTermChanged(QString const& strSearchTerm)
{
   m_strSearchTerm = strSearchTerm;
   reset(false);
}
}  // end namespace gui_wallet
