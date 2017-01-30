#include <graphene/chain/content_object.hpp>

namespace graphene { namespace chain {
   
   content_summary& content_summary::set( const content_object& obj )
   {
      this->author = string( object_id_type( obj.author ) );
      this->price = std::to_string( obj.price.amount.value ) + " " + string( object_id_type( obj.price.asset_id ) );
      this->synopsis = obj.synopsis;
      this->URI = obj.URI;
      this->AVG_rating = std::to_string(obj.AVG_rating);
      return *this;
   }
   
}}
