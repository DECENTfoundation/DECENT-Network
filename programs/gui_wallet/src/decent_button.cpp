/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
#include "stdafx.h"

#include "decent_button.hpp"

#include <QVariant>
#include <QStyle>
#include <QEvent>

namespace gui_wallet
{
DecentButton::DecentButton(QWidget* pParent, eType enType, eName enName) : QPushButton(pParent)
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
   case StarIcon:
      setProperty("type", "staricon");
      break;
   case DialogAction:
      setProperty("type", "dialogaction");
      break;
   case DialogCancel:
      setProperty("type", "dialogcancel");
      break;
   case DialogTextButton:
      setProperty("type", "dialogtextbutton");
      break;
   case SplashAction:
      setProperty("type", "splashaction");
      break;
   case TabChoice:
      setProperty("type", "tabchoice");
      break;
   case PasswordView:
      setProperty("type", "pass_view");
      break;
   case Asset:
      setProperty("type", "asset");
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
      case Resubmit:
         setProperty("name", "resubmit");
         break;
      case Amount:
         setProperty("name", "amount");
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

