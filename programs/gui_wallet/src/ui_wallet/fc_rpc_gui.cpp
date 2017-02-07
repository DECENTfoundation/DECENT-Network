/*
 *	File: fc_rpc_gui.cpp
 *
 *	Created on: 14 Dec 2016
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */

#include "fc_rpc_gui.hpp"
#include <fc/io/json.hpp>
#include <iostream>
#include <stdio.h>
#include <stdarg.h>
#include "decent_gui_inguiloopcaller_glb.hpp"

using namespace fc::rpc;
using namespace fc;
extern int g_nDebugApplication ;

int CallFunctionInUiLoop(SetNewTask_last_args,void* a_owner,TypeCallbackSetNewTaskGlb a_fpFnc);

#if 0
std::map<string,std::function<string(variant,const variants&)> > _result_formatters;
fc::future<void> _run_complete;
decent_tools::UnnamedSemaphoreLite m_semaphore;
#endif

gui::gui()
    :
      _result_formatters(),
      m_semaphore()
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


variant gui::send_call( api_id_type api_id, string method_name, variants args /* = variants() */ )
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
   _run_complete = fc::async( [&](){ run(); } );
}

void gui::stop()
{
   _run_complete.cancel();
   _run_complete.wait();
}

void gui::wait()
{
   _run_complete.wait();
}

void gui::format_result( const string& method, std::function<string(variant,const variants&)> formatter)
{
   _result_formatters[method] = formatter;
}


void gui::SetNewTask(const std::string& a_inp_line, void* a_owner, void* a_clbData, TypeCallbackSetNewTaskGlb a_clbkFunction)
{
    SetNewTask_base(a_inp_line, a_owner, a_clbData,a_clbkFunction);
}


void gui::SetNewTask_base(const std::string& a_inp_line, void* a_owner, void* a_clbData,...)
{
    TypeCallbackSetNewTaskGlb fpTaskDone;
    va_list aFunc;

    va_start( aFunc, a_clbData );
    fpTaskDone = va_arg( aFunc, TypeCallbackSetNewTaskGlb);
    va_end( aFunc );

    m_Fifo.AddNewTask(a_inp_line,a_owner,a_clbData,fpTaskDone);
    m_semaphore.post();
}


void gui::run()
{
   decent::tools::taskListItem<std::string,TypeCallbackSetNewTaskGlb> aTaskItem(NULL,"");
   //int nIteration;

   while( !_run_complete.canceled() )
   {
      try
      {
         m_semaphore.wait();
         //nIteration = 0;

         while(m_Fifo.GetFirstTask(&aTaskItem))
         {
             //printf("!!!!!!!!!!!! %d, aItem.line=%s\n",++nIteration,aTaskItem.line.c_str());
             std::string line = aTaskItem.input;
             //std::cout << line << "\n";
             line += char(EOF);
             fc::variants args = fc::json::variants_from_string(line);
             if( args.size() == 0 )continue;
             const string& method = args[0].get_string();

             //const string& method = m_method;

             auto result = receive_call( 0, method, variants( args.begin()+1,args.end() ) );
             auto itr = _result_formatters.find( method );
             if( itr == _result_formatters.end() )
             {
                 // int CallFunctionInUiLoop(SetNewTask_last_args,void* owner,TypeCallbackSetNewTaskGlb fpFnc);
                //std::cout << "!!!!!!!if\n"<<fc::json::to_pretty_string( result ) << "\n";
                //(*aTaskItem.fn_tsk_dn)(aTaskItem.owner,aTaskItem.callbackArg,0,aTaskItem.input, fc::json::to_pretty_string( result ));
                CallFunctionInUiLoop(aTaskItem.callbackArg,0,aTaskItem.input,fc::json::to_pretty_string( result ),
                                      aTaskItem.owner,aTaskItem.fn_tsk_dn2);
             }
             else
             {
                //std::cout << "!!!!!!!!else\n"<<itr->second( result, args ) << "\n";
                //(*aTaskItem.fn_tsk_dn)(aTaskItem.owner,aTaskItem.callbackArg,0,aTaskItem.input, itr->second( result, args ));
                CallFunctionInUiLoop(aTaskItem.callbackArg,0,aTaskItem.input,itr->second( result, args ),
                                       aTaskItem.owner,aTaskItem.fn_tsk_dn2);
             }


         } // while(GetFirstTask(&aTaskItem))

      }  // try
      catch ( const fc::exception& e )
      {
         if(g_nDebugApplication){printf("file:\"" __FILE__ "\",line:%d\n",__LINE__);}
         if(g_nDebugApplication){std::cout << e.to_detail_string() << "\n";}
         //(*aTaskItem.fn_tsk_dn)(aTaskItem.owner,aTaskItem.callbackArg,e.code(),aTaskItem.input,e.to_detail_string());
         CallFunctionInUiLoop(aTaskItem.callbackArg,e.code(),aTaskItem.input,e.to_detail_string(),
                                aTaskItem.owner,aTaskItem.fn_tsk_dn2);
      }
      catch(...)
      {
          if(g_nDebugApplication){printf("file:\"" __FILE__ "\",line:%d\n",__LINE__);}
          if(g_nDebugApplication){std::cout << "Unknown exception!\n";}
          //(*aTaskItem.fn_tsk_dn)(aTaskItem.owner,aTaskItem.callbackArg,-1,aTaskItem.input,"Unknown exception!\n");
          CallFunctionInUiLoop(aTaskItem.callbackArg,UNKNOWN_EXCEPTION,aTaskItem.input,
                                __FILE__ "\nUnknown exception!",
                                 aTaskItem.owner,aTaskItem.fn_tsk_dn2);
      }
   } // while( !_run_complete.canceled() )
}
