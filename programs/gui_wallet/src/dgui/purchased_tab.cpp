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


static const char* s_vccpItemNames[]={" ", "Title", "Rating", "Size", "Price", "Created", "Status", "Progress"};

static const int   s_cnNumberOfCols = sizeof(s_vccpItemNames)/sizeof(const char*);


PurchasedTab::PurchasedTab()
        :
        m_pTableWidget(new QTableWidget(1,s_cnNumberOfCols))
{

    PrepareTableWidgetHeaderGUI();

    QHBoxLayout* search_lay = new QHBoxLayout();

    m_filterLineEditer.setPlaceholderText(QString("Enter the term to search in title and description"));
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
}



void PurchasedTab::maybeUpdateContent() {
    m_contentUpdateTimer.stop();
    updateContents();
    m_contentUpdateTimer.start();
}

void PurchasedTab::onTextChanged(const QString& text) {
    
    m_doUpdate = true;
}

void PurchasedTab::updateContents() {
    
    
    auto& global_instance = gui_wallet::GlobalEvents::instance();
    std::string str_current_username = global_instance.getCurrentUser();
    
    std::string a_result;
    RunTask("get_buying_history_objects_by_consumer_term "
               "\"" + str_current_username +"\" "
               "\"" + m_filterLineEditer.text().toStdString() +"\"", a_result);
    
    
    if(last_contents == a_result)
    {
        return;
    }
    
    
    try {
        auto contents = json::parse(a_result);
        last_contents = a_result;
        if (contents.size() + 1 != m_pTableWidget->rowCount()) {
          m_pTableWidget->setRowCount(1); //Remove everything but header
          m_pTableWidget->setRowCount(contents.size() + 1);
           
        }
       
        for (int i = 0; i < contents.size(); ++i) {
            
            auto content = contents[i];
            
            
            std::string time = contents[i]["expiration_time"].get<std::string>();
            
            std::string synopsis = unescape_string(contents[i]["synopsis"].get<std::string>());
            std::replace(synopsis.begin(), synopsis.end(), '\t', ' '); // JSON does not like tabs :(
            
            try {
                auto synopsis_parsed = json::parse(synopsis);
                synopsis = synopsis_parsed["title"].get<std::string>();
            } catch (...) {}
            
            double rating = contents[i]["rating"].get<double>() / 1000;
            uint64_t size = contents[i]["size"].get<int>();


            double price = 0;
            if (contents[i]["price"]["amount"].is_number()){
                price =  contents[i]["price"]["amount"].get<double>();
            } else {
                price =  std::stod(contents[i]["price"]["amount"].get<std::string>());
            }
            price /= GRAPHENE_BLOCKCHAIN_PRECISION;
            
            std::string expiration_or_delivery_time = contents[i]["expiration_or_delivery_time"].get<std::string>();
            std::string URI = contents[i]["URI"].get<std::string>();
            
            QLabel* imag_label = new QLabel();
            imag_label->setAlignment(Qt::AlignCenter);

            QPixmap image1(":/icon/images/info1.svg");
            
            
            SDigitalContent contentObject;
            std::string dcresult;
            RunTask("get_content \"" + URI + "\"", dcresult);
            
            auto dcontent_json = json::parse(dcresult);
            
            if (content["delivered"].get<bool>()) {
                contentObject.type = DCT::BOUGHT;
            } else {
                contentObject.type = DCT::WAITING_DELIVERY;
            }
            
            contentObject.author = dcontent_json["author"].get<std::string>();
            contentObject.price.asset_id = dcontent_json["price"]["asset_id"].get<std::string>();
            contentObject.synopsis = dcontent_json["synopsis"].get<std::string>();
            contentObject.URI = dcontent_json["URI"].get<std::string>();
            contentObject.created = dcontent_json["created"].get<std::string>();
            contentObject.expiration = dcontent_json["expiration"].get<std::string>();
            contentObject.size = dcontent_json["size"].get<int>();
            
            if (dcontent_json["times_bougth"].is_number()) {
                contentObject.times_bougth = dcontent_json["times_bougth"].get<int>();
            } else {
                contentObject.times_bougth = 0;
            }
            
            
            if (dcontent_json["price"]["amount"].is_number()){
                contentObject.price.amount =  dcontent_json["price"]["amount"].get<double>();
            } else {
                contentObject.price.amount =  std::stod(dcontent_json["price"]["amount"].get<std::string>());
            }
            
            contentObject.price.amount /= GRAPHENE_BLOCKCHAIN_PRECISION;
            contentObject.AVG_rating = dcontent_json["AVG_rating"].get<double>() / 1000;
        
            
            m_pTableWidget->setCellWidget(i + 1, 0, new TableWidgetItemW<QLabel>(contentObject, this, NULL, &PurchasedTab::DigContCallback, tr("")));
            ((QLabel*)m_pTableWidget->cellWidget(i+1,0))->setPixmap(image1);
            ((QLabel*)m_pTableWidget->cellWidget(i+1,0))->setAlignment(Qt::AlignCenter);

            
            
            m_pTableWidget->setItem(i + 1, 1, new QTableWidgetItem(QString::fromStdString(synopsis)));
            m_pTableWidget->setItem(i + 1, 2, new QTableWidgetItem(QString::number(rating)));
            m_pTableWidget->setItem(i + 1, 3, new QTableWidgetItem(QString::number(size) + tr(" MB")));
            m_pTableWidget->setItem(i + 1, 4, new QTableWidgetItem(QString::number(price) + " DCT"));
            
           

            std::string s_time;
            for(int i = 0; i < time.find("T"); ++i)
            {
                s_time.push_back(time[i]);
            }
            m_pTableWidget->setItem(i + 1, 5, new QTableWidgetItem(QString::fromStdString(s_time)));
            
            
            std::string download_status_str;
            RunTask("get_download_status \"" + gui_wallet::GlobalEvents::instance().getCurrentUser() + "\" \"" + URI + "\"", download_status_str);
            
            auto download_status = json::parse(download_status_str);

            
            int total_key_parts = download_status["total_key_parts"].get<int>();
            int received_key_parts  = download_status["received_key_parts"].get<int>();
            int total_download_bytes  = download_status["total_download_bytes"].get<int>();
            int received_download_bytes  = download_status["received_download_bytes"].get<int>();
            
            
            QString status_text = tr("Keys: ") + QString::number(received_key_parts) + "/" + QString::number(total_key_parts);
            
            if (!content["delivered"].get<bool>()) {
                status_text = "Waiting for delivery";
            } else {
                status_text = status_text + tr(" ") + QString::fromStdString(download_status["status_text"].get<std::string>());
            }

            m_pTableWidget->setItem(i + 1, 6, new QTableWidgetItem(status_text));
            
            if (total_key_parts == 0) {
                total_key_parts = 1;
            }
            
            if (total_download_bytes == 0) {
                total_download_bytes = 1;
            }
            
            
            double progress = (0.1 * received_key_parts) / total_key_parts + (0.9 * received_download_bytes) / total_download_bytes;
            progress *= 100; // Percent
            m_pTableWidget->setItem(i + 1, 7, new QTableWidgetItem(QString::number(progress) + "%"));

            
            for(int j = 1; j < s_cnNumberOfCols; ++j)
            {
                m_pTableWidget->item(i + 1, j)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
                m_pTableWidget->item(i + 1, j)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
            }
            

        }
        
        
    } catch (std::exception& ex) {
        std::cout << ex.what() << std::endl;
    }

    
    
}

