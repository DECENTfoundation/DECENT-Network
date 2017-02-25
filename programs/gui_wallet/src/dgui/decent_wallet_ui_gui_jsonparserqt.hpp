/*
 *	File: decent_wallet_ui_gui_jsonparserqt.hpp
 *
 *	Created on: 25 Feb 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
#ifndef DECENT_WALLET_UI_GUI_JSONPARSERQT_HPP
#define DECENT_WALLET_UI_GUI_JSONPARSERQT_HPP

#include <fc/variant_object.hpp>

namespace decent{ namespace wallet{ namespace ui{ namespace gui{

#ifndef DEFAULT_LOG_LEVEL
#define DEFAULT_LOG_LEVEL 4
#endif

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

struct SKeyValue{
    SKeyValue():type(fc::variant::null_type){key=NULL;pValue=NULL;}
    fc::variant::type_id type;
    union{
        std::string* value;
        union{
            std::string* key;
            class JsonParserQt* pValue;
        };
    };
}; // struct SKeyValue{


class JsonParserQt : public fc::variant::visitor
{
public:
    JsonParserQt();
    virtual ~JsonParserQt();

    void clear();
    bool GetValueByKeyFirst(const std::string& key, SKeyValue* value_ptr);  // recursiv
    bool GetValueByKey(const std::string& key, SKeyValue* value_ptr);
    bool GetValueByIndex(int index, SKeyValue* value_ptr);
    void PrintValues(int tabs=0);

private:
   virtual void handle()const;
   virtual void handle( const int64_t& a_v )const;
   virtual void handle( const uint64_t& a_v )const ;
   virtual void handle( const double& a_v )const;
   virtual void handle( const bool& a_v )const;
   virtual void handle( const std::string& a_v )const;
   virtual void handle( const fc::variant_object& a_vo)const ;
   virtual void handle( const fc::variants& a_vs)const ;

private:
    std::vector<SKeyValue>          m_values;
    mutable std::vector<SKeyValue>* m_pActual;
    mutable std::string*            m_pKeyActive;
public:
    std::string m_inp;
};

}}}}

#endif // DECENT_WALLET_UI_GUI_JSONPARSERQT_HPP
