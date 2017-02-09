//debug_decent_application
/*
 *	File: debug_decent_application.h
 *
 *	Created on: 05 Feb 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */

#ifndef DEBUG_DECENT_APPLICATION_H
#define DEBUG_DECENT_APPLICATION_H

#include <stdio.h>
#include <string.h>
extern int g_nDebugApplication;

#define _DEF_LOG_LEVEL_ 2

#define __SOURCE_FILE__ (strrchr(__FILE__,'/') ? strrchr(__FILE__,'/') + 1 : (strrchr(__FILE__,'\\') ? strrchr(__FILE__,'\\') + 1 : __FILE__))

#define __DEBUG_APP2__(__lev__,...) \
    do{ \
        if(g_nDebugApplication>=(__lev__)){ \
            printf("fl:\"%s\", ln:%d, fnc:%s :   ",__SOURCE_FILE__,__LINE__,__FUNCTION__);printf(__VA_ARGS__);printf("\n");}\
    }while(0)

#endif // DEBUG_DECENT_APPLICATION_H
