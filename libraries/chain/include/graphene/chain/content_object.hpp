/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
#pragma once
#include <graphene/chain/protocol/asset.hpp>
#include <graphene/chain/protocol/decent.hpp>

#include <graphene/db/object.hpp>
#include <graphene/db/generic_index.hpp>

#include <fc/io/json.hpp>

#include <utility>

namespace graphene { namespace chain {

   template <typename basic_type, typename Derived>
   class ContentObjectPropertyBase
   {
   public:
      using value_type = basic_type;

      void load(const std::string& str_synopsis)
      {
         m_arrValues.clear();
         fc::variant variant_synopsis;
         bool bFallBack = false;   // fallback to support synopsis that was used simply as a string
         try
         {
            if (false == str_synopsis.empty())
               variant_synopsis = fc::json::from_string(str_synopsis);
         }
         catch(...)
         {
            if (is_fallback(0))
               bFallBack = true;
         }

         fc::variant variant_value;

         if (variant_synopsis.is_object())
         {
            std::string str_name = Derived::name();

            if (variant_synopsis.get_object().contains(str_name.c_str()))
               variant_value = variant_synopsis[str_name.c_str()];
         }

         if (variant_value.is_array())
         {
            for (size_t iIndex = 0; iIndex < variant_value.size(); ++iIndex)
            {
               fc::variant const& variant_item = variant_value[iIndex];
               std::string str_value = variant_item.as_string();
               value_type item;
               ContentObjectPropertyBase::convert_from_string(str_value, item);
               m_arrValues.push_back(item);
            }
         }
         else if (false == variant_value.is_null())
         {
            std::string str_value = variant_value.as_string();
            value_type item;
            ContentObjectPropertyBase::convert_from_string(str_value, item);
            m_arrValues.push_back(item);
         }

         if (bFallBack &&
             m_arrValues.empty())
         {
            value_type item;
            ContentObjectPropertyBase::convert_from_string(str_synopsis, item);
            m_arrValues.push_back(item);
         }

         if (is_default(0) &&
             m_arrValues.empty())
         {
            std::string str_value;
            value_type item;
            ContentObjectPropertyBase::convert_from_string(str_value, item);
            m_arrValues.push_back(item);
         }

         if (is_unique(0) &&
             1 < m_arrValues.size())
         {
            m_arrValues.resize(1);
         }
      }
      void save(std::string& str_synopsis) const
      {
         std::vector<value_type> arrValues = m_arrValues;

         if (is_unique(0) &&
             1 < arrValues.size())
            arrValues.resize(1);

         if (is_default(0) &&
             arrValues.empty())
         {
            std::string str_value;
            value_type item;
            ContentObjectPropertyBase::convert_from_string(str_value, item);
            arrValues.push_back(item);
         }

         if (arrValues.empty())
            return;

         fc::variant variant_value;
         if (1 == arrValues.size())
         {
            value_type const& item = arrValues[0];
            std::string str_value;
            ContentObjectPropertyBase::convert_to_string(item, str_value);
            variant_value = str_value;
         }
         else
         {
            fc::variants arr_variant_value;
            for (auto const& item : arrValues)
            {
               std::string str_value;
               ContentObjectPropertyBase::convert_to_string(item, str_value);
               arr_variant_value.emplace_back(str_value);
            }
            variant_value = arr_variant_value;
         }

         fc::variant variant_synopsis;
         if (false == str_synopsis.empty())
            variant_synopsis = fc::json::from_string(str_synopsis);

         if (false == variant_synopsis.is_null() &&
             false == variant_synopsis.is_object())
            variant_synopsis.clear();

         if (variant_synopsis.is_null())
         {
            fc::mutable_variant_object mutable_variant_obj;
            variant_synopsis = mutable_variant_obj;
         }

         fc::variant_object& variant_obj = variant_synopsis.get_object();
         fc::mutable_variant_object mutable_variant_obj(variant_obj);

         std::string str_name = Derived::name();
         mutable_variant_obj.set(str_name, variant_value);
         variant_obj = mutable_variant_obj;

         str_synopsis = fc::json::to_string(variant_obj);
      }

