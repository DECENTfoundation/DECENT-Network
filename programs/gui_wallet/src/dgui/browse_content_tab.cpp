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


/********************************************************************************/
/************ class TableWidgetItemW ********************************************/
/********************************************************************************/


template <typename QtType>
template <typename ConstrArgType,typename ClbType>
gui_wallet::TableWidgetItemW<QtType>::TableWidgetItemW(ConstrArgType a_cons_arg,
                                                       const SDigitalContent& a_cDigCont,
                                                       ClbType* a_own,void* a_clbDt,void (ClbType::*a_fpFunction)(_NEEDED_ARGS_))
    :
      QtType(a_cons_arg),
      m_pOwner(a_own),
      m_pCallbackData(a_clbDt),
      m_callback_data(a_cDigCont)
{
    prepare_constructor(1,a_fpFunction);
}


template <typename QtType>
template <typename ClbType>
gui_wallet::TableWidgetItemW<QtType>::TableWidgetItemW(const SDigitalContent& a_cDigCont,
                                                       ClbType* a_own,void* a_clbDt,void (ClbType::*a_fpFunction)(_NEEDED_ARGS_))
    :
      m_pOwner(a_own),
      m_pCallbackData(a_clbDt),
      m_callback_data(a_cDigCont)
{
    prepare_constructor(1,a_fpFunction);
}

template <typename QtType>
gui_wallet::TableWidgetItemW<QtType>::~TableWidgetItemW()
{
    if(g_nDebugApplication){printf("!!!!!!!!!!!!!!!!!!!!!!!!!!! fn:%s, ln%d\n",__FUNCTION__,__LINE__);}
}

template <typename QtType>
void gui_wallet::TableWidgetItemW<QtType>::prepare_constructor(int a_val,...)
{
    va_list aArgs;
    va_start( aArgs, a_val );
    m_fpCallback = va_arg( aArgs, TypeDigContCallback);
    va_end( aArgs );
}


template <typename QtType>
void gui_wallet::TableWidgetItemW<QtType>::mouseDoubleClickEvent(QMouseEvent* a_event)
{
    if(a_event->button()==Qt::LeftButton)
    {
        if(g_nDebugApplication){
            printf("!!!! fn:%s, ln:%d   ",__FUNCTION__,__LINE__);
            printf("m_pOwner=%p, m_pCallbackData=%p, m_fpCallback=%p\n",m_pOwner,m_pCallbackData,m_fpCallback);
        }
        (*m_fpCallback)(m_pOwner,m_pCallbackData,DCA::CALL_GET_CONTENT, &m_callback_data);
    }
}


/********************************************************************************/
/************ class Browse_content_tab ******************************************/
/********************************************************************************/

Browse_content_tab::Browse_content_tab()
        :
        //m_TableWidget(1,s_cnNumberOfRows)
        m_pTableWidget(new QTableWidget(1,s_cnNumberOfRows))
{
    if(!m_pTableWidget){throw "Low memory!";}

    PrepareTableWidgetHeaderGUI();

    m_main_layout.addWidget(m_pTableWidget);
    setLayout(&m_main_layout);


}


Browse_content_tab::~Browse_content_tab()
{
    m_main_layout.removeWidget(m_pTableWidget);
    delete m_pTableWidget;
}


void Browse_content_tab::PrepareTableWidgetHeaderGUI()
{
    QTableWidget& m_TableWidget = *m_pTableWidget;
    QLabel* pLabel;
    m_TableWidget.horizontalHeader()->hide();
    m_TableWidget.verticalHeader()->hide();

    QCheckBox* pCheck = new QCheckBox;
    if(!pCheck){throw "Low memory\n" __FILE__ ;}
    m_TableWidget.setCellWidget(0,DCF::IS_SELECTED,pCheck);

    for( int i(1); i<s_cnNumberOfRows; ++i )
    {
        pLabel = new QLabel(tr(s_vccpItemNames[i]));
        if(!pLabel){throw "Low memory\n" __FILE__ ;}
        m_TableWidget.setCellWidget(0,i,pLabel);
    }

    QPalette plt_tbl = m_TableWidget.palette();
    plt_tbl.setColor(QPalette::Base, palette().color(QPalette::Window));
    m_TableWidget.setPalette(plt_tbl);
}


