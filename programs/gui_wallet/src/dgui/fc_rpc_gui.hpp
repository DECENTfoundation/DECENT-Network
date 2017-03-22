#pragma once


#include <fc/rpc/api_connection.hpp>
#include <unnamedsemaphorelite.hpp>
#include <mutex>
#include "ui_wallet_functions.hpp"
#include "decent_tool_fifo.hpp"
#include <future>
#include <atomic>


namespace fc { namespace rpc {

   /**
    *  Provides a simple wrapper for RPC calls to a given interface.
    */
   /*class gui : public api_connection
   {
   public:
      gui();
      virtual ~gui();

      virtual variant send_call( api_id_type api_id, string method_name, variants args = variants() );
      virtual variant send_callback( uint64_t callback_id, variants args = variants() );
      virtual void    send_notice( uint64_t callback_id, variants args = variants() );

      void start();
      void stop();
      void wait();
      void format_result( const string& method, std::function<string(variant,const variants&)> formatter);

      void SetNewTask(const std::string& inp_line, void* ownr, void* clbData, gui_wallet::TypeCallbackSetNewTaskGlb2 fpTaskDone);
   private:

      void run();


      std::atomic<bool> _b_task_cancelled;
      std::map<string,std::function<string(variant,const variants&)> > _result_formatters;
      std::future<void> _run_complete;

      gui_wallet::FiFo<gui_wallet::TaskListItem>     m_Fifo;
      gui_wallet::UnnamedSemaphoreLite               m_semaphore;
   };*/

} }

