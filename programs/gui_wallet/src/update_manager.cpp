/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */

#ifndef STDAFX_H
#include <QMainWindow>
#include <QTimer>
#include <QApplication>
#endif

#include "update_manager.hpp"
#ifdef UPDATE_MANAGER

#include "../../update/include/update.h"
#include "../../update/include/updatethread.h"
#include "rev_history_dlg.hpp"
#include "update_prog_bar.hpp"
using namespace gui_wallet;
#ifndef _MSC_VER
#include <pthread.h>
typedef void* DCTHANDLE;
#endif

static UpdateManager* s_updateManager = nullptr;

QMainWindow* getMainWindow()
{
   foreach(QWidget *widget, qApp->topLevelWidgets())
      if (QMainWindow *mainWindow = qobject_cast<QMainWindow*>(widget))
         return mainWindow;
   return NULL;
}

MPHANDLE MpThreadCreate(void* params, MP_THREAD_FUNCTION threadProc, uint32_t* tid)
{
#if defined( _MSC_VER )
   return CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)threadProc, params, 0, (DWORD*)tid);
#elif	defined( __GNUC__ )
   pthread_t   threadId;
   pthread_attr_t  attr;
   int	rc = 0;
   memset(&attr, 0, sizeof(attr));

   rc = pthread_attr_init(&attr);
   if (rc)
      return NULL;  // EINVAL, ENOMEM

   //int stacksize = 0xfffff;
   //rc = pthread_attr_setstacksize(&attr, stacksize);
   //if (rc)
     // return NULL;   // EINVAL, ENOSYS

   rc = pthread_create(&threadId, &attr, (void*(*)(void*))threadProc, params);
   pthread_attr_destroy(&attr);
   if (rc)
      return NULL;      // EINVAL, EAGAIN

   return (DCTHANDLE)threadId;
#else
#error "Undefined Compiler platform"
#endif
}


UpdateManager::UpdateManager()
: m_pTimerUpdateProxy(new QTimer(this))
, m_updateProgBarCreate(false)
, m_proxyUpdateProgBarUpperBorder(0)
, m_proxyUpdateProgBarAbort(nullptr)
, m_updateProgBarDestroy(false)
, m_progBar(nullptr)
, m_updateThreadParams(nullptr)
, m_updateThread(nullptr)
{
   s_updateManager = this;
   m_pTimerUpdateProxy->setInterval(200);
   QObject::connect(m_pTimerUpdateProxy, &QTimer::timeout,
                    this, &UpdateManager::slot_updateProxy);

   m_pTimerUpdateProxy->start();

   m_updateThreadParams = new CDetectUpdateThreadParams;
   m_updateThreadParams->m_licenseUserID = 0x1111111111111111ULL;// currently we dont use some kind of identification

   Update::g_fn_StartRevHistoryDlg = StartRevHistoryDlg;
   Update::g_fn_CreateProgBar = CreateProgBar;
   Update::g_fn_SetProgBarTitle = SetProgBarTitle;
   Update::g_fn_SetProgBarPos = SetProgBarPos;
   Update::g_fn_DestroyProgBar = DestroyProgBar;

   uint32_t tid = 0;
   m_updateThreadParams->m_runUpdateFlag = true;
   m_updateThread = MpThreadCreate(m_updateThreadParams, DetectUpdateThread, &tid);

   bool connected = false;// debug purposes
   connected = QObject::connect(this, SIGNAL(signal_progBarSetPos(int)), this, SLOT(slot_progBarSetPos(int)), Qt::QueuedConnection);
   connected = QObject::connect(this, SIGNAL(signal_startRevHistoryDlg(const QString&, long*)), this, SLOT(slot_startRevHistoryDlg(const QString&, long*)), Qt::BlockingQueuedConnection);
}

