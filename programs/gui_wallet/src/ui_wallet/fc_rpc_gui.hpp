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

         template <typename Type>
         void SetNewTask(const std::string& a_inp_line, Type* a_memb, void* a_clbData, void (Type::*a_clbkFunction)(SetNewTask_last_args))
         {
             SetNewTask_base(a_inp_line,a_memb,a_clbData,a_clbkFunction);
         }

         void SetNewTask(const std::string& inp_line, void* ownr, void* clbData, TypeCallbackSetNewTaskGlb clbkFunction);


      private:
         void SetNewTask_base(const std::string& inp_line, void* ownr, void* clbData, ...);
         void run();

         std::map<string,std::function<string(variant,const variants&)> > _result_formatters;
         fc::future<void> _run_complete;

         decent::tools::FiFo<std::string,TypeCallbackSetNewTaskGlb>            m_Fifo;

         //std::string                        m_method;
         //std::string                        m_line;
         decent::tools::UnnamedSemaphoreLite m_semaphore;
   };

} }

#endif // FC_RPC_GUI_HPP
