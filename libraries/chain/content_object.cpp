#include <graphene/chain/content_object.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/database.hpp>

namespace graphene { namespace chain {


   optional<asset> PriceRegions::GetPrice(string const& region_code/* = string()*/) const
   {
      optional<asset> op_price;
      auto it_single_price = map_price.find(std::string());
      if (it_single_price != map_price.end())
      {
         // content has one price for all regions
         op_price = it_single_price->second;
         return op_price;
      }

      auto it_region_price = map_price.find(region_code);
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
   void PriceRegions::SetSimplePrice(asset const& price)
   {
      map_price.clear();
      map_price.insert(pair<string, asset>(string(), price));
   }
   bool PriceRegions::Valid(string const& region_code) const
   {
      optional<asset> op_price = GetPrice(region_code);
      return op_price.valid();
   }
   //
   // database_api_impl::list_content is not implemented to respect region_code
   // it is not used currently in DECENT runtime
   //
   content_summary& content_summary::set( const content_object& co, const account_object& ao, const string& region_code )
   {
      this->author = ao.name;
#ifdef PRICE_REGIONS
      optional<asset> op_price = co.price.GetPrice(region_code);
      FC_ASSERT(op_price.valid());
      this->price = *op_price;
#else
      this->price = co.price;
#endif
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
