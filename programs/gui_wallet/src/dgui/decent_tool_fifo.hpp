#pragma once

#include <mutex>
#include <stddef.h>
#include "ui_wallet_functions_base.hpp"

namespace gui_wallet {
   
   struct SConnectionStruct;
   
   struct TaskListItem {
      typedef std::string            value_type;
      
      
      TaskListItem() : callback(NULL) {}
      TaskListItem(TypeCallbackSetNewTaskGlb2 callback_function, const std::string& a_inp, void* a_owner = NULL, void* a_clbArg=NULL)
      : next(NULL), owner(a_owner), callbackArg(a_clbArg), input(a_inp), callback(callback_function) {
         
      }
      
      TaskListItem*                   next;
      void*                           owner;
      void*                           callbackArg;
      std::string                     input;
      TypeCallbackSetNewTaskGlb2      callback;
   };
   
   
   struct ConnectListItem {
      typedef SConnectionStruct*     value_type;
      
      ConnectListItem() : input(NULL), callback(NULL) {}
      
      ConnectListItem(TypeCallbackSetNewTaskGlb2 callback_function, SConnectionStruct* a_inp, void* a_owner = NULL, void* a_clbArg=NULL)
      : next(NULL), owner(a_owner), callbackArg(a_clbArg), input(a_inp), callback(callback_function) {
         
      }
      
      ConnectListItem*                next;
      void*                           owner;
      void*                           callbackArg;
      SConnectionStruct*              input;
      TypeCallbackSetNewTaskGlb2      callback;
   };
   
   
   
   
   template <class ListItemType>
   class FiFo {
   public:
      FiFo() : m_pFirstTask(NULL), m_pLastTask(NULL) {
      }
      
      virtual ~FiFo() {
         
         m_task_mutex.lock();
         ListItemType* pItemTodelete = m_pFirstTask ? m_pFirstTask->next : NULL;
         
         while(pItemTodelete) {
            ListItemType* pItemTemp = pItemTodelete->next;
            
            if (pItemTodelete != (&m_InitialTaskBuffer)) {
               delete pItemTodelete;
            }
            
            pItemTodelete = pItemTemp;
         }
         m_task_mutex.unlock();
      }
      
      
      bool GetFirstTask(ListItemType* firstTaskBuffer) {
         bool bRet = false;

         m_task_mutex.lock();
         if (m_pFirstTask) {
            *firstTaskBuffer = *m_pFirstTask;
            
            if(m_pFirstTask->next) {
               ListItemType* pTmp = m_pFirstTask->next;

               *m_pFirstTask = *(m_pFirstTask->next);
               if (pTmp != (&m_InitialTaskBuffer)) {
                  delete pTmp;
               }
            } else {
               m_pFirstTask = NULL;
            }
            bRet = true;
         }
         m_task_mutex.unlock();
         return bRet;
      }
      
      
      void AddNewTask(typename ListItemType::value_type a_inp, void* a_owner, void* a_clbData, TypeCallbackSetNewTaskGlb2 callback) {
         
         m_task_mutex.lock();
         
         if (!m_pFirstTask) {
            m_pFirstTask = m_pLastTask = &m_InitialTaskBuffer;
            m_pLastTask->next = NULL;
            m_pLastTask->owner = a_owner;
            m_pLastTask->callbackArg = a_clbData;
            m_pLastTask->input = a_inp;
            m_pLastTask->callback = callback;
            
         } else {
            
            m_pLastTask->next = new ListItemType(callback, a_inp, a_owner, a_clbData);
            m_pLastTask = m_pLastTask->next;
         }
         m_task_mutex.unlock();
      }
      
   protected:
      ListItemType   m_InitialTaskBuffer;
      ListItemType*  m_pFirstTask;
      ListItemType*  m_pLastTask;
      std::mutex     m_task_mutex;
   };
   
   
   
}


