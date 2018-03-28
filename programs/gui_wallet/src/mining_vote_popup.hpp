/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
#pragma once

#include "richdialog.hpp"

class QWidget;

namespace gui_wallet {

    class DecentButton;
    class DecentLineEdit;

    class MiningVotePopup : public StackLayerWidget {
       Q_OBJECT
    public:
       MiningVotePopup(QWidget* pParent);
       ~MiningVotePopup();

    private slots:
       void slot_MinersNumVoteChanged(const QString& value);
       void slot_voteClicked();

    private:
       uint getNumberOfActualMiners();
       void getMinerVotesForAccount(const std::string& account_name);
    private:

       uint m_minersVotedNum;
       uint m_curMinersVotedFor;

       DecentLineEdit* m_pMinersNumVote;
       DecentButton* m_pVoteButton;
       DecentButton* m_pResetButton;

    };


} //namespace