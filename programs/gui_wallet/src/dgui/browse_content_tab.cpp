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
#include <QMouseEvent>
#include <stdio.h>
#include <stdarg.h>

//namespace DCF {enum DIG_CONT_FIELDS{TIME,SYNOPSIS,RATING,SIZE,PRICE,LEFT};}
static const char* s_vccpItemNames[]={"Time","Title","Rating",
                                     "Size","Price","Left"};
static const int   s_cnNumberOfRows = sizeof(s_vccpItemNames)/sizeof(const char*);
static const int   s_cnNumberOfSearchFields(sizeof(ST::s_vcpcSearchTypeStrs)/sizeof(const char*));

using namespace gui_wallet;
extern int g_nDebugApplication;


Browse_content_tab::Browse_content_tab()
        :
        //m_TableWidget(1,s_cnNumberOfRows)
        m_pTableWidget(new QTableWidget(1,s_cnNumberOfRows))
{
    if(!m_pTableWidget){throw "Low memory!";}

    PrepareTableWidgetHeaderGUI();

    for(int i(0); i<s_cnNumberOfSearchFields;++i){m_searchTypeCombo.addItem(tr(ST::s_vcpcSearchTypeStrs[i]));}
    m_searchTypeCombo.setCurrentIndex(0);

    m_filterLineEdit.setStyleSheet( "{"
        "background: #f3f3f3;"
        "background-image: url(:Images/search.svg); /* actual size, e.g. 16x16 */"
        "background-repeat: no-repeat;"
        "background-position: left;"
        "color: #252424;"
        "font-family: SegoeUI;"
        "font-size: 12px;"
        "padding: 2 2 2 20; /* left padding (last number) must be more than the icon's width */"
    "}");

    m_search_layout.addWidget(&m_filterLineEdit);
    m_search_layout.addWidget(&m_searchTypeCombo);

    m_main_layout.addLayout(&m_search_layout);
    m_main_layout.addWidget(m_pTableWidget);
    setLayout(&m_main_layout);

}


Browse_content_tab::~Browse_content_tab()
{
    m_main_layout.removeWidget(m_pTableWidget);
    delete m_pTableWidget;
}


QString Browse_content_tab::getFilterText()const
{
    if(m_filterLineEdit.text() == tr("")){return tr("");}
    return m_searchTypeCombo.currentText() + tr(":") + m_filterLineEdit.text();
}


void Browse_content_tab::PrepareTableWidgetHeaderGUI()
{
    QTableWidget& m_TableWidget = *m_pTableWidget;
    //QLabel* pLabel;
    m_TableWidget.horizontalHeader()->hide();
    m_TableWidget.verticalHeader()->hide();

    for( int i(0); i<s_cnNumberOfRows; ++i )
    {
        //pLabel = new QLabel(tr(s_vccpItemNames[i]));
        //if(!pLabel){throw "Low memory\n" __FILE__ ;}
        //m_TableWidget.setCellWidget(0,i,pLabel);
        m_TableWidget.setItem(0,i,new QTableWidgetItem(tr(s_vccpItemNames[i])));
        m_TableWidget.item(0,i)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);

        m_TableWidget.item(0, i)->setBackground(Qt::lightGray);
    }

    //QPalette plt_tbl = m_TableWidget.palette();
    //plt_tbl.setColor(QPalette::Base, palette().color(QPalette::Window));
    //m_TableWidget.setPalette(plt_tbl);
}


// #define _NEEDED_ARGS_ void* a_clb_data,int a_act,const gui_wallet::SDigitalContent* a_pDigContent
void Browse_content_tab::DigContCallback(_NEEDED_ARGS2_)
{
    __DEBUG_APP2__(3,"clbdata=%p, act=%d, pDigCont=%p\n",a_clb_data,a_act,a_pDigContent);
    emit ShowDetailsOnDigContentSig(a_pDigContent->get_content_str);
}