      template <typename U>
      static void convert_from_string(const std::string& str_value, U& converted_value)
      {
         Derived::convert_from_string(str_value, converted_value);
      }
      template <typename U>
      static void convert_to_string(U const& value, std::string& str_converted_value)
      {
         Derived::convert_to_string(value, str_converted_value);
      }

      static void convert_from_string(const std::string& str_value, std::string& converted_value)
      {
         converted_value = str_value;
      }
      static void convert_to_string(const std::string& str_value, std::string& converted_value)
      {
         converted_value = str_value;
      }

      template <typename U = Derived>
      static bool is_default(...)
      {
         return false;
      }

      template <typename U = Derived>
      static bool is_default(typename U::meta_default)
      {
         return true;
      }

      template <typename U = Derived>
      static bool is_unique(...)
      {
         return false;
      }

      template <typename U = Derived>
      static bool is_unique(typename U::meta_unique)
      {
         return true;
      }
      template <typename U = Derived>
      static bool is_fallback(...)
      {
         return false;
      }

      template <typename U = Derived>
      static bool is_fallback(typename U::meta_fallback)
      {
         return true;
      }

   public:
      std::vector<value_type> m_arrValues;
   };

   enum class EContentObjectApplication
   {
      DecentCore,
      DecentGo,
      Alax
   };
   enum class EContentObjectType
   {
      None,
      Music,
      Movie,
      Book,
      AudioBook,
      Software,
      Game,
      Picture,
      Document
   };

   class ContentObjectTypeValue
   {
   public:
      ContentObjectTypeValue(EContentObjectApplication appID = EContentObjectApplication::DecentCore,
                             EContentObjectType typeID = EContentObjectType::None)
      {
         init(appID, typeID);
      }

      void to_string(std::string& str_value) const
      {
         str_value.clear();
         for (size_t index = 0; index < type.size(); ++index)
         {
            std::string strPart = std::to_string(type[index]);
            str_value += strPart;
            if (index < type.size() - 1)
               str_value += ".";
         }
      }
      void from_string(std::string str_value)
      {
         type.clear();
         uint32_t iValue = 0;
         size_t pos;
         while (true)
         {
            if (str_value.empty())
               break;
            iValue = std::stol(str_value, &pos);

            if (pos == 0)
               break;

            type.push_back(iValue);

            if (pos == std::string::npos)
               break;
            else if (str_value[pos] != '.')
               break;
            else
               str_value = str_value.substr(pos + 1);
         }

         init();
      }

      bool filter(ContentObjectTypeValue const& filter)
      {
         bool bRes = true;
         if (filter.type.size() > type.size())
            bRes = false;
         else
         {
            for (size_t index = 0; index < filter.type.size(); ++index)
            {
               if (type[index] != filter.type[index])
               {
                  bRes = false;
                  break;
               }
            }
         }

         return bRes;
      }
   private:
      void init(EContentObjectApplication appID = EContentObjectApplication::DecentCore,
                EContentObjectType typeID = EContentObjectType::None)
      {
         if (type.size() == 0)
            type.push_back(static_cast<uint32_t>(appID));
         if (type.size() == 1 &&
             typeID != EContentObjectType::None)
            type.push_back(static_cast<uint32_t>(typeID));
      }

   public:
      std::vector<uint32_t> type;
   };

   class ContentObjectType : public ContentObjectPropertyBase<ContentObjectTypeValue, ContentObjectType>
   {
   public:
      using meta_default = bool;
      using meta_unique = bool;

      static std::string name()
      {
         return "content_type_id";
      }

