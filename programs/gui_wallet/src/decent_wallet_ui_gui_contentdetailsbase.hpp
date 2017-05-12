/*
 *	File: decent_wallet_ui_gui_contentdetailsbase.hpp
 *
 *	Created on: 22 Feb 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
#ifndef DECENT_WALLET_UI_GUI_CONTENTDETAILSBASE_HPP
#define DECENT_WALLET_UI_GUI_CONTENTDETAILSBASE_HPP

#define NUMBER_OF_SUB_LAYOUTS2   7

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

#include "decent_wallet_ui_gui_newcheckbox.hpp"

namespace gui_wallet {

struct SDigitalContent;
class DecentTextEdit;

   
/// for content comments
class CommentWidget : public QWidget
{
   Q_OBJECT
public:
   CommentWidget(QWidget*, const SDigitalContent*);
   ~CommentWidget();
   
public:
   bool        next();
   bool        previous();
   void        reset();
   bool        is_last() const;
   bool        is_first() const;
   void        update_run_task();
   void        set_next_comment(std::string const&);
   void        controller();
   std::string next_iterator();
   
public slots:
   void nextButtonSlot();
   void previousButtonSlot();
   void resetButtonSlot();
   
private:
   std::string                m_last_result;
   std::string                m_next_itr;
   std::vector<std::string>   m_iterators;
   size_t                     m_comment_count;
   std::string                m_content_uri;
};
/////// *****/////
   
   
class ContentDetailsBase : public QDialog
{
    Q_OBJECT
public:
    ContentDetailsBase(QWidget* pParent);
    void execCDB(const SDigitalContent& a_cnt_details, bool bSilent = false);
    void popup_for_purchased(int);
    bool empty_comment_or_rating();

    //virtual void execCDD(const decent::wallet::ui::gui::SDigitalContent& a_cnt_details)=0;

private slots:
    void LeaveComment();
    void commentWidgetSlot();
    void MouseEnteredStar(int index) {
        if (m_currentMyRating > 0)
            return;
        
        for (int i = 0; i <= index; ++i) {
            stars_labels[i]->setCheckState(Qt::Checked);
        }
        for (int i = index + 1; i < 5; ++i) {
            stars_labels[i]->setCheckState(Qt::Unchecked);
        }
        
    }
    void MouseLeftStar(int index) {
        if (m_currentMyRating > 0)
            return;
        
        for (int i = 0; i < m_currentMyRating; ++i) {
            stars_labels[i]->setCheckState(Qt::Checked);
        }
        
        
        for (int i = m_currentMyRating; i < 5; ++i) {
            stars_labels[i]->setCheckState(Qt::Unchecked);
        }
        
        
    }
    
    
    void MouseClickedStar(int index);
    
    
    
    
protected:
    const SDigitalContent* m_pContentInfo;
    QVBoxLayout     m_main_layout;
    QHBoxLayout     m_free_for_child;
    QWidget         m_vSub_Widgets[NUMBER_OF_SUB_LAYOUTS2];
    QHBoxLayout     m_vSub_layouts[NUMBER_OF_SUB_LAYOUTS2];
    QLabel          m_vLabels[NUMBER_OF_SUB_LAYOUTS2*2];
    QLabel          m_stars[5];
    int             m_currentMyRating = 0;
    std::vector<NewCheckBox*> stars_labels;
    QLabel*         m_RateText;
    DecentTextEdit* m_desc;
    DecentTextEdit* m_comment;
    CommentWidget*  m_commentWidget;

public:
    std::vector<QString> s_vcpcFieldsGeneral;
    std::vector<QString> s_vcpcFieldsBougth;
};

}

#endif // DECENT_WALLET_UI_GUI_CONTENTDETAILSBASE_HPP
