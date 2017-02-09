/*
 *	File      : upload_tab.hpp
 *
 *	Created on: 21 Nov 2016
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
#ifndef UPLOAD_TAB_HPP
#define UPLOAD_TAB_HPP

#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>

#define USE_TABLE_WIDGET

namespace gui_wallet
{
    struct SStrPair{const char  *str1,*str2;};
    static const SStrPair s_sStrPairs[] ={{"Lifetime",""},{"Seeders",""},{"Key particles",""},
                                          {"Tags","Add tag"},{"Price","0.00"}};
    static const int NUMBER_OF_INFO_LINES = sizeof(s_sStrPairs) / sizeof(struct SStrPair);

    class Upload_tab : public QWidget
    {    
    public:
        Upload_tab();
        virtual ~Upload_tab();

    protected:
        virtual void resizeEvent ( QResizeEvent * event );

    private:
        QHBoxLayout m_main_layout;
        QVBoxLayout m_synopsis_layout;
        QVBoxLayout m_info_layout;
        QTableWidget    m_info_widget;
        QLabel      m_synopsis_label;
        QTextEdit   m_synopsis_text;
        QLabel      m_infoLayoutHeader;

    };

}

#endif // UPLOAD_TAB_HPP
