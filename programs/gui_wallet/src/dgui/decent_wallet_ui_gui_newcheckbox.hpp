/*
 *	File: decent_wallet_ui_gui_newcheckbox.hpp
 *
 *	Created on: 22 Feb 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
#ifndef DECENT_WALLET_UI_GUI_NEWCHECKBOX_HPP
#define DECENT_WALLET_UI_GUI_NEWCHECKBOX_HPP

#include <QCheckBox>
namespace gui_wallet {

class NewCheckBox : public QCheckBox
{
    Q_OBJECT
public:
    NewCheckBox(const char* checkedImg=NULL, const char* uncheckedImg=NULL, int index=0);
    virtual ~NewCheckBox();

    const int& GetIndex()const;
    void SetIndex(int index);

protected slots:
    void StateChangedSlot(int state);

public:
signals:
    void StateChangedNewSignal(int state, int index);

protected:
    int m_index;
};

}

#endif // DECENT_WALLET_UI_GUI_NEWCHECKBOX_HPP
