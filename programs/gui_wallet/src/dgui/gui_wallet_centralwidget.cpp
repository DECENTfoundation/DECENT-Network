/*
 *	File: gui_wallet_centralwigdet.cpp
 *
 *	Created on: Nov 11, 2016
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */

#include "gui_wallet_centralwidget.hpp"
#include <QMessageBox>
#include <QTimer>
#include <QHeaderView>
#include <QResizeEvent>
#include <QScrollBar>
#include "gui_wallet_global.hpp"
#include "gui_wallet_mainwindow.hpp"
#include <QSortFilterProxyModel>

#ifndef _PATH_DELIMER_
#ifdef WIN32
#define _PATH_DELIMER_  '\\'
#else
#define _PATH_DELIMER_  '/'
#endif
#endif

#ifdef WIN32
#include <direct.h>
#ifndef getcwd
#define getcwd _getcwd
#endif
#else
#include <unistd.h>
#endif


using namespace gui_wallet;


AccountBalanceWidget::AccountBalanceWidget()
    :   TableWidgetItemW_base<QWidget,int>(1,this,NULL,
                                                                     &AccountBalanceWidget::ClbFunction),
      m_nCurrentIndex(-1)
{
    m_amount_label.setStyleSheet("color:green;""background-color:white;");
    m_asset_type_label.setStyleSheet("color:black;""background-color:white;");
    m_amount_label.setAlignment(Qt::AlignRight|Qt::AlignVCenter);
    m_asset_type_label.setAlignment(Qt::AlignRight|Qt::AlignVCenter);
    m_main_layout.addWidget(&m_amount_label);
    m_main_layout.addWidget(&m_asset_type_label);
    setLayout(&m_main_layout);
}


void AccountBalanceWidget::SetAccountBalanceFromStringGUIprivate(const std::string& a_balance)
{
    const char* cpcAssetTypeStarts;
    const char* cpcInpStr = a_balance.c_str();
    double lfAmount = strtod(cpcInpStr,const_cast<char**>(&cpcAssetTypeStarts));
    if(!cpcAssetTypeStarts){return;}
    QString tqsAmount = QString::number(lfAmount,'f').remove( QRegExp("0+$") ).remove( QRegExp("\\.$") );
    for(;*cpcAssetTypeStarts!=' ' && *cpcAssetTypeStarts != 0;++cpcAssetTypeStarts);
    QString tqsAssetType = tr(cpcAssetTypeStarts);
    m_amount_label.setText(tqsAmount);
    m_asset_type_label.setText(tqsAssetType);
}


void AccountBalanceWidget::clear()
{
    m_nCurrentIndex = -1;
    m_vBalances.clear();
    m_amount_label.setText(tr(""));
    m_asset_type_label.setText(tr(""));
}


void AccountBalanceWidget::addItem(const std::string& a_balance)
{
    m_vBalances.push_back(a_balance);

    if(m_nCurrentIndex<0)
    {
        SetAccountBalanceFromStringGUIprivate(a_balance);
        m_nCurrentIndex = ((int)m_vBalances.size())-1;
    }
}

QString CentralWigdet::FilterStr()
{
    int nCurTab(m_main_tabs.currentIndex());

    switch(nCurTab)
    {
    case BROWSE_CONTENT:
        return m_browse_cont_tab.m_filterLineEdit.text();
        break;
    case TRANSACTIONS:
        return m_trans_tab.user.text();
        break;
    case UPLOAD:
        return 0;
        break;
    case OVERVIEW:
    {
        return m_Overview_tab.search.text();
        break;
    }
    case PURCHASED:
        return 0;
        break;
    default:
        break;
    }
    return tr("");
}


void AccountBalanceWidget::setCurrentIndex(int a_nIndex)
{
    //__DEBUG_APP2__(0," ");
    if((a_nIndex<((int)m_vBalances.size())) && (a_nIndex!=m_nCurrentIndex))
    {
        SetAccountBalanceFromStringGUIprivate(m_vBalances[a_nIndex]);
        m_nCurrentIndex = a_nIndex;
    }
}


