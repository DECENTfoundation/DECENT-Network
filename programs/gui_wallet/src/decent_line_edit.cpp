#include "stdafx.h"

#include "gui_design.hpp"
#include "decent_line_edit.hpp"

#include <QEvent>
#include <QStyle>
#include <QVariant>

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

