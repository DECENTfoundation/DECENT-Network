
#include <mutex>
#include <stdio.h>
#include <stdarg.h>
#include <iostream>
#include <thread>

#include <graphene/wallet/wallet.hpp>

#include <fc/network/http/websocket.hpp>
#include <fc/rpc/websocket_api.hpp>
#include <fc/thread/thread.hpp>

#include <boost/filesystem.hpp>


#include "decent_gui_inguiloopcaller.hpp"
#include "decent_tools_rwlock.hpp"
#include "ui_wallet_functions.hpp"
#include "fc_rpc_gui.hpp"
#include "unnamedsemaphorelite.hpp"
#include "gui_wallet_global.hpp"
#include "gui_wallet_application.hpp"

#include <QCoreApplication>
#include <QEventLoop>
#include <QTime>

#include <chrono>

#ifdef WIN32
   #include <windows.h>
#else
   #include <unistd.h>
   #define Sleep(__x__) usleep(1000*(__x__))
#endif


using namespace gui_wallet;


namespace {
   
class task_exception : public std::exception
{
public:
   task_exception(std::string const& str_info) noexcept : m_str_info(str_info) {}
   virtual ~task_exception() {}
   
   char const* what() const noexcept override
   {
      return m_str_info.c_str();
   }
private:
   std::string m_str_info;
};

struct task_result {
   bool        m_bDone = false;
   int64_t     m_error = 0;
   std::string m_strResult;
};

}



void QtDelay( int millisecondsToWait )
{
    QTime dieTime = QTime::currentTime().addMSecs( millisecondsToWait );
    while( QTime::currentTime() < dieTime )
    {
        QCoreApplication::processEvents( QEventLoop::AllEvents, 100 );
    }
}



