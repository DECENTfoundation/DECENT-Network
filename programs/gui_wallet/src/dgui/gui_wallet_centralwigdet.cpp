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

extern std::string g_cApplicationPath ;
extern int g_nDebugApplication;


using namespace gui_wallet;

CentralWigdet::CentralWigdet(class QBoxLayout* a_pAllLayout)
    :
#ifdef __TRY_LABEL_INSTEAD_OF_TABLE__
      m_first_line_lbl()
#else
      m_first_line_widget2(1,NUMBER_OF_FRST_LINE_ELEMS)
#endif
{
    setStyleSheet("color:black;""background-color:white;");
    m_main_tabs.setStyleSheet("color:black;""background-color:white;");
    PrepareGUIprivate(a_pAllLayout);
}


CentralWigdet::~CentralWigdet()
{
    //delete m_imageLabel;
}


void CentralWigdet::SetAccountBalancesFromStrGUI(const std::vector<std::string>& a_balances_and_names)
{
    //m_balanceLabel.setText(tr(a_balance_and_name.c_str()));
    QComboBox* pBalanceCombo = (QComboBox*)GetWidgetFromTable2(BALANCE,1);
    pBalanceCombo->clear();
    const int cnBalances(a_balances_and_names.size());

    if(cnBalances)
    {
        for(int i(0); i<cnBalances; ++i)
        {
            pBalanceCombo->addItem(tr(a_balances_and_names[i].c_str()));
        }
    }
    else
    {
        pBalanceCombo->addItem(tr("0 DECENT"));
    }

    pBalanceCombo->setCurrentIndex(0);
    //m_balanceCombo.lineEdit()->setAlignment(Qt::AlignRight);
}



#if 1

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


static void MakeWarning(const char* /*warn*/, const char* /*warn_details*/)
{}


static void SetImageToLabelStaticFixedPath(bool& _bRet_,QPixmap& _image_,const char* _image_name_, const std::string& a_csCurDir)
{
    std::string::size_type nPosFound;
    std::string cFullPath;
    std::string cCurDir(a_csCurDir);

    //printf("g_nDebugApplication=%d\n",g_nDebugApplication);
    if(g_nDebugApplication){printf("!!! dir_to_search=\"%s\"\n",cCurDir.c_str());}

    for(;;)
    {
        // 1. Try to find image in the directory of executable
        cFullPath = cCurDir + "/" + _image_name_;
        if( (_image_).load(cFullPath.c_str()) ){_bRet_ = true;return;}

        // 2. Try to find in the directory of executable + image folder
        cFullPath = cCurDir + ("/" FOLDER_NAME_FOR_IMAGES2 "/") + _image_name_;
        if( (_image_).load(cFullPath.c_str()) ){_bRet_ = true;return;}

        // Go one up and try again
        nPosFound = cCurDir.find_last_of(_PATH_DELIMER_);
        if(nPosFound == std::string::npos){_bRet_ = false;return;} // Not found!
        cCurDir.erase(nPosFound,std::string::npos);
    }
}


static void SetImageToLabelStatic(bool& _bRet_,QPixmap& _image_,const char* _image_name_)
{
    char vcBuffer[512];
    std::string csCurDir(std::string(getcwd(vcBuffer,511)));

    _bRet_ = false;
    SetImageToLabelStaticFixedPath(_bRet_,_image_,_image_name_,csCurDir);
    if(_bRet_){return;}
    SetImageToLabelStaticFixedPath(_bRet_,_image_,_image_name_,g_cApplicationPath);
}


#else  // #if 1/0

#define SetImageToLabelStatic(_bRet_,_image_,_image_name_) \
        do{ \
            (_bRet_)=true; \
            if( !(_image_).load(DECENT_IMGS_INITIAL_PATH _image_name_) ) {\
                /* Search for couple of other places */ \
                if( !(_image_).load("./" _image_name_) ) {\
                    if( !(_image_).load((_image_name_)) ){\
                        std::string cImageFlName = (*g_pApplicationPath) + (DECENT_IMGS_INITIAL_PATH _image_name_); \
                        if( !(_image_).load(cImageFlName.c_str()) ){(_bRet_)=false;} \
                    } \
                }\
             }\
        }while(0)

#endif // #if 1/0


QComboBox* CentralWigdet::usersCombo()
{
    return (QComboBox*)GetWidgetFromTable2(USERNAME,1);
}


