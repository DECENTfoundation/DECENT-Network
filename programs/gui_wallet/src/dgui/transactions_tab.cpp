#include "stdafx.h"

#include "gui_wallet_global.hpp"
#include "transactions_tab.hpp"

#include "gui_design.hpp"

#ifndef _MSC_VER
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QStringList>
#include <QVBoxLayout>
#include <QLineEdit>

#include <boost/algorithm/string/replace.hpp>
#include <graphene/chain/config.hpp>
#include <graphene/chain/content_object.hpp>
#include <string>

#include "json.hpp"
#endif

using string = std::string;

namespace gui_wallet
{
TransactionsTab::TransactionsTab(QWidget* pParent)
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

   QLineEdit* pfilterLineEditor = new QLineEdit(this);
   pfilterLineEditor->setStyleSheet(d_lineEdit);
   pfilterLineEditor->setPlaceholderText(tr("Enter user name to see transaction history"));
   pfilterLineEditor->setAttribute(Qt::WA_MacShowFocusRect, 0);
   pfilterLineEditor->setFixedHeight(54);
   pfilterLineEditor->setFrame(false);

   QPixmap image(icon_search);
   QLabel* pSearchLabel = new QLabel(this);
   pSearchLabel->setSizeIncrement(100,40);
   pSearchLabel->setPixmap(image);

   QHBoxLayout* search_layout = new QHBoxLayout();
   search_layout->setContentsMargins(42, 0, 0, 0);
   search_layout->addWidget(pSearchLabel);
   search_layout->addWidget(pfilterLineEditor);
   
   m_pTableWidget->horizontalHeader()->setStretchLastSection(true);

   QVBoxLayout* pMainLayout = new QVBoxLayout();
   pMainLayout->setContentsMargins(0, 0, 0, 0);
   pMainLayout->setSpacing(0);
   pMainLayout->addLayout(search_layout);
   pMainLayout->addWidget(m_pTableWidget);
   setLayout(pMainLayout);

   QObject::connect(&Globals::instance(), &Globals::currentUserChanged,
                    pfilterLineEditor, &QLineEdit::setText);

   QObject::connect(this, &TransactionsTab::signal_setUserFilter,
                    pfilterLineEditor, &QLineEdit::setText);

   QObject::connect(pfilterLineEditor, &QLineEdit::textChanged,
                    this, &TransactionsTab::slot_SearchTermChanged);

   QObject::connect(m_pTableWidget, &DecentTable::signal_SortingChanged,
                    this, &TransactionsTab::slot_SortingChanged);
}

void TransactionsTab::timeToUpdate(const std::string& result) {
   if (result.empty()) {
      m_pTableWidget->setRowCount(0);
      return;
   }
   
   auto contents = nlohmann::json::parse(result);
   size_t iSize = contents.size();
   if (iSize > m_i_page_size)
      iSize = m_i_page_size;

   m_pTableWidget->setRowCount(iSize);
   
   for (size_t iIndex = 0; iIndex < iSize; ++iIndex)
   {
      auto const& content = contents[iIndex];
      std::string from_account = Globals::instance().getAccountName(content["from_account"].get<std::string>());
      std::string to_account = Globals::instance().getAccountName(content["to_account"].get<std::string>());
      std::string operation_type = content["operation_type"].get<std::string>();
      std::string description = content["description"].get<std::string>();
      std::string timestamp = boost::replace_all_copy(content["timestamp"].get<std::string>(), "T", " ");

      if (operation_type == "Buy" || operation_type == "Content submit")
      {
         auto contentObject = Globals::instance().runTaskParse("get_content \"" + description + "\"");
         from_account = Globals::instance().getAccountName(contentObject["author"].get<std::string>());
         std::string synopsis = contentObject["synopsis"].get<std::string>();
         synopsis = unescape_string(synopsis);
         std::replace(synopsis.begin(), synopsis.end(), '\t', ' '); // JSON does not like tabs
         std::replace(synopsis.begin(), synopsis.end(), '\n', ' '); // JSON does not like newlines either
         graphene::chain::ContentObjectPropertyManager synopsis_parser(synopsis);
         std::string title = synopsis_parser.get<graphene::chain::ContentObjectTitle>();
         description = title;
      }

      Asset transaction_amount_ast = Globals::instance().asset(json_to_int64(content["transaction_amount"]["amount"]));
      Asset transaction_fee_ast = Globals::instance().asset(json_to_int64(content["transaction_fee"]["amount"]));
      
      std::vector<QString> values =
      {
         QString::fromStdString(timestamp),
         QString::fromStdString(operation_type),
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
      set_next_page_iterator(contents[m_i_page_size]["id"].get<std::string>());
   else
      set_next_page_iterator(string());
}

string TransactionsTab::getUpdateCommand()
{
   if (m_strSearchTerm.toStdString().empty())
      return string();

   return   "get_account_history "
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
}
}  // end namespace gui_wallet
