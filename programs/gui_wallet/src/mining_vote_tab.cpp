/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */

#ifndef STDAFX_H
#include <QBoxLayout>
#include <QCheckBox>
#include <QDesktopServices>
#include <QMessageBox>
#include <QUrl>
#endif

#include "mining_vote_tab.hpp"
#include "gui_wallet_global.hpp"
#include "decent_button.hpp"
#include "decent_line_edit.hpp"

namespace gui_wallet {

const char* g_vote_state_id = "vote-state";


MinerVotingTab::MinerVotingTab(QWidget *pParent, DecentLineEdit *pFilterLineEdit, QCheckBox* pOnlyMyVotes)
      : TabContentManager(pParent)
      , m_pTableWidget(new DecentTable(this))
      , m_onlyMyVotes(false)
      , m_minersVotedNum(0)
      , m_curMinersVotedFor(0)
{
   m_pTableWidget->set_columns({
                                   {tr("Miner"),             15, "name"},
                                   {tr("Link to proposal"),  30, "link"},
                                   {tr("Votes"),            -150, "votes"},
                                   {" ",                      5},
                             });


   QVBoxLayout* pMainLayout = new QVBoxLayout();
   pMainLayout->setContentsMargins(0, 5, 0, 0);
   pMainLayout->setMargin(0);
   pMainLayout->addWidget(m_pTableWidget);
   pMainLayout->setSpacing(0);

   setLayout(pMainLayout);

   if (pFilterLineEdit) {
      QObject::connect(pFilterLineEdit, &QLineEdit::textChanged,
                       this, &MinerVotingTab::slot_SearchTermChanged);
      this->setFilterWidget(pFilterLineEdit);
   }

   if (pOnlyMyVotes) {
      QObject::connect(pOnlyMyVotes, &QCheckBox::stateChanged, this, &MinerVotingTab::slot_onlyMyVotes);

   }

   QObject::connect(m_pTableWidget, &DecentTable::signal_SortingChanged,
                    this, &MinerVotingTab::slot_SortingChanged);

   QObject::connect(m_pTableWidget, &DecentTable::cellClicked, this, &MinerVotingTab::slot_cellClicked);

   setRefreshTimer(5000);

}

MinerVotingTab::~MinerVotingTab() = default;

QString MinerVotingTab::getVotesText(uint64_t total_votes)
{
   double votes = total_votes;

   int iCount = 0;
   while(votes >= 1000.0) {
      votes /= 1000.0;
      iCount++;

      if (votes < 500000.0)
         break;
   }

   QString result = QString::number(votes);
   result += ' ';

   switch(iCount)
   {
      case 1:
         result += "kV";  //Kilo Votes
         break;
      case 2:
         result += "MV";  //Mega Votes
         break;
      case 3:
         result += "GV";  //Giga Votes
         break;
      case 4:
         result += "TV";  //Tera Votes
         break;
      case 5:
         result += "PV";  //Penta Votes
         break;
      case 6:
         result += "EV";  //Exa Votes
         break;
   }

   return result;
}

void MinerVotingTab::timeToUpdate(const std::string& result)
{
   if (result.empty()) {
      while(m_pTableWidget->rowCount() > 0) {
         m_pTableWidget->removeRow(0);
      }
      return;
   }

   auto contents = nlohmann::json::parse(result);
   size_t iSize = contents.size();
   if (iSize > m_i_page_size)
      iSize = m_i_page_size;

   m_buttonsToIndex.clear();
   m_indexToUrl.clear();

   //clear table
//   while(m_pTableWidget->rowCount() > 0) {
//      m_pTableWidget->removeRow(0);
//   }

   try {

      m_pTableWidget->setRowCount(iSize);

      Asset main_asset = Globals::instance().asset(0);
      QUrl test_url;

      for (size_t iIndex = 0; iIndex < iSize; ++iIndex) {
         auto const &content = contents[iIndex];

         std::string name = content["name"].get<std::string>();
         std::string url = content["url"].get<std::string>();
         uint64_t total_votes;
         if (content["total_votes"].is_string() ) {
            total_votes = std::stoull(content["total_votes"].get<std::string>());
         }
         else {
            total_votes = content["total_votes"].get<uint64_t>();
         }
         bool voted = content["voted"].get<bool>();

         QTableWidgetItem *tabItem;
         tabItem = new QTableWidgetItem(QString::fromStdString(name));
         tabItem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
         tabItem->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
         m_pTableWidget->setItem(iIndex, 0, tabItem);

         tabItem = new QTableWidgetItem(QString::fromStdString(url));
         tabItem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
         tabItem->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);

         if (!url.empty()) {
            test_url.setUrl(QString::fromStdString(url));
            if (test_url.isValid()) {
               m_indexToUrl.insert(iIndex, QString::fromStdString(url));
            }
         }
         m_pTableWidget->setItem(iIndex, 1, tabItem);

         QString asset_text = QString::number((uint64_t) total_votes / main_asset.m_scale );
         asset_text += ' ';
         asset_text += QString::fromStdString(main_asset.m_str_symbol);

         tabItem = new QTableWidgetItem(asset_text);
         tabItem->setToolTip(QString::number(total_votes));
         tabItem->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
         tabItem->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
         m_pTableWidget->setItem(iIndex, 2, tabItem);

         // Vote Button
         //
         DecentButton* pVoteButton = new DecentButton(m_pTableWidget);
         pVoteButton->setEnabled(false);
         if (!voted) {
            pVoteButton->setText(tr("Vote"));
            pVoteButton->setStyleSheet("");
         }
         else {
            pVoteButton->setText(tr("Un-vote"));
            pVoteButton->setStyleSheet(" QPushButton:disabled { background-color: #E6A900 } ");
         }
         pVoteButton->setProperty(g_vote_state_id, QVariant(voted));


         m_pTableWidget->setCellWidget(iIndex, 3, pVoteButton);
         m_buttonsToIndex.insert(pVoteButton, iIndex);

         QObject::connect(pVoteButton, &DecentButton::clicked, this, &MinerVotingTab::slot_MinerVote);

      }
   }
   catch(const std::exception& ex) {
      //TODO.. handle error
   }

