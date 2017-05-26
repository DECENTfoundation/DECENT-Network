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
         None
      };
      
      DecentLineEdit(QWidget* pParent, eType enType = Default, eName = None);

   protected:
      virtual void changeEvent(QEvent* event) override;
   };
}
