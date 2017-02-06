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

int g_nDebugApplication = 0;
std::string g_cApplicationPath ;

int main(int argc, char* argv[])
{

#ifdef TEST_SIMPLE_APP
    printf("Hello world! argc=%d, argv=%p\n",argc,argv);
#else

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

    if(g_nDebugApplication){printf("argv[0]=\"%s\"\n"
                                   "app dir. = \"%s\"\n",
                                   scAppFullPath.c_str(),g_cApplicationPath.c_str());}

    freopen( "/dev/null", "w", stderr);

    try{

    gui_wallet::application aApp(argc,argv);

#if 0
    //QString testString;
    wprintf(L"fwide=%d\n",fwide(stdout,1));
    wprintf(L"Русский\n");
    wchar_t i = 0x2192;
    //for( i=4000; i<6000;++i)
    {
        //wprintf(L"chr=%c(code=%d)\n",i,(int)i);
        wprintf(L"chr='%lc'(code=%d)\n",i,(int)i);
    }
#else
    gui_wallet::Mainwindow_gui_wallet aMainWindow;
    aMainWindow.show();
    aApp.exec();
    //delete g_pApplicationPath;
#endif
#endif

    }
    catch(const char* a_ext_str)
    {
        printf("%s\n",a_ext_str);
    }
    catch(...)
    {
        printf("Unknown exception!\n");
    }

    return 0;
}

