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


static const char* s_vcpcFieldsGeneral[NUMBER_OF_SUB_LAYOUTS2] = {
    "Author", "Expiration","Created","Price", "Amount","Asset ID",
    "Averege Rating","Size","Times Bought"
};


static const char* s_vcpcFieldsBougth[NUMBER_OF_SUB_LAYOUTS2] = {
    "Author", "Expiration","Created","Price", "Amount","Asset ID",
    "Averege Rating","Size","Times Bought"
};

typedef const char* TypeCpcChar;
typedef TypeCpcChar* NewType;

static NewType  s_vFields[]={s_vcpcFieldsGeneral,s_vcpcFieldsBougth};

decent::wallet::ui::gui::ContentDetailsBase::ContentDetailsBase()
{
    int i, nIndexKent(1), nIndexZuyg(0);

    m_main_layout.setSpacing(0);
    m_main_layout.setContentsMargins(0,0,0,0);
    m_main_layout.addLayout(&m_free_for_child);


    for(i=0;i<NUMBER_OF_SUB_LAYOUTS2;++i,nIndexZuyg+=2,nIndexKent+=2)
    {
        if(i%2==0){m_vSub_Widgets[i].setStyleSheet("background-color:lightGray;");}
        else{m_vSub_Widgets[i].setStyleSheet("background-color:white;");}
        m_vLabels[nIndexZuyg].setStyleSheet("font-weight: bold");
        m_vSub_layouts[i].setSpacing(0);
        m_vSub_layouts[i].setContentsMargins(45,3,0,3);
        m_vSub_layouts[i].addWidget(&m_vLabels[nIndexZuyg]);
        m_vSub_layouts[i].addWidget(&m_vLabels[nIndexKent]);
        m_vSub_Widgets[i].setLayout(&m_vSub_layouts[i]);
        m_main_layout.addWidget(&m_vSub_Widgets[i]);
    }
    setLayout(&m_main_layout);
    setStyleSheet("background-color:white;");
}

decent::wallet::ui::gui::ContentDetailsBase::~ContentDetailsBase()
{
}

// DCF stands for Digital Content Fields
namespace DCF{enum{AMOUNT=9,ASSET_ID=11,TIMES_BOUGHT=17};}

void decent::wallet::ui::gui::ContentDetailsBase::execCDB(const decent::wallet::ui::gui::SDigitalContent& a_cnt_details)
{
#if 1
    int i,nIndexZuyg(0);
    NewType vNames = s_vFields[a_cnt_details.type];
    m_pContentInfo = &a_cnt_details;

    for(i=0;i<NUMBER_OF_SUB_LAYOUTS2;++i,nIndexZuyg+=2)
    {
        m_vLabels[nIndexZuyg].setText(tr(vNames[i]));
    }

    m_vLabels[1].setText(tr(m_pContentInfo->author.c_str()));
    m_vLabels[3].setText(tr(m_pContentInfo->expiration.c_str()));
    m_vLabels[5].setText(tr(m_pContentInfo->created.c_str()));
    //m_vLabels[7].setText(QString::number(m_pContentInfo->price.amount,'f').remove( QRegExp("0+$") ).remove( QRegExp("\\.$") ));
    m_vLabels[7].setText(tr(m_pContentInfo->price.amount2.c_str()));
    m_vLabels[DCF::AMOUNT].setText(tr(m_pContentInfo->size2.c_str()));
    m_vLabels[DCF::ASSET_ID].setText(tr(a_cnt_details.price.asset_id.c_str()));
    m_vLabels[13].setText(tr(m_pContentInfo->AVG_rating2.c_str()));
    //QString::number(aTemporar.AVG_rating,'f').remove( QRegExp("0+$") ).remove( QRegExp("\\.$") )
    QString qsSizeTxt = tr(m_pContentInfo->size2.c_str()) + tr(" MB");
    m_vLabels[15].setText(qsSizeTxt);
    m_vLabels[DCF::TIMES_BOUGHT].setText(tr(a_cnt_details.times_bougth2.c_str()));

    QDialog::exec();
#endif
}
