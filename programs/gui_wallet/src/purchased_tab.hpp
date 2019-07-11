/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
#pragma once

#include "gui_wallet_tabcontentmanager.hpp"

namespace gui_wallet
{
class DecentTable;
struct SDigitalContentPurchase;
class DecentLineEdit;

class PurchasedTab : public TabContentManager
{
   Q_OBJECT

public:
   PurchasedTab(QWidget* pParent,
                DecentLineEdit* pFilterLineEdit);
   ~PurchasedTab();
   
public:
   virtual void timeToUpdate(const std::string& result) override;
   virtual std::string getUpdateCommand() override;
   
protected:
   void ShowMessageBox(std::string const& message);
   void ShowDigitalContentsGUI();

public slots:
   void slot_ExtractPackage(int);
   void slot_Details(int);
   void slot_SortingChanged(int);
   void slot_cellClicked(int row, int col);

   void slot_ExtractionDirSelected(QString const& path);
   void slot_SearchTermChanged(QString const& strSearchTerm);
   
protected:
   DecentTable*            m_pTableWidget;
   int                     m_iActiveItemIndex;
   QString                 m_strSearchTerm;
   std::vector<SDigitalContentPurchase>   _current_content;
};

}//   end namespace gui_wallet
