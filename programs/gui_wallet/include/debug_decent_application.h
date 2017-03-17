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

#define _DEF_LOG_LEVEL_ 2

#define __SOURCE_FILE__ (strrchr(__FILE__,'/') ? strrchr(__FILE__,'/') + 1 : (strrchr(__FILE__,'\\') ? strrchr(__FILE__,'\\') + 1 : __FILE__))


#define __DEBUG_APP2__(__lev__,...)

#endif // DEBUG_DECENT_APPLICATION_H
