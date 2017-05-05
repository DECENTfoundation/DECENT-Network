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
    NewCheckBox(int index=0);
    virtual ~NewCheckBox();

    const int& GetIndex()const;
    void SetIndex(int index);

private:
    virtual void enterEvent(QEvent * event) { emit MouseEnteredSignal(m_index); }
    virtual void leaveEvent(QEvent * event) { emit MouseLeftSignal(m_index); }
    virtual void mousePressEvent(QMouseEvent * event) { emit MouseClickedSignal(m_index); }
    
protected slots:
    void StateChangedSlot(int state);

public:
signals:
    void StateChangedNewSignal(int state, int index);
    
    void MouseEnteredSignal(int index);
    void MouseLeftSignal(int index);
    void MouseClickedSignal(int index);
    

protected:
    int m_index;
};

}

#endif // DECENT_WALLET_UI_GUI_NEWCHECKBOX_HPP
