/*
 *	File: qt_commonheader.hpp
 *
 *	Created on: 30 Jan 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
#ifndef __qt_commonheader_hpp__
#define __qt_commonheader_hpp__

#include <QEvent>
#include <QObject>
#include <stdio.h>
//#include "connected_api_instance.hpp"


namespace gui_wallet{

#if 1

void ConnectToStateChangeUpdate(QObject* object,const char* method);

#else  // #if 1

static const int s_cnGuiStatesEvent = QEvent::User + 1;

class GuiStateEvent : public QEvent{
public:
    GuiStateEvent(_API_STATE a_state=DEFAULT_ST) : QEvent(s_cnGuiStatesEvent),m_nState(a_state){}
    virtual ~GuiStateEvent(){printf("%s\n",__FUNCTION__);}
    _API_STATE  api_state()const{return m_nState;}
    void set_api_state(_API_STATE a_state){m_nState=a_state;}
protected:
    _API_STATE m_nState;
};

#endif

} // namespace decent_gui



#endif // #ifdef __qt_commonheader_hpp__
