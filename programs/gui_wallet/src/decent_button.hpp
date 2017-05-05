/*
 *	File: decent_wallet_ui_gui_newcheckbox.hpp
 *
 *	Created on: 22 Feb 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
#ifndef DECENT_BUTTON_HPP
#define DECENT_BUTTON_HPP

#include <QLabel>
#include <QPushButton>

namespace gui_wallet {

class DecentButton : public QPushButton
{
   Q_OBJECT
public:
   DecentButton(QWidget* parent,
                QString enabledImage = QString(),
                QString disabledImage = QString(),
                bool bOption = false);

protected:
   virtual bool event(QEvent* event) override;
};

}

#endif // DECENT_BUTTON_HPP