   if (contents.size() > m_i_page_size)
      set_next_page_iterator(contents[m_i_page_size]["id"].get<std::string>());
   else
      set_next_page_iterator(std::string());

}

std::string MinerVotingTab::getUpdateCommand()
{
   std::string currentUserName = Globals::instance().getCurrentUser();
   if (currentUserName.empty())
      return std::string();

   graphene::chain::ContentObjectPropertyManager type_composer;
   graphene::chain::ContentObjectTypeValue type(graphene::chain::EContentObjectApplication::DecentCore);
   std::string str_type;
   type.to_string(str_type);

   return "search_miner_voting "
                  "\"" + currentUserName + "\" "
                  "\"" + m_strSearchTerm.toStdString() + "\" "
                  + (m_onlyMyVotes ? "true " : "false ") +
                  "\"" + m_pTableWidget->getSortedColumn() + "\" "
                  "\"" + next_iterator() + "\" "
                  + std::to_string(m_i_page_size + 1);
}

void MinerVotingTab::slot_SearchTermChanged(const QString& strSearchTerm)
{
   m_strSearchTerm = strSearchTerm;
   reset(false);
}

void MinerVotingTab::slot_SortingChanged(int index)
{
   reset();
}

void MinerVotingTab::slot_cellClicked(int row, int col)
{
   if (col != 1)
      return;

   auto find = m_indexToUrl.find(row);
   if (find != m_indexToUrl.end()) {
      QDesktopServices::openUrl(QUrl(find.value()));
   }
}

void MinerVotingTab::slot_onlyMyVotes(int state)
{
   m_onlyMyVotes = (state == Qt::Checked);
   reset(true);
}

void MinerVotingTab::slot_MinerVote()
{
   Q_ASSERT(sender() != nullptr);
   auto find = m_buttonsToIndex.find(qobject_cast<QWidget*>(sender()));
   if (find == m_buttonsToIndex.end()) {
      return;
   }

   bool voteFlag = sender()->property(g_vote_state_id).toBool();
   Asset opFee = Globals::instance().getDCoreFees(2);  //account_update_operation

   QMessageBox info(this);
   info.setIcon(QMessageBox::Information);
   info.setText(QString(tr("For every vote or unvote opeation you will pay %1 fee")).arg(opFee.getString()) );
   info.addButton( !voteFlag ? tr("Vote") : tr("Un-vote"),  QMessageBox::AcceptRole);
   info.addButton( tr("Cancel"), QMessageBox::RejectRole);
   int result = info.exec();
   if (result != QMessageBox::AcceptRole) {
      return;
   }

   int iIndex = find.value();

   QTableWidgetItem* item = m_pTableWidget->item(iIndex, 0);
   std::string miner_name = item->text().toStdString();

   if (voteFlag) {
      getDesiredMinersCount();

      if (m_minersVotedNum != 0 && (m_curMinersVotedFor-1) < m_minersVotedNum) {
         ShowMessageBox(tr("Error"),
                        QString(tr("Your vote for number of miners (%1) should be equal or smaller to number of votes you have given (%2)\n"
                                   "\n"
                                   "In order to un-vote, please change your vote for number of miners"))
                              .arg(m_minersVotedNum)
                              .arg(m_curMinersVotedFor));
         return;
      }
   }

   submit_vote(miner_name, !voteFlag);

   reset(true);
}

void MinerVotingTab::submit_vote(const std::string& miner_name, bool voteFlag)
{
   QString error;

   try {
      std::string submitCommand;
      submitCommand = "vote_for_miner ";
      submitCommand += "\"" + Globals::instance().getCurrentUser() + "\" ";
      submitCommand += "\"" + miner_name + "\" ";
      submitCommand += voteFlag ? "true " : "false ";
      submitCommand += "true";

      Globals::instance().runTask(submitCommand);
   }
   catch(const std::exception& ex) {
      error = QString::fromStdString( ex.what() );
   }
   catch(const fc::exception& ex) {
      error = QString::fromStdString( ex.what() );
   }

   if (!error.isEmpty()) {
      ShowMessageBox(tr("Error"), tr("Failed to submit voting"), error);
   }
}

void MinerVotingTab::getDesiredMinersCount()
{
   std::string cmd = "get_account ";
   cmd += Globals::instance().getCurrentUser();

   nlohmann::json account_obj = Globals::instance().runTaskParse(cmd);

   std::cout << account_obj["options"] << std::endl;

   m_minersVotedNum = account_obj["options"]["num_miner"].get<uint>();
   m_curMinersVotedFor = account_obj["options"]["votes"].size();
}

} //namespace
