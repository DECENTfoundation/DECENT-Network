/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */

#ifndef STDAFX_H
#include <QBoxLayout>
#include <QLabel>
#endif

#include "mining_vote_popup.hpp"
#include "gui_wallet_global.hpp"
#include "decent_button.hpp"
#include "decent_line_edit.hpp"

namespace gui_wallet {

MiningVotePopup::MiningVotePopup(QWidget *pParent) : StackLayerWidget(pParent)
, m_minersVotedNum(0)
, m_curMinersVotedFor(0)
{
   const std::string& account_name = Globals::instance().getCurrentUser();

   uint numOfActiveMiners = getNumberOfActualMiners();
   getMinerVotesForAccount(account_name);

   QLabel* pMinersVoteNumLabel = new QLabel(this);
   pMinersVoteNumLabel->setText(tr("Propose number of miners in mining pool"));

   QIntValidator* numValidator = new QIntValidator(1, 1001, this);   //TODO: make max value read from global_properties

   m_pMinersNumVote = new DecentLineEdit(this, DecentLineEdit::DialogLineEdit);
   m_pMinersNumVote->setValidator(numValidator);
   m_pMinersNumVote->setAttribute(Qt::WA_MacShowFocusRect, 0);
   m_pMinersNumVote->setTextMargins(5, 5, 5, 5);
   if (m_minersVotedNum > 0) {
      m_pMinersNumVote->setText(QString::number(m_minersVotedNum));
   }
   else {
      m_pMinersNumVote->setPlaceholderText(QString(tr("Current miner pool size is %1")).arg(numOfActiveMiners) );
   }

   Asset opFee = Globals::instance().getDCoreFees(2);

   // Info
   QLabel* pInfoLabel = new QLabel(this);
   pInfoLabel->setText(QString(tr("You cannot propose a pool size larger than the number of miners you have currently voted for (%1)\n"
                                  "This operation will cost %2"))
                                  .arg(m_curMinersVotedFor)
                                  .arg(opFee.getString()));

//   QLabel* pFeeInfoLabel = new QLabel(this);
//   pFeeInfoLabel->setText(QString(tr("You will pay %1 fee for voting."))
//                                      .arg() );

   m_pVoteButton = new DecentButton(this, DecentButton::DialogAction);
   m_pResetButton = new DecentButton(this, DecentButton::DialogAction);
   DecentButton* pCancelButton = new DecentButton(this, DecentButton::DialogCancel);

   m_pVoteButton->setText(tr("Vote"));
   m_pVoteButton->setFont(PopupButtonBigFont());
   m_pVoteButton->setEnabled(false);

   m_pResetButton->setText(tr("Un-vote"));
   m_pResetButton->setFont(PopupButtonBigFont());
   m_pResetButton->setEnabled(m_minersVotedNum > 0);

   pCancelButton->setText(tr("Back"));
   pCancelButton->setFont(PopupButtonBigFont());

   QHBoxLayout* pRow1Layout = new QHBoxLayout;
   pRow1Layout->addWidget(pMinersVoteNumLabel);
   pRow1Layout->addWidget(m_pMinersNumVote);

   QHBoxLayout* pRow2Layout = new QHBoxLayout;
   pRow2Layout->addWidget(pInfoLabel);

//   QHBoxLayout* pRow3Layout = new QHBoxLayout;
//   pRow3Layout->addWidget(pFeeInfoLabel);

   QHBoxLayout* pButtonsLayout = new QHBoxLayout;
   pButtonsLayout->setContentsMargins(20, 20, 20, 20);
   pButtonsLayout->addWidget(m_pVoteButton);
   pButtonsLayout->addWidget(m_pResetButton);
   pButtonsLayout->addWidget(pCancelButton);

   QVBoxLayout* pMainLayout = new QVBoxLayout;
   pMainLayout->addLayout(pRow1Layout);
   pMainLayout->addLayout(pRow2Layout);
//   pMainLayout->addLayout(pRow3Layout);
   pMainLayout->addLayout(pButtonsLayout);
   pMainLayout->setContentsMargins(10, 10, 10, 10);
   setLayout(pMainLayout);

   QObject::connect(pCancelButton, &QPushButton::clicked,
                    this, &StackLayerWidget::closed);

   QObject::connect(m_pMinersNumVote, &DecentLineEdit::textChanged,
                    this, &MiningVotePopup::slot_MinersNumVoteChanged);

   QObject::connect(m_pVoteButton, &DecentButton::clicked,
                    this, &MiningVotePopup::slot_voteClicked);

   QObject::connect(m_pResetButton, &DecentButton::clicked,
                    this, &MiningVotePopup::slot_voteResetClicked);

}

MiningVotePopup::~MiningVotePopup()
{
}

uint MiningVotePopup::getNumberOfActualMiners()
{
   nlohmann::json active_miners;
   try
   {
      nlohmann::json global_prop_info = Globals::instance().runTaskParse("get_global_properties");
      active_miners = global_prop_info["active_miners"];

   }
   catch (const std::exception& ex)
   {
      GUI_ELOG("Exception: ${e}", ("e", ex.what()));
   }
   catch (const fc::exception& ex)
   {
      GUI_ELOG("Exception: ${e}", ("e", ex.what()));
   }

   return static_cast<uint>(active_miners.size());
}

void MiningVotePopup::getMinerVotesForAccount(const std::string& account_name)
{
   std::string cmd = "get_account ";
   cmd += account_name;

   nlohmann::json account_obj;
   try
   {
      account_obj = Globals::instance().runTaskParse(cmd);

      m_minersVotedNum = account_obj["options"]["num_miner"].get<uint>();
      m_curMinersVotedFor = static_cast<uint>(account_obj["options"]["votes"].size());
   }
   catch (const std::exception& ex)
   {
      GUI_ELOG("Exception: ${e}", ("e", ex.what()));
   }
   catch (const fc::exception& ex)
   {
      GUI_ELOG("Exception: ${e}", ("e", ex.what()));
   }
}

void MiningVotePopup::slot_MinersNumVoteChanged(const QString& value)
{
   uint numMiners = value.toUInt();
   m_pVoteButton->setEnabled(numMiners > 0 && numMiners <= m_curMinersVotedFor);
}

void MiningVotePopup::slot_voteClicked()
{
   uint numMiners = m_pMinersNumVote->text().toUInt();

   std::string ret = setDesiredNumOfMiners(Globals::instance().getCurrentUser(), numMiners);
   if (!ret.empty()) {
      ShowMessageBox(tr("Error"), tr("Failed to vote for miners"), QString::fromStdString(ret));
   }
   else {
      ShowMessageBox(tr("Info"), tr("Your vote was accepted. Thank you"));
   }

   closed();  //close the popup
}

void MiningVotePopup::slot_voteResetClicked()
{
   std::string ret = setDesiredNumOfMiners(Globals::instance().getCurrentUser(), 0);
   if (!ret.empty()) {
      ShowMessageBox(tr("Error"), tr("Failed to vote for miners"), QString::fromStdString(ret));
   }
   else {
      ShowMessageBox(tr("Info"), tr("Your vote was accepted. Thank you"));
   }

   closed();  //close the popup
}

std::string MiningVotePopup::setDesiredNumOfMiners(const std::string& account_name, uint numMiners)
{
   std::string command = "set_desired_miner_count ";
   command += account_name + " ";
   command += std::to_string(numMiners);
   command += " true";

   std::string a_result, message;
   try
   {
      a_result = Globals::instance().runTask(command);
      if (a_result.find("exception:") != std::string::npos)
      {
         message = a_result;
      }
   }
   catch (const std::exception& ex)
   {
      message = ex.what();
   }
   catch (const fc::exception& ex)
   {
      message = ex.what();
   }

   return message;
}

}
