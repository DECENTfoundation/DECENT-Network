//decent_wallet_ui_gui_jsonparserqt
/*
 *	File: decent_wallet_ui_gui_jsonparserqt.cpp
 *
 *	Created on: 25 Feb 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */


#include "decent_wallet_ui_gui_jsonparserqt.hpp"
#include "debug_decent_application.h"
#include <QString>
#include <QRegExp>

static const char* TypeToString(fc::variant::type_id a_type);


decent::wallet::ui::gui::JsonParserQt::JsonParserQt()
    :
      m_pActual(&m_values),
      m_pKeyActive(NULL)
{
    //
}


decent::wallet::ui::gui::JsonParserQt::~JsonParserQt()
{
    //
}

void decent::wallet::ui::gui::JsonParserQt::clear()
{
    m_values.clear();
}


bool decent::wallet::ui::gui::JsonParserQt::GetValueByKeyFirst(
        const std::string& a_key, SKeyValue* a_value_ptr)
{
    const size_t cunVectorSize(m_values.size());
    for(size_t i(0); i<cunVectorSize;++i)
    {
        if((*m_values[i].key)==a_key)
        {
            if(a_value_ptr){*a_value_ptr=m_values[i];}
            return true;
        }

        if((m_values[i].type==fc::variant::object_type)||(m_values[i].type==fc::variant::array_type))
        {
            if(m_values[i].pValue->GetValueByKeyFirst(a_key,a_value_ptr))
            {
                return true;
            }
        }
    }
    return false;
}


bool decent::wallet::ui::gui::JsonParserQt::GetValueByKey(
        const std::string& a_key, SKeyValue* a_value_ptr)
{
    const size_t cunVectorSize(m_values.size());
    for(size_t i(0); i<cunVectorSize;++i)
    {
        if((*m_values[i].key)==a_key)
        {
            if(a_value_ptr){*a_value_ptr=m_values[i];}
            return true;
        }
    }
    return false;
}


bool decent::wallet::ui::gui::JsonParserQt::GetValueByIndex(
        int a_nIndex, SKeyValue* a_value_ptr)
{
    if(a_nIndex<m_values.size()){if(a_value_ptr){*a_value_ptr=m_values[a_nIndex];}return true;}
    return false;
}

void decent::wallet::ui::gui::JsonParserQt::handle()const
{
    __DEBUG_APP2__(DEFAULT_LOG_LEVEL,"task=%s",m_inp.c_str());
    SKeyValue aNewEntry;
    aNewEntry.type = fc::variant::null_type;
    aNewEntry.key = m_pKeyActive;
    aNewEntry.value = new std::string("");
    m_pActual->push_back(aNewEntry);
    m_pKeyActive = NULL;
}


void decent::wallet::ui::gui::JsonParserQt::handle( const int64_t& a_v )const
{
   __DEBUG_APP2__(DEFAULT_LOG_LEVEL,"task=%s",m_inp.c_str());
   SKeyValue aNewEntry;
   aNewEntry.type = fc::variant::int64_type;
   aNewEntry.key = m_pKeyActive;
   aNewEntry.value = new std::string(QString::number(a_v,10).toStdString());
   m_pActual->push_back(aNewEntry);
   m_pKeyActive = NULL;
}


void decent::wallet::ui::gui::JsonParserQt::handle( const uint64_t& a_v )const
{
   __DEBUG_APP2__(DEFAULT_LOG_LEVEL,"task=%s",m_inp.c_str());
   SKeyValue aNewEntry;
   aNewEntry.type = fc::variant::uint64_type;
   aNewEntry.key = m_pKeyActive;
   aNewEntry.value = new std::string(QString::number(a_v,10).toStdString());
   m_pActual->push_back(aNewEntry);
   m_pKeyActive = NULL;
}


void decent::wallet::ui::gui::JsonParserQt::handle( const double& a_v )const
{
   __DEBUG_APP2__(DEFAULT_LOG_LEVEL,"task=%s",m_inp.c_str());
   SKeyValue aNewEntry;
   QString qsValue = QString::number(a_v,'f').remove( QRegExp("0+$") ).remove( QRegExp("\\.$") );
   aNewEntry.type = fc::variant::double_type;
   aNewEntry.key = m_pKeyActive;
   aNewEntry.value = new std::string(qsValue.toStdString());
   m_pActual->push_back(aNewEntry);
   m_pKeyActive = NULL;
}


