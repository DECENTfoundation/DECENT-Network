#pragma once
#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/protocol/asset.hpp>
#include <graphene/db/object.hpp>
#include <graphene/db/generic_index.hpp>

#include <fc/crypto/ripemd160.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/io/json.hpp>

#include <stdint.h>
#include <vector>
#include <utility>

#ifdef DECENT_TESTNET2
#define PRICE_REGIONS
#endif

namespace graphene { namespace chain {
using namespace decent::encrypt;

   class RegionCodes
   {
   public:
      enum RegionCode
      {
         OO_none = 1,
         OO_all,
         US,
         UK
      };
      static bool bAuxillary;
      static map<uint32_t, string> s_mapCodeToName;
      static map<string, uint32_t> s_mapNameToCode;

      static bool InitCodeAndName();
   };

   struct PriceRegions
   {
      map<uint32_t, asset> map_price;

      optional<asset> GetPrice(uint32_t region_code) const;
      void SetSimplePrice(asset const& price);
      void SetRegionPrice(uint32_t region_code, asset const& price);
      bool Valid(uint32_t region_code) const;
      bool Valid(string const& region_code) const;
   };

   template <typename basic_type, typename Derived>
   class ContentObjectPropertyBase
   {
   public:
      using value_type = basic_type;

      void load(string const& str_synopsis)
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
            string str_name = Derived::name();

            if (variant_synopsis.get_object().contains(str_name.c_str()))
               variant_value = variant_synopsis[str_name.c_str()];
         }

