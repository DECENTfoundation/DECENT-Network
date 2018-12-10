/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
#pragma once

#include "gui_wallet_tabcontentmanager.hpp"

#ifndef STDAFX_H
class QSignalMapper;
#endif

namespace gui_wallet
{
   class DecentTable;
   class DecentLineEdit;
   class DecentButton;
   struct SDigitalContent;

   class Upload_tab : public TabContentManager
   {
      Q_OBJECT
   public:
      Upload_tab(QWidget* pParent,
                 DecentLineEdit* pFilterLineEdit,
                 DecentButton* pUploadButton);
      ~Upload_tab();
   public:
      virtual void timeToUpdate(const std::string& result) override;
      virtual std::string getUpdateCommand() override;

      void ShowDigitalContentsGUI();

   public slots:
      void slot_SearchTermChanged(QString const& strSearchTerm);
      void slot_SortingChanged(int);
      void slot_Bought();
      void slot_ShowContentPopup(int);
      void slot_UploadPopupResubmit(int);
      void slot_UploadPopup();
      void slot_cellClicked(int row, int col);

   protected:
      DecentTable* m_pTableWidget;
      QSignalMapper* m_pDetailsSignalMapper;
      QSignalMapper* m_pResubmitSignalMapper;
      QString m_strSearchTerm;
      std::vector<SDigitalContent> _digital_contents;
   };
}
