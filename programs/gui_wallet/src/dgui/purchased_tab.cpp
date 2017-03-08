/*
 *	File: decent_wallet_ui_gui_purchasedtab.hpp
 *
 *	Created on: 11 Feb 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */

#include "purchased_tab.hpp"
#include <QHeaderView>
#include <iostream>
#include <graphene/chain/config.hpp>
#include "json.hpp"
#include "gui_wallet_global.hpp"


using namespace gui_wallet;
using namespace nlohmann;


static const char* s_vccpItemNames[]={" ","Title","Rating","Size","Price","Created","Purchased"};
static const int   s_cnNumberOfRows = sizeof(s_vccpItemNames)/sizeof(const char*);


PurchasedTab::PurchasedTab()
        :
        m_pTableWidget(new QTableWidget(1,s_cnNumberOfRows))
{

    PrepareTableWidgetHeaderGUI();

    QHBoxLayout* search_lay = new QHBoxLayout();

    m_filterLineEditer.setPlaceholderText(QString("Enter user name to see purchases"));
    m_filterLineEditer.setStyleSheet("border: 1px solid white");
    m_filterLineEditer.setFixedHeight(40);
    m_filterLineEditer.setAttribute(Qt::WA_MacShowFocusRect, 0);

    QPixmap image(":/icon/images/search.svg");

    QLabel* search_label = new QLabel();
    search_label->setSizeIncrement(100,40);
    search_label->setPixmap(image);

    search_lay->addWidget(new QLabel());
    search_lay->addWidget(search_label);
    search_lay->addWidget(new QLabel());
    search_lay->addWidget(&m_filterLineEditer);

    m_main_layout.setContentsMargins(0, 0, 0, 0);

    m_main_layout.addLayout(search_lay);
    m_main_layout.addWidget(m_pTableWidget);

    setLayout(&m_main_layout);
    
    
    connect(&m_filterLineEditer, SIGNAL(textChanged(QString)), this, SLOT(onTextChanged(QString)));
    
    m_contentUpdateTimer.connect(&m_contentUpdateTimer, SIGNAL(timeout()), this, SLOT(maybeUpdateContent()));
    m_contentUpdateTimer.setInterval(1000);
    m_contentUpdateTimer.start();

}







void PurchasedTab::maybeUpdateContent() {
    if (!m_doUpdate) {
        return;
    }
    
    m_doUpdate = false;
    updateContents();
}

void PurchasedTab::onTextChanged(const QString& text) {
    
    m_doUpdate = true;
}