         if (variant_value.is_array())
         {
            for (size_t iIndex = 0; iIndex < variant_value.size(); ++iIndex)
            {
               fc::variant const& variant_item = variant_value[iIndex];
               string str_value = variant_item.as_string();
               value_type item;
               ContentObjectPropertyBase::convert_from_string(str_value, item);
               m_arrValues.push_back(item);
            }
         }
         else if (false == variant_value.is_null())
         {
            string str_value = variant_value.as_string();
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
            string str_value;
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
      void save(string& str_synopsis) const
      {
         vector<value_type> arrValues = m_arrValues;

         if (is_unique(0) &&
             1 < arrValues.size())
            arrValues.resize(1);

         if (is_default(0) &&
             arrValues.empty())
         {
            string str_value;
            value_type item;
            ContentObjectPropertyBase::convert_from_string(str_value, item);
            arrValues.push_back(item);
         }

         if (arrValues.empty())
            return;

         variant variant_value;
         if (1 == arrValues.size())
         {
            value_type const& item = arrValues[0];
            string str_value;
            ContentObjectPropertyBase::convert_to_string(item, str_value);
            variant_value = str_value;
         }
         else
         {
            fc::variants arr_variant_value;
            for (auto const& item : arrValues)
            {
               string str_value;
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

         string str_name = Derived::name();
         mutable_variant_obj.set(str_name, variant_value);
         variant_obj = mutable_variant_obj;

         str_synopsis = fc::json::to_string(variant_obj);
      }

      template <typename U>
      static void convert_from_string(string const& str_value, U& converted_value)
      {
         Derived::convert_from_string(str_value, converted_value);
      }
      template <typename U>
      static void convert_to_string(U const& value, string& str_converted_value)
      {
         Derived::convert_to_string(value, str_converted_value);
      }

      static void convert_from_string(string const& str_value, string& converted_value)
      {
         converted_value = str_value;
      }
      static void convert_to_string(string const& str_value, string& converted_value)
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
      vector<value_type> m_arrValues;
   };

   enum class EContentObjectType
   {
      GUI,
      Book
   };

   class ContentObjectType : public ContentObjectPropertyBase<EContentObjectType, ContentObjectType>
   {
   public:
      using meta_default = bool;
      using meta_unique = bool;

      static string name()
      {
         return "type";
      }

      static void convert_from_string(string const& str_value, EContentObjectType& type)
      {
         if (str_value == "GUI")
            type = EContentObjectType::GUI;
         else if (str_value == "Book")
            type = EContentObjectType::Book;
         else if (str_value == "")
            type = EContentObjectType::GUI;
         else
            throw std::runtime_error("EContentObjectType is in exceptional situation");
      }
      static void convert_to_string(EContentObjectType const& type, string& str_value)
      {
         switch (type)
         {
         case EContentObjectType::GUI:
            str_value = "GUI";
            break;
         case EContentObjectType::Book:
            str_value = "Book";
            break;
         }
      }
   };
   class ContentObjectTitle : public ContentObjectPropertyBase<string, ContentObjectTitle>
   {
   public:
      using meta_fallback = bool;
      using meta_unique = bool;
      static string name()
      {
         return "title";
      }
   };
   class ContentObjectDescription : public ContentObjectPropertyBase<string, ContentObjectDescription>
   {
   public:
      using meta_default = bool;
      using meta_unique = bool;
      static string name()
      {
         return "description";
      }
   };
   class ContentObjectISBN : public ContentObjectPropertyBase<string, ContentObjectISBN>
   {
   public:
      using meta_unique = bool;
      static string name()
      {
         return "isbn";
      }
   };

   class ContentObjectPropertyManager
   {
   public:
      ContentObjectPropertyManager(string const& str_synopsis = string())
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

      string m_str_synopsis;
   };

   struct content_summary
   {
      string author;
      asset price;
      string synopsis;
      string status;
      string URI;
      uint32_t AVG_rating = 0;
      uint64_t size = 0;
      time_point_sec expiration;
      time_point_sec created;
      uint32_t times_bought = 0;

      content_summary& set( const content_object& co, const account_object& ao, uint32_t region_code );
      content_summary& set( const content_object& co, const account_object& ao, string const& region_code );
   };

   class content_object : public graphene::db::abstract_object<content_object>
   {
   public:
      static const uint8_t space_id = implementation_ids;
      static const uint8_t type_id  = impl_content_object_type;
      
      account_id_type author;
      time_point_sec expiration;
      time_point_sec created;
#ifdef PRICE_REGIONS
      PriceRegions price;
#else
      asset price;
#endif
      string synopsis;
      uint64_t size;
      uint32_t quorum;
      string URI;
      map<account_id_type, CiphertextString> key_parts;
      map<account_id_type, time_point_sec> last_proof;

      fc::ripemd160 _hash;
      uint64_t AVG_rating;
      uint32_t total_rating;
      uint32_t times_bought;
      asset publishing_fee_escrow;
      decent::encrypt::CustodyData cd;


#ifdef PRICE_REGIONS
      template <RegionCodes::RegionCode code>
      share_type get_price_amount_template() const
      {
         FC_ASSERT(price.Valid(code));
         return price.GetPrice(code)->amount;
      }
#else
      share_type get_price_amount() const
      {
         return price.amount;
      }
#endif
   };
   
   struct by_author;
   struct by_URI;
   struct by_AVG_rating;
   struct by_size;
   struct by_price;
   struct by_times_bought;
   struct by_expiration;
   struct by_created;
   
   
   typedef multi_index_container<
      content_object,
         indexed_by<
            ordered_unique< tag<by_id>,
               member< object, object_id_type, &object::id >
            >,
   
   
            ordered_non_unique<tag<by_author>,
               member<content_object, account_id_type, &content_object::author>
            >,
            ordered_unique<tag<by_URI>,
               member<content_object, string, &content_object::URI>
            >,
   
#ifdef PRICE_REGIONS
            ordered_non_unique<tag<by_price>,
            const_mem_fun<content_object, share_type, &content_object::get_price_amount_template<RegionCodes::OO_none>>
            >,
#else
            ordered_non_unique<tag<by_price>,
            const_mem_fun<content_object, share_type, &content_object::get_price_amount>
            >,
#endif

            ordered_non_unique<tag<by_size>,
               member<content_object, uint64_t, &content_object::size>
            >,
            ordered_non_unique<tag<by_AVG_rating>,
               member<content_object, uint64_t, &content_object::AVG_rating>
            >,
            ordered_non_unique<tag<by_times_bought>,
               member<content_object, uint32_t, &content_object::times_bought>,
               std::greater<uint32_t>
            >,
   
            ordered_non_unique<tag<by_expiration>,
               member<content_object, time_point_sec, &content_object::expiration>
            >,
   
            ordered_non_unique<tag<by_created>,
               member<content_object, time_point_sec, &content_object::created>
            >
   
         >
   > content_object_multi_index_type;
   
   
   typedef generic_index< content_object, content_object_multi_index_type > content_index;
   
}}

FC_REFLECT_DERIVED(graphene::chain::content_object,
                   (graphene::db::object),
                   (author)(expiration)(created)(price)(size)(synopsis)
                   (URI)(quorum)(key_parts)(_hash)(last_proof)
                   (AVG_rating)(total_rating)(times_bought)(publishing_fee_escrow)(cd) )

FC_REFLECT( graphene::chain::content_summary, (author)(price)(synopsis)(status)(URI)(AVG_rating)(size)(expiration)(created)(times_bought) )
FC_REFLECT( graphene::chain::PriceRegions, (map_price) )
