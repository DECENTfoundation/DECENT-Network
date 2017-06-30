/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
#pragma once

#include <QTextEdit>

namespace gui_wallet
{
   class DecentTextEdit : public QTextEdit
   {
      Q_OBJECT
   public:
      enum eType
      {
         Default,
         Info,
         Editor
      };

      enum eName
      {
         None
      };
      
      DecentTextEdit(QWidget* pParent, eType enType = Default, eName = None);

   protected:
      virtual void changeEvent(QEvent* event) override;
   };
}