void PurchasedTab::updateContents() {
    m_pTableWidget->setRowCount(1); //Remove everything but header
    
    
    if (m_filterLineEditer.text().toStdString().empty()) {
        return;
    }
    auto& global_instance = gui_wallet::GlobalEvents::instance();
    std::string str_current_username = global_instance.getCurrentUser();
    
    SetNewTask("get_buying_history_objects_by_consumer_title "
               "\"" + str_current_username +"\" "
               "\"" + m_filterLineEditer.text().toStdString() +"\"",
               this, NULL,
               +[](void* owner, void* a_clbkArg, int64_t a_err, const std::string& a_task, const std::string& a_result) {
        
        if (a_err != 0) {
            return;
        }
        
        PurchasedTab* obj = (PurchasedTab*)owner;
        
        try {
            auto contents = json::parse(a_result);
            
            obj->m_pTableWidget->setRowCount(contents.size() + 1);
            
            for (int i = 0; i < contents.size(); ++i) {
                
                auto content = contents[i];
                
                
                std::string time = contents[i]["expiration_time"].get<std::string>();
                
                std::string synopsis = unescape_string(contents[i]["synopsis"].get<std::string>());
                std::replace(synopsis.begin(), synopsis.end(), '\t', ' '); // JSON does not like tabs :(
                
                try {
                    auto synopsis_parsed = json::parse(synopsis);
                    synopsis = synopsis_parsed["title"].get<std::string>();
                } catch (...) {}
                
                double rating = contents[i]["rating"].get<double>();
                uint64_t size = contents[i]["size"].get<int>();


                double price = 0;
                if (contents[i]["price"]["amount"].is_number()){
                    price =  contents[i]["price"]["amount"].get<double>();
                } else {
                    price =  std::stod(contents[i]["price"]["amount"].get<std::string>());
                }
                price /= GRAPHENE_BLOCKCHAIN_PRECISION;
                
                std::string expiration_or_delivery_time = contents[i]["expiration_or_delivery_time"].get<std::string>();
                
                QLabel* imag_label = new QLabel();
                imag_label->setAlignment(Qt::AlignCenter);

                QPixmap image1(":/icon/images/info1.svg");
                imag_label->setPixmap(image1);
                obj->m_pTableWidget->setCellWidget(i + 1, 0, imag_label);
                obj->m_pTableWidget->setItem(i + 1, 1, new QTableWidgetItem(QString::fromStdString(synopsis)));
                obj->m_pTableWidget->setItem(i + 1, 2, new QTableWidgetItem(QString::number(rating)));
                obj->m_pTableWidget->setItem(i + 1, 3, new QTableWidgetItem(QString::number(size) + tr(" MB")));
                obj->m_pTableWidget->setItem(i + 1, 4, new QTableWidgetItem(QString::number(price) + " DCT"));
                
                std::string s_time;
                for(int i = 0; i < time.find("T"); ++i)
                {
                    s_time.push_back(time[i]);
                }
                obj->m_pTableWidget->setItem(i + 1, 5, new QTableWidgetItem(QString::fromStdString(s_time)));
                
                s_time = "";
                for(int i = 0; i < expiration_or_delivery_time.find("T"); ++i)
                {
                    s_time.push_back(expiration_or_delivery_time[i]);
                }
                obj->m_pTableWidget->setItem(i + 1, 6, new QTableWidgetItem(QString::fromStdString(s_time)));
                
                for(int j = 1; j <= 6; ++j)
                {
                    obj->m_pTableWidget->item(i + 1, j)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
                }

            }
            
            
        } catch (std::exception& ex) {
            std::cout << ex.what() << std::endl;
        }
    });
    
    
}


PurchasedTab::~PurchasedTab()
{
    m_main_layout.removeWidget(m_pTableWidget);
    delete m_pTableWidget;
}


void PurchasedTab::PrepareTableWidgetHeaderGUI()
{
    m_pTableWidget->horizontalHeader()->setDefaultSectionSize(300);
    m_pTableWidget->setRowHeight(0,35);
    m_pTableWidget->horizontalHeader()->hide();
    m_pTableWidget->verticalHeader()->hide();
    QFont f( "Open Sans Bold", 14, QFont::Bold);
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
void PurchasedTab::DigContCallback(_NEEDED_ARGS2_)
{
    __DEBUG_APP2__(3,"clbdata=%p, act=%d, pDigCont=%p\n",a_clb_data,a_act,a_pDigContent);
    emit ShowDetailsOnDigContentSig(*a_pDigContent);
}


void PurchasedTab::ArrangeSize()
{
    QSize tqsTableSize = m_pTableWidget->size();
    int nSizeForOne = tqsTableSize.width()/(DCF_PURCHASE::NUM_OF_DIG_CONT_FIELDS)-1;
    for(int i(0); i<DCF_PURCHASE::NUM_OF_DIG_CONT_FIELDS;++i){
        m_pTableWidget->setColumnWidth(i,nSizeForOne);
    }

    m_pTableWidget->setStyleSheet("QTableView{border : 1px solid lightGray}");
    m_pTableWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_pTableWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    m_pTableWidget->setColumnWidth(0, (tqsTableSize.width()*10)/100);
    for(int i = 1; i < 7; ++i)
    {
        m_pTableWidget->setColumnWidth(i,(tqsTableSize.width()*15)/100);
    }
}


void PurchasedTab::resizeEvent ( QResizeEvent * a_event )
{
    QWidget::resizeEvent(a_event);
    ArrangeSize();
}
