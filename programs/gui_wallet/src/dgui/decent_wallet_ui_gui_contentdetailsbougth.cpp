/*
 *	File: decent_wallet_ui_gui_contentdetails_bougth.cpp
 *
 *	Created on: 22 Feb 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */

#include "decent_wallet_ui_gui_contentdetailsbougth.hpp"


void SetNewTaskQtMainWnd2Glb(const std::string& a_inp_line, void* a_clbData);
void SetNewTaskQtMainWnd3Glb(const std::string& a_inp_line, void* a_clbData);

decent::wallet::ui::gui::ContentDetailsBougth::ContentDetailsBougth()
{
    m_rate_layout_right.setMargin(1);
    m_rate_layout_right.setContentsMargins(3,0,20,0);
    for(int i(0);i<MAX_RATE_VALUE;++i)
    {
        //setScaledContents
        m_vRate_check_boxes[i].SetIndex(i);
        m_rate_layout_right.addWidget(&m_vRate_check_boxes[i]);
        connect(&m_vRate_check_boxes[i],SIGNAL(StateChangedNewSignal(int,int)),
                this,SLOT(RateContentSlot(int,int)));
    }

    m_asterix_widget.setLayout(&m_rate_layout_right);
    m_asterix_widget.setStyleSheet("background-color:white;");
    m_asterix_widget.setFixedSize(m_asterix_widget.sizeHint().width(),36);

    m_rate_layout_left.setContentsMargins(0,0,20,0);
    m_rate_layout_left.addWidget(&m_RateText);
    m_rate_layout_all.addLayout(&m_rate_layout_left);
    m_rate_layout_all.addWidget(&m_asterix_widget);
    m_free_for_child.addLayout(&m_rate_layout_all);

    m_RateText.setText( tr("Please Rate:"));
    m_RateText.setStyleSheet("color:green;" "background-color:white;" "font-weight: bold");
    //setStyleSheet("font-weight: bold");
    m_RateText.setAlignment(Qt::AlignVCenter|Qt::AlignRight);
    setFixedSize(397,381);
}


decent::wallet::ui::gui::ContentDetailsBougth::~ContentDetailsBougth()
{
}


void decent::wallet::ui::gui::ContentDetailsBougth::execCDD(
        const QString& a_user_name,
        const decent::wallet::ui::gui::SDigitalContent& a_cnt_details)
{
    m_user_name = a_user_name;
    execCDB(a_cnt_details);
}


void decent::wallet::ui::gui::ContentDetailsBougth::RateContentSlot(int a_nSelected, int a_nIndex)
{

    int nContinue = 1;
    uint64_t ullnRating((uint64_t)a_nIndex);

    switch(a_nSelected)
    {
    case Qt::Checked:
        if(a_nIndex>0)
        {
            if(m_vRate_check_boxes[a_nIndex-1].checkState() != Qt::Checked)
            {
                m_vRate_check_boxes[a_nIndex].setCheckState(Qt::Unchecked);
                nContinue = 0;
            }
        }
        ++ullnRating;
        break;
    case Qt::Unchecked:
        if(a_nIndex<(MAX_RATE_VALUE-1))
        {
            if(m_vRate_check_boxes[a_nIndex+1].checkState() != Qt::Unchecked)
            {
                m_vRate_check_boxes[a_nIndex].setCheckState(Qt::Checked);
                nContinue = 0;
            }
        }
        break;
    default:
        nContinue = 0;
        break;
    }



    QString qsRatingStr =
            tr("leave_rating ") +
            m_user_name + tr(" \"") +
            tr(m_pContentInfo->URI.c_str()) + tr("\" ") +
            QString::number(ullnRating,10) + tr(" true");

    std::string inp_str = qsRatingStr.toStdString();
#if 1
    if(!nContinue){return;}
    SetNewTaskQtMainWnd2Glb(inp_str,NULL);
#endif

    __DEBUG_APP2__(0,"selected=%d, index=%d, continue=%d, rating=%ld, str=\"%s\"",
                   a_nSelected,a_nIndex,nContinue,(long)ullnRating,inp_str.c_str());

}
