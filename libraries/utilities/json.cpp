/* (c) 2019 DECENT Services. For details refers to LICENSE.txt */
#include <decent/json.hpp>

namespace fc {

   void to_variant( const nlohmann::json& o, variant& v )
   {
      to_variant( o.dump(), v );
   }

   void from_variant( const variant& v, nlohmann::json& o )
   {
      string s;
      from_variant( v, s );
      try {
         o = nlohmann::json::parse( s );
      }
      catch(const nlohmann::json::parse_error &e) {
         FC_THROW_EXCEPTION(parse_error_exception, e.what());
      }
   }

}
