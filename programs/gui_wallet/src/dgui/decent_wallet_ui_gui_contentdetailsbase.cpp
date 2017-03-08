/*
 *	File: decent_wallet_ui_gui_contentdetailsbase.cpp
 *
 *	Created on: 22 Feb 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */

#include "decent_wallet_ui_gui_contentdetailsbase.hpp"
#include <QDateTime>
#include "gui_wallet_global.hpp"
#include "json.hpp"

using namespace nlohmann;
using namespace gui_wallet;

static const char* s_vcpcFieldsGeneral[NUMBER_OF_SUB_LAYOUTS2] = {
    "Author", "Expiration","Created","Price",
    "Averege Rating","Size","Times Bought", "Description"
};


static const char* s_vcpcFieldsBougth[NUMBER_OF_SUB_LAYOUTS2] = {
    "Author", "Expiration","Created","Price",
    "Averege Rating","Size","Times Bought", "Description"
};

typedef const char* TypeCpcChar;
typedef TypeCpcChar* NewType;

static NewType  s_vFields[]={s_vcpcFieldsGeneral,s_vcpcFieldsBougth};

ContentDetailsBase::ContentDetailsBase()
{
    int i, nIndexKent(1), nIndexZuyg(0);
    
    m_main_layout.setSpacing(0);
    m_main_layout.setContentsMargins(0,0,0,0);

    for(i=0;i<NUMBER_OF_SUB_LAYOUTS2;++i,nIndexZuyg+=2,nIndexKent+=2)
    {
        if(i%2==0){m_vSub_Widgets[i].setStyleSheet("background-color:rgb(244,244,244);");}
        else{m_vSub_Widgets[i].setStyleSheet("background-color:white;");}
        m_vLabels[nIndexZuyg].setStyleSheet("font-weight: bold");
        m_vSub_layouts[i].setSpacing(0);
        m_vSub_layouts[i].setContentsMargins(45,3,0,3);
        
        if(nIndexKent == 9)
        {
            QVBoxLayout* text_layout = new  QVBoxLayout;
            QHBoxLayout* stars = new QHBoxLayout;
            QHBoxLayout* main_layout = new QHBoxLayout;
            text_layout->addWidget(&m_vLabels[nIndexZuyg]);
            text_layout->addWidget(&m_vLabels[nIndexKent]);
            for(int i = 0; i <5; ++i)
            {
                stars->addWidget(&m_stars[i]);
            }
            main_layout->addLayout(text_layout);
            main_layout->addWidget(new QLabel());
            main_layout->addWidget(new QLabel());
            main_layout->addLayout(stars);
            main_layout->addWidget(new QLabel());
            m_vSub_layouts[i].addLayout(main_layout);
            m_vSub_Widgets[i].setLayout(&m_vSub_layouts[i]);
            m_main_layout.addWidget(&m_vSub_Widgets[i]);
        }
        else
        {
            m_vSub_layouts[i].addWidget(&m_vLabels[nIndexZuyg]);
            m_vSub_layouts[i].addWidget(&m_vLabels[nIndexKent]);
            m_vSub_Widgets[i].setLayout(&m_vSub_layouts[i]);
            m_main_layout.addWidget(&m_vSub_Widgets[i]);
        }
    }
    
    
    
    
    
    
    
   
    setStyleSheet("background-color:white;");
}

ContentDetailsBase::~ContentDetailsBase()
{
}

// DCF stands for Digital Content Fields
namespace DCF{enum{AMOUNT=9, TIMES_BOUGHT=15};}