void PurchasedTab::DigContCallback(_NEEDED_ARGS2_)
{
    emit ShowDetailsOnDigContentSig(*a_pDigContent);
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
    for( int i = 0; i<s_cnNumberOfCols; ++i )
    {

        m_pTableWidget->setItem(0,i,new QTableWidgetItem(tr(s_vccpItemNames[i])));
        m_pTableWidget->item(0,i)->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        m_pTableWidget->item(0,i)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        m_pTableWidget->item(0,i)->setBackground(QColor(228,227,228));
        m_pTableWidget->item(0,i)->setFont(f);
        m_pTableWidget->item(0,i)->setForeground(QColor::fromRgb(51,51,51));

    }
    m_pTableWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_pTableWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

}



void PurchasedTab::ArrangeSize()
{
    QSize tqsTableSize = m_pTableWidget->size();

    m_pTableWidget->setStyleSheet("QTableView{border : 1px solid lightGray}");
    m_pTableWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_pTableWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    m_pTableWidget->setColumnWidth(0, tqsTableSize.width() * 0.1);
    
    for(int i = 1; i < s_cnNumberOfCols; ++i)
    {
        m_pTableWidget->setColumnWidth(i, (0.9 * tqsTableSize.width()) / (s_cnNumberOfCols - 1) );
    }
}


void PurchasedTab::resizeEvent ( QResizeEvent * a_event )
{
    QWidget::resizeEvent(a_event);
    ArrangeSize();
}
