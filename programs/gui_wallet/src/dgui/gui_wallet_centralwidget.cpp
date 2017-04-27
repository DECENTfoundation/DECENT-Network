/*
 *	File: gui_wallet_centralwigdet.cpp
 *
 *	Created on: Nov 11, 2016
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
#include "stdafx.h"

#include "gui_design.hpp"
#include "gui_wallet_centralwidget.hpp"


#ifndef _MSC_VER
#include <QMessageBox>
#include <QTimer>
#include <QStatusBar>
#include <QHeaderView>
#include <QResizeEvent>
#include <QScrollBar>
#endif

#include "gui_wallet_global.hpp"
#include "gui_wallet_mainwindow.hpp"

#ifndef _MSC_VER
#include <QSortFilterProxyModel>
#include <QStyleFactory>
#endif

#include "decent_wallet_ui_gui_newcheckbox.hpp"

#define __SIZE_FOR_IMGS__   40
#define __HEIGHT__  60


using namespace gui_wallet;

AccountBalanceWidget::AccountBalanceWidget() : m_nCurrentIndex(-1) {
   
    m_amount_label.setStyleSheet(d_amount_label);
    m_asset_type_label.setStyleSheet(d_label);
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
    m_amount_label.setText("");
    m_asset_type_label.setText("");
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

void AccountBalanceWidget::setCurrentIndex(int a_nIndex)
{
    //__DEBUG_APP2__(0," ");
    if((a_nIndex<((int)m_vBalances.size())) && (a_nIndex!=m_nCurrentIndex))
    {
        SetAccountBalanceFromStringGUIprivate(m_vBalances[a_nIndex]);
        m_nCurrentIndex = a_nIndex;
    }
}


/*//////////////////////////////////////////////////*/

CentralWigdet::CentralWigdet(QBoxLayout* a_pAllLayout, Mainwindow_gui_wallet* a_pPar)
    : m_first_line_lbl(),
      m_parent_main_window(a_pPar),
      m_browse_cont_tab(a_pPar),
      m_Overview_tab(a_pPar),
      m_Upload_tab(a_pPar),
      m_Purchased_tab(a_pPar),
      m_trans_tab(a_pPar),
      sendButton(new DecentSmallButton(icon_inactive_send,icon_send))
{


         
    m_allTabs.push_back(&m_browse_cont_tab);
    m_allTabs.push_back(&m_trans_tab);
    m_allTabs.push_back(&m_Upload_tab);
    m_allTabs.push_back(&m_Overview_tab);
    m_allTabs.push_back(&m_Purchased_tab);
    m_currentTab = -1;



    m_main_tabs.setStyleSheet(d_main_tabs);

    PrepareGUIprivate(a_pAllLayout);

    QTimer::singleShot(200, this, SLOT(initTabChanged()));
}

void  CentralWigdet::initTabChanged() {
    tabChanged(0);
}

CentralWigdet::~CentralWigdet()
{
}


void CentralWigdet::SetAccountBalancesFromStrGUI(const std::vector<std::string>& a_balances_and_names)
{
    AccountBalanceWidget* pBalanceCombo = (AccountBalanceWidget*)GetWidgetFromTable5(BALANCE,1);
    
    pBalanceCombo->clear();
    
    const int cnBalances = a_balances_and_names.size();

    if(cnBalances) {
        for (int i = 0; i < cnBalances; ++i) {
            pBalanceCombo->addItem(a_balances_and_names[i]);
        }
    } else {
        pBalanceCombo->addItem("0 DCT");
    }

    pBalanceCombo->setCurrentIndex(0);
}





QComboBox* CentralWigdet::usersCombo()
{
    return (QComboBox*)GetWidgetFromTable5(USERNAME,1);
}

DecentButton* CentralWigdet::importButton()
{
   return (DecentButton*)GetWidgetFromTable5(USERNAME,2);
}

