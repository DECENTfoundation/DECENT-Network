#include "stdafx.h"

#include "gui_design.hpp"
#include "decent_button.hpp"
#include <QHBoxLayout>
#include <QSvgWidget>
#include <QPixmap>
#include <QIcon>
#include <QEvent>
#include <QPainter>
#include <QSvgRenderer>
#include <QVariant>
#include <QStyle>

#ifndef _MSC_VER
#include <QGraphicsDropShadowEffect>
#include <string>
#endif

namespace gui_wallet
{
DecentButton::DecentButton(QWidget *parent,
                           eType enType/* = Default*/,
                           eName enName/* = None*/)
: QPushButton(parent)
{
   setMouseTracking(true);

   switch (enType)
   {
   case Send:
      setProperty("type", "send");
      break;
   case TableIcon:
      setProperty("type", "tableicon");
      break;
   case Default:
   default:
      break;
   }

   switch (enName)
   {
      case Transaction:
         setProperty("name", "transaction");
         break;
      case Detail:
         setProperty("name", "detail");
         break;
      case Transfer:
         setProperty("name", "transfer");
         break;
      case Export:
         setProperty("name", "export");
         break;
      case None:
      default:
         break;
   }
}

bool DecentButton::event(QEvent* event)
{
   if (event->type() == QEvent::MouseMove)
      return false;
   else
      return QWidget::event(event);
}

void DecentButton::changeEvent(QEvent* event)
{
   if (event->type() == QEvent::EnabledChange)
   {
      style()->unpolish(this);
      style()->polish(this);
   }
   QPushButton::changeEvent(event);
}
}

