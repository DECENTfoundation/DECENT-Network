#pragma once

#include <vector>
#include <string>
#include <QString>

#include "gui_wallet_tabcontentmanager.hpp"

class QSignalMapper;

namespace gui_wallet
{
   class DecentTable;
   struct SDigitalContent;

   class BrowseContentTab : public TabContentManager
   {
      Q_OBJECT
   public:
      BrowseContentTab(QWidget* pParent);
      
      void ShowDigitalContentsGUI();
      
   public:
      virtual void timeToUpdate(const std::string& result) override;
      virtual std::string getUpdateCommand() override;
       
   public slots:

      void slot_Details(int);
      void slot_Bought();
      void slot_SearchTermChanged(QString const& strSearchTerm);
      void slot_SortingChanged(int);
      
   protected:
      DecentTable*   m_pTableWidget;
      QSignalMapper* m_pDetailsSignalMapper;
      QString        m_strSearchTerm;
      
      std::vector<SDigitalContent>  _digital_contents;
   };
   
   
}
