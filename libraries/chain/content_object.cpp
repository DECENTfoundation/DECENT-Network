/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
#include <graphene/chain/content_object.hpp>
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

}}
