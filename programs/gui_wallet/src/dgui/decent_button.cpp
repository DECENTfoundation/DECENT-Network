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
, m_isHighlight(false)
, m_standardImage(standardImage)
, m_highlightedImage(highlightedImage)
, m_disabledImage(disabledImage)
{
   setMouseTracking(true);
   
   if (m_highlightedImage.isEmpty())
      m_highlightedImage = m_standardImage;
   if (m_disabledImage.isEmpty())
      m_disabledImage = m_standardImage;
   
   if(m_standardImage.isEmpty() && m_highlightedImage.isEmpty() && m_disabledImage.isEmpty())
   {
      this->setStyleSheet(DecentButtonNormal);
   }
   else
   {
      putImage(m_standardImage);
      setStyleSheet("QPushButton{border: 0px ; background-color :rgb(255, 255, 255); color : rgb(0, 0, 0);}"
                    "QPushButton:!enabled{background-color :rgb(180,180,180); color : rgb(30, 30, 30);}");
   }
}

void DecentButton::putImage(const QString& imageName)
{
   if(!imageName.isEmpty())
   {
      if (imageName.endsWith(".svg"))
      {
         setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
         QSvgWidget *icon = new QSvgWidget(imageName, this);
         icon->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
         //icon->setFixedSize(40, 40);
         setLayout( new QHBoxLayout() );
         layout()->addWidget( icon );
      }
      else if (imageName.endsWith(".png"))
      {
         QPixmap pixmap(imageName);
         QIcon ButtonIcon(pixmap);
         setIconSize(QSize(80,80));
         setIcon(ButtonIcon);
      }
   }
};

void DecentButton::setHighlighted(bool bIsHighlighted)
{
   
   if (!isEnabled())
      putImage(m_disabledImage);
   else if (bIsHighlighted)
      putImage(m_highlightedImage);
   else
      putImage(m_standardImage);
   
   if (bIsHighlighted)
      setStyleSheet("QPushButton{border: 0px ; background-color :rgb(27,176,104); color : white;}"
                    "QPushButton:!enabled{background-color :rgb(180,180,180); color : rgb(30, 30, 30);}");
   else
      setStyleSheet("QPushButton{border: 0px ; background-color :rgb(255, 255, 255); color : rgb(0, 0, 0);}"
                    "QPushButton:!enabled{background-color :rgb(180,180,180); color : rgb(30, 30, 30);}");
   
}

bool DecentButton::event(QEvent *event)
{
   if (event->type() == QEvent::MouseMove)
      return false;
   else
      return QWidget::event(event);
}
