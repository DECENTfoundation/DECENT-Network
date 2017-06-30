/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
#if 0
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


#ifndef _MSC_VER
#include <QSortFilterProxyModel>
#include <QStyleFactory>
#endif

#define __SIZE_FOR_IMGS__   40
#define __HEIGHT__  60


using namespace gui_wallet;


CentralWigdet::CentralWigdet(QWidget* pParent)
    : m_parent_main_window(pParent),
      m_browse_cont_tab(this, nullptr),
      m_Overview_tab(this, nullptr),
      m_Upload_tab(this, nullptr, nullptr),
      m_Purchased_tab(this, nullptr),
      m_trans_tab(this, nullptr)
{
   QVBoxLayout* pMainLayout = new QVBoxLayout;
   pMainLayout->setContentsMargins(0, 0, 0, 0);
         
    m_allTabs.push_back(&m_browse_cont_tab);
    m_allTabs.push_back(&m_trans_tab);
    m_allTabs.push_back(&m_Upload_tab);
    m_allTabs.push_back(&m_Overview_tab);
    m_allTabs.push_back(&m_Purchased_tab);
    m_currentTab = -1;



    m_main_tabs.setStyleSheet(d_main_tabs);

    PrepareGUIprivate(pMainLayout);

    QTimer::singleShot(200, this, SLOT(initTabChanged()));

   setLayout(pMainLayout);
}

void  CentralWigdet::initTabChanged() {
    tabChanged(0);
}

CentralWigdet::~CentralWigdet()
{
}


void CentralWigdet::SetAccountBalancesFromStrGUI(const std::vector<std::string>& a_balances_and_names)
{
}




void CentralWigdet::PrepareGUIprivate(class QBoxLayout* a_pAllLayout)
{
 
    m_main_tabs.addTab(&m_browse_cont_tab,tr("Browse Content"));
    m_main_tabs.addTab(&m_trans_tab,tr("Transactions"));
    m_main_tabs.addTab(&m_Upload_tab,tr("Publish"));
    m_main_tabs.addTab(&m_Overview_tab,tr("Users"));
    m_main_tabs.addTab(&m_Purchased_tab,tr("Purchased"));

   a_pAllLayout->setContentsMargins(0, 0, 0, 0);
   a_pAllLayout->setSpacing(0);

    QTabBar* pTabBar = m_main_tabs.tabBar();
    pTabBar->setDocumentMode(true);
    pTabBar->setExpanding(false);

    QHBoxLayout* tab_lay = new QHBoxLayout();
    tab_lay->addWidget(&m_main_tabs);
    tab_lay->activate();
    tab_lay->setContentsMargins(0, 0, 0, 0);
   tab_lay->setSpacing(0);
    m_main_layout.addLayout(tab_lay);
    //m_main_layout.addWidget(&m_main_tabs);
   
   QHBoxLayout* pagination_layout = new QHBoxLayout();
   pagination_layout->setSpacing(0);
   pagination_layout->setContentsMargins(0, 0, 0, 0);

   DecentButton* prev_button = new DecentButton(this);
   DecentButton* next_button = new DecentButton(this);
   DecentButton* reset_button = new DecentButton(this);
   
   prev_button->setText("Previous");
   prev_button->setStyleSheet(d_pagination_buttons);
   prev_button->setFixedHeight(35);
   prev_button->setFont(PaginationFont());
   
   next_button->setText("Next");
   next_button->setStyleSheet(d_pagination_buttons);
   next_button->setFixedHeight(35);
   next_button->setFont(PaginationFont());
   
   reset_button->setText("First Page");
   reset_button->setStyleSheet(d_pagination_buttons);
   reset_button->setFixedHeight(35);
   reset_button->setFont(PaginationFont());
   
   pagination_layout->addWidget(prev_button);
   pagination_layout->addWidget(next_button);
   pagination_layout->addWidget(reset_button);
   
   m_main_layout.addLayout(pagination_layout);
   
   QTimer* time = new QTimer;
   QObject::connect( time, SIGNAL(timeout()), SLOT(paginationController()) );
   time->start(500);

   QObject::connect( prev_button, SIGNAL(clicked()), this, SLOT(prevButtonSlot()) );
   QObject::connect( next_button, SIGNAL(clicked()), this, SLOT(nextButtonSlot()) );
   QObject::connect( reset_button, SIGNAL(clicked()), this, SLOT(resetButtonSlot()) );

   QObject::connect(this, &CentralWigdet::signal_SetNextPageDisabled,
                    next_button, &QPushButton::setDisabled);
   QObject::connect(this, &CentralWigdet::signal_SetPreviousPageDisabled,
                    prev_button, &QPushButton::setDisabled);
   
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

void CentralWigdet::paginationController()
{
   m_allTabs[m_currentTab]->m_i_page_size = ( m_allTabs[m_currentTab]->size().height() - 35)/35; // 30 for buttons layout, 35-cloumn height

   emit signal_SetNextPageDisabled(m_allTabs[m_currentTab]->is_last());
   emit signal_SetPreviousPageDisabled(m_allTabs[m_currentTab]->is_first());
}

void CentralWigdet::prevButtonSlot()
{
   m_allTabs[m_currentTab]->previous();
   paginationController();
}

void CentralWigdet::nextButtonSlot()
{
   m_allTabs[m_currentTab]->next();
   paginationController();
}

void CentralWigdet::resetButtonSlot()
{
   m_allTabs[m_currentTab]->reset();
   paginationController();
}

void CentralWigdet::sendDCTSlot()
{
   emit sendDCT();
}


void CentralWigdet::walletUnlockedSlot() {
   QTimer::singleShot(1000, this, SLOT(updateActiveTab()));
}


void CentralWigdet::updateActiveTab() {
   if (false == Globals::instance().connected())
      return;

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
    m_trans_tab.signal_setUserFilter(info_from_other_tab.c_str());
}

Overview_tab* CentralWigdet::getUsersTab()
{
   return &m_Overview_tab;
}

#endif


