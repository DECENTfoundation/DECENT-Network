#pragma once

#include <QLabel>

namespace gui_wallet
{
   class DecentLabel : public QLabel
   {
      Q_OBJECT
   public:
      enum eType
      {
         Default,
         RowLabel
      };

      enum eName
      {
         None,
         Highlighted
      };
      
      DecentLabel(QWidget* pParent, eType enType = Default, eName = None);

   protected:
      virtual void changeEvent(QEvent* event) override;
   };
}
