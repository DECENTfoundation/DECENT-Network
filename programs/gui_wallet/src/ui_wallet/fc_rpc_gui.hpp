/*
 *	File: fc_rpc_gui.hpp
 *
 *	Created on: 14 Dec 2016
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
#ifndef FC_RPC_GUI_HPP
#define FC_RPC_GUI_HPP

#include <fc/rpc/api_connection.hpp>
#include <unnamedsemaphorelite.hpp>
#include <mutex>
#include "ui_wallet_functions_base.hpp"
#include "decent_tool_fifo.hpp"
#include <future>
#include <atomic>

typedef int (*TYPE_REPORTER)(void*owner,const char* form,...);

namespace fc { namespace rpc {

   /**
    *  Provides a simple wrapper for RPC calls to a given interface.
    */
   class gui : public api_connection
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

      void SetNewTask_base(int a_nType,const std::string& inp_line, void* ownr, void* clbData, ...);
   private:

      void run();


      std::atomic<bool> _b_task_cancelled;
      std::map<string,std::function<string(variant,const variants&)> > _result_formatters;
      std::future<void> _run_complete;

      decent::tools::FiFo<std::string,TypeCallbackSetNewTaskGlb2>            m_Fifo;
      decent::tools::UnnamedSemaphoreLite m_semaphore;
   };

} }

#endif // FC_RPC_GUI_HPP