void Browse_content_tab::SetDigitalContentsGUI(const std::vector<decent::wallet::ui::gui::SDigitalContent>& a_vContents)
{
    //
    decent::wallet::ui::gui::TableWidgetItemW<QCheckBox>* pCheck;
    decent::wallet::ui::gui::TableWidgetItemW<QLabel>* pLabel;
    decent::wallet::ui::gui::SDigitalContent aTemporar;
    const int cnNumberOfContentsPlus1((int)a_vContents.size()+1);

    if(g_nDebugApplication){printf("cnNumberOfContentsPlus1=%d\n",cnNumberOfContentsPlus1);}

    //m_TableWidget.setColumnCount(cnNumberOfContentsPlus1);

    int nWidth = m_pTableWidget->width();
    m_main_layout.removeWidget(m_pTableWidget);
    delete m_pTableWidget;
    m_pTableWidget = new QTableWidget(cnNumberOfContentsPlus1,s_cnNumberOfRows);
    if(!m_pTableWidget){throw "Low memory!";}

    QTableWidget& m_TableWidget = *m_pTableWidget;

    PrepareTableWidgetHeaderGUI();

    for(int i(1); i<cnNumberOfContentsPlus1; ++i)
    {
        __DEBUG_APP2__(4,"i=%d",i);
        aTemporar = a_vContents[i-1];
        // To be continue
        // namespace DGF {enum DIG_CONT_FIELDS{IS_SELECTED,TIME,SYNOPSIS,RATING,LEFT,SIZE,PRICE};}
        //const SDigitalContent& clbData,ClbType* own,void*clbDt,void (ClbType::*a_fpFunction)(_NEEDED_ARGS_)

        pLabel = new decent::wallet::ui::gui::TableWidgetItemW<QLabel>(
                                              aTemporar,this,NULL,&Browse_content_tab::DigContCallback,
                                              tr(aTemporar.created.c_str()));
        if(!pLabel){throw "Low memory!";}
        m_TableWidget.setCellWidget(i,DCF::TIME,pLabel);

        pLabel = new decent::wallet::ui::gui::TableWidgetItemW<QLabel>(
                                              aTemporar,this,NULL,&Browse_content_tab::DigContCallback,
                                              tr(aTemporar.synopsis.c_str()));
        if(!pLabel){throw "Low memory!";}
        m_TableWidget.setCellWidget(i,DCF::SYNOPSIS,pLabel);

        pLabel = new decent::wallet::ui::gui::TableWidgetItemW<QLabel>(
                                               aTemporar,this,NULL,&Browse_content_tab::DigContCallback,
                                               QString::number(aTemporar.AVG_rating,'f').remove( QRegExp("0+$") ).remove( QRegExp("\\.$") ));
        if(!pLabel){throw "Low memory!";}
        m_TableWidget.setCellWidget(i,DCF::RATING,pLabel);

        pLabel = new decent::wallet::ui::gui::TableWidgetItemW<QLabel>(
                                              aTemporar,this,NULL,&Browse_content_tab::DigContCallback,
                                              tr(aTemporar.expiration.c_str()));
        if(!pLabel){throw "Low memory!";}
        m_TableWidget.setCellWidget(i,DCF::LEFT,pLabel);

        pLabel = new decent::wallet::ui::gui::TableWidgetItemW<QLabel>(
                                              aTemporar,this,NULL,&Browse_content_tab::DigContCallback,
                                              QString::number(aTemporar.size,'f').remove( QRegExp("0+$") ).remove( QRegExp("\\.$") ));
        if(!pLabel){throw "Low memory!";}
        m_TableWidget.setCellWidget(i,DCF::SIZE,pLabel);

        pLabel = new decent::wallet::ui::gui::TableWidgetItemW<QLabel>(
                                               aTemporar,this,NULL,&Browse_content_tab::DigContCallback,
                                               QString::number(aTemporar.price.amount,'f').remove( QRegExp("0+$") ).remove( QRegExp("\\.$") ));
        if(!pLabel){throw "Low memory!";}
        m_TableWidget.setCellWidget(i,DCF::PRICE,pLabel);
    }

    __DEBUG_APP2__(3," ");
    m_main_layout.addWidget(&m_TableWidget);
    m_pTableWidget->resize(nWidth,m_pTableWidget->height());
    m_pTableWidget->horizontalHeader()->setStretchLastSection(true);
    m_pTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ArrangeSize();
}


void Browse_content_tab::ArrangeSize()
{
    QSize tqsTableSize = m_pTableWidget->size();
    int nSizeForOne = tqsTableSize.width()/(DCF::NUM_OF_DIG_CONT_FIELDS)-1;
    for(int i(0); i<DCF::NUM_OF_DIG_CONT_FIELDS;++i){m_pTableWidget->setColumnWidth(i,nSizeForOne);}
    //printf("!!!!!!!!!!!!!!!!!!!!!!\n");
}


void Browse_content_tab::resizeEvent ( QResizeEvent * a_event )
{
    QWidget::resizeEvent(a_event);
    ArrangeSize();
}
