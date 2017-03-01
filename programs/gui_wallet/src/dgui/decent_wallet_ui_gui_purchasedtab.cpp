/*
 *	File: decent_wallet_ui_gui_purchasedtab.hpp
 *
 *	Created on: 11 Feb 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */

#include "decent_wallet_ui_gui_purchasedtab.hpp"
#include <QHeaderView>
#include <iostream>

//namespace DCF {enum DIG_CONT_FIELDS{TIME,SYNOPSIS,RATING,SIZE,PRICE,LEFT};}
static const char* s_vccpItemNames[]={"Time","Title","Rating",
                                     "Size","Price","Purchased"};
static const int   s_cnNumberOfRows = sizeof(s_vccpItemNames)/sizeof(const char*);


decent::wallet::ui::gui::PurchasedTab::PurchasedTab()
        :
        //m_TableWidget(1,s_cnNumberOfRows)
        m_pTableWidget(new QTableWidget(1,s_cnNumberOfRows))
{
    if(!m_pTableWidget){throw "Low memory!";}

    PrepareTableWidgetHeaderGUI();

    QHBoxLayout* search_lay = new QHBoxLayout();

    m_filterLineEditer.setPlaceholderText(QString("Search"));
    m_filterLineEditer.setStyleSheet("border: 1px solid white");
    m_filterLineEditer.setFixedHeight(40);

    QPixmap image(":/icon/images/search.svg");

    QLabel* search_label = new QLabel();
    search_label->setSizeIncrement(100,40);
    search_label->setPixmap(image);

    search_lay->addWidget(new QLabel());
    search_lay->addWidget(new QLabel());
    search_lay->addWidget(new QLabel());
    search_lay->addWidget(search_label);
    search_lay->addWidget(&m_filterLineEditer);

    m_main_layout.setContentsMargins(0, 0, 0, 0);

    m_main_layout.addLayout(search_lay);
    //m_main_layout.addWidget(&m_filterLineEditer);
    m_main_layout.addWidget(m_pTableWidget);
    setLayout(&m_main_layout);

}


decent::wallet::ui::gui::PurchasedTab::~PurchasedTab()
{
    m_main_layout.removeWidget(m_pTableWidget);
    delete m_pTableWidget;
}


void decent::wallet::ui::gui::PurchasedTab::PrepareTableWidgetHeaderGUI()
{
    m_pTableWidget->horizontalHeader()->setDefaultSectionSize(300);
    m_pTableWidget->setRowHeight(0,35);
    m_pTableWidget->horizontalHeader()->hide();
    m_pTableWidget->verticalHeader()->hide();
    QFont f( "Open Sans Bold", 10, QFont::Bold);
    for( int i(0); i<s_cnNumberOfRows; ++i )
    {
        //pLabel = new QLabel(tr(s_vccpItemNames[i]));
        //if(!pLabel){throw "Low memory\n" __FILE__ ;}
        //m_TableWidget.setCellWidget(0,i,pLabel);
        m_pTableWidget->setItem(0,i,new QTableWidgetItem(tr(s_vccpItemNames[i])));
        m_pTableWidget->item(0,i)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        m_pTableWidget->item(0,i)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        m_pTableWidget->item(0,i)->setBackground(QColor(228,227,228));
        m_pTableWidget->item(0,i)->setFont(f);
        m_pTableWidget->item(0,i)->setForeground(QColor::fromRgb(51,51,51));

    }
    m_pTableWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_pTableWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    //QPalette plt_tbl = m_TableWidget.palette();
    //plt_tbl.setColor(QPalette::Base, palette().color(QPalette::Window));
    //m_TableWidget.setPalette(plt_tbl);
}


// #define _NEEDED_ARGS_ void* a_clb_data,int a_act,const gui_wallet::SDigitalContent* a_pDigContent
void decent::wallet::ui::gui::PurchasedTab::DigContCallback(_NEEDED_ARGS2_)
{
    __DEBUG_APP2__(3,"clbdata=%p, act=%d, pDigCont=%p\n",a_clb_data,a_act,a_pDigContent);
    emit ShowDetailsOnDigContentSig(*a_pDigContent);
}


