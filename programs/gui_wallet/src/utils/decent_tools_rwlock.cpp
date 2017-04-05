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
    m_wrMutex = CreateMutex(NULL,FALSE,NULL);
    m_rdHelperMutex = CreateMutex(NULL, FALSE, NULL);
    m_noReaderEvent = CreateEvent(NULL,TRUE,TRUE,NULL);
#else  // #ifdef WIN32
    pthread_rwlock_init(&m_rwLock,NULL);
#endif  // #ifdef WIN32

}



decent::tools::RWLock::~RWLock()
{
#ifdef WIN32
    CloseHandle(m_wrMutex);
    CloseHandle(m_rdHelperMutex);
    CloseHandle(m_noReaderEvent);
#else  // #ifdef WIN32
    pthread_rwlock_destroy(&m_rwLock);
#endif  // #ifdef WIN32
}


void decent::tools::RWLock::lock()
{

    __DEBUG_APP2__(__LOG_LEVEL__,"++++ locking!!!\n");

#ifdef WIN32
    WaitForSingleObject(m_rdHelperMutex,INFINITE);
    m_nReadersCount++;
    ResetEvent(m_noReaderEvent);
    ReleaseMutex(m_rdHelperMutex);
#else  // #ifdef WIN32
    pthread_rwlock_rdlock(&m_rwLock);
#endif  // #ifdef WIN32

    __DEBUG_APP2__(__LOG_LEVEL__,"++++ locked!!!\n");
}


void decent::tools::RWLock::write_lock()
{
    __DEBUG_APP2__(__LOG_LEVEL__,"++++ write_locking!!!\n");

#ifdef WIN32
    HANDLE hArray[3] = { m_wrMutex, m_rdHelperMutex, m_noReaderEvent };
    WaitForMultipleObjects(3, hArray,TRUE/*wait all*/,INFINITE);
#else  // #ifdef WIN32
    pthread_rwlock_wrlock(&m_rwLock);
#endif  // #ifdef WIN32

    __DEBUG_APP2__(__LOG_LEVEL__,"++++ write_locked!!!\n");
}


void decent::tools::RWLock::unlock()
{
   __DEBUG_APP2__(__LOG_LEVEL__,"----- unlocking!!!\n");

#ifdef WIN32
   WaitForSingleObject(m_rdHelperMutex, INFINITE);
   if (m_nReadersCount)
   {
      m_nReadersCount--;
      SetEvent(m_noReaderEvent);
   }
   else
      ReleaseMutex(m_wrMutex);

   ReleaseMutex(m_rdHelperMutex);
  
#else  // #ifdef WIN32
    pthread_rwlock_unlock(&m_rwLock);
#endif  // #ifdef WIN32

    __DEBUG_APP2__(__LOG_LEVEL__,"----- unlocked!!!\n");
}