void ContentDetailsBase::execCDB(const SDigitalContent& a_cnt_details)
{
    
    
    m_pContentInfo = &a_cnt_details;
    m_currentMyRating = 0;
    
    std::string result;
    try {
        RunTask("get_rating \"" + GlobalEvents::instance().getCurrentUser() + "\" \"" + m_pContentInfo->URI + "\"", result);
        m_currentMyRating = QString::fromStdString(result).toInt(); // Returns 0 on fail so everything will work as intended
        
    } catch (...) {} // Ignore for now;


    
    if(a_cnt_details.type == DCT::BOUGHT) {
        if (stars_labels.size() == 0) {
            QLabel* m_RateText = new QLabel;
            m_RateText->setText( tr("Please Rate:"));
            m_RateText->setStyleSheet("color:green;" "background-color:white;" "font-weight: bold");
            
            QPixmap green_star(":/icon/images/green_asterix.png");
            QPixmap white_star(":/icon/images/white_asterix.png");
            
            white_star = white_star.scaled(QSize(20,20));
            green_star = green_star.scaled(QSize(20,20));
            
            
            
            
            QHBoxLayout* stars = new QHBoxLayout;
            stars->addWidget(m_RateText);
            stars->setContentsMargins(250, 10, 20, 20);
            

            QHBoxLayout* stars_lay = new QHBoxLayout;
            for(int i = 0; i < 5; ++i)
            {

                stars_labels.push_back(new NewCheckBox());
                stars_labels.back()->SetIndex(i);
                stars_lay->addWidget(stars_labels[i]);
                
                connect(stars_labels.back(), SIGNAL(MouseEnteredSignal(int)), this, SLOT(MouseEnteredStar(int)));
                connect(stars_labels.back(), SIGNAL(MouseLeftSignal(int)), this, SLOT(MouseLeftStar(int)));
                connect(stars_labels.back(), SIGNAL(MouseClickedSignal(int)), this, SLOT(MouseClickedStar(int)));

            }
            stars->addLayout(stars_lay);
            m_main_layout.addLayout(stars);
            
        }
        
    } else {
        m_main_layout.addLayout(&m_free_for_child);
    }
    
    setLayout(&m_main_layout);
    
    int i,nIndexZuyg(0);
    
    NewType vNames = s_vFields[a_cnt_details.type];
    
    for(i=0;i<NUMBER_OF_SUB_LAYOUTS2;++i,nIndexZuyg+=2)
    {
        if((i == 1) && a_cnt_details.type == DCT::BOUGHT)
        {
            m_vLabels[nIndexZuyg].setText(tr("Purchased"));
        }
        else
        {
            m_vLabels[nIndexZuyg].setText(tr(vNames[i]));
        }
    }
    
    std::string e_str = "";
    if(a_cnt_details.type == DCT::BOUGHT)
    {
        e_str = std::to_string(m_pContentInfo->times_bougth);
    }
    else
    {
        QDateTime time = QDateTime::fromString(QString::fromStdString(m_pContentInfo->expiration), "yyyy-MM-ddTHH:mm:ss");
        e_str = CalculateRemainingTime(QDateTime::currentDateTime(), time);
    }
        
    m_vLabels[1].setText(tr(m_pContentInfo->author.c_str()));
    m_vLabels[3].setText(tr(e_str.c_str()));
    std::string creat;
    for(int i = 0; i < m_pContentInfo->created.find("T"); ++i)
    {
        creat.push_back(m_pContentInfo->created[i]);
    }
    m_vLabels[5].setText(tr(creat.c_str()));
    
    
    
    std::string str_price = std::to_string(a_cnt_details.price.amount) + " DCT";
    
    m_vLabels[7].setText(tr(str_price.c_str()));
    
    
    
    
    
    
    QPixmap green_star(":/icon/images/green_asterix.png");
    QPixmap white_star(":/icon/images/white_asterix.png");
    
    white_star = white_star.scaled(QSize(20,20));
    green_star = green_star.scaled(QSize(20,20));
    
    m_vLabels[9].setText(QString::number(m_pContentInfo->AVG_rating));
    
    for(int i = 0; i < m_pContentInfo->AVG_rating; ++i) {
        m_stars[i].setPixmap(green_star);
    }
    
    for(int i = m_pContentInfo->AVG_rating; i < 5; ++i) {
        m_stars[i].setPixmap(white_star);
    }

    QString qsSizeTxt = QString::number(m_pContentInfo->size) + tr(" MB");
    m_vLabels[11].setText(qsSizeTxt);
    m_vLabels[13].setText(QString::number(a_cnt_details.times_bougth));
    
   
    std::string synopsis = m_pContentInfo->synopsis;
    std::string desc = "";
    try {
        auto synopsis_parsed = json::parse(m_pContentInfo->synopsis);
        synopsis = synopsis_parsed["title"].get<std::string>();
        desc = synopsis_parsed["description"].get<std::string>();
        
    } catch (...) {}
    this->setWindowTitle(QString::fromStdString(synopsis));
    m_vLabels[15].setText(QString::fromStdString(desc));

    
    setFixedSize(620,400);

    QDialog::exec();
}


void ContentDetailsBase::MouseClickedStar(int index) {
    std::cout << "Rate: " << index << std::endl;
    std::string result;
    try {
        RunTask("leave_rating \"" + GlobalEvents::instance().getCurrentUser() + "\" \"" + m_pContentInfo->URI + "\" " + std::to_string(index + 1) + " true", result);
        
        m_currentMyRating = (index + 1);
    } catch (...) {} // Ignore for now;
    
}