void decent::wallet::ui::gui::PurchasedTab::SetDigitalContentsGUI(const std::vector<decent::wallet::ui::gui::SDigitalContent>& a_vContents)
{
    //
    decent::wallet::ui::gui::TableWidgetItemW<QLabel>* pLabel;
    decent::wallet::ui::gui::SDigitalContent aTemporar;
    const int cnNumberOfContentsPlus1((int)a_vContents.size()+1);

    __DEBUG_APP2__(1,"cnNumberOfContentsPlus1=%d\n",cnNumberOfContentsPlus1);

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
                                    aTemporar,this,NULL,
                                    &decent::wallet::ui::gui::PurchasedTab::DigContCallback,
                                    tr(aTemporar.created.c_str()));
        if(!pLabel){throw "Low memory!";}
        m_TableWidget.setCellWidget(i,DCF::TIME,pLabel);

        pLabel = new decent::wallet::ui::gui::TableWidgetItemW<QLabel>(
                                              aTemporar,this,NULL,
                                              &decent::wallet::ui::gui::PurchasedTab::DigContCallback,
                                              tr(aTemporar.synopsis.c_str()));
        if(!pLabel){throw "Low memory!";}
        m_TableWidget.setCellWidget(i,DCF::SYNOPSIS,pLabel);

        pLabel = new decent::wallet::ui::gui::TableWidgetItemW<QLabel>(
                                               aTemporar,this,NULL,
                                               &decent::wallet::ui::gui::PurchasedTab::DigContCallback,
                                               QString::number(aTemporar.AVG_rating,'f').remove( QRegExp("0+$") ).remove( QRegExp("\\.$") ));
        if(!pLabel){throw "Low memory!";}
        m_TableWidget.setCellWidget(i,DCF::RATING,pLabel);

        pLabel = new decent::wallet::ui::gui::TableWidgetItemW<QLabel>(
                                              aTemporar,this,NULL,
                                              &decent::wallet::ui::gui::PurchasedTab::DigContCallback,
                                              tr(aTemporar.expiration.c_str()));
        if(!pLabel){throw "Low memory!";}
        m_TableWidget.setCellWidget(i,DCF::PURCHASED,pLabel);

        pLabel = new decent::wallet::ui::gui::TableWidgetItemW<QLabel>(
                                              aTemporar,this,NULL,
                                              &decent::wallet::ui::gui::PurchasedTab::DigContCallback,
                                              QString::number(aTemporar.size,'f').remove( QRegExp("0+$") ).remove( QRegExp("\\.$") ));
        if(!pLabel){throw "Low memory!";}
        m_TableWidget.setCellWidget(i,DCF::SIZE,pLabel);

        pLabel = new decent::wallet::ui::gui::TableWidgetItemW<QLabel>(
                                               aTemporar,this,NULL,
                                               &decent::wallet::ui::gui::PurchasedTab::DigContCallback,
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


void decent::wallet::ui::gui::PurchasedTab::ArrangeSize()
{
    QSize tqsTableSize = m_pTableWidget->size();
    int nSizeForOne = tqsTableSize.width()/(DCF::NUM_OF_DIG_CONT_FIELDS)-1;
    for(int i(0); i<DCF::NUM_OF_DIG_CONT_FIELDS;++i){m_pTableWidget->setColumnWidth(i,nSizeForOne);}
    //printf("!!!!!!!!!!!!!!!!!!!!!!\n");

    m_pTableWidget->setStyleSheet("QTableView{border : 1px solid lightGray}");
    m_pTableWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_pTableWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    for(int i = 0; i < 6; ++i)
    {
        m_pTableWidget->setColumnWidth(i,(tqsTableSize.width()*16.7)/100);
    }
}


void decent::wallet::ui::gui::PurchasedTab::resizeEvent ( QResizeEvent * a_event )
{
    QWidget::resizeEvent(a_event);
    ArrangeSize();
}
