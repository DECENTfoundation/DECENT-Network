/* (c) 2019 DECENT Services. For details refers to LICENSE.txt */
#pragma once

#include <nlohmann/json.hpp>

#include <fc/variant.hpp>
#include <fc/exception/exception.hpp>
#include <fc/io/datastream.hpp>

namespace fc {

   void to_variant( const nlohmann::json& o, variant& v );
   void from_variant( const variant& v, nlohmann::json& o );

   template<typename ST>
   datastream<ST>& operator<<(datastream<ST>& ds, const nlohmann::json& d) {
      string s = d.dump();
      string::size_type len = s.size();
      ds.write( (const char*)&len, sizeof(len) );
      if( len )
         ds.write( s.data(), len );

      return ds;
   }

   template<typename ST>
   datastream<ST>& operator>>(datastream<ST>& ds, nlohmann::json& d) {
      string::size_type len = 0;
      ds.read( (char*)&len, sizeof(len) );
      if( len ) {
         std::unique_ptr<string::value_type> s( new string::value_type[len] );
         ds.read( s.get(), len );

         try {
            d = nlohmann::json::parse( s.get() );
         }
         catch(const nlohmann::json::parse_error &e) {
            FC_THROW_EXCEPTION(parse_error_exception, e.what());
         }
      }

      return ds;
   }

}

FC_REFLECT_EMPTY( nlohmann::json )
