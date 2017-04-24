#pragma once

#include <vector>
#include <string>
#include <QString>

#include "gui_wallet_tabcontentmanager.hpp"


namespace gui_wallet
{
   class DecentTable;
   class SDigitalContent;

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
      
      void show_content_popup();
      void content_was_bought();
      void slot_SearchTermChanged(QString const& strSearchTerm);
      
   protected:
      DecentTable*   m_pTableWidget;
      QString        m_strSearchTerm;
      
      std::vector<SDigitalContent>  _digital_contents;
   };
   
   
}