DecentSmallButton* CentralWigdet::getSendButton()
{
   return sendButton;
}

QWidget* CentralWigdet::GetWidgetFromTable5(int a_nColumn, int a_nWidget)
{
    a_nColumn = 2*a_nColumn;
    return m_first_line_lbl.itemAt(a_nColumn)->widget()->layout()->itemAt(a_nWidget)->widget();
}



void CentralWigdet::PrepareGUIprivate(class QBoxLayout* a_pAllLayout)
{

    m_main_tabs.addTab(&m_browse_cont_tab,tr("Browse Content"));
    m_main_tabs.addTab(&m_trans_tab,tr("Transactions"));
    m_main_tabs.addTab(&m_Upload_tab,tr("Publish"));
    m_main_tabs.addTab(&m_Overview_tab,tr("Users"));
    m_main_tabs.addTab(&m_Purchased_tab,tr("Purchased"));


    QTabBar* pTabBar = m_main_tabs.tabBar();
    pTabBar->setDocumentMode(true);
    pTabBar->setExpanding(false);


    QLabel* pLabelTmp = nullptr;
    QPixmap image;
    QHBoxLayout *pHBoxLayoutTmp = nullptr;
    QComboBox* pComboTmp1 = nullptr;
    QFrame* line = nullptr;
    DecentButton* importKeyButton = nullptr;

    AccountBalanceWidget* pCombo2;

    /*////////////////////////////////////////////////////////////////////////////////////*/

    /*//////////////////////////////////////////*/
    m_pDcLogoWgt = new QWidget;

    pHBoxLayoutTmp = new QHBoxLayout;
    pLabelTmp = new QLabel("");
    pLabelTmp->setScaledContents(true);

    QPixmap m_image1(icon_decent);
    pHBoxLayoutTmp->setContentsMargins(0, 0, 0, 90);
    pLabelTmp->setPixmap(m_image1);
    pHBoxLayoutTmp->addWidget(pLabelTmp);
    
    pLabelTmp->setFixedSize(55,55);
    m_pDcLogoWgt->setLayout(pHBoxLayoutTmp);
    m_pDcLogoWgt->setFixedHeight(__HEIGHT__);
    m_pDcLogoWgt->setMaximumWidth(120);
    m_first_line_lbl.addWidget(m_pDcLogoWgt);


    /*//////////////////////////////////////////*/
    line = new QFrame(this);
    line->setFrameShape(QFrame::VLine); // Horizontal line
    line->setLineWidth(1);
    line->setStyleSheet(d_color);
    line->setFixedHeight(68);
    m_first_line_lbl.addWidget(line);


    
    
    m_pUsernameWgt = new QWidget;
    
    pHBoxLayoutTmp = new QHBoxLayout;
    
    pLabelTmp = new QLabel("");
    
    pLabelTmp->setScaledContents(true);

    QPixmap m_image2(icon_user);
    pLabelTmp->setPixmap(m_image2);
    pHBoxLayoutTmp->addWidget(pLabelTmp);
    pLabelTmp->setFixedSize(28,28);
    
    pComboTmp1 = new QComboBox;
    pComboTmp1->setStyle(QStyleFactory::create("fusion"));
    importKeyButton = new DecentButton();

    importKeyButton->setText(tr("Import Key"));
#ifdef WINDOWS_HIGH_DPI
    importKeyButton->setFixedSize(150, 30);
#else
    importKeyButton->setFixedSize(100, 25);
#endif
    pHBoxLayoutTmp->addWidget(pComboTmp1);
    pHBoxLayoutTmp->addWidget(importKeyButton);
    m_pUsernameWgt->setLayout(pHBoxLayoutTmp);
    m_first_line_lbl.addWidget(m_pUsernameWgt);
#ifdef WINDOWS_HIGH_DPI
    m_pUsernameWgt->setFixedHeight(100);
    m_pUsernameWgt->setMaximumWidth(300);
#else
    m_pUsernameWgt->setFixedHeight(__HEIGHT__);
    m_pUsernameWgt->setMaximumWidth(271);
#endif
   
    /*//////////////////////////////////////////*/
    line = new QFrame(this);
    line->setFrameShape(QFrame::VLine); // Horizontal line
    
    line->setLineWidth(1);
    line->setStyleSheet(c_line);
    line->setFixedHeight(68);
    m_first_line_lbl.addWidget(line);

    /*//////////////////////////////////////////*/
    m_pBalanceWgt1 = new QWidget;
    pHBoxLayoutTmp = new QHBoxLayout;
    pLabelTmp = new QLabel("");
    pLabelTmp->setScaledContents(true);

    QPixmap m_image3(icon_balance);
    pLabelTmp->setPixmap(m_image3);
    pLabelTmp->setFixedSize(30,30);
    pHBoxLayoutTmp->addWidget(pLabelTmp);
    pHBoxLayoutTmp->setAlignment(Qt::AlignCenter);
    pHBoxLayoutTmp->setSpacing(0);
   
    pCombo2 = new AccountBalanceWidget;

    pCombo2->setFont(AccountBalanceFont());
    pHBoxLayoutTmp->addWidget(pCombo2);
    
    m_pBalanceWgt1->setLayout(pHBoxLayoutTmp);
    m_first_line_lbl.addWidget(m_pBalanceWgt1);
    pHBoxLayoutTmp->setContentsMargins(220, 0, 40, 0);
#ifdef WINDOWS_HIGH_DPI
    m_pBalanceWgt1->setFixedHeight(100);
#else
    m_pBalanceWgt1->setFixedHeight(__HEIGHT__);
#endif
    m_browse_cont_tab.setStyleSheet(d_label);

   /*//////////////////////////////////////////*/
   line = new QFrame(this);
   line->setFrameShape(QFrame::VLine); // Vertical line
   
   line->setLineWidth(1);
   line->setStyleSheet(d_color);
   line->setFixedHeight(68);
   m_first_line_lbl.addWidget(line);
   
   /*//////////////////////////////////////////*/
   m_pSendWgt1 = new QWidget;
   pHBoxLayoutTmp = new QHBoxLayout;
   sendButton->setScaledContents(true);
   DecentButton* send_text = new DecentButton();
   send_text->setText("  " + tr("Send"));
   send_text->setStyleSheet("QLabel { color : rgb(0,0,0);}");

   sendButton->setFixedSize(30,30);
   connect(sendButton, SIGNAL(clicked()), this, SLOT(sendDCTSlot()));
   connect(send_text, SIGNAL(LabelClicked()), this, SLOT(sendDCTSlot()));
   pHBoxLayoutTmp->setAlignment(Qt::AlignCenter);
   pHBoxLayoutTmp->addWidget(sendButton);
   pHBoxLayoutTmp->addWidget(send_text);
   
   m_pSendWgt1->setLayout(pHBoxLayoutTmp);
   m_first_line_lbl.addWidget(m_pSendWgt1);
   pHBoxLayoutTmp->setContentsMargins(50, 0, 50, 0);
   m_pSendWgt1->setFixedHeight(__HEIGHT__);
   
    m_browse_cont_tab.setStyleSheet(d_label);
    SetAccountBalancesFromStrGUI(std::vector<std::string>());


    m_main_layout.setContentsMargins(0, 0, 0, 0);
    m_main_layout.setMargin(0);
    m_main_layout.setSpacing(0);


    m_main_layout.addLayout(&m_first_line_lbl);
    
    line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setLineWidth(500);
    line->setStyleSheet(c_line);
    line->setFixedHeight(1);

    
    m_main_layout.addWidget(line);
    QHBoxLayout* tab_lay = new QHBoxLayout();
    tab_lay->addWidget(&m_main_tabs);
    tab_lay->activate();
    tab_lay->setContentsMargins(0, 0, 0, 0);
    m_main_layout.addLayout(tab_lay);
    //m_main_layout.addWidget(&m_main_tabs);
    
   QStatusBar* status = new QStatusBar(this);
   m_main_layout.addWidget(status);
   QObject::connect(&Globals::instance(), &Globals::statusShowMessage,
                    status, &QStatusBar::showMessage);
   QObject::connect(&Globals::instance(), &Globals::statusClearMessage,
                    status, &QStatusBar::clearMessage);

    a_pAllLayout->addLayout(&m_main_layout);
    
   connect(&m_main_tabs, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));
   connect(&Globals::instance(), SIGNAL(walletUnlocked()), this, SLOT(walletUnlockedSlot()));
   
}

