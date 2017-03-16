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


Browse_content_tab::Browse_content_tab() : m_pTableWidget(new BTableWidget(0,s_cnNumberOfCols)), green_row(-1)
{
    
    PrepareTableWidgetHeaderGUI();
    green_row = 0;
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
    
    m_filterLineEdit.setPlaceholderText("Enter search term");
    m_filterLineEdit.setFixedHeight(40);
    m_filterLineEdit.setStyleSheet("border: 1px solid white");
    m_filterLineEdit.setAttribute(Qt::WA_MacShowFocusRect, 0);
    
    m_search_layout.setContentsMargins(42, 0, 0, 0);
    m_search_layout.addWidget(lab);
    m_search_layout.addWidget(&m_filterLineEdit);
    
    m_main_layout.addLayout(&m_search_layout);
    m_main_layout.addWidget(m_pTableWidget);
    setLayout(&m_main_layout);
    
    connect(&m_filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(onTextChanged(QString)));

    
    m_contentUpdateTimer.connect(&m_contentUpdateTimer, SIGNAL(timeout()), this, SLOT(maybeUpdateContent()));
    m_contentUpdateTimer.setInterval(1000);
    m_contentUpdateTimer.start();
    connect(m_pTableWidget,SIGNAL(mouseMoveEventDid()),this,SLOT(doRowColor()));
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
    BTableWidget& m_TableWidget = *m_pTableWidget;
    
    QFont font( "Open Sans Bold", 14, QFont::Bold);
    
//    m_TableWidget.setStyleSheet("QTableWidget{border : 1px solid red}");
    
    m_TableWidget.horizontalHeader()->setDefaultSectionSize(300);
    m_TableWidget.setRowHeight(0,35);
    m_TableWidget.verticalHeader()->hide();
    
    m_TableWidget.setHorizontalHeaderLabels(QStringList() << "" << "Title" << "Rating" << "Size" << "Price" << "Created" << "Expiration" );
    m_TableWidget.horizontalHeader()->setFixedHeight(35);
    m_TableWidget.horizontalHeader()->setFont(font);
//    m_TableWidget.horizontalHeader()->setStyleSheet("color:rgb(228,227,228)");
    
    m_main_layout.setContentsMargins(0, 0, 0, 0);

    m_TableWidget.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_TableWidget.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_TableWidget.setSelectionMode(QAbstractItemView::NoSelection);

    Connects();
    ArrangeSize();
}




void Browse_content_tab::updateContents() {
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
        }
    });
    Connects();
    ArrangeSize();
}



void Browse_content_tab::ShowDigitalContentsGUI(std::vector<SDigitalContent>& contents)
{
    
    m_main_layout.removeWidget(m_pTableWidget);
    delete m_pTableWidget;
    
    
    m_pTableWidget = new BTableWidget(contents.size(), s_cnNumberOfCols);
    
    
    
    BTableWidget& m_TableWidget = *m_pTableWidget;
    
    PrepareTableWidgetHeaderGUI();
    
    int index = 0;
    for(SDigitalContent& aTemporar: contents)
    {
        
        QPixmap image1(":/icon/images/info1_white.svg");
        m_TableWidget.setCellWidget(index, 0, new TableWidgetItemW<CButton>(aTemporar,this,NULL,&Browse_content_tab::DigContCallback,
                                                                                                            tr("")));
        ((CButton*)m_TableWidget.cellWidget(index,0))->setPixmap(image1);
        ((CButton*)m_TableWidget.cellWidget(index,0))->setAlignment(Qt::AlignCenter);
        
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
    
    Connects();
    
    m_pTableWidget->horizontalHeader()->setStretchLastSection(true);
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
    for(int i = 2; i < 7; ++i)
    {
        m_pTableWidget->setColumnWidth(i,(tqs_TableSize.width()*14)/100);
    }
}


void Browse_content_tab::resizeEvent ( QResizeEvent * a_event )
{
    QWidget::resizeEvent(a_event);
    ArrangeSize();
}


void Browse_content_tab::Connects()
{
    connect(m_pTableWidget,SIGNAL(mouseMoveEventDid()),this,SLOT(doRowColor()));
    for(int i = 0; i < m_pTableWidget->rowCount(); ++i)
        connect((CButton*)m_pTableWidget->cellWidget(i, 0),SIGNAL(mouseWasMoved()),this,SLOT(doRowColor()));
}

void Browse_content_tab::doRowColor()
{
    if(m_pTableWidget->rowCount() < 1)  { return; }
    if(green_row >= 0)
    {
        m_pTableWidget->cellWidget(green_row , 0)->setStyleSheet("* { background-color: rgb(255,255,255); color : white; }");
        for(int i = 1; i < 7; ++i)
        {
            m_pTableWidget->item(green_row,i)->setBackgroundColor(QColor(255,255,255));
            m_pTableWidget->item(green_row,i)->setForeground(QColor::fromRgb(0,0,0));
        }
    }
    QPoint mouse_pos = m_pTableWidget->mapFromGlobal(QCursor::pos());
    if(mouse_pos.x() > 0 && mouse_pos.x() < 400)
    {
        mouse_pos.setX(mouse_pos.x() + 300);
    }
    mouse_pos.setY(mouse_pos.y() - 41);
    QTableWidgetItem *ite = m_pTableWidget->itemAt(mouse_pos);
    if(ite != NULL)
    {
        int row = ite->row();
        if(row < 0) {return;}
        
        m_pTableWidget->cellWidget(row , 0)->setStyleSheet("* { background-color: rgb(27,176,104); color : white; }");
        
        for(int i = 1; i < 7; ++i)
        {
            m_pTableWidget->item(row,i)->setBackgroundColor(QColor(27,176,104));
            m_pTableWidget->item(row,i)->setForeground(QColor::fromRgb(255,255,255));
        }

            green_row = row;
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
