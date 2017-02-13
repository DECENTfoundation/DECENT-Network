/*
 *	File: browse_content_tab.hpp
 *
 *	Created on: 11 Nov 2016
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
#ifndef BROWSE_CONTENT_TAB_H
#define BROWSE_CONTENT_TAB_H

#include <QWidget>
#include <QTableWidget>
#include <QVBoxLayout>

namespace gui_wallet
{
    class Browse_content_tab : public QWidget
    {    
    public:
        Browse_content_tab();
        virtual ~Browse_content_tab();

        void showEvent ( QShowEvent * event )  ;

    private:
        QVBoxLayout     m_main_layout;
        QTableWidget    m_TableWidget;
    };
}

#endif // BROWSE_CONTENT_TAB_H
