//decent_tools_rwlock.hpp
/*
 *	File: decent_tools_rwlock.hpp
 *
 *	Created on: 05 Feb 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
#ifndef DECENT_TOOLS_RWLOCK_HPP
#define DECENT_TOOLS_RWLOCK_HPP

#ifdef WIN32
#include <windows.h>
#else  // #ifdef WIN32
#include <pthread.h>
#endif  // #ifdef WIN32

namespace decent{ namespace tools{

class RWLock
{
public:
    RWLock();
    virtual ~RWLock();

    void lock();
    void write_lock();
    void unlock();

protected:
#ifdef WIN32
    int m_nReadersCount;
    HANDLE      m_vRWMutexes[2];
#else  // #ifdef WIN32
    pthread_rwlock_t    m_rwLock;
#endif  // #ifdef WIN32
};

}}

#endif // DECENT_TOOLS_RWLOCK_HPP
