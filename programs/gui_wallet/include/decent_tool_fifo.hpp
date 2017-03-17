/*
 *	File: decent_tool_fifo.hpp
 *
 *	Created on: 04 Feb 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
#ifndef DECENT_TOOL_FIFO_HPP
#define DECENT_TOOL_FIFO_HPP

//#define USE_FIFO_SIMPLE

#include <mutex>
#include <stddef.h>
#include "ui_wallet_functions_base.hpp"

namespace decent { namespace tools{

#ifdef USE_FIFO_SIMPLE

template <typename TypeFifoVrb>
class FiFo
{
public:
    FiFo();
    virtual ~FiFo();

    /*
     *  return
     *      NULL     -> there is no any task to handle
     *      non NULL -> pointer to tast to fullfill
     */
    bool GetFirstTask(TypeFifoVrb* firstElementBuffer);
    void AddNewTask(const TypeFifoVrb& a_new);

protected:
    template <typename TypeFifoVrb>
    struct fifoListItem{
        fifoListItem(const TypeFifoVrb& inp);
        ~fifoListItem();

        struct fifoListItem*    next;
        TypeFifoVrb             elemnt;
    };

protected:
    fifoListItem<TypeFifoVrb>*  m_pFirstTask;
    fifoListItem<TypeFifoVrb>*  m_pLastTask;
    std::mutex                          m_task_mutex;
};


#else  // #ifdef USE_FIFO_SIMPLE

template <typename TypeInp, typename TypeTaskFnc>
struct taskListItem{
    taskListItem(TypeTaskFnc fn_tsk_dn,const TypeInp& inp,void* owner = NULL,void* clbArg=NULL);
    ~taskListItem();

    struct taskListItem*    next;
    int                     type;
    void*                   owner;
    void*                   callbackArg;
    TypeInp                 input;
    union{
    TypeTaskFnc             fn_tsk_dn2;
    TypeCallbackSetNewTaskGlb3   fn_tsk_dn3;
    void*                       fn_tsk_ptr;
    };
};

template <typename TypeInp,typename TypeTaskFnc>
class FiFo
{
public:
    FiFo();
    virtual ~FiFo();

    /*
     *  return
     *      NULL     -> there is no any task to handle
     *      non NULL -> pointer to tast to fullfill
     */
    bool GetFirstTask(decent::tools::taskListItem<TypeInp,TypeTaskFnc>* firstTaskBuffer);
    void AddNewTask(int a_nType,const TypeInp& a_inp, void* a_owner, void* a_clbData,...);

protected:
    taskListItem<TypeInp,TypeTaskFnc>   m_InitialTaskBuffer;
    taskListItem<TypeInp,TypeTaskFnc>*  m_pFirstTask;
    taskListItem<TypeInp,TypeTaskFnc>*  m_pLastTask;
    std::mutex                          m_task_mutex;
};


#endif  // #ifdef USE_FIFO_SIMPLE

}}


#include "decent_tool_fifo.tos"

#endif // DECENT_TOOL_FIFO_HPP