UpdateManager::~UpdateManager()
{
   m_pTimerUpdateProxy->stop();
   if (m_updateThread)
   {
      m_updateThreadParams->m_stopUpdateThreadFlag = true;
      m_updateThreadParams->m_abort = 1;
      AbortUpdate(nullptr, m_updateThreadParams->m_curlSession);
   }
}

void UpdateManager::EmitStartRevHistoryDlg(const std::string& revHistory, uint32_t& returnValue)
{
   const QString s(revHistory.c_str());
   long returnVal = 0;
   emit signal_startRevHistoryDlg(s, &returnVal);
   returnValue = returnVal;
}

void UpdateManager::EmitProgBarSetPos(int pos)
{
   emit signal_progBarSetPos(pos);
}

void UpdateManager::ProxyCreateProgBar(int upperBorder, uint32_t* abort)
{
   m_proxyUpdateProgBarUpperBorder = upperBorder;
   m_proxyUpdateProgBarAbort = abort;
   m_updateProgBarCreate = true;
}

void UpdateManager::ProxyDestroyProgBar(void)
{
   m_updateProgBarDestroy = true;
}

void UpdateManager::ProxySetProgBarTitle(const QString& title)
{
   m_proxyUpdateProgBarSetTitle = title;
}

void UpdateManager::slot_updateProxy()
{
   if (m_updateProgBarCreate)
   {
      m_updateProgBarCreate = false;
      progBarCreate(m_proxyUpdateProgBarUpperBorder, m_proxyUpdateProgBarAbort);
   }
   if (m_updateProgBarDestroy)
   {
      m_updateProgBarDestroy = false;
      progBarDestroy();
   }
   if (m_proxyUpdateProgBarSetTitle.length())
   {
      progBarSetTitle(m_proxyUpdateProgBarSetTitle);
      m_proxyUpdateProgBarSetTitle = "";
   }
}

void UpdateManager::progBarCreate(int upperBorder, uint32_t* abort)
{
   CProgBar* progBar = nullptr;
   if (m_progBar) {
       delete m_progBar;
       m_progBar = nullptr;
   }

   QMainWindow* w = getMainWindow();
   progBar = new CProgBar(100, QString(), false, true, w);
   progBar->Init(upperBorder, QString(), abort, w);
   progBar->SetMyStandardSize();
   progBar->Show();// to apply change of size
   progBar->Raise();

   progBar->EnableCancel(true);

   m_progBar = progBar;
}

void UpdateManager::progBarDestroy(void)
{
   if (m_progBar)
   {
      m_progBar->Hide();
      delete m_progBar;
   }
   m_progBar = nullptr;
}

void UpdateManager::slot_startRevHistoryDlg(const QString& revHistory, long* returnValue)
{
   QMainWindow* w = getMainWindow();
   Rev_history_dlg revHistDlg(revHistory, w);
   *returnValue = revHistDlg.exec();
}

void UpdateManager::progBarSetTitle(const QString& title)
{
   if (m_progBar)
      m_progBar->SetLabelText(title);
}

void UpdateManager::slot_progBarSetPos(int pos)
{
   if (m_progBar)
      m_progBar->SetValue(pos);
}

uint32_t __cdecl UpdateManager::StartRevHistoryDlg(const std::string& revHistory)
{
   uint32_t returnValue = 0;

   s_updateManager->EmitStartRevHistoryDlg(revHistory, returnValue);
   return returnValue;
}

void __cdecl UpdateManager::CreateProgBar(int upperBorder, uint32_t* abort)
{
   s_updateManager->ProxyCreateProgBar(upperBorder, abort);
}

void __cdecl UpdateManager::DestroyProgBar(void)
{
   s_updateManager->ProxyDestroyProgBar();
}

void __cdecl UpdateManager::SetProgBarPos(int pos)
{
   s_updateManager->EmitProgBarSetPos(pos);
}

void __cdecl UpdateManager::SetProgBarTitle(const char* title)
{
   QString t(title);
   s_updateManager->ProxySetProgBarTitle(t);
}


#endif