void decent::wallet::ui::gui::JsonParserQt::handle( const bool& a_v )const
{
   __DEBUG_APP2__(DEFAULT_LOG_LEVEL,"task=%s",m_inp.c_str());
   SKeyValue aNewEntry;
   aNewEntry.type = fc::variant::bool_type;
   aNewEntry.key = m_pKeyActive;
   aNewEntry.value = new std::string(a_v ? "true" : "false");
   m_pActual->push_back(aNewEntry);
   m_pKeyActive = NULL;
}


void decent::wallet::ui::gui::JsonParserQt::handle( const std::string& a_v )const
{
   __DEBUG_APP2__(DEFAULT_LOG_LEVEL,"task=%s, str=%s",m_inp.c_str(),a_v.c_str());
   SKeyValue aNewEntry;
   aNewEntry.type = fc::variant::string_type;
   aNewEntry.key = m_pKeyActive;
   aNewEntry.value = new std::string(a_v);
   m_pActual->push_back(aNewEntry);
   m_pKeyActive = NULL;
}


void decent::wallet::ui::gui::JsonParserQt::handle( const fc::variant_object& a_vo)const
{
   size_t unSize = a_vo.size();
   __DEBUG_APP2__(DEFAULT_LOG_LEVEL,"task=%s, unSize=%d",m_inp.c_str(),(int)unSize);

   SKeyValue aNewEntry;
   aNewEntry.type = fc::variant::object_type;
   aNewEntry.key = m_pKeyActive;
   aNewEntry.pValue = new JsonParserQt;
   m_pActual->push_back(aNewEntry);
   std::vector<SKeyValue>* pActual = m_pActual;
   m_pActual = &(aNewEntry.pValue->m_values);

   const fc::variant_object::entry* pEntry;
   fc::variant_object::iterator pItr=a_vo.begin();

   for(size_t i(0); i<unSize;++i,++pItr)
   {
       pEntry = &(*pItr);
       m_pKeyActive = new std::string(pEntry->key());
       pEntry->value().visit(*this);
   }
   m_pActual = pActual;
}


void decent::wallet::ui::gui::JsonParserQt::handle( const fc::variants& a_vs)const
{
   size_t unSize = a_vs.size();
   __DEBUG_APP2__(DEFAULT_LOG_LEVEL,"task=%s, size=%d",m_inp.c_str(),(int)unSize);
   const fc::variant* pVariant;


   SKeyValue aNewEntry;
   aNewEntry.type = fc::variant::array_type;
   aNewEntry.key = m_pKeyActive;
   aNewEntry.pValue = new JsonParserQt;
   m_pActual->push_back(aNewEntry);
   std::vector<SKeyValue>* pActual = m_pActual;
   m_pActual = &(aNewEntry.pValue->m_values);
   m_pKeyActive = NULL;

   for(size_t i(0);i<unSize;++i)
   {
       pVariant = &(a_vs[i]);
       pVariant->visit(*this);
   }
   m_pActual = pActual;
}


void decent::wallet::ui::gui::JsonParserQt::PrintValues(int a_nTabs)
{
    int nLoopIndex;
    const size_t cunVectSize(m_values.size());

    for(size_t i(0); i<cunVectSize;++i)
    {
        if((m_values[i].type==fc::variant::object_type)||(m_values[i].type==fc::variant::array_type))
        {
            m_values[i].pValue->PrintValues(a_nTabs+1);
        }
        else
        {
            for(nLoopIndex=0;nLoopIndex<a_nTabs;++nLoopIndex){printf("\t");}
            printf("type=%s, key=%s, value=%s",
                   TypeToString(m_values[i].type),m_values[i].key->c_str(),m_values[i].value->c_str());
        }
        printf("\n");
    }
}


/*/////////////////////////////////////////////////////////////////////////////////*/

#if 0
enum type_id
{
   null_type   = 0,
   int64_type  = 1,
   uint64_type = 2,
   double_type = 3,
   bool_type   = 4,
   string_type = 5,
   array_type  = 6,
   object_type = 7,
   blob_type   = 8
};
#endif

static const char* TypeToString(fc::variant::type_id a_type)
{
    switch(a_type)
    {
    case fc::variant::null_type:
        return "null_type";
    case fc::variant::int64_type:
        return "int64_type";
    case fc::variant::uint64_type:
        return "uint64_type";
    case fc::variant::double_type:
        return "double_type";
    case fc::variant::bool_type:
        return "bool_type";
    case fc::variant::string_type:
        return "string_type";
    case fc::variant::array_type:
        return "array_type";
    case fc::variant::object_type:
        return "object_type";
    case fc::variant::blob_type:
        return "blob_type";
    default:break;
    }
    return "Unknown";
}
