#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <vector>
#include <string>

#include <QLineEdit>
#include <QHBoxLayout>
#include <QComboBox>

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
      
   protected:
      QVBoxLayout     m_main_layout;
      QHBoxLayout     m_search_layout;
      DecentTable*    m_pTableWidget;
      QLineEdit       m_filterLineEdit;
      QComboBox       m_searchTypeCombo;
      
      std::vector<SDigitalContent>  _digital_contents;
   };
   
   
}