QWidget* CentralWigdet::GetWidgetFromTable2(int a_nColumn, int a_nWidget)
{
#ifdef __TRY_LABEL_INSTEAD_OF_TABLE__

    a_nColumn = 2*a_nColumn;
    //if(a_nColumn>1){++a_nColumn;}
    __DEBUG_APP2__(0," ");
    return m_first_line_lbl.itemAt(a_nColumn)->layout()->itemAt(a_nWidget)->widget();

#else  // #ifdef __TRY_LABEL_INSTEAD_OF_TABLE__
    QWidget* pWidget = m_first_line_widget2.cellWidget(0, a_nColumn);
    QHBoxLayout* pHLayout = (QHBoxLayout*)pWidget->layout();
    return pHLayout->itemAt(a_nWidget)->widget();
#endif  // #ifdef __TRY_LABEL_INSTEAD_OF_TABLE__
}


#define __SIZE_FOR_IMGS__   50
#define __HEIGHT__  100

void CentralWigdet::PrepareGUIprivate(class QBoxLayout* a_pAllLayout)
{
    bool bImageFound(true);
    m_main_tabs.addTab(&m_browse_cont_tab,tr("Browse Content"));
    m_main_tabs.addTab(&m_trans_tab,tr("Transactions"));
    m_main_tabs.addTab(&m_Upload_tab,tr("Upload"));
    m_main_tabs.addTab(&m_Overview_tab,tr("Overview"));
    m_main_tabs.addTab(&m_Purchased_tab,tr("Purchased"));

    QTabBar* pTabBar = m_main_tabs.tabBar();

    pTabBar->setStyleSheet("QTabBar::tab:disabled {"
        "width: 200px;"
        "color: transparent;"
        "background: transparent;"
     "}");
    //pTabBar->setStyleSheet("QTabBar{disabled{width:100px;color:transparent;background:transparent;}}");

#ifdef __TRY_LABEL_INSTEAD_OF_TABLE__
    //
    //m_first_line_lbl
    //QWidget* pWidgetTmp;
    QPixmap image;
    QLabel* pLabelTmp;
    QHBoxLayout* pHBoxLayoutTmp;
    QComboBox* pComboTmp;
    QFrame *line;
    /*////////////////////////////////////////////////////////////////////////////////////*/

    pLabelTmp = new QLabel(tr(""));
    if(!pLabelTmp){throw __FILE__ "Low memory";}
    pLabelTmp->setScaledContents(true);
    SetImageToLabelStatic(bImageFound,image,DECENT_LOGO_FILE_NAME2);
    if(bImageFound){pLabelTmp->setPixmap(image);}
    else {pLabelTmp->setText("DC");MakeWarning("no file", "");}
    m_first_line_lbl.addWidget(pLabelTmp);
    pLabelTmp->setFixedSize(__SIZE_FOR_IMGS__,__SIZE_FOR_IMGS__);

    line = new QFrame(this);
    line->setFrameShape(QFrame::VLine); // Horizontal line
    line->setFrameShadow(QFrame::Sunken);
    line->setLineWidth(1);
    m_first_line_lbl.addWidget(line);

    //pWidgetTmp = new QWidget;
    //if(!pWidgetTmp){throw __FILE__ "Low memory";}
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
    pComboTmp = new QComboBox;
    if(!pComboTmp){throw __FILE__ "Low memory";}
    pComboTmp->setStyleSheet("color: black;""background-color:white;");
    pHBoxLayoutTmp->addWidget(pComboTmp);
    //pWidgetTmp->setLayout(pHBoxLayoutTmp);
    //m_first_line_widget2.setCellWidget(0,USERNAME,pWidgetTmp);
    m_first_line_lbl.addLayout(pHBoxLayoutTmp);

    line = new QFrame(this);
    line->setFrameShape(QFrame::VLine); // Horizontal line
    line->setFrameShadow(QFrame::Sunken);
    line->setLineWidth(1);
    m_first_line_lbl.addWidget(line);

    //pWidgetTmp = new QWidget;
    //if(!pWidgetTmp){throw __FILE__ "Low memory";}
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
    pComboTmp = new QComboBox;
    if(!pComboTmp){throw __FILE__ "Low memory";}
    pComboTmp->setStyleSheet("color:black;""background-color:white;");
    //pComboTmp->setFixedWidth(190);
    pComboTmp->setEditable(true);
    // Second : Put the lineEdit in read-only mode
    pComboTmp->lineEdit()->setReadOnly(true);
    // Third  : Align the lineEdit to right
    pComboTmp->lineEdit()->setAlignment(Qt::AlignRight);
    pHBoxLayoutTmp->addWidget(pComboTmp);
    //pWidgetTmp->setLayout(pHBoxLayoutTmp);
    //m_first_line_widget2.setCellWidget(0,BALANCE,pWidgetTmp);
    m_first_line_lbl.addLayout(pHBoxLayoutTmp);

    line = new QFrame(this);
    line->setFrameShape(QFrame::VLine); // Horizontal line
    line->setFrameShadow(QFrame::Sunken);
    line->setLineWidth(1);
    m_first_line_lbl.addWidget(line);

    //pWidgetTmp = new QWidget;
    //if(!pWidgetTmp){throw __FILE__ "Low memory";}
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
    //pWidgetTmp->setLayout(pHBoxLayoutTmp);
    //m_first_line_widget2.setCellWidget(0,SEND_,pWidgetTmp);
    m_first_line_lbl.addLayout(pHBoxLayoutTmp);

    //m_first_line_widget2.setStyleSheet("background-color:white;");


#else  // #ifdef __TRY_LABEL_INSTEAD_OF_TABLE__
    QWidget* pWidgetTmp;
    QPixmap image;
    QLabel* pLabelTmp;
    QHBoxLayout* pHBoxLayoutTmp;
    QComboBox* pComboTmp;
    /*////////////////////////////////////////////////////////////////////////////////////*/
    m_first_line_widget2.horizontalHeader()->hide();
    m_first_line_widget2.verticalHeader()->hide();

    pLabelTmp = new QLabel(tr(""));
    if(!pLabelTmp){throw __FILE__ "Low memory";}
    pLabelTmp->setScaledContents(true);
    SetImageToLabelStatic(bImageFound,image,DECENT_LOGO_FILE_NAME2);
    if(bImageFound){pLabelTmp->setPixmap(image);}
    else {pLabelTmp->setText("DC");MakeWarning("no file", "");}
    m_first_line_widget2.setCellWidget(0,DECENT_LOGO,pLabelTmp);

    pWidgetTmp = new QWidget;
    if(!pWidgetTmp){throw __FILE__ "Low memory";}
    pHBoxLayoutTmp = new QHBoxLayout;
    if(!pHBoxLayoutTmp){throw __FILE__ "Low memory";}
    pLabelTmp = new QLabel(tr(""));
    if(!pLabelTmp){throw __FILE__ "Low memory";}
    pLabelTmp->setScaledContents(true);
    SetImageToLabelStatic(bImageFound,image,USER_FILE_NAME2);
    if(bImageFound){pLabelTmp->setPixmap(image);}
    else {MakeWarning("no file", "");}
    pHBoxLayoutTmp->addWidget(pLabelTmp);
    pComboTmp = new QComboBox;
    if(!pComboTmp){throw __FILE__ "Low memory";}
    pComboTmp->setStyleSheet("color: black;""background-color:white;");
    pHBoxLayoutTmp->addWidget(pComboTmp);
    pWidgetTmp->setLayout(pHBoxLayoutTmp);
    m_first_line_widget2.setCellWidget(0,USERNAME,pWidgetTmp);

    pWidgetTmp = new QWidget;
    if(!pWidgetTmp){throw __FILE__ "Low memory";}
    pHBoxLayoutTmp = new QHBoxLayout;
    if(!pHBoxLayoutTmp){throw __FILE__ "Low memory";}
    pLabelTmp = new QLabel(tr(""));
    if(!pLabelTmp){throw __FILE__ "Low memory";}
    pLabelTmp->setScaledContents(true);
    SetImageToLabelStatic(bImageFound,image,BALANCE_FILE_NAME2);
    if(bImageFound){pLabelTmp->setPixmap(image);}
    else {MakeWarning("no file", "");}
    pHBoxLayoutTmp->addWidget(pLabelTmp);
    pComboTmp = new QComboBox;
    if(!pComboTmp){throw __FILE__ "Low memory";}
    pComboTmp->setStyleSheet("color:black;""background-color:white;");
    //pComboTmp->setFixedWidth(190);
    pComboTmp->setEditable(true);
    // Second : Put the lineEdit in read-only mode
    pComboTmp->lineEdit()->setReadOnly(true);
    // Third  : Align the lineEdit to right
    pComboTmp->lineEdit()->setAlignment(Qt::AlignRight);
    pHBoxLayoutTmp->addWidget(pComboTmp);
    pWidgetTmp->setLayout(pHBoxLayoutTmp);
    m_first_line_widget2.setCellWidget(0,BALANCE,pWidgetTmp);

    pWidgetTmp = new QWidget;
    if(!pWidgetTmp){throw __FILE__ "Low memory";}
    pHBoxLayoutTmp = new QHBoxLayout;
    if(!pHBoxLayoutTmp){throw __FILE__ "Low memory";}
    pLabelTmp = new QLabel(tr(""));
    if(!pLabelTmp){throw __FILE__ "Low memory";}
    pLabelTmp->setScaledContents(true);
    SetImageToLabelStatic(bImageFound,image,SEND_FILE_NAME2);
    if(bImageFound){pLabelTmp->setPixmap(image);}
    else {MakeWarning("no file", "");}
    pHBoxLayoutTmp->addWidget(pLabelTmp);
    pLabelTmp = new QLabel(tr("Send"));
    if(!pLabelTmp){throw __FILE__ "Low memory";}
    pHBoxLayoutTmp->addWidget(pLabelTmp);
    pWidgetTmp->setLayout(pHBoxLayoutTmp);
    m_first_line_widget2.setCellWidget(0,SEND_,pWidgetTmp);

    m_first_line_widget2.setStyleSheet("background-color:white;");
    /*////////////////////////////////////////////////////////////////////////////////////*/
#endif  // #ifdef __TRY_LABEL_INSTEAD_OF_TABLE__

    m_main_tabs.setStyleSheet("color: green;""background-color:white;");
    m_browse_cont_tab.setStyleSheet("color: black;""background-color:white;");

    SetAccountBalancesFromStrGUI(std::vector<std::string>());

#ifdef __TRY_LABEL_INSTEAD_OF_TABLE__
    m_main_layout.addLayout(&m_first_line_lbl);
#else  // #ifdef __TRY_LABEL_INSTEAD_OF_TABLE__
    m_main_layout.addWidget(&m_first_line_widget2);
#endif  // #ifdef __TRY_LABEL_INSTEAD_OF_TABLE__
    m_main_layout.addWidget(&m_main_tabs);

    a_pAllLayout->addLayout(&m_main_layout);
}


