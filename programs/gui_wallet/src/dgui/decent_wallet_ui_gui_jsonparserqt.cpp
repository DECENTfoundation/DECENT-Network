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

using namespace gui_wallet;

static const char* TypeToString(fc::variant::type_id a_type);


JsonParserQt::JsonParserQt()
{
}


JsonParserQt::JsonParserQt(const std::string& a_key)
    :
      m_type(fc::variant::null_type),
      m_key(a_key)
{
}


#if 0
JsonParserQt*               m_pParent;
fc::variant::type_id        m_type;
std::string                 m_key;
std::string                 m_value;
std::vector<JsonParserQt*>  m_vValue;
#endif


JsonParserQt::~JsonParserQt()
{
    this->clear();
}


bool JsonParserQt::isVector()const
{
    return ((m_type==fc::variant::object_type)||(m_type==fc::variant::array_type));
}


int JsonParserQt::size()const
{
    return m_vValue.size();
}


void JsonParserQt::clear()
{
    const size_t cunSize(m_vValue.size());
    for(size_t i(0); i<cunSize; ++i){delete m_vValue[i];}
    m_vValue.clear();
}


const std::string& JsonParserQt::value()const
{
    return m_value;
}


const JsonParserQt&
JsonParserQt::GetByKeyFirst(
        const std::string& a_key, bool* a_bIsFound)const
{
    const JsonParserQt* pReturn;
    bool bIsFoundTmp;
    bool& bIsFound = a_bIsFound ? *a_bIsFound : bIsFoundTmp;
    const size_t cunVectorSize(m_vValue.size());

    bIsFound = false;

    if(a_key == m_key)
    {
        bIsFound = true;
        return *this;
    }

    for(size_t i(0); i<cunVectorSize;++i)
    {
        if((m_vValue[i]->m_key)==a_key)
        {
            bIsFound = true;
            return *m_vValue[i];
        }

        if(m_vValue[i]->isVector())
        {
            pReturn = &(m_vValue[i]->GetByKeyFirst(a_key,&bIsFound));
            if(bIsFound){return *pReturn;}
        }
    }
    return *this;
}


const JsonParserQt&
JsonParserQt::GetByKey(
        const std::string& a_key, bool* a_bIsFound)const
{
    const size_t cunVectorSize(m_vValue.size());
    for(size_t i(0); i<cunVectorSize;++i)
    {
        if((m_vValue[i]->m_key)==a_key)
        {
            if(a_bIsFound){*a_bIsFound = true;}
            return *m_vValue[i];
        }
    }
    if(a_bIsFound){*a_bIsFound = false;}
    return *this;
}


const JsonParserQt&
JsonParserQt::GetByIndex(
        int a_nIndex, bool* a_bIsFound)const
{
    if(a_nIndex<m_vValue.size())
    {
        if(a_bIsFound){*a_bIsFound=true;}
        return *m_vValue[a_nIndex];
    }
    if(a_bIsFound){*a_bIsFound=false;}
    return *this;
}

void JsonParserQt::handle()const
{
    __DEBUG_APP2__(DEFAULT_LOG_LEVEL,"task=%s, key=%s",m_inp.c_str(),m_key.c_str());
    m_type = fc::variant::null_type;
    m_value = "";
}


void JsonParserQt::handle( const int64_t& a_v )const
{
   m_type = fc::variant::int64_type;
   m_value = QString::number(a_v,10).toStdString();
   __DEBUG_APP2__(DEFAULT_LOG_LEVEL,"task=%s, key=%s, value=%s",
                  m_inp.c_str(),m_key.c_str(),m_value.c_str());
}


void JsonParserQt::handle( const uint64_t& a_v )const
{
   m_type = fc::variant::uint64_type;
   m_value = QString::number(a_v,10).toStdString();
   __DEBUG_APP2__(DEFAULT_LOG_LEVEL,"task=%s, key=%s, value=%s",
                  m_inp.c_str(),m_key.c_str(),m_value.c_str());
}


