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
#include <QTextEdit>
#include "decent_wallet_ui_gui_newcheckbox.hpp"


namespace gui_wallet {

class SDigitalContent;

class ContentDetailsBase : public QDialog
{
    Q_OBJECT
    
public:
    ContentDetailsBase(QWidget* pParent);
    void execCDB(const SDigitalContent& a_cnt_details, bool bSilent = false);
    void popup_for_purchased(int);

    //virtual void execCDD(const decent::wallet::ui::gui::SDigitalContent& a_cnt_details)=0;

private slots:
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
    QTextEdit       desc_text;
    int             m_currentMyRating = 0;
    std::vector<NewCheckBox*> stars_labels;
    QLabel*         m_RateText;
    QTextEdit       m_desc;
public:
    std::vector<QString> s_vcpcFieldsGeneral;
    std::vector<QString> s_vcpcFieldsBougth;
};

}

#endif // DECENT_WALLET_UI_GUI_CONTENTDETAILSBASE_HPP
