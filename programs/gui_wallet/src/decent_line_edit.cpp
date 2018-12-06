/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */

#ifndef STDAFX_H
#include <QEvent>
#include <QStyle>
#endif

#include "decent_line_edit.hpp"

namespace gui_wallet
{
   DecentLineEdit::DecentLineEdit(QWidget* pParent,
                                  eType enType/* = Default*/,
                                  eName enName/* = None*/)
   : QLineEdit(pParent)
   {
      switch (enType)
      {
         case TableSearch:
            setProperty("type", "tablesearch");
            break;
         case DialogLineEdit:
            setProperty("type", "dialoglineedit");
            break;
         case Default:
         default:
            break;
      }

      switch (enName)
      {
         case DlgImport:
            setProperty("name", "import_dlg");
            break;
         case Amount:
            setProperty("name", "amount");
            break;
         case None:
         default:
            break;
      }
   }

   void DecentLineEdit::changeEvent(QEvent* event)
   {
      if (event->type() == QEvent::EnabledChange)
      {
         style()->unpolish(this);
         style()->polish(this);
      }
      QLineEdit::changeEvent(event);
   }
}
