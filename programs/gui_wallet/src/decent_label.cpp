#include "stdafx.h"

#include "gui_design.hpp"
#include "decent_label.hpp"

#include <QEvent>
#include <QStyle>
#include <QVariant>
#include <QTimer>

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
         case ConnectingSplash:
            setProperty("type", "connectingsplash");
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

   StatusLabel::StatusLabel(QWidget* pParent)
   : gui_wallet::DecentLabel(pParent, DecentLabel::ConnectingSplash)
   {}

   void StatusLabel::showMessage(QString const& str_message, int timeout)
   {
      emit signal_removeTimers();
      setText(str_message);

      QTimer* pTimer = new QTimer(this);
      pTimer->start(timeout);
      pTimer->setSingleShot(true);

      QObject::connect(pTimer, &QTimer::timeout,
                       this, &StatusLabel::clearMessage);

      QObject::connect(pTimer, &QTimer::timeout,
                       pTimer, &QTimer::deleteLater);

      QObject::connect(this, &StatusLabel::signal_removeTimers,
                       pTimer, &QTimer::deleteLater);

   }

   void StatusLabel::clearMessage()
   {
      setText(QString());
   }
}

