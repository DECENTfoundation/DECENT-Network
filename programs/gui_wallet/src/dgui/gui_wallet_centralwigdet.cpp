/*
 *	File: gui_wallet_centralwigdet.cpp
 *
 *	Created on: Nov 11, 2016
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */

#define BALANCE_FILE_NAME2           "balance.png"
#define DECENT_LOGO_FILE_NAME2       "decent_logo.png"
#define SEND_FILE_NAME2              "send.png"
#define USER_FILE_NAME2              "user.png"
#define FOLDER_NAME_FOR_IMAGES2      "images"
#ifdef __APPLE__
#define DECENT_IMGS_INITIAL_PATH2    "../../../../../../" FOLDER_NAME_FOR_IMAGES2 "/"
#else
#define DECENT_IMGS_INITIAL_PATH2    "../../../" FOLDER_NAME_FOR_IMAGES2 "/"
#endif

#include "gui_wallet_centralwigdet.hpp"
#include <QMessageBox>
#include <QTimer>
#include <QHeaderView>
#include <QResizeEvent>
#include <QScrollBar>
#include "gui_wallet_global.hpp"
#include "gui_wallet_mainwindow.hpp"

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

extern std::string g_cApplicationPath ;
std::string FindImagePath(bool& a_bRet,const char* a_image_name);

static void SetImageToLabelStatic(bool& _bRet_,QPixmap& _image_,const char* _image_name_);
static void MakeWarning(const char* /*warn*/, const char* /*warn_details*/){}

using namespace gui_wallet;


