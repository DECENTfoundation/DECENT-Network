/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
#pragma once

#include <string>

#include "gui_wallet_tabcontentmanager.hpp"
#include <QMap>

class QSignalMapper;
class QWidget;

namespace gui_wallet
{
    class DecentTable;
    class DecentLineEdit;
    class DecentButton;
    struct SDigitalContent;

    class MinerVotingTab : public TabContentManager {
    Q_OBJECT
    public:
       MinerVotingTab(QWidget *pParent, DecentLineEdit *pFilterLineEdit);
       ~MinerVotingTab();

       virtual void timeToUpdate(const std::string& result);
       virtual std::string getUpdateCommand();

    public slots:
       void slot_SearchTermChanged(QString const& strSearchTerm);
       void slot_SortingChanged(int index);
       void slot_cellClicked(int row, int col);
       void slot_MinerVote();

    private:
       void submit_vote(const std::string& miner_name, bool voteFlag);

    public:
       DecentTable* m_pTableWidget;
       QString m_strSearchTerm;

       QMap<QWidget*, int> m_buttonsToIndex;
       QMap<int, QString> m_indexToUrl;
    };
}