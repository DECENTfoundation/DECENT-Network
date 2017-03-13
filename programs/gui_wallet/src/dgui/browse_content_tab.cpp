/*
 *	File: browse_content_tab.cpp
 *
 *	Cted on: 11 Nov 2016
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

#include <ctime>
#include <limits>
#include <iostream>
#include <graphene/chain/config.hpp>


#include <QDateTime>
#include <QDate>
#include <QTime>

//namespace DCF {enum DIG_CONT_FIELDS{TIME,SYNOPSIS,RATING,SIZE,PRICE,LEFT};}
static const char* s_vccpItemNames[]={"","Title","Rating",
    "Size","Price","Created","Expiration"};
static const int   s_cnNumberOfCols = sizeof(s_vccpItemNames)/sizeof(const char*);

static const int   s_cnNumberOfSearchFields(sizeof(gui_wallet::ST::s_vcpcSearchTypeStrs)/sizeof(const char*));

using namespace gui_wallet;
using namespace nlohmann;


Browse_content_tab::Browse_content_tab() : m_pTableWidget(new BTableWidget(1,s_cnNumberOfCols)), green_row(-1)
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
    
    m_filterLineEdit.setPlaceholderText("Enter search term");
    m_filterLineEdit.setFixedHeight(40);
    m_filterLineEdit.setStyleSheet("border: 1px solid white");
    m_filterLineEdit.setAttribute(Qt::WA_MacShowFocusRect, 0);
    
    m_search_layout.addWidget(new QLabel());
    m_search_layout.addWidget(new QLabel());
    m_search_layout.addWidget(new QLabel());
    m_search_layout.addWidget(lab);
    m_search_layout.addWidget(&m_filterLineEdit);
    
    m_main_layout.addLayout(&m_search_layout);
    m_main_layout.addWidget(m_pTableWidget);
    setLayout(&m_main_layout);
    
    connect(&m_filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(onTextChanged(QString)));
    
    m_contentUpdateTimer.connect(&m_contentUpdateTimer, SIGNAL(timeout()), this, SLOT(maybeUpdateContent()));
    m_contentUpdateTimer.setInterval(1000);
    m_contentUpdateTimer.start();
    Connects();
    ArrangeSize();
}


Browse_content_tab::~Browse_content_tab()
{
    m_main_layout.removeWidget(m_pTableWidget);
    delete m_pTableWidget;
}

void Browse_content_tab::DigContCallback(_NEEDED_ARGS2_)
{
    emit ShowDetailsOnDigContentSig(*a_pDigContent);
    ArrangeSize();
}

void Browse_content_tab::maybeUpdateContent() {
    if (!m_doUpdate) {
        return;
    }
    
    m_doUpdate = false;
    updateContents();
    ArrangeSize();
}

void Browse_content_tab::onTextChanged(const QString& text) {
    
    m_doUpdate = true;
    ArrangeSize();
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
    
    for( int i(0); i<s_cnNumberOfCols; ++i )
    {
        m_TableWidget.setItem(0,i,new QTableWidgetItem(tr(s_vccpItemNames[i])));
        m_TableWidget.item(0,i)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        m_TableWidget.item(0,i)->setFont(f);
        m_TableWidget.item(0,i)->setBackground(QColor(228,227,228));
        m_TableWidget.item(0,i)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        m_TableWidget.item(0,i)->setForeground(QColor::fromRgb(51,51,51));
        
    }
    
    m_TableWidget.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_TableWidget.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    ArrangeSize();
}




void Browse_content_tab::updateContents() {
    /*
     * sync task usage example
    std::string str_result;
    try
    {
        RunTask("lst_content \"tigran\" 100", str_result);
        
        int a = 0;
        ++a;
    }
    catch (std::exception const& ex)
    {
        std::cout << ex.what() << std::endl;
    }*/
    
    std::string filterText = m_filterLineEdit.text().toStdString();

    SetNewTask("search_content \"" + filterText + "\" 100", this, NULL, +[](void* owner, void* a_clbkArg, int64_t a_err, const std::string& a_task, const std::string& a_result) {
        Browse_content_tab* obj = (Browse_content_tab*)owner;
        
        try {
            auto contents = json::parse(a_result);
            
            std::vector<SDigitalContent> dContents;
            dContents.clear();
            dContents.resize(contents.size());
            
            
            for (int i = 0; i < contents.size(); ++i) {
                dContents[i].type = DCT::GENERAL;
                
                dContents[i].author = contents[i]["author"].get<std::string>();
                
                
                
                dContents[i].price.asset_id = contents[i]["price"]["asset_id"].get<std::string>();
                dContents[i].synopsis = contents[i]["synopsis"].get<std::string>();
                dContents[i].URI = contents[i]["URI"].get<std::string>();
                dContents[i].created = contents[i]["created"].get<std::string>();
                dContents[i].expiration = contents[i]["expiration"].get<std::string>();
                dContents[i].size = contents[i]["size"].get<int>();
                
                if (contents[i]["times_bougth"].is_number()) {
                    dContents[i].times_bougth = contents[i]["times_bougth"].get<int>();
                } else {
                    dContents[i].times_bougth = 0;
                }
                
                
                if (contents[i]["price"]["amount"].is_number()){
                    dContents[i].price.amount =  contents[i]["price"]["amount"].get<double>();
                } else {
                    dContents[i].price.amount =  std::stod(contents[i]["price"]["amount"].get<std::string>());
                }
                
                dContents[i].price.amount /= GRAPHENE_BLOCKCHAIN_PRECISION;
                
                dContents[i].AVG_rating = contents[i]["AVG_rating"].get<double>()  / 1000;
            }
            
            obj->ShowDigitalContentsGUI(dContents);
            obj->ArrangeSize();
        } catch (std::exception& ex) {
            std::cout << ex.what() << std::endl;
        }
    });
    
    ArrangeSize();
}



