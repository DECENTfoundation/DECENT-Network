#pragma once

#include <string>
#include <QString>

#include "gui_wallet_tabcontentmanager.hpp"

class QSignalMapper;

namespace gui_wallet
{
class DecentTable;
struct SDigitalContentPurchase;

class PurchasedTab : public TabContentManager
{
   Q_OBJECT

public:
   PurchasedTab(QWidget* pParent);
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

   void slot_ExtractionDirSelected(QString const& path);
   void slot_SearchTermChanged(QString const& strSearchTerm);
   
protected:
   QSignalMapper*          m_pExtractSignalMapper;
   QSignalMapper*          m_pDetailsSignalMapper;
   DecentTable*            m_pTableWidget;
   int                     m_iActiveItemIndex;
   QString                 m_strSearchTerm;
   std::vector<SDigitalContentPurchase>   _current_content;
};
}//   end namespace gui_wallet
