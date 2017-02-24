/*
 *	File: main_gui_wallet.cpp
 *
 *	Created on: Nov 11, 2016
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */

#include <stdio.h>
#include <wchar.h>
#include <string.h>
#include <string>
#ifndef TEST_SIMPLE_APP
#include "gui_wallet_application.hpp"
#include "gui_wallet_mainwindow.hpp"
#endif

#ifdef WIN32
#define LAST_SYS_CHAR   '\\'
#else
#define LAST_SYS_CHAR   '/'
#endif

extern int g_nDebugApplication;
std::string g_cApplicationPath ;
extern InGuiThreatCaller* s_pWarner;

#if 0

class QZebraWidget : public QWidget
{
public:
    QZebraWidget()
    {
        m_main_layout.setMargin(0);
        m_main_layout.setContentsMargins(0,0,0,0);
        QWidget* pNextWidget;

        pNextWidget = new QWidget;
        pNextWidget->setStyleSheet("background-color:white;");
        m_main_layout.addWidget(pNextWidget);


        pNextWidget = new QWidget;
        pNextWidget->setStyleSheet("background-color:black;");
        m_main_layout.addWidget(pNextWidget);

        pNextWidget = new QWidget;
        pNextWidget->setStyleSheet("background-color:white;");
        m_main_layout.addWidget(pNextWidget);


        pNextWidget = new QWidget;
        pNextWidget->setStyleSheet("background-color:black;");
        m_main_layout.addWidget(pNextWidget);

        setLayout(&m_main_layout);

        resize(200,500);
    }


private:
    QVBoxLayout m_main_layout;
};

#endif


int main(int argc, char* argv[])
{

#ifdef TEST_SIMPLE_APP

    printf("Hello world! argc=%d, argv=%p\n",argc,argv);

#else  // #ifdef TEST_SIMPLE_APP

    const char* cpcPath = strrchr(argv[0],LAST_SYS_CHAR);

    if(cpcPath)
    {
        int nStrLen = (int)(((size_t)cpcPath) - ((size_t)argv[0]));
        if(nStrLen){g_cApplicationPath = std::string(argv[0],nStrLen);}
        else {g_cApplicationPath = std::string(".");}
    }
    else {g_cApplicationPath = std::string(".");}

    static std::string scAppFullPath ( argv[0] );


    for(int i(1); i<argc;)
    {
        if((strcmp(argv[i],"--debug-application")==0)||(strcmp(argv[i],"-dba")==0))
        {
            g_nDebugApplication = 1;
            if (i<(--argc))
            {
                memmove(argv+i,argv+i+1,(argc-i)*sizeof(char*));
                g_nDebugApplication = atoi(argv[i]);
                if (i<(--argc)){memmove(argv+i,argv+i+1,(argc-i)*sizeof(char*));}
            }
        }
        else if((strcmp(argv[i],"--bgr-color")==0)||(strcmp(argv[i],"-b")==0))
        {
            if (i<(--argc))
            {
                memmove(argv+i,argv+i+1,(argc-i)*sizeof(char*));
                // ...
            }
        }
        else
        {
            ++i;
        }
    }

    __DEBUG_APP2__(1, "argv[0]=\"%s\"\n"
                                   "app dir. = \"%s\"\n",
                                   scAppFullPath.c_str(),g_cApplicationPath.c_str());

    freopen( "/dev/null", "w", stderr);

    try{

#ifdef CREATE_NEW_APP
    gui_wallet::application aApp(argc,argv);
#else  // #ifndef CREATE_NEW_APP
    QApplication aApp(argc,argv);
#endif // #ifndef CREATE_NEW_APP

#if 0
    decent::wallet::ui::gui::ContentDetailsBougth aBouth;
    decent::wallet::ui::gui::SDigitalContent    aContent;
    aContent.author = "poxos";
    aContent.AVG_rating = 0.;
    aContent.created = "10.02.2017";
    aContent.URI = "URI111";
    aBouth.execCDD("davit",aContent);
    return 0;

    QZebraWidget aWidg;
    aWidg.show();
    aApp.exec();
    return 0;

#endif // #if 0

    gui_wallet::Mainwindow_gui_wallet aMainWindow;

#ifndef CREATE_NEW_APP
    qRegisterMetaType<std::string>( "std::string" );
    qRegisterMetaType<WarnYesOrNoFuncType>( "WarnYesOrNoFuncType" );
    qRegisterMetaType<int64_t>( "int64_t" );
    qRegisterMetaType<TypeCallbackSetNewTaskGlb2>( "TypeCallbackSetNewTaskGlb2" );
    qRegisterMetaType<TypeCallbackSetNewTaskGlb3>( "TypeCallbackSetNewTaskGlb3" );
    qRegisterMetaType<fc::variant>( "fc::variant" );
    qRegisterMetaType<decent::wallet::ui::gui::SDigitalContent>( "decent::wallet::ui::gui::SDigitalContent" );

    s_pWarner = new InGuiThreatCaller;
    if(!s_pWarner)
    {
        throw "No enough memory";
    }
#endif // #ifndef CREATE_NEW_APP

    aMainWindow.show();
    aApp.exec();

#ifndef CREATE_NEW_APP
    delete s_pWarner;
#endif // #ifndef CREATE_NEW_APP

#endif  // #ifdef TEST_SIMPLE_APP

    }
    catch(const char* a_ext_str)
    {
        __DEBUG_APP2__(0,"%s\n",a_ext_str);
    }
    catch(...)
    {
        __DEBUG_APP2__(0,"Unknown exception!\n");
    }

    return 0;
}

