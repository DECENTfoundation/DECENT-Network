/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
#pragma once

#include <QLineEdit>

namespace gui_wallet
{
   class DecentLineEdit : public QLineEdit
   {
      Q_OBJECT
   public:
      enum eType
      {
         Default,
         TableSearch,
         DialogLineEdit
      };

      enum eName
      {
         None,
         DlgImport,
         Amount
      };
      
      DecentLineEdit(QWidget* pParent, eType enType = Default, eName = None);

   protected:
      virtual void changeEvent(QEvent* event) override;
   };
}