void Browse_content_tab::ShowDigitalContentsGUI(std::vector<SDigitalContent>& contents)
{
    
    m_main_layout.removeWidget(m_pTableWidget);
    delete m_pTableWidget;
    
    
    m_pTableWidget = new BTableWidget(contents.size() + 1, s_cnNumberOfCols);
    
    
    
    QTableWidget& m_TableWidget = *m_pTableWidget;
    
    PrepareTableWidgetHeaderGUI();
    
    int index = 1;
    for(SDigitalContent& aTemporar: contents)
    {
        
        QPixmap image1(":/icon/images/info1_white.svg");
        m_TableWidget.setCellWidget(index, 0, new TableWidgetItemW<QLabel>(aTemporar,this,NULL,&Browse_content_tab::DigContCallback,
                                                                                                            tr("")));
        ((QLabel*)m_TableWidget.cellWidget(index,0))->setPixmap(image1);
        ((QLabel*)m_TableWidget.cellWidget(index,0))->setAlignment(Qt::AlignCenter);
        
        std::string created_str;
        for(int i = 0; i < 10; ++i)
        {
            created_str.push_back(aTemporar.created[i]);
        }

        m_TableWidget.setItem(index,5,new QTableWidgetItem(QString::fromStdString(created_str)));
        m_TableWidget.item(index, 5)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        m_TableWidget.item(index, 5)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        
        
        std::string synopsis = unescape_string(aTemporar.synopsis);
        std::replace(synopsis.begin(), synopsis.end(), '\t', ' '); // JSON does not like tabs :(
        std::replace(synopsis.begin(), synopsis.end(), '\n', ' '); // JSON does not like newlines either :(
        //massageBox_title.push_back(	)
        
        try {
            auto synopsis_parsed = json::parse(synopsis);
            synopsis = synopsis_parsed["title"].get<std::string>();
            
        } catch (...) {}
        

        
        m_TableWidget.setItem(index,1,new QTableWidgetItem(QString::fromStdString(synopsis)));
        m_TableWidget.item(index, 1)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        m_TableWidget.item(index, 1)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        
        std::string rating;
        for(int i = 0; i < std::to_string(aTemporar.AVG_rating).find(".") + 2; ++i)
        {
            rating.push_back(std::to_string(aTemporar.AVG_rating)[i]);
        }
        m_TableWidget.setItem(index,2,new QTableWidgetItem(QString::fromStdString(rating)));
        m_TableWidget.item(index, 2)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        m_TableWidget.item(index, 2)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        
        
        
        if(aTemporar.size < 1024)
        {
            m_TableWidget.setItem(index,3,new QTableWidgetItem(QString::fromStdString(std::to_string(aTemporar.size) + " MB")));
        }
        else
        {
            float size = (float)aTemporar.size/1024;
            std::string size_s;
            std::string s = std::to_string(size);
            for(int i = 0; i < s.find(".") + 3; ++i)
            {
                size_s.push_back(s[i]);
            }
            m_TableWidget.setItem(index,3,new QTableWidgetItem(QString::fromStdString(size_s + " GB")));
        }
        m_TableWidget.item(index, 3)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        m_TableWidget.item(index, 3)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);


        m_TableWidget.setItem(index,4,new QTableWidgetItem(QString::number(aTemporar.price.amount) + " DCT"));
        m_TableWidget.item(index, 4)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        m_TableWidget.item(index, 4)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        
        
        QDateTime time = QDateTime::fromString(QString::fromStdString(aTemporar.expiration), "yyyy-MM-ddTHH:mm:ss");
        
        e_str = CalculateRemainingTime(QDateTime::currentDateTime(), time);

        m_TableWidget.setItem(index,6,new QTableWidgetItem(QString::fromStdString(e_str)));
        m_TableWidget.item(index, 6)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        m_TableWidget.item(index, 6)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        
        ++index;
    }
    
    m_main_layout.addWidget(&m_TableWidget);

    m_pTableWidget->horizontalHeader()->setStretchLastSection(true);
    m_pTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ArrangeSize();
}


void Browse_content_tab::ArrangeSize()
{
    m_pTableWidget->setStyleSheet("QTableView{border : 1px solid white}");
    m_pTableWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_pTableWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    QSize tqs_TableSize = m_pTableWidget->size();
    m_pTableWidget->setColumnWidth(0,(tqs_TableSize.width()*10)/100);
    m_pTableWidget->setColumnWidth(1,(tqs_TableSize.width()*20)/100);
    m_pTableWidget->setColumnWidth(2,(tqs_TableSize.width()*14)/100);
    m_pTableWidget->setColumnWidth(3,(tqs_TableSize.width()*14)/100);
    m_pTableWidget->setColumnWidth(4,(tqs_TableSize.width()*14)/100);
    m_pTableWidget->setColumnWidth(5,(tqs_TableSize.width()*14)/100);
    m_pTableWidget->setColumnWidth(6,(tqs_TableSize.width()*14)/100);
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
    
    if(green_row > 0)
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
