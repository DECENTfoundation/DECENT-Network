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
#include "gui_wallet_global.hpp"
#include <QLayout>
#include <QCheckBox>
#include <stdio.h>
#include <QHeaderView>
#include <QLabel>
#include <QMouseEvent>
#include <stdio.h>
#include <stdarg.h>
#include "json.hpp"

#include <iostream>

//namespace DCF {enum DIG_CONT_FIELDS{TIME,SYNOPSIS,RATING,SIZE,PRICE,LEFT};}
static const char* s_vccpItemNames[]={"Time","Title","Rating",
                                     "Size","Price","Left"};
static const int   s_cnNumberOfRows = sizeof(s_vccpItemNames)/sizeof(const char*);
static const int   s_cnNumberOfSearchFields(sizeof(gui_wallet::ST::s_vcpcSearchTypeStrs)/sizeof(const char*));

using namespace gui_wallet;
using namespace nlohmann;
extern int g_nDebugApplication;


Browse_content_tab::Browse_content_tab() : green_row(0),

        //m_TableWidget(1,s_cnNumberOfRows)
        m_pTableWidget(new BTableWidget(1,s_cnNumberOfRows))
{

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
    QLabel* lab = new QLabel();
    QPixmap image(":/icon/images/search.svg");
    lab->setPixmap(image);
    //lab.setSizeIncrement(100, 40);

    m_filterLineEdit.setPlaceholderText("Search");
    m_filterLineEdit.setFixedHeight(40);
    m_filterLineEdit.setStyleSheet("border: 1px solid white");


    m_search_layout.addWidget(new QLabel());
    m_search_layout.addWidget(new QLabel());
    m_search_layout.addWidget(new QLabel());
    m_search_layout.addWidget(lab);
    m_search_layout.addWidget(&m_filterLineEdit);
    //m_search_layout.addWidget(&m_searchTypeCombo);
    //m_main_layout.addLayout(searchlay);

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


    //m_TableWidget.setShowGrid(false);

    m_TableWidget.setStyleSheet("QTableWidget{border : 1px solid red}");

    m_TableWidget.horizontalHeader()->setDefaultSectionSize(300);
    m_TableWidget.setRowHeight(0,35);
    m_TableWidget.horizontalHeader()->hide();
    m_TableWidget.verticalHeader()->hide();
    m_main_layout.setContentsMargins(0, 0, 0, 0);

    QFont f( "Open Sans Bold", 14, QFont::Bold);

    for( int i(0); i<s_cnNumberOfRows; ++i )
    {
        //pLabel = new QLabel(tr(s_vccpItemNames[i]));
        //if(!pLabel){throw "Low memory\n" __FILE__ ;}
        //m_TableWidget.setCellWidget(0,i,pLabel);
        m_TableWidget.setItem(0,i,new QTableWidgetItem(tr(s_vccpItemNames[i])));
        m_TableWidget.item(0,i)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        m_TableWidget.item(0,i)->setFont(f);
        m_TableWidget.item(0,i)->setBackground(QColor(228,227,228));
        m_TableWidget.item(0,i)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        m_TableWidget.item(0,i)->setForeground(QColor::fromRgb(51,51,51));

    }

    m_TableWidget.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_TableWidget.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    //QPalette plt_tbl = m_TableWidget.palette();
    //plt_tbl.setColor(QPalette::Base, palette().color(QPalette::Window));
    //m_TableWidget.setPalette(plt_tbl);
}


// #define _NEEDED_ARGS_ void* a_clb_data,int a_act,const gui_wallet::SDigitalContent* a_pDigContent
void Browse_content_tab::DigContCallback(_NEEDED_ARGS2_)
{
    __DEBUG_APP2__(3,"clbdata=%p, act=%d, pDigCont=%p\n",a_clb_data,a_act,a_pDigContent);
    emit ShowDetailsOnDigContentSig(*a_pDigContent);
}


