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
#include "ui_wallet_functions.hpp"
#include "gui_wallet_mainwindow.hpp"
#include "json.hpp"
#include <QFrame>
using namespace nlohmann;
using namespace gui_wallet;

static const char* s_vcpcFieldsGeneral[NUMBER_OF_SUB_LAYOUTS2] = {
    "Author", "Expiration","Uploaded","Amount",
    "Average Rating","Size","Times Bought" 
};


static const char* s_vcpcFieldsBougth[NUMBER_OF_SUB_LAYOUTS2 - 1] = {
    "Author","Created","Amount",
    "Average Rating","Size","Times Bought"
};

typedef const char* TypeCpcChar;
typedef TypeCpcChar* NewType;

static NewType  s_vFields[]={ s_vcpcFieldsGeneral, s_vcpcFieldsBougth, s_vcpcFieldsBougth };

ContentDetailsBase::ContentDetailsBase(Mainwindow_gui_wallet* pMainWindow)
: m_pMainWindow(pMainWindow)
{}




// DCF stands for Digital Content Fields
namespace DCF{enum{AMOUNT=9, TIMES_BOUGHT=15};}

void ContentDetailsBase::execCDB(const SDigitalContent& a_cnt_details)
{
    
    
    m_pContentInfo = &a_cnt_details;
    m_currentMyRating = 0;
    
    if (a_cnt_details.type == DCT::GENERAL)
    {
        popup_for_purchased(9);
    }
    else
    {
        popup_for_purchased(7);
    }
    
    std::string result;
    try {
        RunTask("get_rating \"" + GlobalEvents::instance().getCurrentUser() + "\" \"" + m_pContentInfo->URI + "\"", result);
        m_currentMyRating = QString::fromStdString(result).toInt(); // Returns 0 on fail so everything will work as intended
        
    } catch (...) {} // Ignore for now;


    
//    if(a_cnt_details.type == DCT::BOUGHT) {m_main_layout.removeWidget(&m_vSub_Widgets[4]);
    if(a_cnt_details.type == DCT::BOUGHT) {
        
        if (stars_labels.size() == 0) {
            m_RateText = new QLabel;
            
            if (m_currentMyRating > 0) {
                m_RateText->setText( tr("You rated:"));
            } else {
                m_RateText->setText( tr("Please Rate:"));
            }
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
            
        } else {
            
            m_RateText->setVisible(true);
            for(int i = 0; i < stars_labels.size(); ++i) {
                
                stars_labels[i]->setVisible(true);
            }
        }
        
        
        
        
        if (m_currentMyRating > 0) { // To show stars when opened
            for (int i = 0; i < m_currentMyRating; ++i) {
                stars_labels[i]->setCheckState(Qt::Checked);
            }
            for (int i = m_currentMyRating; i < 5; ++i) {
                stars_labels[i]->setCheckState(Qt::Unchecked);
            }
        }
        
        
    }
    
    if(a_cnt_details.type == DCT::WAITING_DELIVERY) {
        if (stars_labels.size() != 0) {
            m_RateText->setVisible(false);
            for(int i = 0; i < stars_labels.size(); ++i) {
                
                stars_labels[i]->setVisible(false);
            }
        }
    }
    
    if (a_cnt_details.type == DCT::GENERAL) {
        m_main_layout.addLayout(&m_free_for_child);
    }

    
    setLayout(&m_main_layout);
    
    int i,nIndexZuyg(0);
    
    NewType vNames = s_vFields[a_cnt_details.type];
    
    for(i = 0; i < NUMBER_OF_SUB_LAYOUTS2;++i,nIndexZuyg+=2)
    {
       m_vLabels[nIndexZuyg].setText(tr(vNames[i]));
    }
    
    std::string e_str = "";
    if (a_cnt_details.type == DCT::BOUGHT || a_cnt_details.type == DCT::WAITING_DELIVERY) {
        e_str = (m_pContentInfo->expiration);
    }
    else
    {
        QDateTime time = QDateTime::fromString(QString::fromStdString(m_pContentInfo->expiration), "yyyy-MM-ddTHH:mm:ss");
        e_str = CalculateRemainingTime(QDateTime::currentDateTime(), time);
    }
    
    m_vLabels[1].setText(tr(m_pContentInfo->author.c_str()));
    std::string creat;
    for(int i = 0; i < m_pContentInfo->created.find("T"); ++i)
    {
        creat.push_back(m_pContentInfo->created[i]);
    }
    
    if (a_cnt_details.type == DCT::GENERAL)
    {
        m_vLabels[3].setText(QString::fromStdString(e_str));
        m_vLabels[5].setText(tr(creat.c_str()));
        
        QString str_price = QString::number(a_cnt_details.price.amount) + " DCT";
        m_vLabels[7].setText(str_price);
        
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
    }
    else
    {
        m_vLabels[3].setText(tr(creat.c_str()));
        
        QString str_price = QString::number(a_cnt_details.price.amount) + " DCT";
        m_vLabels[5].setText(str_price);
        
        QPixmap green_star(":/icon/images/green_asterix.png");
        QPixmap white_star(":/icon/images/white_asterix.png");
        
        white_star = white_star.scaled(QSize(20,20));
        green_star = green_star.scaled(QSize(20,20));
        
        m_vLabels[7].setText(QString::number(m_pContentInfo->AVG_rating));
        
        for(int i = 0; i < m_pContentInfo->AVG_rating; ++i) {
            m_stars[i].setPixmap(green_star);
        }
        
        for(int i = m_pContentInfo->AVG_rating; i < 5; ++i) {
            m_stars[i].setPixmap(white_star);
        }
        
        QString qsSizeTxt = QString::number(m_pContentInfo->size) + tr(" MB");
        m_vLabels[9].setText(qsSizeTxt);
        
        m_vLabels[11].setText(QString::number(a_cnt_details.times_bougth));
    }

    
    
    
    
    
    
   
    std::string synopsis = m_pContentInfo->synopsis;
    std::string desc = "";
    try {
        auto synopsis_parsed = json::parse(m_pContentInfo->synopsis);
        synopsis = synopsis_parsed["title"].get<std::string>();
        desc = synopsis_parsed["description"].get<std::string>();
        
    } catch (...) {}
    this->setWindowTitle(QString::fromStdString(synopsis));
    m_desc.setText(m_desc.toPlainText() + QString::fromStdString(desc));
    
    setFixedSize(620,400);

    QDialog::exec();
}


