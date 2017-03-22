
#include "fc_rpc_gui.hpp"
#include <fc/variant.hpp>
#include <fc/io/json.hpp>
#include <iostream>
#include <stdio.h>
#include <stdarg.h>

#include "ui_wallet_functions.hpp"

using namespace fc::rpc;
using namespace fc;
using namespace gui_wallet;

static std::thread*   guiRunThread;



/*gui::gui() : _b_task_cancelled(false) {

}


gui::~gui()
{
   if( _run_complete.valid() )
   {
      stop();
   }
}


variant gui::send_call( api_id_type api_id, string method_name, variants args /-* = variants() *-/ )
{
   FC_ASSERT(false);
}

variant gui::send_callback( uint64_t callback_id, variants args /-* = variants() *-/ )
{
   FC_ASSERT(false);
}

void gui::send_notice( uint64_t callback_id, variants args /-* = variants() *-/ )
{
   FC_ASSERT(false);
}

void gui::start()
{
   //guiRunThread = new std::thread(&gui::run, this);
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

void gui::format_result( const string& method, std::function<string(variant,const variants&)> formatter)
{
   _result_formatters[method] = formatter;
}


void gui::SetNewTask(const std::string& a_inp_line, void* a_owner, void* a_clbData, TypeCallbackSetNewTaskGlb2 fpTaskDone) {
   std::cout << "Queue task: " << a_inp_line << std::endl;
    m_Fifo.AddNewTask(a_inp_line,a_owner,a_clbData,fpTaskDone);
    m_semaphore.post();
}


void gui::run()
{
   TaskListItem aTaskItem(NULL,"");


   while( false == _b_task_cancelled ) {
      
      try {
         
         m_semaphore.wait();

         while(m_Fifo.GetFirstTask(&aTaskItem)) {
            std::cout << "Process task: " << aTaskItem.input << std::endl;
             std::string line = aTaskItem.input;
             line += char(EOF);
             fc::variants args = fc::json::variants_from_string(line);
            
             if (args.size() == 0)
                continue;
            
             const string& method = args[0].get_string();


             auto result = receive_call( 0, method, variants( args.begin()+1,args.end() ) );
             auto itr = _result_formatters.find( method );
             
             
             std::string tsResult;
             if( itr == _result_formatters.end() ) {
                 tsResult = fc::json::to_pretty_string( result );
             } else {
                 tsResult = itr->second( result, args );
             }
            if (aTaskItem.owner == nullptr) {
               aTaskItem.callback(nullptr, aTaskItem.callbackArg, 0, aTaskItem.input, tsResult);
            } else {
               WalletInterface::callFunctionInGuiLoop(aTaskItem.callbackArg, 0, aTaskItem.input, tsResult, aTaskItem.owner, aTaskItem.callback);
            }
         }

      } catch ( const fc::exception& e ) {
         if (aTaskItem.owner == nullptr) {
            aTaskItem.callback(nullptr, aTaskItem.callbackArg, e.code(), aTaskItem.input, e.to_detail_string());
         } else {
            WalletInterface::callFunctionInGuiLoop(aTaskItem.callbackArg, e.code(), aTaskItem.input, e.to_detail_string(), aTaskItem.owner, aTaskItem.callback);
         }

      } catch ( const std::exception& e ) {
         if (aTaskItem.owner == nullptr) {
            aTaskItem.callback(nullptr, aTaskItem.callbackArg, UNKNOWN_EXCEPTION, aTaskItem.input, e.what());
         } else {
            WalletInterface::callFunctionInGuiLoop(aTaskItem.callbackArg, UNKNOWN_EXCEPTION, aTaskItem.input, e.what(), aTaskItem.owner, aTaskItem.callback);
         }
      } catch(...) {
         if (aTaskItem.owner == nullptr) {
            aTaskItem.callback(nullptr, aTaskItem.callbackArg, UNKNOWN_EXCEPTION, aTaskItem.input, "Unknown exception");
         } else {
          WalletInterface::callFunctionInGuiLoop(aTaskItem.callbackArg,UNKNOWN_EXCEPTION,aTaskItem.input, "Unknown exception", aTaskItem.owner, aTaskItem.callback);
         }
      }
   }
}*/
