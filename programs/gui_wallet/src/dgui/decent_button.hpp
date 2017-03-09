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

namespace gui_wallet {

class DecentButton : public QLabel
{
    Q_OBJECT
public:
    DecentButton();
    virtual ~DecentButton();
    
protected:
    void mousePressEvent(QMouseEvent *event){emit LabelClicked();};
public:
signals:
    void LabelClicked();
};
    

}

#endif // DECENT_BUTTON_HPP
