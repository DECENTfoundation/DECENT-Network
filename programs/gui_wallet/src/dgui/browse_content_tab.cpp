/*
 *	File: browse_content_tab.cpp
 *
 *	Created on: 11 Nov 2016
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
#include "browse_content_tab.hpp"
#include <QLayout>
#include <QCheckBox>
#include <stdio.h>
#include <QHeaderView>
#include <QLabel>

#if 0
pItem = new TableWidgetItem_test(tr("Time"));
if(!pItem){/*warn on low memory*/return;}
m_TableWidget.setHorizontalHeaderItem(1,pItem);

pItem = new TableWidgetItem_test(tr("Synopsis"));
if(!pItem){/*warn on low memory*/return;}
m_TableWidget.setHorizontalHeaderItem(2,pItem);

pItem = new TableWidgetItem_test(tr("Rating"));
if(!pItem){/*warn on low memory*/return;}
m_TableWidget.setHorizontalHeaderItem(3,pItem);

pItem = new TableWidgetItem_test(tr("Left"));
if(!pItem){/*warn on low memory*/return;}
m_TableWidget.setHorizontalHeaderItem(4,pItem);

pItem = new TableWidgetItem_test(tr("Size"));
if(!pItem){/*warn on low memory*/return;}
m_TableWidget.setHorizontalHeaderItem(5,pItem);

pItem = new TableWidgetItem_test(tr("Price"));
if(!pItem){/*warn on low memory*/return;}
m_TableWidget.setHorizontalHeaderItem(6,pItem);
#endif

static const char* s_vccpItemNames[]={"","Time","Synopsis","Rating",
                                     "Left","Size","Price"};
static const int   s_cnNumberOfRows = sizeof(s_vccpItemNames)/sizeof(const char*);

using namespace gui_wallet;
extern int g_nDebugApplication;


Browse_content_tab::Browse_content_tab()
        :
        m_TableWidget(1,s_cnNumberOfRows)
{
    QLabel* pLabel;
    m_TableWidget.horizontalHeader()->hide();
    m_TableWidget.verticalHeader()->hide();

    QCheckBox* pCheck = new QCheckBox;
    m_TableWidget.setCellWidget(0,0,pCheck);

    for( int i(1); i<s_cnNumberOfRows; ++i )
    {
        pLabel = new QLabel(tr(s_vccpItemNames[i]));
        m_TableWidget.setCellWidget(0,i,pLabel);
    }

    QPalette plt_tbl = m_TableWidget.palette();
    plt_tbl.setColor(QPalette::Base, palette().color(QPalette::Window));
    m_TableWidget.setPalette(plt_tbl);

    m_main_layout.addWidget(&m_TableWidget);
    setLayout(&m_main_layout);


}


Browse_content_tab::~Browse_content_tab()
{
}


void Browse_content_tab::showEvent ( QShowEvent * event )
{
    QWidget::showEvent(event);
}

