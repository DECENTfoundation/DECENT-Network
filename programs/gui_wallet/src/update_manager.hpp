/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */

//#define UPDATE_MANAGER
#ifdef UPDATE_MANAGER

#ifndef _MSC_VER
#include <QObject>
#endif

class QTimer;
class CProgBar;
class CDetectUpdateThreadParams;

#ifdef __GNUC__
#ifndef __cdecl
#define __cdecl __attribute__((cdecl))
#endif
#endif

// class UpdateManager
// This class can be optionally added to GUI application with help of 
// #define UPDATE_MANAGER
// Class UpdateManager provides interface to update library (update client)
// which requests newer versions of installation package from update server.  
// Update library uses callbacks in this class to display GUI items like 
// prompt for user - revision history dialog with "Install" button 
// and progress bar displaying downloading of newer installation package.
// GUI items of update manager are part of this C++ project.

class UpdateManager : public QObject
{
   Q_OBJECT
public:
   UpdateManager();
   ~UpdateManager();
   
   // callbacks
   // They are called from inside of update library.
   // Callbacks are assigned in csontructor
   static uint32_t __cdecl StartRevHistoryDlg(const std::string& revHistory);
   static void __cdecl CreateProgBar(int upperBorder, uint32_t* abort);
   static void __cdecl DestroyProgBar(void);
   static void __cdecl SetProgBarPos(int pos);
   static void __cdecl SetProgBarTitle(const char* title);

protected:
   bool m_updateProgBarCreate;
   bool m_updateProgBarDestroy;
   int m_proxyUpdateProgBarUpperBorder;
   QString m_proxyUpdateProgBarSetTitle;
   uint32_t* m_proxyUpdateProgBarAbort;
   CProgBar* m_progBar;
   CDetectUpdateThreadParams* m_updateThreadParams;
   void* m_updateThread;
   QTimer* m_pTimerUpdateProxy;

   void progBarCreate(int upperBorder, uint32_t* abort);
   void progBarSetTitle(const QString& title);
   void progBarDestroy(void);

   void EmitStartRevHistoryDlg(const std::string& revHistory, uint32_t& returnValue);
   void EmitProgBarSetPos(int pos);

   // Update thread cannot directly manage progress bar dialog commands are issued through proxy.
   // Progress bar dialog must be running in GUI thread
   void ProxyCreateProgBar(int upperBorder, uint32_t* abort);
   void ProxyDestroyProgBar(void);
   void ProxySetProgBarTitle(const QString& title);


signals:
   void signal_startRevHistoryDlg(const QString& revHistory, long* returnValue);
   void signal_progBarSetPos(int pos);

   protected slots:
   void slot_startRevHistoryDlg(const QString& revHistory, long* returnValue);
   void slot_updateProxy();
   void slot_progBarSetPos(int pos);
};
#endif