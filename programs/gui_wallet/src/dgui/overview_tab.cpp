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

Overview_tab::Overview_tab()
{
    QHBoxLayout* up = new QHBoxLayout();
    up->addWidget(&search);

    QVBoxLayout* main = new QVBoxLayout();
    main->addLayout(up);
    main->addWidget(&text);
    //connect(&search, SIGNAL(valueChanged(static QString)), this, SLOT(isChanged()));
    connect(&search,SIGNAL(textChanged(QString)),this,SLOT(isChanged()));


    setLayout(main);
}

void Overview_tab::isChanged()
{
    changed = true;
}

//void Overview_tab::resizeEvent(QResizeEvent *event)
//{
//    resizeEvent(event);
//}

Overview_tab::~Overview_tab()
{

}

