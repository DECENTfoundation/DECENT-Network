#pragma once


#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QTabWidget>
#include "browse_content_tab.hpp"
#include "transactions_tab.hpp"
#include "upload_tab.hpp"
#include "overview_tab.hpp"
#include "purchased_tab.hpp"
#include <stdio.h>
#include <QLabel>
#include <QComboBox>
#include <vector>
#include <QTableWidget>
#include "purchased_tab.hpp"
#include <QString>
#include "decent_button.hpp"



namespace gui_wallet
{

enum FRST_LINE_ELEMS{DECENT_LOGO,USERNAME,BALANCE,SEND_,NUMBER_OF_FRST_LINE_ELEMS};
enum MAIN_TABS_ENM{BROWSE_CONTENT,TRANSACTIONS,UPLOAD,OVERVIEW,PURCHASED};





    
class CentralWigdet : public QWidget
{
    Q_OBJECT
public:
    CentralWigdet(QWidget* pParent);
    virtual ~CentralWigdet(); /* virtual because may be this class will be */
                              /* used by inheritance */

    void SetAccountBalancesFromStrGUI(const std::vector<std::string>& a_balances_and_names);
    void AddNewUserGUI(const std::string& user_name);
    
    BrowseContentTab* GetBrowseContentTab() { return &m_browse_cont_tab; }
    PurchasedTab* GetPurchasedTab() { return &m_Purchased_tab; }
    
    
    void SetDigitalContentsGUI(const std::vector<SDigitalContent>& vContents);

   
    QString getFilterText()const;
   
    int GetMyCurrentTabIndex()const {
        return m_main_tabs.currentIndex();
    }   

    void SetMyCurrentTabIndex(int index) {
        m_main_tabs.setCurrentIndex(index);
    }
   
    void SetTransactionInfo(std::string info_from_other_tab);
   
    Overview_tab* getUsersTab();
   
public slots:
   void initTabChanged();
   void tabChanged(int index);
   void walletUnlockedSlot();
   void updateActiveTab();
   void sendDCTSlot();
   void nextButtonSlot();
   void prevButtonSlot();
   void resetButtonSlot();
   void paginationController();
   
public:
signals:
   void sendDCT();
   void signal_SetNextPageDisabled(bool);
   void signal_SetPreviousPageDisabled(bool);
protected:
    virtual void showEvent ( QShowEvent * event ) ;
    virtual void resizeEvent ( QResizeEvent * event );

private:
    void PrepareGUIprivate(class QBoxLayout* pAllLayout);
    

private:
    QVBoxLayout         m_main_layout;

    
    QWidget* m_parent_main_window;
    
    QTabWidget          m_main_tabs;
    
    BrowseContentTab    m_browse_cont_tab;
    TransactionsTab     m_trans_tab;
    Upload_tab          m_Upload_tab;
    Overview_tab        m_Overview_tab;
    PurchasedTab        m_Purchased_tab;
   
   
   
    std::vector<TabContentManager*>  m_allTabs;
    int                              m_currentTab = -1;

    QString             m_DelayedWaringTitle;
    QString             m_DelayedWaringText;
    QString             m_DelayedWaringDetails;

    QWidget*            m_pDcLogoWgt;
    QWidget*            m_pUsernameWgt;
    QWidget*            m_pBalanceWgt1;
    QWidget*            m_pSendWgt1;

    
};

}