void AccountBalanceWidget::ClbFunction(_NEEDED_ARGS1_(int))
{
    //
}


/*//////////////////////////////////////////////////*/

CentralWigdet::CentralWigdet(QBoxLayout* a_pAllLayout, Mainwindow_gui_wallet* a_pPar)
    : m_first_line_lbl(), m_parent_main_window(a_pPar), m_Overview_tab(a_pPar)
{

    m_allTabs.push_back(&m_browse_cont_tab);
    m_allTabs.push_back(&m_trans_tab);
    m_allTabs.push_back(&m_Upload_tab);
    m_allTabs.push_back(&m_Overview_tab);
    m_allTabs.push_back(&m_Purchased_tab);
    m_currentTab = -1;

    
    setStyleSheet("color:black;""background-color:white;");

    m_main_tabs.setStyleSheet("QTabBar::tab{"
                              " height: 40px; width: 179px; "
                              "color:rgb(27,176,104);background-color:white;"
                              "border-right: 1 solid rgb(240,240,240);"
                              "border-top: 1 solid rgb(240,240,240);"
                              "border-bottom: 1 solid rgb(240,240,240);}"
                              "QTabBar::tab:selected{"
                              "color:white;background-color:rgb(27,176,104);}"
                               );

    
    PrepareGUIprivate(a_pAllLayout);
    
    QTimer::singleShot(200, this, &CentralWigdet::initTabChanged);

}

void  CentralWigdet::initTabChanged() {
    tabChanged(0);
}

CentralWigdet::~CentralWigdet()
{
}


void CentralWigdet::SetAccountBalancesFromStrGUI(const std::vector<std::string>& a_balances_and_names)
{
    //m_balanceLabel.setText(tr(a_balance_and_name.c_str()));
    AccountBalanceWidget* pBalanceCombo =
            (AccountBalanceWidget*)GetWidgetFromTable5(BALANCE,1);
    //AccountBalanceWidget* pBalanceCombo = m_pBalanceWgt2;
    pBalanceCombo->clear();
    const int cnBalances(a_balances_and_names.size());

    if(cnBalances)
    {
        for(int i(0); i<cnBalances; ++i)
        {
            pBalanceCombo->addItem(a_balances_and_names[i]);
        }
    }
    else
    {
        pBalanceCombo->addItem("0 DCT");
    }

    pBalanceCombo->setCurrentIndex(0);
    //m_balanceCombo.lineEdit()->setAlignment(Qt::AlignRight);
}





QComboBox* CentralWigdet::usersCombo()
{
    return (QComboBox*)GetWidgetFromTable5(USERNAME,1);
}


QWidget* CentralWigdet::GetWidgetFromTable5(int a_nColumn, int a_nWidget)
{
    a_nColumn = 2*a_nColumn;
    return m_first_line_lbl.itemAt(a_nColumn)->widget()->layout()->itemAt(a_nWidget)->widget();
}


#define __SIZE_FOR_IMGS__   40
#define __HEIGHT__  60
#include "decent_wallet_ui_gui_newcheckbox.hpp"

