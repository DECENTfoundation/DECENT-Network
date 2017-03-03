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


void Browse_content_tab::updateContents() {
    
    SetNewTask("list_content \"\" 100", this, NULL, +[](void* owner, void* a_clbkArg, int64_t a_err, const std::string& a_task, const std::string& a_result) {
        Browse_content_tab* obj = (Browse_content_tab*)owner;
        
        try {
            auto contents = json::parse(a_result);
            
            std::vector<SDigitalContent>& dContents = obj->m_dContents;
            dContents.clear();
            dContents.resize(contents.size());
            
            obj->m_waitingUpdates = contents.size();


            for (int i = 0; i < contents.size(); ++i) {
                dContents[i].type = DCT::GENERAL;

                dContents[i].author = contents[i]["author"].get<std::string>();
                
                
                
                dContents[i].price.asset_id = contents[i]["price"]["asset_id"].get<std::string>();
                dContents[i].synopsis = contents[i]["synopsis"].get<std::string>();
                dContents[i].URI = contents[i]["URI"].get<std::string>();
                
                // This part is ugly as hell, but will be rewritten (hopefully)
                
                SetNewTask("get_content \"" + dContents[i].URI + "\"", obj, (void*)i, +[](void* owner, void* a_clbkArg, int64_t a_err, const std::string& a_task, const std::string& a_result) {

                    
                    Browse_content_tab* obj = (Browse_content_tab*)owner;

                    int myIndex = (long long)a_clbkArg;
                    
                    auto content = json::parse(a_result);

                    
                    obj->m_dContents[myIndex].created = content["created"].get<std::string>();
                    obj->m_dContents[myIndex].expiration = content["expiration"].get<std::string>();
                    obj->m_dContents[myIndex].size = content["size"].get<int>();
                    if (content["times_bougth"].is_number()) {
                        obj->m_dContents[myIndex].times_bougth = content["times_bougth"].get<int>();
                    } else {
                        obj->m_dContents[myIndex].times_bougth = 0;
                    }
                    
                    
                    
                    if (content["price"]["amount"].is_number()){
                        obj->m_dContents[myIndex].price.amount =  content["price"]["amount"].get<double>();
                    } else {
                        obj->m_dContents[myIndex].price.amount =  atof(content["price"]["amount"].get<std::string>().c_str());
                    }
                    
                    obj->m_dContents[myIndex].AVG_rating = content["AVG_rating"].get<double>();

                    
                    --obj->m_waitingUpdates;
                    if (obj->m_waitingUpdates == 0) {
                        obj->SetDigitalContentsGUI();
                    }
                });

          
            }
            
        } catch (std::exception& ex) {
            std::cout << ex.what() << std::endl;
        }
    });
               
               
}




void Browse_content_tab::SetDigitalContentsGUI()
{
    //
    //TableWidgetItemW<QCheckBox>* pCheck;
    TableWidgetItemW<QLabel>* pLabel;
    SDigitalContent aTemporar;
    const int cnNumberOfContentsPlus1((int)m_dContents.size()+1);

    
    int nWidth = m_pTableWidget->width();
    m_main_layout.removeWidget(m_pTableWidget);
    delete m_pTableWidget;
    m_pTableWidget = new BTableWidget(cnNumberOfContentsPlus1,s_cnNumberOfRows);

    
    QTableWidget& m_TableWidget = *m_pTableWidget;

    PrepareTableWidgetHeaderGUI();

    for(int i(1); i<cnNumberOfContentsPlus1; ++i)
    {
        aTemporar = m_dContents[i-1];
        
        pLabel = new TableWidgetItemW<QLabel>(aTemporar,this,NULL,&Browse_content_tab::DigContCallback,
                                              tr(aTemporar.created.c_str()));
        
        m_TableWidget.setCellWidget(i,DCF::TIME,pLabel);
        
        std::string synopsis = unescape_string(aTemporar.synopsis);
        std::replace(synopsis.begin(), synopsis.end(), '\t', ' '); // JSON does not like tabs :(    

        try {
            auto synopsis_parsed = json::parse(synopsis);
            synopsis = synopsis_parsed["title"].get<std::string>();
            
        } catch (...) {}
        
        pLabel = new TableWidgetItemW<QLabel>(
                                              aTemporar,this,NULL,&Browse_content_tab::DigContCallback,
                                              tr(synopsis.c_str()));
        
        
        m_TableWidget.setCellWidget(i,DCF::SYNOPSIS,pLabel);

        pLabel = new TableWidgetItemW<QLabel>(
                                               aTemporar,this,NULL,&Browse_content_tab::DigContCallback,
                                               QString::number(aTemporar.AVG_rating));
        
        m_TableWidget.setCellWidget(i,DCF::RATING,pLabel);

        pLabel = new TableWidgetItemW<QLabel>(
                                              aTemporar,this,NULL,&Browse_content_tab::DigContCallback,
                                              tr(aTemporar.expiration.c_str()));
        
        m_TableWidget.setCellWidget(i,DCF::LEFT,pLabel);

        pLabel = new TableWidgetItemW<QLabel>(
                                              aTemporar,this,NULL,&Browse_content_tab::DigContCallback,
                                              QString::number(aTemporar.size));
        
        m_TableWidget.setCellWidget(i,DCF::SIZE,pLabel);

        pLabel = new TableWidgetItemW<QLabel>(
                                               aTemporar,this,NULL,&Browse_content_tab::DigContCallback,
                                               QString::number(aTemporar.price.amount));
        
        m_TableWidget.setCellWidget(i,DCF::PRICE,pLabel);
    }

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
//    if(green_row != 0)
//    {
//        m_pTableWidget->item(green_row,0)->setBackgroundColor(QColor(255,255,255));
//        m_pTableWidget->item(green_row,1)->setBackgroundColor(QColor(255,255,255));
//        m_pTableWidget->item(green_row,2)->setBackgroundColor(QColor(255,255,255));
//        m_pTableWidget->item(green_row,3)->setBackgroundColor(QColor(255,255,255));

//        m_pTableWidget->item(green_row,0)->setForeground(QColor::fromRgb(0,0,0));
//        m_pTableWidget->item(green_row,1)->setForeground(QColor::fromRgb(0,0,0));
//        m_pTableWidget->item(green_row,2)->setForeground(QColor::fromRgb(0,0,0));
//        m_pTableWidget->item(green_row,3)->setForeground(QColor::fromRgb(0,0,0));
//    }
    QPoint mouse_pos = m_pTableWidget->mapFromGlobal(QCursor::pos());
    QTableWidgetItem *ite = m_pTableWidget->itemAt(mouse_pos);

    if(ite != NULL)
    {

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
            //green_row = a;
        }
    }
    else
    {
        green_row = 0;
    }
}

//BTableWidget::BTableWidget(int a , int b) : QTableWidget(a,b)
//{
//    this->setMouseTracking(true);
//}


void BTableWidget::mouseMoveEvent(QMouseEvent *event)
{
    mouseMoveEventDid();
}


