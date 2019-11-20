/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
#include <graphene/chain/content_object.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/database.hpp>

namespace graphene { namespace chain {

   std::map<uint32_t, std::string> RegionCodes::s_mapCodeToName;
   std::map<std::string, uint32_t> RegionCodes::s_mapNameToCode;

   bool RegionCodes::bAuxillary = RegionCodes::InitCodeAndName();

   fc::optional<asset> PriceRegions::GetPrice(uint32_t region_code) const
   {
      fc::optional<asset> op_price;
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
   void PriceRegions::ClearPrices()
   {
      map_price.clear();
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
      fc::optional<asset> op_price = GetPrice(region_code);
      return op_price.valid();
   }
   bool PriceRegions::Valid(const std::string& region_code) const
   {
      fc::optional<asset> op_price;
      auto it = RegionCodes::s_mapNameToCode.find(region_code);
      if (it != RegionCodes::s_mapNameToCode.end())
         return Valid(it->second);
      return false;
   }
   //
   content_summary& content_summary::set( const content_object& co, const account_object& ao, uint32_t region_code )
   {
      this->id = std::string(co.id);
      this->author = ao.name;
      fc::optional<asset> op_price = co.price.GetPrice(region_code);
      FC_ASSERT(op_price.valid());
      this->price = *op_price;

      this->synopsis = co.synopsis;
      this->URI = co.URI;
      this->AVG_rating = co.AVG_rating;
      this->_hash = co._hash;
      this->size = co.size;
      this->expiration = co.expiration;
      this->created = co.created;
      this->times_bought = co.times_bought;
      if(co.last_proof.size() >= co.quorum)
         this->status = "Uploaded";
      else if( co.last_proof.size() > 0 )
         this->status = "Partially uploaded";
      else
         this->status = "Uploading";

      if(co.expiration <= fc::time_point::now() )
         this->status = "Expired";
      return *this;
   }
   content_summary& content_summary::set( const content_object& co, const account_object& ao, const std::string& region_code )
   {
      auto it = RegionCodes::s_mapNameToCode.find(region_code);
      FC_ASSERT(it != RegionCodes::s_mapNameToCode.end());

      return set(co, ao, it->second);
   }

}}
