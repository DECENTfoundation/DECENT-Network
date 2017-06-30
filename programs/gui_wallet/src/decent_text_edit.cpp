/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
#include "stdafx.h"

#include "gui_design.hpp"
#include "decent_text_edit.hpp"

#include <QEvent>
#include <QStyle>
#include <QVariant>

namespace gui_wallet
{
   DecentTextEdit::DecentTextEdit(QWidget* pParent,
                                  eType enType/* = Default*/,
                                  eName enName/* = None*/)
   : QTextEdit(pParent)
   {
      switch (enType)
      {
         case Info:
            setProperty("type", "info");
            break;
         case Editor:
            setProperty("type", "editor");
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

   void DecentTextEdit::changeEvent(QEvent* event)
   {
      if (event->type() == QEvent::EnabledChange)
      {
         style()->unpolish(this);
         style()->polish(this);
      }
      QTextEdit::changeEvent(event);
   }

}

