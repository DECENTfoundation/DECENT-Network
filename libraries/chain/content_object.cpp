#include <graphene/chain/content_object.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/database.hpp>

namespace graphene { namespace chain {


   content_summary& content_summary::set( const content_object& co, const account_object& ao )
   {
      this->author = ao.name;
      this->price = co.price;
      this->synopsis = co.synopsis;
      this->URI = co.URI;
      this->AVG_rating = co.AVG_rating;
      return *this;
   }
   
}}