void CentralWigdet::PrepareGUIprivate(class QBoxLayout* a_pAllLayout)
{

    m_main_tabs.addTab(&m_browse_cont_tab,tr("Browse Content"));
    m_main_tabs.addTab(&m_trans_tab,tr("Transactions"));
    m_main_tabs.addTab(&m_Upload_tab,tr("Upload"));
    m_main_tabs.addTab(&m_Overview_tab,tr("Overview"));
    m_main_tabs.addTab(&m_Purchased_tab,tr("Purchased"));


    QTabBar* pTabBar = m_main_tabs.tabBar();
    pTabBar->setDocumentMode(true);
    pTabBar->setExpanding(false);


    QWidget* pWidgetTmp2 = nullptr;
    QLabel* pLabelTmp = nullptr;
    QPixmap image;
    QHBoxLayout *pHBoxLayoutTmp = nullptr;
    QComboBox* pComboTmp1 = nullptr;
    QFrame* line = nullptr;

    AccountBalanceWidget* pCombo2;

    /*////////////////////////////////////////////////////////////////////////////////////*/

    /*//////////////////////////////////////////*/
    m_pDcLogoWgt = new QWidget;

    pHBoxLayoutTmp = new QHBoxLayout;
    pLabelTmp = new QLabel(tr(""));
    pLabelTmp->setScaledContents(true);

    QPixmap m_image1(":/icon/images/decent_logo.svg");
    m_image1 = m_image1.scaled(200, 200, Qt::KeepAspectRatio);
    pLabelTmp->setPixmap(m_image1);
    pHBoxLayoutTmp->addWidget(pLabelTmp);
    
    pLabelTmp->setFixedSize(__SIZE_FOR_IMGS__,__SIZE_FOR_IMGS__);
    m_pDcLogoWgt->setLayout(pHBoxLayoutTmp);
    m_pDcLogoWgt->setFixedHeight(__HEIGHT__);
    m_pDcLogoWgt->setMaximumWidth(126);
    m_first_line_lbl.addWidget(m_pDcLogoWgt);


    /*//////////////////////////////////////////*/
    line = new QFrame(this);
    line->setFrameShape(QFrame::VLine); // Horizontal line
    line->setLineWidth(1);
    line->setStyleSheet("color: #f0f0f0");
    line->setFixedHeight(68);
    m_first_line_lbl.addWidget(line);


    
    
    m_pUsernameWgt = new QWidget;
    
    pHBoxLayoutTmp = new QHBoxLayout;
    
    pLabelTmp = new QLabel(tr(""));
    
    pLabelTmp->setScaledContents(true);

    QPixmap m_image2(":/icon/images/user.png");
    pLabelTmp->setPixmap(m_image2);
    pHBoxLayoutTmp->addWidget(pLabelTmp);
    pLabelTmp->setFixedSize(__SIZE_FOR_IMGS__,__SIZE_FOR_IMGS__);
    
    pComboTmp1 = new QComboBox;
    pComboTmp1->setStyleSheet("color: black;""background-color:white;"
                              "border: 1px solid #D3D3D3 ");
    if(!pComboTmp1){throw __FILE__ "Low memory";}
    
//    pComboTmp1->setStyleSheet("color: black;""background-color:white;");
    pHBoxLayoutTmp->addWidget(pComboTmp1);
    m_pUsernameWgt->setLayout(pHBoxLayoutTmp);
    m_first_line_lbl.addWidget(m_pUsernameWgt);
    m_pUsernameWgt->setFixedHeight(__HEIGHT__);
    m_pUsernameWgt->setMaximumWidth(271);

    
    /*//////////////////////////////////////////*/
    line = new QFrame(this);
    line->setFrameShape(QFrame::VLine); // Horizontal line
    
    line->setLineWidth(1);
    line->setStyleSheet("color: #f0f0f0");
    line->setFixedHeight(68);
    m_first_line_lbl.addWidget(line);

    /*//////////////////////////////////////////*/
    m_pBalanceWgt1 = new QWidget;
    pHBoxLayoutTmp = new QHBoxLayout;
    pLabelTmp = new QLabel(tr(""));
    pLabelTmp->setScaledContents(true);

    QPixmap m_image3(":/icon/images/balance.png");
    pLabelTmp->setPixmap(m_image3);
    pHBoxLayoutTmp->addWidget(pLabelTmp);
    pLabelTmp->setFixedSize(__SIZE_FOR_IMGS__,__SIZE_FOR_IMGS__);
    pCombo2 = new AccountBalanceWidget;
    pHBoxLayoutTmp->addWidget(pCombo2);
    m_pBalanceWgt1->setLayout(pHBoxLayoutTmp);
    m_first_line_lbl.addWidget(m_pBalanceWgt1);
    m_pBalanceWgt1->setFixedHeight(__HEIGHT__);
    m_pBalanceWgt1->setMaximumWidth(353);

    /*//////////////////////////////////////////*/
    line = new QFrame(this);
    line->setFrameShape(QFrame::VLine); // Horizontal line

    line->setLineWidth(1);
    line->setStyleSheet("color: #f0f0f0");
    line->setFixedHeight(68);
    m_first_line_lbl.addWidget(line);

    /*//////////////////////////////////////////*/
    pWidgetTmp2 = new QWidget;
    pHBoxLayoutTmp = new QHBoxLayout;
    pLabelTmp = new QLabel(tr(""));
    pLabelTmp->setScaledContents(true);

    QPixmap m_image4(":/icon/images/send.png");
    pLabelTmp->setPixmap(m_image4);
    pHBoxLayoutTmp->addWidget(pLabelTmp);
    pLabelTmp->setFixedSize(__SIZE_FOR_IMGS__,__SIZE_FOR_IMGS__);
    pLabelTmp = new QLabel(tr("Send"));

    pHBoxLayoutTmp->addWidget(pLabelTmp);
    pWidgetTmp2->setLayout(pHBoxLayoutTmp);
    m_first_line_lbl.addWidget(pWidgetTmp2);
    pWidgetTmp2->setFixedHeight(__HEIGHT__);
    pWidgetTmp2->setMaximumWidth(190);

    
    m_browse_cont_tab.setStyleSheet("color: black;""background-color:white;");
    SetAccountBalancesFromStrGUI(std::vector<std::string>());


    m_main_layout.setContentsMargins(0, 0, 0, 0);
    m_main_layout.setMargin(0);
    m_main_layout.setSpacing(0);


    m_main_layout.addLayout(&m_first_line_lbl);

    m_main_layout.addWidget(&m_main_tabs);

    a_pAllLayout->addLayout(&m_main_layout);
    
    connect(&m_main_tabs, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));
}