void Browse_content_tab::SetDigitalContentsGUI(const std::vector<decent::wallet::ui::gui::SDigitalContent>& a_vContents)
{
    //
    //decent::wallet::ui::gui::TableWidgetItemW<QCheckBox>* pCheck;
    decent::wallet::ui::gui::TableWidgetItemW<QLabel>* pLabel;
    decent::wallet::ui::gui::SDigitalContent aTemporar;
    const int cnNumberOfContentsPlus1((int)a_vContents.size()+1);

    if(g_nDebugApplication){printf("cnNumberOfContentsPlus1=%d\n",cnNumberOfContentsPlus1);}

    //m_TableWidget.setColumnCount(cnNumberOfContentsPlus1);

    int nWidth = m_pTableWidget->width();
    m_main_layout.removeWidget(m_pTableWidget);
    delete m_pTableWidget;
    m_pTableWidget = new BTableWidget(cnNumberOfContentsPlus1,s_cnNumberOfRows);

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
        m_TableWidget.setCellWidget(i,DCF::TIME,pLabel);
        
        std::string synopsis = unescape_string(aTemporar.synopsis);
        std::replace(synopsis.begin(), synopsis.end(), '\t', ' '); // JSON does not like tabs :(    

        try {
            auto synopsis_parsed = json::parse(synopsis);
            synopsis = synopsis_parsed["title"].get<std::string>();
            
        } catch (...) {}
        
        pLabel = new decent::wallet::ui::gui::TableWidgetItemW<QLabel>(
                                              aTemporar,this,NULL,&Browse_content_tab::DigContCallback,
                                              tr(synopsis.c_str()));
        m_TableWidget.setCellWidget(i,DCF::SYNOPSIS,pLabel);

        pLabel = new decent::wallet::ui::gui::TableWidgetItemW<QLabel>(
                                               aTemporar,this,NULL,&Browse_content_tab::DigContCallback,
                                               tr(aTemporar.AVG_rating.c_str()));
        m_TableWidget.setCellWidget(i,DCF::RATING,pLabel);

        pLabel = new decent::wallet::ui::gui::TableWidgetItemW<QLabel>(
                                              aTemporar,this,NULL,&Browse_content_tab::DigContCallback,
                                              tr(aTemporar.expiration.c_str()));
        m_TableWidget.setCellWidget(i,DCF::LEFT,pLabel);

        pLabel = new decent::wallet::ui::gui::TableWidgetItemW<QLabel>(
                                              aTemporar,this,NULL,&Browse_content_tab::DigContCallback,
                                              tr(aTemporar.size.c_str()));
        m_TableWidget.setCellWidget(i,DCF::SIZE,pLabel);

        pLabel = new decent::wallet::ui::gui::TableWidgetItemW<QLabel>(
                                               aTemporar,this,NULL,&Browse_content_tab::DigContCallback,
                                               tr(aTemporar.price.amount.c_str()));
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
    m_pTableWidget->setStyleSheet("QTableView{border : 1px solid lightGray}");
    m_pTableWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_pTableWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QSize tqs_TableSize = m_pTableWidget->size();
    m_pTableWidget->setColumnWidth(0,(tqs_TableSize.width()*16.7)/100);
    m_pTableWidget->setColumnWidth(1,(tqs_TableSize.width()*16.7)/100);
    m_pTableWidget->setColumnWidth(2,(tqs_TableSize.width()*16.7)/100);
    m_pTableWidget->setColumnWidth(3,(tqs_TableSize.width()*16.7)/100);
    m_pTableWidget->setColumnWidth(4,(tqs_TableSize.width()*16.7)/100);
    m_pTableWidget->setColumnWidth(5,(tqs_TableSize.width()*16.7)/100);
}


void Browse_content_tab::resizeEvent ( QResizeEvent * a_event )
{
    QWidget::resizeEvent(a_event);
    ArrangeSize();
}


void Browse_content_tab::Connects()
{
    connect(m_pTableWidget,SIGNAL(mouseMoveEventDid()),this,SLOT(doRowColor()));
}

void Browse_content_tab::doRowColor()
{
    if(green_row != 0)
    {
        m_pTableWidget->item(green_row,0)->setBackgroundColor(QColor(255,255,255));
        m_pTableWidget->item(green_row,1)->setBackgroundColor(QColor(255,255,255));
        m_pTableWidget->item(green_row,2)->setBackgroundColor(QColor(255,255,255));
        m_pTableWidget->item(green_row,3)->setBackgroundColor(QColor(255,255,255));

        m_pTableWidget->item(green_row,0)->setForeground(QColor::fromRgb(0,0,0));
        m_pTableWidget->item(green_row,1)->setForeground(QColor::fromRgb(0,0,0));
        m_pTableWidget->item(green_row,2)->setForeground(QColor::fromRgb(0,0,0));
        m_pTableWidget->item(green_row,3)->setForeground(QColor::fromRgb(0,0,0));
    }
    std::cout<<"-----------------"<<m_pTableWidget->columnCount()<<std::endl;
    std::cout<<"-----------------"<<m_pTableWidget->rowCount()<<std::endl;
    QPoint mouse_pos = m_pTableWidget->mapFromGlobal(QCursor::pos());
    QTableWidgetItem *ite = m_pTableWidget->itemAt(mouse_pos);
    if(ite != NULL)
    {
        std::cout<<"DO ROE COlor"<<std::endl;
        int a = ite->row();
        if(a != 0)
        {
            m_pTableWidget->item(a,0)->setBackgroundColor(QColor(27,176,104));
            m_pTableWidget->item(a,1)->setBackgroundColor(QColor(27,176,104));
            m_pTableWidget->item(a,2)->setBackgroundColor(QColor(27,176,104));
            m_pTableWidget->item(a,3)->setBackgroundColor(QColor(27,176,104));
            m_pTableWidget->item(a,4)->setBackgroundColor(QColor(27,176,104));
            m_pTableWidget->item(a,5)->setBackgroundColor(QColor(27,176,104));

            m_pTableWidget->item(a,0)->setForeground(QColor::fromRgb(255,255,255));
            m_pTableWidget->item(a,1)->setForeground(QColor::fromRgb(255,255,255));
            m_pTableWidget->item(a,2)->setForeground(QColor::fromRgb(255,255,255));
            m_pTableWidget->item(a,3)->setForeground(QColor::fromRgb(255,255,255));
            m_pTableWidget->item(a,4)->setForeground(QColor::fromRgb(255,255,255));
            m_pTableWidget->item(a,5)->setForeground(QColor::fromRgb(255,255,255));
            green_row = a;
        }
    }
    else
    {
        green_row = 0;
    }
}

void BTableWidget::mouseMoveEvent(QMouseEvent *event)
{
    mouseMoveEventDid();
}