void CentralWigdet::SetDigitalContentsGUI(const std::vector<decent::wallet::ui::gui::SDigitalContent>& a_vContents)
{
    m_browse_cont_tab.SetDigitalContentsGUI(a_vContents);
}



QString CentralWigdet::getFilterText()const
{
    // enum MAIN_TABS_ENM{BROWSE_CONTENT,TRANSACTIONS,UPLOAD,OVERVIEW,PURCHASED};
    int nActiveTab = m_main_tabs.currentIndex();

    switch(nActiveTab)
    {
    case BROWSE_CONTENT:
        return m_browse_cont_tab.getFilterText();
    case PURCHASED:
        return tr("bought:") + m_Purchased_tab.getFilterText();
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

#ifdef __TRY_LABEL_INSTEAD_OF_TABLE__

    m_main_tabs.resize(a_event->size().width(),m_main_tabs.size().height());
    m_browse_cont_tab.resize(a_event->size().width(),m_browse_cont_tab.size().height());

#else // #ifdef __TRY_LABEL_INSTEAD_OF_TABLE__

    //m_first_line_widget2.setFixedSize(a_event->size().width(),100);
    QWidget* pWidgetToResize;
    QSize tqsTableSize = m_first_line_widget2.size();
    int nSizeForOne = tqsTableSize.width()/NUMBER_OF_FRST_LINE_ELEMS-1;

    //enum FRST_LINE_ELEMS{DECENT_LOGO,USERNAME,BALANCE,SEND_,NUMBER_OF_FRST_LINE_ELEMS};
    pWidgetToResize = m_first_line_widget2.cellWidget(0,DECENT_LOGO);
    pWidgetToResize->setFixedSize(__SIZE_FOR_IMGS__,__SIZE_FOR_IMGS__);
    m_first_line_widget2.setColumnWidth(0,nSizeForOne);
    //m_first_line_widget2.
    for(int i(1); i<NUMBER_OF_FRST_LINE_ELEMS;++i)
    {
        m_first_line_widget2.setColumnWidth(i,nSizeForOne);
        pWidgetToResize = GetWidgetFromTable2(i,0);
        pWidgetToResize->setFixedSize(__SIZE_FOR_IMGS__,__SIZE_FOR_IMGS__);
        //pWidgetToResize = GetWidgetFromTable2(i,1);
        //pWidgetToResize->setFixedHeight(__HEIGHT__);
    }

    //m_first_line_widget2.setRowHeight(0,50);

    //m_main_table.setColumnWidth(0,40*aInfWidgSize.width()/100);
    //m_main_table.setColumnWidth(1,59*aInfWidgSize.width()/100);

    //m_first_line_widget2.verticalScrollBar()->hide();
    m_first_line_widget2.setRowHeight(0,__HEIGHT__);
    m_first_line_widget2.setFixedHeight(__HEIGHT__+5);

#endif // #ifdef __TRY_LABEL_INSTEAD_OF_TABLE__

}