// #define _NEEDED_ARGS_ void* a_clb_data,int a_act,const gui_wallet::SDigitalContent* a_pDigContent
void Browse_content_tab::DigContCallback(_NEEDED_ARGS_)
{
    if(g_nDebugApplication){
        printf("!!!! fn:%s, ln:%d  ",__FUNCTION__,__LINE__);
        printf("clbdata=%p, act=%d, pDigCont=%p\n",a_clb_data,a_act,a_pDigContent);
    }

    emit ShowDetailsOnDigContentSig(a_pDigContent->get_content_str);

#if 0
    switch(a_act)
    {
    case DCA::CALL_GET_CONTENT:
        emit GetdetailsOnDigContentSig(a_act,a_pDigContent->URI);
        break;
    default:
        break;
    }
#endif  // #if 0
}


void Browse_content_tab::SetDigitalContentsGUI(const std::vector<gui_wallet::SDigitalContent>& a_vContents)
{
    //
    TableWidgetItemW<QCheckBox>* pCheck;
    TableWidgetItemW<QLabel>* pLabel;
    gui_wallet::SDigitalContent aTemporar;
    const int cnNumberOfContentsPlus1((int)a_vContents.size() + 1);

    if(g_nDebugApplication){printf("cnNumberOfContentsPlus1=%d\n",cnNumberOfContentsPlus1);}

    //m_TableWidget.setColumnCount(cnNumberOfContentsPlus1);

    m_main_layout.removeWidget(m_pTableWidget);
    delete m_pTableWidget;
    m_pTableWidget = new QTableWidget(cnNumberOfContentsPlus1,s_cnNumberOfRows);
    if(!m_pTableWidget){throw "Low memory!";}

    QTableWidget& m_TableWidget = *m_pTableWidget;

    PrepareTableWidgetHeaderGUI();

    for(int i(1); i<cnNumberOfContentsPlus1; ++i)
    {
        if(g_nDebugApplication){printf("fn:%s, ln:%d, i=%d\n",__FUNCTION__,__LINE__,i);}
        aTemporar = a_vContents[i-1];
        // To be continue
        // namespace DGF {enum DIG_CONT_FIELDS{IS_SELECTED,TIME,SYNOPSIS,RATING,LEFT,SIZE,PRICE};}
        //const SDigitalContent& clbData,ClbType* own,void*clbDt,void (ClbType::*a_fpFunction)(_NEEDED_ARGS_)
        pCheck = new TableWidgetItemW<QCheckBox>(aTemporar,this,NULL,&Browse_content_tab::DigContCallback);
        if(!pCheck){throw "Low memory!";}
        m_TableWidget.setCellWidget(i,DCF::IS_SELECTED,pCheck);

        pLabel = new TableWidgetItemW<QLabel>(tr(aTemporar.created.c_str()),
                                              aTemporar,this,NULL,&Browse_content_tab::DigContCallback);
        if(!pLabel){throw "Low memory!";}
        m_TableWidget.setCellWidget(i,DCF::TIME,pLabel);

        pLabel = new TableWidgetItemW<QLabel>(tr(aTemporar.synopsis.c_str()),
                                              aTemporar,this,NULL,&Browse_content_tab::DigContCallback);
        if(!pLabel){throw "Low memory!";}
        m_TableWidget.setCellWidget(i,DCF::SYNOPSIS,pLabel);

        pLabel = new TableWidgetItemW<QLabel>( QString::number(aTemporar.AVG_rating,'f').remove( QRegExp("0+$") ).remove( QRegExp("\\.$") ),
                                               aTemporar,this,NULL,&Browse_content_tab::DigContCallback );
        if(!pLabel){throw "Low memory!";}
        m_TableWidget.setCellWidget(i,DCF::RATING,pLabel);

        pLabel = new TableWidgetItemW<QLabel>(tr(aTemporar.expiration.c_str()),
                                              aTemporar,this,NULL,&Browse_content_tab::DigContCallback);
        if(!pLabel){throw "Low memory!";}
        m_TableWidget.setCellWidget(i,DCF::LEFT,pLabel);

        pLabel = new TableWidgetItemW<QLabel>(QString::number(aTemporar.size,'f').remove( QRegExp("0+$") ).remove( QRegExp("\\.$") ),
                                              aTemporar,this,NULL,&Browse_content_tab::DigContCallback);
        if(!pLabel){throw "Low memory!";}
        m_TableWidget.setCellWidget(i,DCF::SIZE,pLabel);

        pLabel = new TableWidgetItemW<QLabel>( QString::number(aTemporar.price.amount,'f').remove( QRegExp("0+$") ).remove( QRegExp("\\.$") ),
                                               aTemporar,this,NULL,&Browse_content_tab::DigContCallback);
        if(!pLabel){throw "Low memory!";}
        m_TableWidget.setCellWidget(i,DCF::PRICE,pLabel);
    }

    if(g_nDebugApplication){printf("fn:%s, ln:%d\n",__FUNCTION__,__LINE__);}
    m_main_layout.addWidget(&m_TableWidget);
}
