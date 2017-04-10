#include <graphene/chain/content_object.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/database.hpp>

namespace graphene { namespace chain {

   map<uint32_t, string> RegionCodes::s_mapCodeToName;
   map<string, uint32_t> RegionCodes::s_mapNameToCode;

   bool RegionCodes::bAuxillary = RegionCodes::InitCodeAndName();

   bool RegionCodes::InitCodeAndName()
   {
      vector<pair<uint32_t, string>> arr
      {
         std::make_pair(uint32_t(RegionCodes::OO_none), ""),
         std::make_pair(uint32_t(RegionCodes::OO_all), "default"),
         std::make_pair(uint32_t(RegionCodes::US), "US"),
         std::make_pair(uint32_t(RegionCodes::UK), "UK")
      };

      for (auto const& item : arr)
      {
         s_mapCodeToName.insert(std::make_pair(item.first, item.second));
         s_mapNameToCode.insert(std::make_pair(item.second, item.first));
      }
   }

   optional<asset> PriceRegions::GetPrice(uint32_t region_code) const
   {
      optional<asset> op_price;
      auto it_single_price = map_price.find(uint32_t(RegionCodes::OO_none));
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

      auto it_default_price = map_price.find(uint32_t(RegionCodes::OO_all));
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
      map_price.insert(std::make_pair(uint32_t(RegionCodes::OO_none), price));
   }
   void PriceRegions::SetRegionPrice(uint32_t region_code, asset const& price)
   {
      map_price.insert(std::make_pair(region_code, price));
   }
   bool PriceRegions::Valid(uint32_t region_code) const
   {
      optional<asset> op_price = GetPrice(region_code);
      return op_price.valid();
   }
   bool PriceRegions::Valid(string const& region_code) const
   {
      optional<asset> op_price;
      auto it = RegionCodes::s_mapNameToCode.find(region_code);
      if (it != RegionCodes::s_mapNameToCode.end())
         return Valid(it->second);
      return false;
   }
   //
   // database_api_impl::list_content is not implemented to respect region_code
   // it is not used currently in DECENT runtime
   //
   content_summary& content_summary::set( const content_object& co, const account_object& ao, uint32_t region_code )
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
   content_summary& content_summary::set( const content_object& co, const account_object& ao, string const& region_code )
   {
      auto it = RegionCodes::s_mapNameToCode.find(region_code);
      FC_ASSERT(it != RegionCodes::s_mapNameToCode.end());

      return set(co, ao, it->second);
   }

}}
