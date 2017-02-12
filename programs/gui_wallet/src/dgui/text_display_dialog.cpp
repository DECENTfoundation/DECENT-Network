/*
 *	File: text_display_dialog.cpp
 *
 *	Created on: 3 Jan, 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */

#include "text_display_dialog.hpp"
#include <QResizeEvent>

gui_wallet::TextDisplayDialog::TextDisplayDialog()
    :
      m_info_textedit(this)
{
    //
}


void gui_wallet::TextDisplayDialog::resizeEvent(QResizeEvent * a_event)
{
    m_info_textedit.resize(a_event->size());
}


QTextEdit* gui_wallet::TextDisplayDialog::operator->()
{
    return &m_info_textedit;
}