      static void convert_from_string(const std::string& str_value, ContentObjectTypeValue& type)
      {
         type.from_string(str_value);
      }
      static void convert_to_string(ContentObjectTypeValue const& type, std::string& str_value)
      {
         type.to_string(str_value);
      }
   };

   class ContentObjectTitle : public ContentObjectPropertyBase<std::string, ContentObjectTitle>
   {
   public:
      using meta_fallback = bool;
      using meta_unique = bool;
      static std::string name()
      {
         return "title";
      }
   };
   class ContentObjectDescription : public ContentObjectPropertyBase<std::string, ContentObjectDescription>
   {
   public:
      using meta_default = bool;
      using meta_unique = bool;
      static std::string name()
      {
         return "description";
      }
   };
   class ContentObjectISBN : public ContentObjectPropertyBase<std::string, ContentObjectISBN>
   {
   public:
      using meta_unique = bool;
      static std::string name()
      {
         return "isbn";
      }
   };

   class ContentObjectPropertyManager
   {
   public:
      ContentObjectPropertyManager(const std::string& str_synopsis = std::string())
         : m_str_synopsis(str_synopsis)
      {

      }

      template <typename P>
      typename P::value_type get() const
      {
         P property;
         property.load(m_str_synopsis);

         if (1 != property.m_arrValues.size())
            throw std::runtime_error("ContentObjectPropertyManager expects ideal conditions for now");

         return property.m_arrValues.front();
      }

      template <typename P>
      void set(typename P::value_type const& value)
      {
         P property;
         property.m_arrValues.clear();
         property.m_arrValues.push_back(value);
         property.save(m_str_synopsis);
      }

      std::string m_str_synopsis;
   };

   struct content_summary
   {
      std::string id;
      std::string author;
      asset price;
      std::string synopsis;
      fc::ripemd160 _hash;
      std::string status;
      std::string URI;
      uint32_t AVG_rating = 0;
      uint64_t size = 0;
      fc::time_point_sec expiration;
      fc::time_point_sec created;
      uint32_t times_bought = 0;

      content_summary& set( const content_object& co, const account_object& ao, uint32_t region_code );
      content_summary& set( const content_object& co, const account_object& ao, const std::string& region_code );
   };

   struct content_keys {
       fc::sha256                 key;
       std::vector<ciphertext_type> parts;
       uint32_t                   quorum = 2;
   };

   class content_object : public graphene::db::abstract_object<implementation_ids, impl_content_object_type, content_object>
   {
   public:
      account_id_type author;
      // If co_authors map is not empty, payout will be splitted.
      // Maps co-authors to split based on basis points.
      std::map<account_id_type, uint32_t> co_authors;

      fc::time_point_sec expiration;
      fc::time_point_sec created;
      PriceRegions price;

      std::string synopsis;
      uint64_t size;
      uint32_t quorum;
      std::string URI;
      std::map<account_id_type, decent::encrypt::CiphertextString> key_parts;
      std::map<account_id_type, fc::time_point_sec> last_proof;
      std::map<account_id_type, share_type> seeder_price;
      bool is_blocked = false;

      fc::ripemd160 _hash;
      uint32_t AVG_rating = 0;
      uint32_t num_of_ratings = 0;
      uint32_t times_bought = 0;
      asset publishing_fee_escrow;
      fc::optional<decent::encrypt::CustodyData> cd;

      template <RegionCodes::RegionCode code>
      share_type get_price_amount_template() const
      {
         // this function is used only by db index
         // later will avoid using index for prices
         // but anyway, make sure the index will not throw
         // for regions not having a price
         // use fallback to any other region price that is defined
         // (at least one price will be defined)
         // corresponding code that uses the index "by_price" will check
         // for such wrongly placed content items and skip those
         fc::optional<asset> region_price = price.GetPrice(code);
         if (false == region_price.valid())
         {
            FC_ASSERT(false == price.map_price.empty());
            auto it_any_price = price.map_price.begin();
            region_price = it_any_price->second;
         }

         return region_price->amount;
      }

