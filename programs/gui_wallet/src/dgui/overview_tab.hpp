#pragma once

#include <QLineEdit>

#include "gui_wallet_tabcontentmanager.hpp"

class QSignalMapper;

namespace gui_wallet
{
   class DecentTable;

   class Overview_tab : public TabContentManager
   {
      Q_OBJECT
   public:
      Overview_tab(class Mainwindow_gui_wallet* pPar);
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

   public:
      QSignalMapper* m_pAccountSignalMapper;
      DecentTable* m_pTableWidget;
      QString m_strSearchTerm;
      QString m_strSelectedAccount;

      
   protected:
      class Mainwindow_gui_wallet* m_pPar;
      
   };
}