void ContentDetailsBase::MouseClickedStar(int index) {
    if (m_currentMyRating > 0)
        return;
    
    std::string result;
    try {
        RunTask("leave_rating \"" + GlobalEvents::instance().getCurrentUser() + "\" \"" + m_pContentInfo->URI + "\" " + std::to_string(index + 1) + " true", result);
        
        m_currentMyRating = (index + 1);
    } catch (...) {} // Ignore for now;
    
}




void ContentDetailsBase::popup_for_purchased(int row_star)
{
    int i, nIndexKent(1), nIndexZuyg(0);
    
    m_main_layout.setSpacing(0);
    m_main_layout.setContentsMargins(0,0,0,0);
    
    int row_count = NUMBER_OF_SUB_LAYOUTS2;
    if(row_star == 7) {row_count = 6;}

    
    for(i=0;i<row_count;++i,nIndexZuyg+=2,nIndexKent+=2)
    {
        if(i%2==0){m_vSub_Widgets[i].setStyleSheet("background-color:rgb(244,244,244);");}
        else{m_vSub_Widgets[i].setStyleSheet("background-color:white;");}
        m_vLabels[nIndexKent].setStyleSheet("font-weight: bold");
        m_vLabels[nIndexKent].setContentsMargins(0, 10, 50, 10);
        m_vLabels[nIndexKent].setAlignment(Qt::AlignRight);
        //m_vLabels[nIndexKent].setAlignment(Qt::AlignCenter);
        m_vSub_layouts[i].setSpacing(0);
        m_vSub_layouts[i].setContentsMargins(45,3,0,3);
        
        if(nIndexKent == row_star)
        {
            QHBoxLayout* text_layout = new  QHBoxLayout;
            QHBoxLayout* stars = new QHBoxLayout;
            QHBoxLayout* main_layout = new QHBoxLayout;
            
            stars->setContentsMargins(250, 10, 50, 10);
            text_layout->addWidget(&m_vLabels[nIndexZuyg]);
            stars->addWidget(&m_vLabels[nIndexKent]);
            for(int i = 0; i <5; ++i)
            {
                stars->addWidget(&m_stars[i]);
            }
            main_layout->addLayout(text_layout);
            main_layout->addLayout(stars);
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
    QFrame* line;
    
    line = new QFrame(this);
    line->setFrameShape(QFrame::HLine); // Horizontal line
    
    line->setLineWidth(300);
    line->setStyleSheet("color: rgb(193,192,193)");
    line->setFixedHeight(1);
    m_main_layout.addWidget(line);
    
    QHBoxLayout* desc_lay = new QHBoxLayout();
    m_desc.setText("Description\n");
    m_desc.setStyleSheet("border-top: 0px solid rgb(193,192,193); border-bottom: 0px solid rgb(193,192,193); border-left: 0px; border-right: 0px;");
    m_desc.setReadOnly(true);
    m_desc.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_desc.setMinimumHeight(20);
    m_desc.setMaximumHeight(50);
    desc_lay->setContentsMargins(45, 3, 0, 3);
    desc_lay->addWidget(&m_desc);
    
    
    
    m_main_layout.addLayout(desc_lay);
    
    line = new QFrame(this);
    line->setFrameShape(QFrame::HLine); // Horizontal line
    
    line->setLineWidth(300);
    line->setStyleSheet("color: rgb(193,192,193)");
    line->setFixedHeight(1);
    m_main_layout.addWidget(line);
    //m_main_layout.addWidget(&m_desc);
    
    setStyleSheet("background-color:white;");
}






