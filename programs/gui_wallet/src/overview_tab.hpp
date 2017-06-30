/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
#pragma once

#include <QLineEdit>

#include "gui_wallet_tabcontentmanager.hpp"

class QSignalMapper;

namespace gui_wallet
{
   class DecentTable;
   class DecentLineEdit;

   class Overview_tab : public TabContentManager
   {
      Q_OBJECT
   public:
      Overview_tab(QWidget* pParent,
                   DecentLineEdit* pFilterLineEdit);
      void CreateTable();
      void ArrangeSize();
      
   public:
      virtual void timeToUpdate(const std::string& result);
      virtual std::string getUpdateCommand();

   public slots:
      void slot_Details();
      void slot_Transactions();
      void slot_Transfer();
      void slot_SearchTermChanged(QString const& strSearchTerm);
      void slot_AccountChanged(QString const& strAccountName);
      void slot_SortingChanged(int index);

   public:
      QSignalMapper* m_pAccountSignalMapper;
      DecentTable* m_pTableWidget;
      QString m_strSearchTerm;
      QString m_strSelectedAccount;      
   };
}


