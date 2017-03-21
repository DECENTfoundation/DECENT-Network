#pragma once

#if defined(WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <dispatch/dispatch.h>
#else
#include <semaphore.h>
#define SHARING_TYPE	0 /* 0 means semaphores is shared between threads in same process */
#endif

namespace gui_wallet {

   
#ifndef SEMAPHORE_TYPE_DEFINED
   
   #define SEMAPHORE_TYPE_DEFINED
   
   #if defined(WIN32)
      typedef HANDLE SemaphoreType;
   #elif defined(__APPLE__)
      typedef dispatch_semaphore_t SemaphoreType;
   #else
      typedef sem_t SemaphoreType;
   #endif
   
#endif

   
class UnnamedSemaphoreLite
{
public:
    UnnamedSemaphoreLite()
    {
#if defined(WIN32)
        m_Semaphore = CreateSemaphore( 0, (LONG)0, (LONG)100, 0 );
#elif defined(__APPLE__)
        m_Semaphore = dispatch_semaphore_create(0); // init with value of 0
#else
        sem_init( &m_Semaphore, SHARING_TYPE, 0 );
#endif
    }

    ~UnnamedSemaphoreLite()
    {
#if defined(WIN32)
        CloseHandle( m_Semaphore );
#elif defined(__APPLE__)
        dispatch_release(m_Semaphore);
#else
        sem_destroy( &m_Semaphore );
#endif
    }

    void post()
    {
#if defined(WIN32)
        ReleaseSemaphore( m_Semaphore, 1, 0  );
#elif defined(__APPLE__)
        dispatch_semaphore_signal(m_Semaphore);
#else
        sem_post( &m_Semaphore );
#endif
    }

    void wait()
    {
#if defined(WIN32)
        WaitForSingleObject( m_Semaphore, INFINITE );
#elif defined(__APPLE__)
        dispatch_semaphore_wait(m_Semaphore, DISPATCH_TIME_FOREVER);
#else
        sem_wait( &m_Semaphore );
#endif
    }

private:
    SemaphoreType m_Semaphore;
};

}

