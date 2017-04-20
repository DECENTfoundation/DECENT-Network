/*
 *	File: text_display_dialog.cpp
 *
 *	Created on: 3 Jan, 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
#ifdef _MSC_VER
#include "stdafx.h"
#endif

#include "text_display_dialog.hpp"
#ifndef _MSC_VER
#include <QResizeEvent>
#endif

gui_wallet::TextDisplayDialog::TextDisplayDialog()
    :
      m_info_textedit(this)
{
    //
#ifdef _MSC_VER
   int height = style()->pixelMetric(QStyle::PM_TitleBarHeight);
   setWindowIcon(height > 32 ? QIcon(":/icon/images/windows_decent_icon_32x32.png")
      : QIcon(":/icon/images/windows_decent_icon_16x16.png"));
#endif
}


void gui_wallet::TextDisplayDialog::resizeEvent(QResizeEvent * a_event)
{
    m_info_textedit.resize(a_event->size());
}


QTextEdit* gui_wallet::TextDisplayDialog::operator->()
{
    return &m_info_textedit;
}
