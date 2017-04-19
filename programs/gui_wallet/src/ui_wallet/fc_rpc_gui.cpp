/*
 *	File: fc_rpc_gui.cpp
 *
 *	Created on: 14 Dec 2016
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
#include "stdafx.h"


#include "fc_rpc_gui.hpp"

#ifndef _MSC_VER
#include <fc/variant.hpp>
#include <fc/io/json.hpp>
#include <iostream>
#include <stdio.h>
#include <stdarg.h>
#endif
#include "decent_gui_inguiloopcaller_glb.hpp"

using namespace fc::rpc;
using namespace fc;

int CallFunctionInUiLoopGeneral(int a_nType,SetNewTask_last_args2,
                                const fc::variant& a_resultVar,const std::string& a_resultStr,
                                void* a_owner,void* a_fpFnc);

gui::gui()
   : _b_task_cancelled(false)
   , _result_formatters()
   , _run_complete()
   , m_Fifo()
   , m_semaphore()
{
    //m_pTaskInitial = new taskList;
}


gui::~gui()
{
   if( _run_complete.valid() )
   {
      stop();
   }
}


variant gui::send_call( api_id_type api_id, std::string method_name, variants args /* = variants() */ )
{
   FC_ASSERT(false);
}

variant gui::send_callback( uint64_t callback_id, variants args /* = variants() */ )
{
   FC_ASSERT(false);
}

void gui::send_notice( uint64_t callback_id, variants args /* = variants() */ )
{
   FC_ASSERT(false);
}

void gui::start()
{
   //cli_commands() = get_method_names(0);
   _run_complete = std::async(std::launch::deferred, [&](){ run(); } );
}

void gui::stop()
{
   _b_task_cancelled = true;
   _run_complete.wait();
}

void gui::wait()
{
   _run_complete.wait();
}

void gui::format_result( const std::string& method, std::function<std::string(variant,const variants&)> formatter)
{
   _result_formatters[method] = formatter;
}


void gui::SetNewTask_base(int a_nType, const std::string& a_inp_line, void* a_owner, void* a_clbData,...)
{
    TypeCallbackSetNewTaskGlb2 fpTaskDone;
    va_list aFunc;

    va_start( aFunc, a_clbData );
    fpTaskDone = va_arg( aFunc, TypeCallbackSetNewTaskGlb2);
    va_end( aFunc );

    m_Fifo.AddNewTask(a_nType, a_inp_line,a_owner,a_clbData,fpTaskDone);
    m_semaphore.post();
}


void gui::run()
{
   decent::tools::taskListItem<std::string,TypeCallbackSetNewTaskGlb2> aTaskItem(NULL,"");
   //int nIteration;

   while( false == _b_task_cancelled )
   {
      try
      {
         m_semaphore.wait();

         while(m_Fifo.GetFirstTask(&aTaskItem))
         {
             std::string line = aTaskItem.input;
             line += char(EOF);
             fc::variants args = fc::json::variants_from_string(line);
             if( args.size() == 0 )continue;
             const string& method = args[0].get_string();


             auto result = receive_call( 0, method, variants( args.begin()+1,args.end() ) );
             auto itr = _result_formatters.find( method );
             
             
             std::string tsResult;
             if( itr == _result_formatters.end() ) {
                 tsResult = fc::json::to_pretty_string( result );
             } else {
                 tsResult = itr->second( result, args );
             }

             CallFunctionInUiLoopGeneral(aTaskItem.type,
                                         aTaskItem.callbackArg,0,aTaskItem.input,
                                         result,tsResult,
                                         aTaskItem.owner,aTaskItem.fn_tsk_ptr);
         }

      } catch ( const fc::exception& e ) {
         
          CallFunctionInUiLoopGeneral(aTaskItem.type,
                                     aTaskItem.callbackArg,e.code(),aTaskItem.input,
                                     fc::variant(),e.to_detail_string(),
                                     aTaskItem.owner,aTaskItem.fn_tsk_ptr);
          
      } catch ( const std::exception& e ) {
           
            CallFunctionInUiLoopGeneral(aTaskItem.type,
                                       aTaskItem.callbackArg,UNKNOWN_EXCEPTION,aTaskItem.input,
                                       fc::variant(),e.what(),
                                       aTaskItem.owner,aTaskItem.fn_tsk_ptr);
      } catch(...) {
          
          CallFunctionInUiLoopGeneral(aTaskItem.type,
                                      aTaskItem.callbackArg,UNKNOWN_EXCEPTION,aTaskItem.input,
                                      /*__FILE__ "\nUnknown exception!"*/fc::variant(),__FILE__ "\nUnknown exception!",
                                      aTaskItem.owner,aTaskItem.fn_tsk_ptr);
      }
   }
}
