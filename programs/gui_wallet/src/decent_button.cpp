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

#ifndef _MSC_VER
#include <QGraphicsDropShadowEffect>
#include <string>
#endif

using namespace gui_wallet;


DecentButton::DecentButton(QWidget *parent/* = Q_NULLPTR*/,
                           const QString& standardImage/* = QString()*/,
                           const QString& highlightedImage/* = QString()*/,
                           const QString& disabledImage/* = QString()*/)
: QPushButton(parent)
, m_standardImage(standardImage)
, m_highlightedImage(highlightedImage)
, m_disabledImage(disabledImage)
{
   setMouseTracking(true);
   
   if (m_highlightedImage.isEmpty())
      m_highlightedImage = m_standardImage;
   if (m_disabledImage.isEmpty())
      m_disabledImage = m_standardImage;
   
   if(m_standardImage.isEmpty())
   {
      setStyleSheet(GreenDecentButtonEnabled);
   }
   else
   {
      putImage(m_standardImage);
      setStyleSheet(DecentButtonNormal);
   }
}

void DecentButton::putImage(const QString& imageName)
{
   if(!imageName.isEmpty())
   {
      QIcon ButtonIcon;
      setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
      setFlat(true);
      ButtonIcon.addFile(imageName, QSize(), QIcon::Normal, QIcon::On);
      ButtonIcon.addFile(m_disabledImage, QSize(), QIcon::Disabled, QIcon::Off);
      setIcon(ButtonIcon);
   }
};

void DecentButton::setHighlighted(bool bIsHighlighted)
{
   if (bIsHighlighted)
      putImage(m_highlightedImage);
   else
      putImage(m_standardImage);
   
   if(m_standardImage.isEmpty())
   {
      if (bIsHighlighted)
         setStyleSheet(GreenDecentButtonEnabled);
      else
         setStyleSheet(DecentButtonEnabled);
   }
   else
   {
      if (bIsHighlighted)
         setStyleSheet(GreenDecentButtonNormal);
      else
         setStyleSheet(DecentButtonNormal);
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
      if (isEnabled())
      {
         putImage(m_standardImage);
      }
      else
      {
         putImage(m_disabledImage);
      }
   }

   QPushButton::changeEvent(event);
}

