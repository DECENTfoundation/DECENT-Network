#include "stdafx.h"

#include "gui_design.hpp"
#include "decent_label.hpp"

#include <QEvent>
#include <QStyle>
#include <QVariant>

namespace gui_wallet
{
   DecentLabel::DecentLabel(QWidget* pParent,
                            eType enType/* = Default*/,
                            eName enName/* = None*/)
   : QLabel(pParent)
   {
      switch (enType)
      {
         case RowLabel:
            setProperty("type", "rowlabel");
            break;
         case Default:
         default:
            break;
      }

      switch (enName)
      {
         case Highlighted:
            setProperty("name", "highlighted");
            break;
         case Right:
            setProperty("name", "right");
            break;
         case HighlightedRight:
            setProperty("name", "highlightedright");
            break;
         case None:
         default:
            break;
      }
   }

   void DecentLabel::changeEvent(QEvent* event)
   {
      if (event->type() == QEvent::EnabledChange)
      {
         style()->unpolish(this);
         style()->polish(this);
      }
      QLabel::changeEvent(event);
   }

}

