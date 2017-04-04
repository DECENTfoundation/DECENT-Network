#include <graphene/chain/content_object.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/database.hpp>

namespace graphene { namespace chain {


   content_summary& content_summary::set( const content_object& co, const account_object& ao )
   {
      string str_region_code;
      optional<asset> op_price = co.GetPrice(str_region_code);
      this->author = ao.name;
      if (op_price.valid())
         this->price = *op_price;
      this->synopsis = co.synopsis;
      this->URI = co.URI;
      this->AVG_rating = co.AVG_rating;
      
      this->size = co.size;
      this->expiration = co.expiration;
      this->created = co.created;
      this->times_bought = co.times_bought;
       
      return *this;
   }

   optional<asset> content_object::GetPrice(string const& str_region_code) const
   {
      optional<asset> op_price;
      auto it_single_price = map_price.find(string());
      if (it_single_price != map_price.end())
      {
         // content has one price for all regions
         op_price = it_single_price->second;
         return op_price;
      }

      auto it_region_price = map_price.find(str_region_code);
      if (it_region_price != map_price.end())
      {
         // content has price corresponding to this region
         op_price = it_region_price->second;
         return op_price;
      }

      auto it_default_price = map_price.find("default");
      if (it_default_price != map_price.end())
      {
         // content has default price covering this and all other regions
         op_price = it_default_price->second;
         return op_price;
      }

      return op_price;
   }

   void content_object::SetSimplePrice(asset const& price)
   {
      map_price.clear();
      map_price.insert(pair<string, asset>(string(), price));
   }
   
}}
