/*
 *	File: gui_wallet_centralwigdet.h
 *
 *	Created on: Nov 11, 2016
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
#ifndef TAB_CONTENT_MANAGER_HPP
#define TAB_CONTENT_MANAGER_HPP


#include <QWidget>

namespace gui_wallet {


class TabContentManager : public QWidget {
    
public:
    virtual void content_activated() = 0;
    virtual void content_deactivated() = 0;
    
};


}


#endif // TAB_CONTENT_MANAGER_HPP