      bool recent_proof( uint64_t validity_seconds ) const {
         auto  now = fc::time_point::now();
         for( auto i: last_proof ){
            if( i.second > now - fc::seconds(validity_seconds) )
               return true;
         }
         return false;
      }
   };

   struct by_author;
   struct by_URI;
   struct by_AVG_rating;
   struct by_size;
   struct by_price;
   struct by_times_bought;
   struct by_expiration;
   struct by_created;

   template <typename TAG, typename _t_object>
   struct key_extractor;

   template <>
   struct key_extractor<by_author, content_object>
   {
      static account_id_type get(content_object const& ob)
      {
         return ob.author;
      }
   };

   template <>
   struct key_extractor<by_URI, content_object>
   {
      static std::string get(content_object const& ob)
      {
         return ob.URI;
      }
   };

   template <>
   struct key_extractor<by_AVG_rating, content_object>
   {
      static uint32_t get(content_object const& ob)
      {
         return ob.AVG_rating;
      }
   };

   template <>
   struct key_extractor<by_size, content_object>
   {
      static uint64_t get(content_object const& ob)
      {
         return ob.size;
      }
   };

   template <>
   struct key_extractor<by_price, content_object>
   {
      static share_type get(content_object const& ob)
      {
         return ob.get_price_amount_template<RegionCodes::OO_none>();
      }
   };

   template <>
   struct key_extractor<by_expiration, content_object>
   {
      static fc::time_point_sec get(content_object const& ob)
      {
         return ob.expiration;
      }
   };

   template <>
   struct key_extractor<by_created, content_object>
   {
      static fc::time_point_sec get(content_object const& ob)
      {
         return ob.created;
      }
   };

   using namespace boost::multi_index;

   typedef multi_index_container<
      content_object,
         indexed_by<
            graphene::db::object_id_index,
            ordered_non_unique<tag<by_author>,
               member<content_object, account_id_type, &content_object::author>
            >,
            ordered_unique<tag<by_URI>,
               member<content_object, std::string, &content_object::URI>
            >,
            ordered_non_unique<tag<by_price>,
            const_mem_fun<content_object, share_type, &content_object::get_price_amount_template<RegionCodes::OO_none>>
            >,
            ordered_non_unique<tag<by_size>,
               member<content_object, uint64_t, &content_object::size>
            >,
            ordered_non_unique<tag<by_AVG_rating>,
               member<content_object, uint32_t, &content_object::AVG_rating>
            >,
            ordered_non_unique<tag<by_times_bought>,
               member<content_object, uint32_t, &content_object::times_bought>,
               std::greater<uint32_t>
            >,

            ordered_non_unique<tag<by_expiration>,
               member<content_object, fc::time_point_sec, &content_object::expiration>
            >,

            ordered_non_unique<tag<by_created>,
               member<content_object, fc::time_point_sec, &content_object::created>
            >

         >
   > content_object_multi_index_type;

   typedef graphene::db::generic_index< content_object, content_object_multi_index_type > content_index;

}}

FC_REFLECT_DERIVED(graphene::chain::content_object,
                   (graphene::db::object),
                   (author)(co_authors)(expiration)(created)(price)(size)(synopsis)
                   (URI)(quorum)(key_parts)(_hash)(last_proof)(is_blocked)
                   (AVG_rating)(num_of_ratings)(times_bought)(publishing_fee_escrow)(cd)(seeder_price) )

FC_REFLECT( graphene::chain::content_summary, (id)(author)(price)(synopsis)(status)(URI)(_hash)(AVG_rating)(size)(expiration)(created)(times_bought) )
FC_REFLECT( graphene::chain::PriceRegions, (map_price) )
FC_REFLECT( graphene::chain::ContentObjectTypeValue, (type) )

FC_REFLECT( graphene::chain::content_keys, (key)(parts)(quorum) )
