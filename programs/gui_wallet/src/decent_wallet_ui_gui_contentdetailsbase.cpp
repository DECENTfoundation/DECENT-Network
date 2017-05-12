/*
 *	File: decent_wallet_ui_gui_contentdetailsbase.cpp
 *
 *	Created on: 22 Feb 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
#include "stdafx.h"

#include "gui_design.hpp"
#include "decent_wallet_ui_gui_contentdetailsbase.hpp"
#include "decent_text_edit.hpp"

#ifndef _MSC_VER
#include <QDateTime>
#endif

#include "gui_wallet_global.hpp"
#include "gui_wallet_mainwindow.hpp"

#ifndef _MSC_VER
#include "json.hpp"
#include <QFrame>
#include <QObject>
#include <graphene/chain/content_object.hpp>
#endif

using namespace nlohmann;
using namespace gui_wallet;


ContentDetailsBase::ContentDetailsBase(QWidget* pParent)
: QDialog(pParent)
, m_desc (new DecentTextEdit(this, DecentTextEdit::Info))
{
   s_vcpcFieldsGeneral.push_back(tr("Author"));
   s_vcpcFieldsGeneral.push_back(tr("Expiration"));
   s_vcpcFieldsGeneral.push_back(tr("Uploaded"));
   s_vcpcFieldsGeneral.push_back(tr("Amount"));
   s_vcpcFieldsGeneral.push_back(tr("Average Rating"));
   s_vcpcFieldsGeneral.push_back(tr("Size"));
   s_vcpcFieldsGeneral.push_back(tr("Times Bought"));
   
   s_vcpcFieldsBougth.push_back(tr("Author"));
   s_vcpcFieldsBougth.push_back(tr("Created"));
   s_vcpcFieldsBougth.push_back(tr("Amount"));
   s_vcpcFieldsBougth.push_back(tr("Average Rating"));
   s_vcpcFieldsBougth.push_back(tr("Size"));
   s_vcpcFieldsBougth.push_back(tr("Times Bought"));
   
#ifdef _MSC_VER
   int height = style()->pixelMetric(QStyle::PM_TitleBarHeight);
   setWindowIcon(height > 32 ? QIcon(":/icon/images/windows_decent_icon_32x32.png")
      : QIcon(":/icon/images/windows_decent_icon_16x16.png"));
#endif
}

// DCF stands for Digital Content Fields
namespace DCF{enum{AMOUNT=9, TIMES_BOUGHT=15};}

void ContentDetailsBase::execCDB(const SDigitalContent& a_cnt_details, bool bSilent/* = false*/)
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
        RunTask("get_rating \"" + Globals::instance().getCurrentUser() + "\" \"" + m_pContentInfo->URI + "\"", result);
        m_currentMyRating = QString::fromStdString(result).toInt(); // Returns 0 on fail so everything will work as intended
        
    } catch (...) {} // Ignore for now;


    
    if(a_cnt_details.type == DCT::BOUGHT) {
       
        if (stars_labels.size() == 0) {
            m_RateText = new QLabel;
            
            if (m_currentMyRating > 0) {
                m_RateText->setText( tr("You rated") + ":");
            } else {
                m_RateText->setText( tr("Please Rate") + ":");
            }
            m_RateText->setStyleSheet(m_RateText_design);
            
            QPixmap green_star(green_star_image);
            QPixmap white_star(white_star_image);
            
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
       
       /*std::string comment_result;
       try {
          RunTask("get_rating_and_comment \"" + Globals::instance().getCurrentUser() + "\" \"" + m_pContentInfo->URI + "\"", comment_result);

       } catch (...) { }
    
       auto c_result = json::parse(comment_result);*/

       nlohmann::json c_result;

       try
       {
          c_result = Globals::instance().runTaskParse("search_feedback "
                                                         "\"" + Globals::instance().getCurrentUser() + "\" "
                                                         "\"" + m_pContentInfo->URI + "\" "
                                                         "\"" "\" "   // iterator id
                                                         "1");
       }
       catch(...) {}
       
       QHBoxLayout* comment_status = new QHBoxLayout;
       QLabel*    m_commentOrRate_Text = new QLabel;

       m_commentOrRate_Text->setStyleSheet(d_m_comment_label_text);
       m_commentOrRate_Text->setFixedHeight(30);
       
       m_comment = new DecentTextEdit(this);
       m_comment->setStyleSheet(d_m_comment);
       m_comment->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
       
       comment_status->addWidget(m_commentOrRate_Text);
       comment_status->setAlignment(Qt::AlignRight);
       comment_status->setContentsMargins(0, 0, 45, 1);
       
       m_main_layout.addLayout(comment_status);
       m_main_layout.addWidget(m_comment);
       
       if ( c_result.empty() )
       {
          m_commentOrRate_Text->setText(tr("You can comment with your opinion on this item"));
          m_comment->setPlaceholderText(tr("Comment here..."));
          
          QHBoxLayout* button = new QHBoxLayout;
          button->setAlignment(Qt::AlignCenter);
          
          DecentButton* leave_feedback_button = new DecentButton(this);
          leave_feedback_button->setText(tr("Leave feedback"));
          leave_feedback_button->setFixedHeight(40);
          leave_feedback_button->setFixedWidth(130);
          
          button->setContentsMargins(10, 10, 10, 10);
          button->addWidget(leave_feedback_button);
          
          m_main_layout.addLayout(button);
          QObject::connect(leave_feedback_button, &QPushButton::clicked,
                           this, &ContentDetailsBase::LeaveComment);
       }
       else
       {
          std::string str_feedback;

          for (size_t iIndex = 0; iIndex < c_result.size(); ++iIndex)
          {
             auto const& rating_item = c_result[iIndex];
             str_feedback += "Author - ";
             str_feedback += rating_item["author"].get<std::string>();
             str_feedback += "\n";

             str_feedback += "Comment - ";
             str_feedback += rating_item["comment"].get<std::string>();
             str_feedback += "\n";

             str_feedback += "Rating - ";
             str_feedback += std::to_string(rating_item["rating"].get<uint8_t>());
             str_feedback += "\n";
          }
          m_commentOrRate_Text->setText(tr("You have already commented"));
          m_comment->setText( QString::fromStdString(str_feedback) );
          m_comment->setReadOnly(true);
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
   
    int i,nIndexZuyg(0);
    
    if (a_cnt_details.type == DCT::GENERAL)
    {
       m_commentWidget = new CommentWidget(this, m_pContentInfo);
       m_main_layout.addWidget(m_commentWidget);
       for(i = 0; i < NUMBER_OF_SUB_LAYOUTS2;++i,nIndexZuyg+=2)
       {
          m_vLabels[nIndexZuyg].setText(s_vcpcFieldsGeneral[i]);
       }
    }
   else
   {
      for(i = 0; i < NUMBER_OF_SUB_LAYOUTS2 - 1;++i,nIndexZuyg+=2)
      {
         m_vLabels[nIndexZuyg].setText(s_vcpcFieldsBougth[i]);
      }
   }
   
   setLayout(&m_main_layout);

    std::string e_str = "";
    if (a_cnt_details.type == DCT::BOUGHT || a_cnt_details.type == DCT::WAITING_DELIVERY) {
        e_str = (m_pContentInfo->expiration);
    }
    else
    {
        QDateTime time = QDateTime::fromString(QString::fromStdString(m_pContentInfo->expiration), "yyyy-MM-ddTHH:mm:ss");
        e_str = CalculateRemainingTime(QDateTime::currentDateTime(), time);
    }
    
    m_vLabels[1].setText(m_pContentInfo->author.c_str());
    
    if (a_cnt_details.type == DCT::GENERAL)
    {
        m_vLabels[3].setText(QString::fromStdString(e_str));
        m_vLabels[5].setText(tr(m_pContentInfo->created.c_str()));
        
        QString str_price = a_cnt_details.price.getString().c_str();
        m_vLabels[7].setText(str_price);
        
        QPixmap green_star(green_star_image);
        QPixmap white_star(white_star_image);
        
        white_star = white_star.scaled(QSize(20,20));
        green_star = green_star.scaled(QSize(20,20));
        
        m_vLabels[9].setText(QString::number(m_pContentInfo->AVG_rating));
        
        for(int i = 0; i < m_pContentInfo->AVG_rating; ++i) {
            m_stars[i].setPixmap(green_star);
        }
        
        for(int i = m_pContentInfo->AVG_rating; i < 5; ++i) {
            m_stars[i].setPixmap(white_star);
        }
        
        QString qsSizeTxt = QString::number(m_pContentInfo->size) + " MB";
        m_vLabels[11].setText(qsSizeTxt);
        
        m_vLabels[13].setText(QString::number(a_cnt_details.times_bought));
    }
    else
    {
        m_vLabels[3].setText(m_pContentInfo->author.c_str());
        
        QString str_price = a_cnt_details.price.getString().c_str();
        m_vLabels[5].setText(str_price);
        
        QPixmap green_star(green_star_image);
        QPixmap white_star(white_star_image);
        
        white_star = white_star.scaled(QSize(20,20));
        green_star = green_star.scaled(QSize(20,20));
        
        m_vLabels[7].setText(QString::number(m_pContentInfo->AVG_rating));
        
        for(int i = 0; i < m_pContentInfo->AVG_rating; ++i) {
            m_stars[i].setPixmap(green_star);
        }
        
        for(int i = m_pContentInfo->AVG_rating; i < 5; ++i) {
            m_stars[i].setPixmap(white_star);
        }
        
        QString qsSizeTxt = QString::number(m_pContentInfo->size) + " MB";
        m_vLabels[9].setText(qsSizeTxt);
        
        m_vLabels[11].setText(QString::number(a_cnt_details.times_bought));
    }
   
    std::string synopsis = m_pContentInfo->synopsis;
    std::string title;
    std::string desc;

    graphene::chain::ContentObjectPropertyManager synopsis_parser(synopsis);
    title = synopsis_parser.get<graphene::chain::ContentObjectTitle>();
    desc = synopsis_parser.get<graphene::chain::ContentObjectDescription>();

    this->setWindowTitle(QString::fromStdString(title));
    m_desc->setText(m_desc->toPlainText() + QString::fromStdString(desc) + "\n");

   if (false == bSilent)
      QDialog::exec();
}

void ContentDetailsBase::LeaveComment()
{
   if ( m_currentMyRating < 0 ){
      return;
   }
   
   std::string leave_result;
   try {
      RunTask("leave_rating_and_comment "
              "\"" + Globals::instance().getCurrentUser() + "\" "
              "\"" + m_pContentInfo->URI + "\" "
              "\"" + std::to_string(m_currentMyRating) + "\" "
              "\"" + escape_string(m_comment->toPlainText().toStdString()) + "\" "
              "\"true\"", leave_result);

   }catch (...) {}
}

void ContentDetailsBase::MouseClickedStar(int index) {
//    if (m_currentMyRating > 0)
//        return;
//    std::string result;
//    try {
//        RunTask("leave_rating \"" + Globals::instance().getCurrentUser() + "\" \"" + m_pContentInfo->URI + "\" " + std::to_string(index + 1) + " true", result);
//        } catch (...) {} // Ignore for now;
   m_currentMyRating = (index + 1);
   
   for (int i = m_currentMyRating; i < 5; ++i) {
      stars_labels[i]->setCheckState(Qt::Unchecked);
   }
   for (int i = 0; i < m_currentMyRating; ++i) {
      stars_labels[i]->setCheckState(Qt::Checked);
   }

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
        if(i%2==0){m_vSub_Widgets[i].setStyleSheet(bg_color_grey);}
        //else{m_vSub_Widgets[i].setStyleSheet(bg_color_wgite);}
        m_vLabels[nIndexKent].setStyleSheet(font_bold);
        m_vLabels[nIndexKent].setContentsMargins(0, 17, 50, 17);
        m_vLabels[nIndexKent].setAlignment(Qt::AlignRight);
#ifdef WINDOWS_HIGH_DPI
        m_vLabels[nIndexKent].setFixedHeight(70);
        m_vLabels[nIndexZuyg].setFixedHeight(70);
#else
        m_vLabels[nIndexKent].setFixedHeight(54);
        m_vLabels[nIndexZuyg].setFixedHeight(54);
#endif
        m_vSub_layouts[i].setSpacing(0);
        m_vSub_layouts[i].setContentsMargins(45,0,0,0);
        
        if(nIndexKent == row_star)
        {
            QHBoxLayout* text_layout = new  QHBoxLayout;
            QHBoxLayout* stars = new QHBoxLayout;
            QHBoxLayout* main_layout = new QHBoxLayout;
            
            stars->setContentsMargins(270, 0, 50, 0);
            text_layout->addWidget(&m_vLabels[nIndexZuyg]);
            stars->addWidget(&m_vLabels[nIndexKent]);
            m_vLabels[nIndexKent].setContentsMargins(0, 19, 10, 19);
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
    if(row_star == 7)
    {
       line = new QFrame(this);
       line->setFrameShape(QFrame::HLine); // Horizontal line
   
       line->setLineWidth(300);
       line->setStyleSheet(col_grey);
       line->setFixedHeight(1);
       m_main_layout.addWidget(line);
    }

   
    QHBoxLayout* desc_lay = new QHBoxLayout();

    m_desc->setText(tr("Description") + "\n\n");
    m_desc->setReadOnly(true);
    m_desc->setFont(DescriptionDetailsFont());
   
    desc_lay->setContentsMargins(42, 17, 0, 3);
    desc_lay->addWidget(m_desc);
   
    m_main_layout.addLayout(desc_lay);
   
    line = new QFrame(this);
    line->setFrameShape(QFrame::HLine); // Horizontal line
    
    line->setLineWidth(300);
    line->setStyleSheet(col_grey);
    line->setFixedHeight(1);
    m_main_layout.addWidget(line);
    
    //setStyleSheet(d_qdialog);
}

void ContentDetailsBase::commentWidgetSlot()
{
   m_commentWidget->setVisible(!m_commentWidget->isVisible());
}

/* /////////////Comment Widget////////////////*/
CommentWidget::CommentWidget(QWidget* parent, const SDigitalContent* content_info)
:  QWidget(parent),
   m_comment_count(1),
   m_content_uri(content_info->URI)
{
   update_run_task();
}

void CommentWidget::update_run_task()
{
   nlohmann::json comment;
   try
   {
      comment = Globals::instance().runTaskParse("search_feedback "
                                                 "\"" /*    empty user    */"\" "
                                                 "\"" + m_content_uri + "\" "
                                                 "\"" + next_iterator()   + "\" " +
                                                 std::to_string(m_comment_count + 1) );
   }catch(...){}
   
   if(comment.size() > m_comment_count){
      set_next_comment(comment[m_comment_count]["id"].get<std::string>());
   }else{
      set_next_comment(std::string());
   }

   QVBoxLayout* main_lay    = new QVBoxLayout();
   QHBoxLayout* comment_lay = new QHBoxLayout();
   QHBoxLayout* buttons_lay = new QHBoxLayout();
   
   DecentButton* next     = new DecentButton(this);
   DecentButton* previous = new DecentButton(this);
   DecentButton* reset    = new DecentButton(this);
   
   next->setText(tr("Next"));
   previous->setText(tr("Previous"));
   reset->setText(tr("First"));
   
   auto result = comment[0];
   
   QLabel* r_user = new QLabel;
   QLabel* r_comment = new QLabel;
   
   r_user->setText( QString::fromStdString(result["author"].get<std::string>()) );
   r_comment->setText( QString::fromStdString(result["comment"].get<std::string>()) );
   
   comment_lay->addWidget(r_user);
   comment_lay->addWidget(r_comment);
   
   buttons_lay->addWidget(next);
   buttons_lay->addWidget(previous);
   buttons_lay->addWidget(reset);
   buttons_lay->setAlignment(Qt::AlignBottom);
   
   connect(next, SIGNAL(clicked()), this, SLOT(nextButtonSlot()));
   connect(previous, SIGNAL(clicked()), this, SLOT(previousButtonSlot()));
   connect(reset, SIGNAL(clicked()), this, SLOT(resetButtonSlot()));
   
   main_lay->addLayout(comment_lay);
   main_lay->addLayout(buttons_lay);
   setLayout(main_lay);
}

bool CommentWidget::next()
{
   if( is_last() )
   {
      return false;
   }
   m_iterators.push_back(m_next_itr);
   update_run_task();
   return true;
}

bool CommentWidget::previous()
{
   if( is_last() )
   {
      return false;
   }
   
   m_iterators.pop_back();
   update_run_task();
   return true;
}

void CommentWidget::reset()
{
   m_iterators.clear();
   m_next_itr.clear();
   update_run_task();
}

bool CommentWidget::is_first() const
{
   return m_iterators.empty();
}

bool CommentWidget::is_last() const
{
   return m_next_itr.empty();
}

void CommentWidget::set_next_comment(std::string const& last)
{
   m_next_itr = last;
}

std::string CommentWidget::next_iterator()
{
   if( !m_iterators.empty() )
   {
      return m_iterators.back();
   }
   
   return std::string();
}

void CommentWidget::controller()
{
   
}

void CommentWidget::nextButtonSlot()
{
   next();
   controller();
}

void CommentWidget::previousButtonSlot()
{
   previous();
   controller();
}

void CommentWidget::resetButtonSlot()
{
   reset();
   controller();
}

CommentWidget::~CommentWidget()
{
   
}


