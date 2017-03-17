// decent_tools_rwlock
/*
 *	File: decent_tools_rwlock.cpp
 *
 *	Created on: 05 Feb 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */


#include "decent_tools_rwlock.hpp"
#include <stddef.h>
#include <stdio.h>
#include "debug_decent_application.h"

#define __LOG_LEVEL__   3


decent::tools::RWLock::RWLock()
{

#ifdef WIN32
    m_nReadersCount = 0;
    m_vRWMutexes[0] = CreateMutex(NULL,FALSE,NULL);
    m_vRWMutexes[1] = CreateEvent(NULL,TRUE,TRUE,NULL);
#else  // #ifdef WIN32
    pthread_rwlock_init(&m_rwLock,NULL);
#endif  // #ifdef WIN32

}



decent::tools::RWLock::~RWLock()
{
#ifdef WIN32
    CloseHandle(m_vRWMutexes[0]);
    CloseHandle(m_vRWMutexes[1]);
#else  // #ifdef WIN32
    pthread_rwlock_destroy(&m_rwLock);
#endif  // #ifdef WIN32
}


void decent::tools::RWLock::lock()
{

    __DEBUG_APP2__(__LOG_LEVEL__,"++++ locking!!!\n");

#ifdef WIN32
    WaitForSingleObject(m_vRWMutexes[0],INFINITE);
    if(!m_nReadersCount) // This should be done with real atomic routine (InterlockedExchange...)
    {
        ResetEvent(m_vRWMutexes[1]);
    }
    ReleaseMutex(m_vRWMutexes[0]);
#else  // #ifdef WIN32
    pthread_rwlock_rdlock(&m_rwLock);
#endif  // #ifdef WIN32

    __DEBUG_APP2__(__LOG_LEVEL__,"++++ locked!!!\n");
}


void decent::tools::RWLock::write_lock()
{
    __DEBUG_APP2__(__LOG_LEVEL__,"++++ write_locking!!!\n");

#ifdef WIN32
    WaitForMultipleObjectsEx(2,m_vRWMutexes,TRUE/*wait all*/,INFINITE,TRUE);
#else  // #ifdef WIN32
    pthread_rwlock_wrlock(&m_rwLock);
#endif  // #ifdef WIN32

    __DEBUG_APP2__(__LOG_LEVEL__,"++++ write_locked!!!\n");
}


void decent::tools::RWLock::unlock()
{
    __DEBUG_APP2__(__LOG_LEVEL__,"----- unlocking!!!\n");

#ifdef WIN32
#error windows part should be done (lasy to do now)
#else  // #ifdef WIN32
    pthread_rwlock_unlock(&m_rwLock);
#endif  // #ifdef WIN32

    __DEBUG_APP2__(__LOG_LEVEL__,"----- unlocked!!!\n");
}
