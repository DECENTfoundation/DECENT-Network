/*
 *	File: text_display_dialog.hpp
 *
 *	Created on: 3 Jan, 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
#ifndef TEXT_DISPLAY_DIALOG_HPP
#define TEXT_DISPLAY_DIALOG_HPP

#include <QDialog>
#include <QTextEdit>

namespace gui_wallet
{

class TextDisplayDialog : public QDialog
{
public:
    TextDisplayDialog();
    virtual ~TextDisplayDialog(){}

    QTextEdit* operator->();

protected:
    virtual void resizeEvent(QResizeEvent * event);

private:
    QTextEdit       m_info_textedit;
};

}

#endif // TEXT_DISPLAY_DIALOG_HPP
