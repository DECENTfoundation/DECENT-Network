/*
 *	File      : overview_tab.cpp
 *
 *	Created on: 21 Nov 2016
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
#include "overview_tab.hpp"

using namespace gui_wallet;

Overview_tab::Overview_tab() : find("find")
{
    QHBoxLayout* up = new QHBoxLayout();
    up->addWidget(&find);
    up->addWidget(&search);

    QVBoxLayout* main = new QVBoxLayout();
    main->addLayout(up);
    main->addWidget(&text);
    connect(&find,SIGNAL(clicked()),this,SLOT(ClickFind()));

    setLayout(main);
}

void Overview_tab::ClickFind()
{
    text.setText(search.text());
       //return text.setText(search.text());
}

//void Overview_tab::resizeEvent(QResizeEvent *event)
//{
//    resizeEvent(event);
//}

Overview_tab::~Overview_tab()
{

}

