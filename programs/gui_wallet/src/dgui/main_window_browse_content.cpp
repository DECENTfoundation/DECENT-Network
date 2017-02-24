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

#ifndef DEFAULT_LOG_LEVEL
#define DEFAULT_LOG_LEVEL 4
#endif

class FinalResult
{
public:
    void Clear2();

private:
    struct SKeyValue{
        //fc::variant::type_id type;
        union{
            std::string value;
            union{
                std::string key;
                class FinalResult* pValue;
            };
        };
    }; // struct SKeyValue{

    std::vector<SKeyValue> m_values;
};


class my_visitor : public fc::variant::visitor
{
public:
    my_visitor():m_IsStructField(0){}

   virtual void handle()const                          {
        __DEBUG_APP2__(DEFAULT_LOG_LEVEL,"task=%s",m_inp.c_str());}
   virtual void handle( const int64_t& v )const        {
       __DEBUG_APP2__(DEFAULT_LOG_LEVEL,"task=%s",m_inp.c_str());
   }
   virtual void handle( const uint64_t& v )const       {
       __DEBUG_APP2__(DEFAULT_LOG_LEVEL,"task=%s",m_inp.c_str());
   }
   virtual void handle( const double& v )const         {
       __DEBUG_APP2__(DEFAULT_LOG_LEVEL,"task=%s",m_inp.c_str());
   }
   virtual void handle( const bool& v )const           {
       __DEBUG_APP2__(DEFAULT_LOG_LEVEL,"task=%s",m_inp.c_str());
   }
   virtual void handle( const std::string& a_s )const
   {
       //m_str = a_s;
       __DEBUG_APP2__(DEFAULT_LOG_LEVEL,"task=%s, str=%s",m_inp.c_str(),a_s.c_str());
   }
   virtual void handle( const fc::variant_object& a_vo)const {
       size_t unSize = a_vo.size();
       __DEBUG_APP2__(DEFAULT_LOG_LEVEL,"task=%s, unSize=%d",m_inp.c_str(),(int)unSize);
       const fc::variant_object::entry* pEntry;
       fc::variant_object::iterator pItr=a_vo.begin();

       //printf("struct\n");
       m_IsStructField = 1;

       for(size_t i(0); i<unSize;++i,++pItr)
       {
           pEntry = &(*pItr);
           //printf("key=%s\n",pEntry->key().c_str());
           pEntry->value().visit(*this);
       }
       m_IsStructField = 0;
       //printf("end struct\n");
   }
   virtual void handle( const fc::variants& a_vs)const {

       size_t unSize = a_vs.size();
       __DEBUG_APP2__(DEFAULT_LOG_LEVEL,"task=%s, size=%d",m_inp.c_str(),(int)unSize);
       const fc::variant* pVariant;

       for(size_t i(0);i<unSize;++i)
       {
           pVariant = &(a_vs[i]);
           pVariant->visit(*this);
       }
   }

   std::string m_inp;
   mutable int m_IsStructField;
   FinalResult* m_pRes;
};


/*///////////////////////////////////////////////////////*/

static int s_nActive = 0;

void gui_wallet::Mainwindow_gui_wallet::ManagementBrowseContentGUI()
{
    QString cqsNewFilter = m_pCentralWidget->getFilterText();

    if(s_nActive==0)
    {
#if DEFAULT_LOG_LEVEL==0
        SetNewTask3("list_content a 10",this,NULL,&Mainwindow_gui_wallet::TaskDoneBrowseContentGUI3);
#endif
        s_nActive = 1;
    }

#if 1

    if(cqsNewFilter==tr(""))
    {
        //char vcFilter[2] = {(char)1,'\0'};
        char vcFilter[2] = {'a','\0'};
        cqsNewFilter = tr("URI_start:") + tr(vcFilter);
    }

    __DEBUG_APP2__(DEFAULT_LOG_LEVEL,"filter=%s",cqsNewFilter.toStdString().c_str());

    if(cqsNewFilter==m_cqsPreviousFilter){return;}

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
    s_nActive = 0;
}



/*////////////////////////////////////////////////////*/

//void FinalResult::Clear2(){m_values.clear();}
void FinalResult::Clear2()
{
    //m_values.clear();
}