void JsonParserQt::handle( const double& a_v )const
{
   QString qsValue = QString::number(a_v,'f').remove( QRegExp("0+$") ).remove( QRegExp("\\.$") );
   m_type = fc::variant::double_type;
   m_value = qsValue.toStdString();
   __DEBUG_APP2__(DEFAULT_LOG_LEVEL,"task=%s, key=%s, value=%s",
                  m_inp.c_str(),m_key.c_str(),m_value.c_str());
}


void JsonParserQt::handle( const bool& a_v )const
{
   m_type = fc::variant::bool_type;
   m_value = a_v ? "true" : "false";
   __DEBUG_APP2__(DEFAULT_LOG_LEVEL,"task=%s, key=%s, value=%s",
                  m_inp.c_str(),m_key.c_str(),m_value.c_str());
}


void JsonParserQt::handle( const std::string& a_v )const
{
   m_type = fc::variant::string_type;
   m_value = a_v;
   __DEBUG_APP2__(DEFAULT_LOG_LEVEL,"task=%s, key=%s, value=%s",
                  m_inp.c_str(),m_key.c_str(),m_value.c_str());
}


void JsonParserQt::handle( const fc::variant_object& a_vo)const
{
   JsonParserQt* pMembersToAdd;
   const size_t cunSize( a_vo.size());
   __DEBUG_APP2__(DEFAULT_LOG_LEVEL,"task=%s, key=%s, size=%d",
                  m_inp.c_str(),m_key.c_str(),(int)cunSize);

   m_type = fc::variant::object_type;

   const fc::variant_object::entry* pEntry;
   fc::variant_object::iterator pItr=a_vo.begin();

   for(size_t i(0); i<cunSize;++i,++pItr)
   {
       pEntry = &(*pItr);

       pMembersToAdd = new JsonParserQt(pEntry->key());
       if(!pMembersToAdd){break;}// Should be some error handling
       pMembersToAdd->m_inp = m_inp;

       pEntry->value().visit(*pMembersToAdd);
       m_vValue.push_back(pMembersToAdd);
   }
}


void JsonParserQt::handle( const fc::variants& a_vs)const
{
   JsonParserQt* pMembersToAdd;
   const size_t cunSize ( a_vs.size() );
   __DEBUG_APP2__(DEFAULT_LOG_LEVEL,"task=%s, key=%s, size=%d",
                  m_inp.c_str(),m_key.c_str(),(int)cunSize);
   const fc::variant* pVariant;

   m_type = fc::variant::array_type;

   for(size_t i(0);i<cunSize;++i)
   {
       pMembersToAdd = new JsonParserQt("");
       if(!pMembersToAdd){break;}// Should be some error handling
       pMembersToAdd->m_inp = m_inp;

       pVariant = &(a_vs[i]);
       pVariant->visit(*pMembersToAdd);
       m_vValue.push_back(pMembersToAdd);
   }
}


void JsonParserQt::PrintValues(int a_nTabs)const
{
    int nLoopIndex;
    const size_t cunVectSize(m_vValue.size());

    for(nLoopIndex=0;nLoopIndex<a_nTabs;++nLoopIndex){printf("\t");}

    if(isVector()){printf("start of ");}

    printf("type=%s, key=%s",TypeToString(m_type),m_key.c_str());

    if(!isVector()){printf(", value=%s",m_value.c_str());}

    for(size_t i(0); i<cunVectSize;++i)
    {
        printf("\n");
        m_vValue[i]->PrintValues(a_nTabs+1);
    }

    if(isVector()){printf("\nend of type=%s, key=%s\n",TypeToString(m_type),m_key.c_str());}
}


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

const char* JsonParserQt::TypeToString()const
{
    return JsonParserQt::TypeToString(m_type);
}

const char* JsonParserQt::TypeToString(fc::variant::type_id a_type)
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
