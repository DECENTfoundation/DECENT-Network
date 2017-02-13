/*
 *	File: gui_wallet_centralwigdet.cpp
 *
 *	Created on: Nov 11, 2016
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */

#define DECENT_LOGO_FILE_NAME       "dc_logo.jpg"
#define GLOBE_FILE_NAME             "globe.jpg"
#define MAN_LOGO_FILE_NAME          "man_logo.jpg"
#define FOLDER_NAME_FOR_IMAGES      "images"
#ifdef __APPLE__
#define DECENT_IMGS_INITIAL_PATH    "../../../../../../" FOLDER_NAME_FOR_IMAGES "/"
#else
#define DECENT_IMGS_INITIAL_PATH    "../../../" FOLDER_NAME_FOR_IMAGES "/"
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
      m_llnBalance(0)
{
    m_imageLabel = new QLabel;
    PrepareGUIprivate(a_pAllLayout);
}


CentralWigdet::~CentralWigdet()
{
    //delete m_imageLabel;
}


void CentralWigdet::SetAccountBalanceGUI(long long int a_llnBallance,const std::string& a_balance_name)
{
    if(a_llnBallance>=0){m_llnBalance = a_llnBallance;}
    QString aBalance = QString::number(m_llnBalance,10) + tr(" ") + tr(a_balance_name.c_str());
    m_balanceLabel.setText(aBalance);
}


