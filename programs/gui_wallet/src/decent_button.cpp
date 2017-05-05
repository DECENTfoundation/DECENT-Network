/*
 *	File: decent_wallet_ui_gui_newcheckbox.cpp
 *
 *	Created on: 22 Feb 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
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

using namespace gui_wallet;


DecentButton::DecentButton(QWidget *parent/* = Q_NULLPTR*/,
                           QString enabledImage/* = QString()*/,
                           QString disabledImage/* = QString()*/)
: QPushButton(parent)
{
   setMouseTracking(true);

   if (disabledImage.isEmpty())
      disabledImage = enabledImage;
   
   if (false == enabledImage.isEmpty())
   {
      QIcon ButtonIcon;
      setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
      setFlat(true);
      ButtonIcon.addFile(enabledImage, QSize(), QIcon::Normal, QIcon::On);
      ButtonIcon.addFile(disabledImage, QSize(), QIcon::Disabled, QIcon::Off);
      setIcon(ButtonIcon);

      const char* const style =  "QPushButton{border: 0px ; background-color :rgb(27,176,104); color : white;}"
                                 "QPushButton:!enabled{border: 0px ; background-color :rgb(255, 255, 255); color : rgb(0, 0, 0);}";

      setStyleSheet(style);
   }
   else
   {
      const char* const style =  "QPushButton{border: 0px ; background-color :rgb(27,176,104); color : white;}"
                                 "QPushButton:!enabled{background-color :rgb(180,180,180); color : rgb(30, 30, 30);}";

      setStyleSheet(style);
   }
}

DecentButton::DecentButton(QWidget *parent, eType enType)
: QPushButton(parent)
{
   switch (enType)
   {
   case Send:
      setProperty("type", "send");
      setObjectName("send");
      break;
   case Default:
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


