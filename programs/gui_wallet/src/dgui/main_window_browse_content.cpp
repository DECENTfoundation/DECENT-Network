// main_window_browse_content
/*
 *	File: main_window_browse_content.cpp
 *
 *	Created on: 20 Feb 2017
 *	Created by:
 *
 *  This file implements ...
 *
 */

#include "gui_wallet_mainwindow.hpp"
#include <fc/variant_object.hpp>


struct my_visitor : public fc::variant::visitor
{
#if 0
   typedef void result_type;
   template<typename Type>
   result_type operator()( const Type& op )const {
       std::cout<< "visited " <<std::endl;
   }
#endif

   virtual void handle()const                        {__DEBUG_APP2__(0,"task=%s",m_inp.c_str());}
   virtual void handle( const int64_t& v )const        {__DEBUG_APP2__(0,"task=%s",m_inp.c_str());}
   virtual void handle( const uint64_t& v )const       {__DEBUG_APP2__(0,"task=%s",m_inp.c_str());}
   virtual void handle( const double& v )const         {__DEBUG_APP2__(0,"task=%s",m_inp.c_str());}
   virtual void handle( const bool& v )const           {__DEBUG_APP2__(0,"task=%s",m_inp.c_str());}
   virtual void handle( const std::string& a_s )const
   {
       m_str = a_s;
       __DEBUG_APP2__(0,"task=%s, str=%s",m_inp.c_str(),a_s.c_str());
   }
   virtual void handle( const fc::variant_object& a_vo)const {
       size_t unSize = a_vo.size();
       __DEBUG_APP2__(0,"task=%s, unSize=%d",m_inp.c_str(),(int)unSize);
       const fc::variant_object::entry* pEntry;
       fc::variant_object::iterator pItr=a_vo.begin();

       for(size_t i(0); i<unSize;++i,++pItr)
       {
           pEntry = &(*pItr);
           //pEntry->key();
           pEntry->value().visit(*this);
       }
   }
   virtual void handle( const fc::variants& a_vs)const {

       size_t unSize = a_vs.size();
       __DEBUG_APP2__(0,"task=%s, size=%d",m_inp.c_str(),(int)unSize);
       const fc::variant* pVariant;

       for(size_t i(0);i<unSize;++i)
       {
           pVariant = &(a_vs[i]);
           pVariant->visit(*this);
       }
   }

   std::string m_inp;
   mutable std::string m_str;
};


/*///////////////////////////////////////////////////////*/

void gui_wallet::Mainwindow_gui_wallet::ManagementBrowseContentGUI()
{
    __DEBUG_APP2__(2," ");
    //SetNewTask3("info",this,NULL,&gui_wallet::Mainwindow_gui_wallet::TaskDoneBrowseContentGUI3);

    QString cqsNewFilter = m_pCentralWidget->getFilterText();
    SetNewTask3("list_content a 10",this,NULL,&Mainwindow_gui_wallet::TaskDoneBrowseContentGUI3);
#if 1
    if(cqsNewFilter==m_cqsPreviousFilter){return;}
    else if(cqsNewFilter==tr(""))
    {
        // may be in the case of empty filter all contents should be displayed?
        m_cqsPreviousFilter = cqsNewFilter;
        return;
    }

    m_cqsPreviousFilter = cqsNewFilter;
    ShowDigitalContextesGUI(cqsNewFilter);
#endif
}


void gui_wallet::Mainwindow_gui_wallet::TaskDoneBrowseContentGUI(void* a_clbkArg,int64_t a_err,
                                                                 const std::string& a_task,const std::string& a_result)
{
    //
}


void gui_wallet::Mainwindow_gui_wallet::TaskDoneBrowseContentGUI3(void* a_clbkArg,int64_t a_err,
                                                                  const std::string& a_task,const fc::variant& a_result)
{
    my_visitor aVisitor;
    aVisitor.m_inp = a_task;
    __DEBUG_APP2__(2," ");

    a_result.visit(aVisitor);
}