decent::wallet::ui::gui::AccountBalanceWidget::AccountBalanceWidget()
    :   decent::wallet::ui::gui::TableWidgetItemW_base<QWidget,int>(1,this,NULL,
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


void decent::wallet::ui::gui::AccountBalanceWidget::SetAccountBalanceFromStringGUIprivate(const std::string& a_balance)
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


void decent::wallet::ui::gui::AccountBalanceWidget::clear()
{
    m_nCurrentIndex = -1;
    m_vBalances.clear();
    m_amount_label.setText(tr(""));
    m_asset_type_label.setText(tr(""));
}


void decent::wallet::ui::gui::AccountBalanceWidget::addItem(const std::string& a_balance)
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
    int nCurTab(m_main_tabs2.currentIndex());
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


void decent::wallet::ui::gui::AccountBalanceWidget::setCurrentIndex(int a_nIndex)
{
    //__DEBUG_APP2__(0," ");
    if((a_nIndex<((int)m_vBalances.size())) && (a_nIndex!=m_nCurrentIndex))
    {
        SetAccountBalanceFromStringGUIprivate(m_vBalances[a_nIndex]);
        m_nCurrentIndex = a_nIndex;
    }
}


void decent::wallet::ui::gui::AccountBalanceWidget::ClbFunction(_NEEDED_ARGS1_(int))
{
    //
}


/*//////////////////////////////////////////////////*/

CentralWigdet::CentralWigdet(class QBoxLayout* a_pAllLayout, class Mainwindow_gui_wallet* a_pPar)
    :
#ifdef __TRY_LABEL_INSTEAD_OF_TABLE__
      m_first_line_lbl()
#else
      m_first_line_widget2(1,NUMBER_OF_FRST_LINE_ELEMS)
#endif
    ,m_Overview_tab(a_pPar)
{
    setStyleSheet("color:black;""background-color:white;");
    m_main_tabs2.setStyleSheet("QTabBar::tab{"
                               "color:green;background-color:white;}"
                               "QTabBar::tab:selected{"
                               "color:white;background-color:green;}");
    PrepareGUIprivate(a_pAllLayout);
}


CentralWigdet::~CentralWigdet()
{
    //delete m_imageLabel;
}


void CentralWigdet::SetAccountBalancesFromStrGUI(const std::vector<std::string>& a_balances_and_names)
{
    //m_balanceLabel.setText(tr(a_balance_and_name.c_str()));
    decent::wallet::ui::gui::AccountBalanceWidget* pBalanceCombo =
            (decent::wallet::ui::gui::AccountBalanceWidget*)GetWidgetFromTable5(BALANCE,1);
    //decent::wallet::ui::gui::AccountBalanceWidget* pBalanceCombo = m_pBalanceWgt2;
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
        pBalanceCombo->addItem("0 DECENT");
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
#ifdef __TRY_LABEL_INSTEAD_OF_TABLE__

    a_nColumn = 2*a_nColumn;
    //if(a_nColumn>1){++a_nColumn;}
    __DEBUG_APP2__(4," ");
    return m_first_line_lbl.itemAt(a_nColumn)->widget()->layout()->itemAt(a_nWidget)->widget();

#else  // #ifdef __TRY_LABEL_INSTEAD_OF_TABLE__
    QWidget* pWidget = m_first_line_widget2.cellWidget(0, a_nColumn);
    QHBoxLayout* pHLayout = (QHBoxLayout*)pWidget->layout();
    return pHLayout->itemAt(a_nWidget)->widget();
#endif  // #ifdef __TRY_LABEL_INSTEAD_OF_TABLE__
}


#define __SIZE_FOR_IMGS__   40
#define __HEIGHT__  90
#include "decent_wallet_ui_gui_newcheckbox.hpp"

void CentralWigdet::PrepareGUIprivate(class QBoxLayout* a_pAllLayout)
{
    bool bImageFound(true);

    m_main_tabs2.addTab(&m_browse_cont_tab,tr("Browse Content"));
    m_main_tabs2.addTab(&m_trans_tab,tr("Transactions"));
    m_main_tabs2.addTab(&m_Upload_tab,tr("Upload"));
    m_main_tabs2.addTab(&m_Overview_tab,tr("Overview"));
    m_main_tabs2.addTab(&m_Purchased_tab,tr("Purchased"));

#if 1
    QTabBar* pTabBar = m_main_tabs2.tabBar();
    pTabBar->setDocumentMode(true);
    pTabBar->setExpanding(true);
#else
    QTabBar* pTabBar = m_main_tabs.tabBar();
    pTabBar->setStyleSheet("QTabBar::tab:disabled {"
        "width: 200px;"
        "color: transparent;"
        "background: transparent;"
     "}");
#endif

    QWidget* pWidgetTmp2;
    QPixmap image;
    QLabel* pLabelTmp;
    QHBoxLayout *pHBoxLayoutTmp;
    QComboBox* pComboTmp1;
    QFrame *line;
    decent::wallet::ui::gui::AccountBalanceWidget* pCombo2;
    /*////////////////////////////////////////////////////////////////////////////////////*/

    /*//////////////////////////////////////////*/
    m_pDcLogoWgt = new QWidget;
    if(!m_pDcLogoWgt){throw __FILE__ "Low memory";}
    pHBoxLayoutTmp = new QHBoxLayout;
    if(!pHBoxLayoutTmp){throw __FILE__ "Low memory";}
    pLabelTmp = new QLabel(tr(""));
    if(!pLabelTmp){throw __FILE__ "Low memory";}
    pLabelTmp->setScaledContents(true);
    SetImageToLabelStatic(bImageFound,image,DECENT_LOGO_FILE_NAME2);
    if(bImageFound){pLabelTmp->setPixmap(image);}
    else {pLabelTmp->setText("DC");MakeWarning("no file", "");}
    pHBoxLayoutTmp->addWidget(pLabelTmp);
    pLabelTmp->setFixedSize(__SIZE_FOR_IMGS__,__SIZE_FOR_IMGS__);
    m_pDcLogoWgt->setLayout(pHBoxLayoutTmp);
    m_first_line_lbl.addWidget(m_pDcLogoWgt);
    m_pDcLogoWgt->setFixedHeight(__HEIGHT__);

    /*//////////////////////////////////////////*/
    line = new QFrame(this);
    line->setFrameShape(QFrame::VLine); // Horizontal line
    line->setFrameShadow(QFrame::Sunken);
    line->setLineWidth(1);
    m_first_line_lbl.addWidget(line);

    /*//////////////////////////////////////////*/
    m_pUsernameWgt = new QWidget;
    if(!m_pUsernameWgt){throw __FILE__ "Low memory";}
    pHBoxLayoutTmp = new QHBoxLayout;
    if(!pHBoxLayoutTmp){throw __FILE__ "Low memory";}
    pLabelTmp = new QLabel(tr(""));
    if(!pLabelTmp){throw __FILE__ "Low memory";}
    pLabelTmp->setScaledContents(true);
    SetImageToLabelStatic(bImageFound,image,USER_FILE_NAME2);
    if(bImageFound){pLabelTmp->setPixmap(image);}
    else {MakeWarning("no file", "");}
    pHBoxLayoutTmp->addWidget(pLabelTmp);
    pLabelTmp->setFixedSize(__SIZE_FOR_IMGS__,__SIZE_FOR_IMGS__);
    pComboTmp1 = new QComboBox;
    if(!pComboTmp1){throw __FILE__ "Low memory";}
    pComboTmp1->setStyleSheet("color: black;""background-color:white;");
    pHBoxLayoutTmp->addWidget(pComboTmp1);
    m_pUsernameWgt->setLayout(pHBoxLayoutTmp);
    m_first_line_lbl.addWidget(m_pUsernameWgt);
    m_pUsernameWgt->setFixedHeight(__HEIGHT__);

    /*//////////////////////////////////////////*/
    line = new QFrame(this);
    line->setFrameShape(QFrame::VLine); // Horizontal line
    line->setFrameShadow(QFrame::Sunken);
    line->setLineWidth(1);
    m_first_line_lbl.addWidget(line);

    /*//////////////////////////////////////////*/
    m_pBalanceWgt1 = new QWidget;
    if(!m_pBalanceWgt1){throw __FILE__ "Low memory";}
    pHBoxLayoutTmp = new QHBoxLayout;
    if(!pHBoxLayoutTmp){throw __FILE__ "Low memory";}
    pLabelTmp = new QLabel(tr(""));
    if(!pLabelTmp){throw __FILE__ "Low memory";}
    pLabelTmp->setScaledContents(true);
    SetImageToLabelStatic(bImageFound,image,BALANCE_FILE_NAME2);
    if(bImageFound){pLabelTmp->setPixmap(image);}
    else {MakeWarning("no file", "");}
    pHBoxLayoutTmp->addWidget(pLabelTmp);
    pLabelTmp->setFixedSize(__SIZE_FOR_IMGS__,__SIZE_FOR_IMGS__);
    pCombo2 = new decent::wallet::ui::gui::AccountBalanceWidget;
    if(!pCombo2){throw __FILE__ "Low memory";}
    pHBoxLayoutTmp->addWidget(pCombo2);
    m_pBalanceWgt1->setLayout(pHBoxLayoutTmp);
    m_first_line_lbl.addWidget(m_pBalanceWgt1);
    m_pBalanceWgt1->setFixedHeight(__HEIGHT__);

    /*//////////////////////////////////////////*/
    line = new QFrame(this);
    line->setFrameShape(QFrame::VLine); // Horizontal line
    line->setFrameShadow(QFrame::Sunken);
    line->setLineWidth(1);
    m_first_line_lbl.addWidget(line);

    /*//////////////////////////////////////////*/
    pWidgetTmp2 = new QWidget;
    if(!pWidgetTmp2){throw __FILE__ "Low memory";}
    pHBoxLayoutTmp = new QHBoxLayout;
    if(!pHBoxLayoutTmp){throw __FILE__ "Low memory";}
    pLabelTmp = new QLabel(tr(""));
    if(!pLabelTmp){throw __FILE__ "Low memory";}
    pLabelTmp->setScaledContents(true);
    SetImageToLabelStatic(bImageFound,image,SEND_FILE_NAME2);
    if(bImageFound){pLabelTmp->setPixmap(image);}
    else {MakeWarning("no file", "");}
    pHBoxLayoutTmp->addWidget(pLabelTmp);
    pLabelTmp->setFixedSize(__SIZE_FOR_IMGS__,__SIZE_FOR_IMGS__);
    pLabelTmp = new QLabel(tr("Send"));
    if(!pLabelTmp){throw __FILE__ "Low memory";}
    pHBoxLayoutTmp->addWidget(pLabelTmp);
    pWidgetTmp2->setLayout(pHBoxLayoutTmp);
    m_first_line_lbl.addWidget(pWidgetTmp2);
    pWidgetTmp2->setFixedHeight(__HEIGHT__);

#if 0
    bool bRet;
    m_first_line_lbl.addWidget(new decent::wallet::ui::gui::NewCheckBox());
#endif // #if 1

    m_browse_cont_tab.setStyleSheet("color: black;""background-color:white;");
    SetAccountBalancesFromStrGUI(std::vector<std::string>());

    m_main_layout.addLayout(&m_first_line_lbl);
    m_main_layout.addWidget(&m_main_tabs2);

    a_pAllLayout->addLayout(&m_main_layout);
}


void CentralWigdet::SetDigitalContentsGUI(const std::vector<decent::wallet::ui::gui::SDigitalContent>& a_vContents)
{
    m_browse_cont_tab.SetDigitalContentsGUI(a_vContents);
}



QString CentralWigdet::getFilterText()const
{
    // enum MAIN_TABS_ENM{BROWSE_CONTENT,TRANSACTIONS,UPLOAD,OVERVIEW,PURCHASED};
    int nActiveTab = m_main_tabs2.currentIndex();

    switch(nActiveTab)
    {
    case BROWSE_CONTENT:
        return m_browse_cont_tab.getFilterText();
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
    // PrepareGUIprivate();
}


void CentralWigdet::make_deleyed_warning()
{
    gui_wallet::makeWarningImediatly(m_DelayedWaringTitle.toLatin1().data(),
                                     m_DelayedWaringText.toLatin1().data(),
                                     m_DelayedWaringDetails.toLatin1().data(),this);
}


void CentralWigdet::resizeEvent ( QResizeEvent * a_event )
{
    QWidget::resizeEvent(a_event);


    /*QString tqsStyle = tr("QTabBar::tab {width: ") +
            QString::number(a_event->size().width()/5-1,10) + tr("px;}");
    QTabBar* pTabBar = m_main_tabs2.tabBar();
    pTabBar->setStyleSheet(tqsStyle);*/

    QTabBar* pTabBar = m_main_tabs2.tabBar();
    pTabBar->resize(size().width(),pTabBar->height());

    int nWidth_small (size().width()*15/100);
    int nWidth_big (size().width()*35/100);
    m_pDcLogoWgt->resize(nWidth_small,m_pDcLogoWgt->height());
    m_pUsernameWgt->resize(nWidth_big,m_pUsernameWgt->height());
    m_pBalanceWgt1->resize(nWidth_big,m_pBalanceWgt1->height());

}



/*/////////////////////////////////////////////////////////////////////////////////////////////////////////*/

static std::string FindImagePathFixedDir(bool& a_bRet,const char* a_image_name, const std::string& a_csCurDir)
{
    std::string::size_type nPosFound;
    std::string cFullPath;
    std::string cCurDir(a_csCurDir);
    FILE* fpFile;

    __DEBUG_APP2__(1,"!!! dir_to_search=\"%s\"\n",cCurDir.c_str());

    for(;;)
    {
        // 1. Try to find image in the directory of executable
        cFullPath = cCurDir + "/" + a_image_name;
        fpFile = fopen(cFullPath.c_str(),"r");
        if( fpFile ){fclose(fpFile);a_bRet = true;return cFullPath;}

        // 2. Try to find in the directory of executable + image folder
        cFullPath = cCurDir + ("/" FOLDER_NAME_FOR_IMAGES2 "/") + a_image_name;
        //if( (_image_).load(cFullPath.c_str()) ){_bRet_ = true;return;}
        fpFile = fopen(cFullPath.c_str(),"r");
        if( fpFile ){fclose(fpFile);a_bRet = true;return cFullPath;}

        // Go one up and try again
        nPosFound = cCurDir.find_last_of(_PATH_DELIMER_);
        if(nPosFound == std::string::npos){a_bRet = false;return "";} // Not found!
        cCurDir.erase(nPosFound,std::string::npos);
    }
}


std::string FindImagePath(bool& a_bRet,const char* a_image_name)
{
    std::string sReturn;
    char vcBuffer[512];
    std::string csCurDir(std::string(getcwd(vcBuffer,511)));

    a_bRet = false;
    sReturn = FindImagePathFixedDir(a_bRet,a_image_name,csCurDir);
    if(a_bRet){return sReturn;}
    sReturn = FindImagePathFixedDir(a_bRet,a_image_name,g_cApplicationPath);
    return sReturn;
}


static void SetImageToLabelStatic(bool& _bRet_,QPixmap& _image_,const char* _image_name_)
{
    std::string sFullPath = FindImagePath(_bRet_,_image_name_);
    if(_bRet_){(_image_).load(sFullPath.c_str());}
}
