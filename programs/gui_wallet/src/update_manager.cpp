/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
#ifdef UPDATE_MANAGER

#include "stdafx.h"
#include "gui_wallet_global.hpp"

#include "update_manager.hpp"

// looks like a copy-pasted black box code
// shall we get rid of legacy - difficult to understand code?
#include "update_callbacks.hpp"
#include "../../update/update.h"
#include "../../update/updatethread.h"
#include "rev_history_dlg.hpp"
#include "update_prog_bar.hpp"

UpdateManager::UpdateManager()
: m_pTimerUpdateProxy(new QTimer(this))
, m_updateProgBarCreate(false)
, m_proxyUpdateProgBarUpperBorder(0)
, m_proxyUpdateProgBarAbort(nullptr)
, m_updateProgBarDestroy(false)
, m_progBar(nullptr)
, m_updateThreadParams(nullptr)
{
   m_pTimerUpdateProxy->setInterval(200);
   QObject::connect(m_pTimerUpdateProxy, &QTimer::timeout,
                    this, &UpdateManager::slot_updateProxy);

   m_pTimerUpdateProxy->start();


   // what's all this about?
   m_updateThreadParams = new CDetectUpdateThreadParams;
   m_updateThreadParams->m_runUpdateEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
   m_updateThreadParams->m_stopUpdateThread = CreateEvent(NULL, FALSE, FALSE, NULL);
   m_updateThreadParams->m_licenseUserID = 0x1111111111111111ULL;


   Update::g_fn_StartRevHistoryDlg = StartRevHistoryDlg;
   Update::g_fn_CreateProgBar = CreateProgBar;
   Update::g_fn_SetProgBarTitle = SetProgBarTitle;
   Update::g_fn_SetProgBarPos = SetProgBarPos;
   Update::g_fn_DestroyProgBar = DestroyProgBar;

   uint32_t tid = 0;
   SetEvent(m_updateThreadParams->m_runUpdateEvent);
   m_updateThread = MpThreadCreate(m_updateThreadParams, DetectUpdateThread, &tid);

   bool connected = false;
   connected = QObject::connect(this, SIGNAL(signal_progBarSetPos(int)), this, SLOT(slot_progBarSetPos(int)), Qt::QueuedConnection);
   connected = QObject::connect(this, SIGNAL(signal_startRevHistoryDlg(const QString&, long*)), this, SLOT(slot_startRevHistoryDlg(const QString&, long*)), Qt::BlockingQueuedConnection);
}

UpdateManager::~UpdateManager()
{
   if (m_updateThread)
   {
      SetEvent(m_updateThreadParams->m_stopUpdateThread);
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

void UpdateManager::EmitProgBarDestroy(void)
{

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
   if (m_progBar)
      delete m_progBar;
   m_progBar = nullptr;

   progBar = new CProgBar(100, QString(""), false, true, this);
   progBar->Init(upperBorder, "", abort, this);
   progBar->SetMyStandardSize();
   progBar->Show();// ked je tu toto taksa prejavi zmena velkosti, inak nie    .. english please?
   progBar->Raise();
   //progBar->activateWindow();
   //QApplication::setActiveWindow(progBar);

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
   Rev_history_dlg revHistDlg(revHistory, nullptr);
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

#endif
