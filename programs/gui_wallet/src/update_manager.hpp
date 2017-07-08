/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */

// this looks to be difficult to understand code
// with external copy-pasted dependencies
// we must not add such code, let's respect Qt style and
// write a code that is more or less using the same idioms such as QThread
// this needs to be simplified a lot to be usable
//
// please write comments in english

class CProgBar;
class CDetectUpdateThreadParams;


class UpdateManager
{
public:
   UpdateManager();
   
   void EmitStartRevHistoryDlg(const std::string& revHistory, uint32_t& returnValue);
   void EmitProgBarSetPos(int pos);
   void EmitProgBarDestroy(void);
   void ProxyCreateProgBar(int upperBorder, uint32_t* abort);
   void ProxyDestroyProgBar(void);
   void ProxySetProgBarTitle(const QString& title);

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

signals:
   void signal_startRevHistoryDlg(const QString& revHistory, long* returnValue);
   void signal_progBarSetPos(int pos);

   protected slots:
   void slot_startRevHistoryDlg(const QString& revHistory, long* returnValue);
   void slot_updateProxy();
   void slot_progBarSetPos(int pos);
};