const long long int& CentralWigdet::GetAccountBalance()const
{
    return m_llnBalance;
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
        cFullPath = cCurDir + ("/" FOLDER_NAME_FOR_IMAGES "/") + _image_name_;
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



void CentralWigdet::PrepareGUIprivate(class QBoxLayout* a_pAllLayout)
{
    bool bImageFound(true);
    m_main_tabs.addTab(&m_browse_cont_tab,tr("BROWSE CONTENT"));
    m_main_tabs.addTab(&m_trans_tab,tr("TRANSACTIONS"));
    m_main_tabs.addTab(&m_Upload_tab,tr("UPLOAD"));
    m_main_tabs.addTab(&m_Overview_tab,tr("OVERVIEW"));

    QTabBar* pTabBar = m_main_tabs.tabBar();

    pTabBar->setStyleSheet("QTabBar::tab:disabled {"
        "width: 200px;"
        "color: transparent;"
        "background: transparent;"
     "}");
    //pTabBar->setStyleSheet("QTabBar{disabled{width:100px;color:transparent;background:transparent;}}");
    //m_main_tabs.addTab(new QWidget(),tr(""));
    m_main_tabs.addTab(new QWidget(),tr(""));
    m_main_tabs.setTabEnabled(4,false);
    m_main_tabs.setTabEnabled(5,false);
    /* /// Should be fixed, and set to widget of project defined class*/
    QString aSendReceiveText;
#if 0
    wchar_t vwcStr[2]={0};
    vwcStr[0] = 0x2192;
    vwcStr[1] = (wchar_t)0;
    aSendReceiveText.fromWCharArray(vwcStr);
#else
    aSendReceiveText = tr("-> SEND");
    //aSendReceiveText.fromWCharArray(L"Русский");
#endif
    m_main_tabs.addTab(new QWidget(),aSendReceiveText);
    m_main_tabs.addTab(new QWidget(),tr("<- RECEIVE"));

    QPixmap image;

    SetImageToLabelStatic(bImageFound,image,DECENT_LOGO_FILE_NAME);
    if(bImageFound){m_imageLabel->setPixmap(image);}
    else
    {   
        m_DelayedWaringTitle = tr("no logo file");
        m_DelayedWaringText = tr(DECENT_LOGO_FILE_NAME " file can not be found!");
        m_DelayedWaringDetails = tr(
                "file '" DECENT_LOGO_FILE_NAME "' could not be found\n"
                "The search paths are the following:\n"
                "1. the current directory \n"
                "2. the 'image'' folder in the current directory\n"
                "3. the folder" DECENT_IMGS_INITIAL_PATH "\n"
                "To see the logo, please put the logo file to the directories\n"
                "mentioned above and then rerun the application");
        QTimer::singleShot(100, this, SLOT(make_deleyed_warning()));
        m_imageLabel->setText("DC");
    }

    m_first_line_widget.setStyleSheet("background-color:black;");
    m_search_box.setStyleSheet("background-color:white;");

    m_first_line_layout.addWidget(m_imageLabel);
    m_first_line_layout.addWidget(&m_search_box);
    /* /// Probably should be Modified, to skip new ...! */
    QLabel* pBalanceLabel = new QLabel(tr("Balance"));
    QPalette aPal = pBalanceLabel->palette();
    aPal.setColor(QPalette::Window, Qt::black);
    aPal.setColor(QPalette::WindowText, Qt::white);
    pBalanceLabel->setPalette(aPal);
    m_first_line_layout.addWidget(pBalanceLabel);

    /*/////////////// pGlobeLabel ////////////////////////*/
    QLabel* pGlobeLabel = new QLabel;
    SetImageToLabelStatic(bImageFound,image,GLOBE_FILE_NAME);
    if(bImageFound){pGlobeLabel->setPixmap(image);}
    else
    {
        m_DelayedWaringTitle = tr("no glob file");
        m_DelayedWaringText = tr(GLOBE_FILE_NAME " file can not be found!");
        m_DelayedWaringDetails = tr(
                "file '" GLOBE_FILE_NAME "' could not be found\n"
                "The search paths are the following:\n"
                "1. the current directory \n"
                "2. the 'image'' folder in the current directory\n"
                "3. the folder" DECENT_IMGS_INITIAL_PATH "\n"
                "To see the logo, please put the logo file to the directories\n"
                "mentioned above and then rerun the application");
        QTimer::singleShot(100, this, SLOT(make_deleyed_warning()));
        pGlobeLabel->setText("Glb");
    }
    m_first_line_layout.addWidget(pGlobeLabel);
    /*/////////////// end pGlobeLabel ////////////////////*/

    /* /// End Probably should be Modified, to skip new ...! */

    //m_balanceLabel.setText(tr("0 DTC"));
    SetAccountBalanceGUI();
    aPal = m_balanceLabel.palette();
    aPal.setColor(QPalette::Window, Qt::black);
    aPal.setColor(QPalette::WindowText, Qt::white);
    m_balanceLabel.setPalette(aPal);
    m_first_line_layout.addWidget(&m_balanceLabel);

    /*/////////////// pManLabel ////////////////////////*/
    QLabel* pManLabel = new QLabel;
    SetImageToLabelStatic(bImageFound,image,MAN_LOGO_FILE_NAME);
    if(bImageFound){pManLabel->setPixmap(image);}
    else
    {
        m_DelayedWaringTitle = tr("no glob file");
        m_DelayedWaringText = tr(MAN_LOGO_FILE_NAME" file can not be found!");
        m_DelayedWaringDetails = tr(
                "file '" MAN_LOGO_FILE_NAME "' could not be found\n"
                "The search paths are the following:\n"
                "1. the current directory \n"
                "2. the 'image'' folder in the current directory\n"
                "3. the 'image'' folder in the \"../../../\"\n"
                "To see the logo, please put the logo file to the directories\n"
                "mentioned above and then rerun the application");
        QTimer::singleShot(100, this, SLOT(make_deleyed_warning()));
        pManLabel->setText("Man");
    }
    m_first_line_layout.addWidget(pManLabel);
    /*/////////////// end pManLabel ////////////////////*/

    //m_users_list
    m_users_list.setStyleSheet("color: white;""background-color:red;");
    aPal = m_users_list.palette();
    aPal.setColor(QPalette::Window, Qt::red);
    aPal.setColor(QPalette::WindowText, Qt::white);
    m_users_list.setPalette(aPal);
    //m_users_list.addItem(tr("Username"));
    m_first_line_layout.addWidget(&m_users_list);

    m_first_line_widget.setLayout(&m_first_line_layout);
    m_main_layout.addWidget(&m_first_line_widget);

    m_main_layout.addWidget(&m_main_tabs);
    //setLayout(&m_main_layout);
    a_pAllLayout->addLayout(&m_main_layout);
}


void CentralWigdet::AddNewUserGUI(const std::string& a_user_name)
{
    m_users_list.addItem(tr(a_user_name.c_str()));
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


void CentralWigdet::resizeEvent ( QResizeEvent * event )
{
    QWidget::resizeEvent(event);
#if 0
    QSize aImgLbSize = m_imageLabel->size();

    m_first_line_twidget.setFixedHeight(/*event->size().width()-4,*/aImgLbSize.height());
    m_first_line_twidget.setRowHeight(0,aImgLbSize.height());
    m_first_line_twidget.setColumnWidth(0,aImgLbSize.width());
#endif

}
