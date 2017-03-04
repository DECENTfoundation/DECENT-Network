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
      
      this->size = co.size;
      this->expiration = co.expiration;
      this->created = co.created;
      this->times_bought = co.times_bought;
       
      return *this;
   }
   
}}