void CentralWigdet::sendDCTSlot()
{
   emit sendDCT();
}


void CentralWigdet::walletUnlockedSlot() {
   QTimer::singleShot(1000, this, SLOT(updateActiveTab()));
}


void CentralWigdet::updateActiveTab() {
   if (m_currentTab >= 0) {
      m_allTabs[m_currentTab]->tryToUpdate();

   }
   QTimer::singleShot(1000, this, SLOT(updateActiveTab()));
}


void CentralWigdet::tabChanged(int index) {
    if (m_allTabs.size() == 0) {
        return;
    }
    
    if (index != m_currentTab) {
        if (m_currentTab >= 0) {
            m_allTabs[m_currentTab]->contentDeactivated();
        }
    }

    m_currentTab = index;
    m_allTabs[m_currentTab]->contentActivated();

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

    return "";
}


void CentralWigdet::showEvent ( QShowEvent * event )
{
    QWidget::showEvent(event);
}


void CentralWigdet::resizeEvent ( QResizeEvent * a_event )
{
    QWidget::resizeEvent(a_event);
    
    int each_width = m_parent_main_window->size().width()/5-3;
    QString s = QString::number(each_width);
    if(a_event->oldSize().width() > a_event->size().width())
    {
        s = QString::number(each_width - 11);
        m_main_tabs.setStyleSheet("QTabBar::tab{"
#ifdef WINDOWS_HIGH_DPI
                                  "font-size: 10pt;"
#endif
                                  "font:bold;"
                                  " height: 40px; width: " + s + "px;"
                                  "color:rgb(0,0,0);background-color:white;"
                                  "border-left:0;"
                                  "border-top: 1 solid rgb(240,240,240);"
                                  "border-bottom: 1 solid rgb(240,240,240);}"
                                  "QTabBar::tab:selected{"
                                  "color: rgb(27,176,104);"
                                  "border-bottom:3px solid rgb(27,176,104);"
                                  "border-top: 1px solid rgb(240,240,240);"
                                  "border-left:0px;"
                                  "border-right:0px;}"
                                  );
    }
    else
    {
        m_main_tabs.setStyleSheet("QTabBar::tab{"
#ifdef WINDOWS_HIGH_DPI
                                 "font-size: 10pt;"
#endif
                                  "font:bold;"
                                  " height: 40px; width: " + s + "px;"
                                  "color:rgb(0,0,0);background-color:white;"
                                  "border-left:0;"
                                  "border-top: 1 solid rgb(240,240,240);"
                                  "border-bottom: 1 solid rgb(240,240,240);}"
                                  "QTabBar::tab:selected{"
                                  "color: rgb(27,176,104);"
                                  "border-bottom:3px solid rgb(27,176,104);"
                                  "border-top: 1px solid rgb(240,240,240);"
                                  "border-left:0px;"
                                  "border-right:0px;}"
                                  );
    }
}

void CentralWigdet::SetTransactionInfo(std::string info_from_other_tab)
{
    m_trans_tab.set_user_filter(info_from_other_tab);
}

Overview_tab* CentralWigdet::getUsersTab()
{
   return &m_Overview_tab;
}