void CentralWigdet::tabChanged(int index) {
    if (m_allTabs.size() == 0) {
        return;
    }
    
    if (index != m_currentTab) {
        if (m_currentTab >= 0) {
            m_allTabs[m_currentTab]->content_deactivated();
        }
    }

    m_currentTab = index;
    m_allTabs[m_currentTab]->content_activated();

}


QString CentralWigdet::getFilterText()const
{
    int nActiveTab = m_main_tabs.currentIndex();

    switch(nActiveTab)
    {
    case BROWSE_CONTENT:
        return "";
    case PURCHASED:
        //return tr("bought:") + m_Purchased_tab.getFilterText();
    default:
        break;
    }

    return tr("");
}


void CentralWigdet::showEvent ( QShowEvent * event )
{
    QWidget::showEvent(event);
}


void CentralWigdet::make_deleyed_warning()
{
    gui_wallet::makeWarningImediatly(m_DelayedWaringTitle.toLatin1().data(),
                                     m_DelayedWaringText.toLatin1().data(),
                                     m_DelayedWaringDetails.toLatin1().data(),this);
}


void CentralWigdet::resizeEvent ( QResizeEvent * a_event )
{
    //return ;
    QWidget::resizeEvent(a_event);

    /*QString tqsStyle = tr("QTabBar::tab {width: ") +
            QString::number(a_event->size().width()/5-1,10) + tr("px;}");
    QTabBar* pTabBar = m_main_tabs2.tabBar();
    pTabBar->setStyleSheet(tqsStyle);*/

//    QTabBar* pTabBar = m_main_tabs2.tabBar();

    QTabBar* pTabBar = m_main_tabs.tabBar();
    pTabBar->resize(size().width(),pTabBar->height());

    int nWidth_small (size().width()*13/100);
    int nWidth_big (size().width()*28/100);
    int nWidth_medium (size().width()*38/100);
    m_pDcLogoWgt->resize(nWidth_small,m_pDcLogoWgt->height());
    m_pUsernameWgt->resize(nWidth_big,m_pUsernameWgt->height());
    m_pBalanceWgt1->resize(nWidth_medium,m_pBalanceWgt1->height());

}


