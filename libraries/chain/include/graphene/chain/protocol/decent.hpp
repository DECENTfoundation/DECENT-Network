/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
#pragma once
#include <graphene/chain/protocol/base.hpp>
#include <graphene/chain/protocol/asset.hpp>
#include <boost/preprocessor/seq/seq.hpp>

#include <fc/reflect/reflect.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/io/json.hpp>
#include <fc/time.hpp>

#include <vector>
#include <utility>

namespace graphene { namespace chain {

   /**
    * @ingroup operations
    * @brief This operation is used to promote account to publishing manager.
    * Such an account can grant or remove right to publish a content. Only DECENT account has permission to use this method.
    */
   struct set_publishing_manager_operation : public base_operation<false>
   {
      struct fee_parameters_type { uint64_t fee = 0; };

      asset fee;
      account_id_type from;
      std::vector<account_id_type> to;
      bool can_create_publishers;

      account_id_type fee_payer()const { return from; }
      void validate()const;

      void get_required_active_authorities( boost::container::flat_set<account_id_type>& a )const { a.insert( account_id_type(15) ); }
   };

   /**
    * @ingroup operations
    * @brief Allows account to publish a content. Only account with publishing manager status has permission to use this method.
    */
   struct set_publishing_right_operation : public base_operation<false>
   {
      struct fee_parameters_type { uint64_t fee = 0; };

      asset fee;
      account_id_type from;
      std::vector<account_id_type> to;
      bool is_publisher;

      account_id_type fee_payer()const { return from; }
      void validate()const;
   };

   struct regional_price
   {
      uint32_t region;
      asset    price;
   };

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
            const char* str_name = Derived::name();

            if (variant_synopsis.get_object().contains(str_name))
               variant_value = variant_synopsis[str_name];
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

         const char* str_name = Derived::name();
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
      DecentCore
   };

   class ContentObjectTypeValue
   {
   public:
      ContentObjectTypeValue(EContentObjectApplication appID = EContentObjectApplication::DecentCore)
      {
         type.push_back(static_cast<uint32_t>(appID));
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

         if (type.empty())
            type.push_back(static_cast<uint32_t>(EContentObjectApplication::DecentCore));
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

   public:
      std::vector<uint32_t> type;
   };

   class ContentObjectType : public ContentObjectPropertyBase<ContentObjectTypeValue, ContentObjectType>
   {
   public:
      using meta_default = bool;
      using meta_unique = bool;
      static const char* name() { return "content_type_id"; }

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
      static const char* name() { return "title"; }
   };

   class ContentObjectDescription : public ContentObjectPropertyBase<std::string, ContentObjectDescription>
   {
   public:
      using meta_default = bool;
      using meta_unique = bool;
      static const char* name() { return "description"; }
   };

   class ContentObjectPropertyManager
   {
   public:
      ContentObjectPropertyManager(const std::string& synopsis)
         : _synopsis(synopsis)
      {
      }

      template <typename P>
      typename P::value_type get() const
      {
         P property;
         property.load(_synopsis);

         FC_ASSERT(property.m_arrValues.size() == 1, "Failed to get '${p}' from '${s}'", ("p", P::name())("s", _synopsis));
         return property.m_arrValues.front();
      }

      template <typename P>
      void set(typename P::value_type const& value)
      {
         P property;
         property.m_arrValues.clear();
         property.m_arrValues.push_back(value);
         property.save(_synopsis);
      }

      std::string _synopsis;
   };

   /**
    * @ingroup operations
    * @brief Submits content to the blockchain.
    */
   struct content_submit_operation : public base_operation<false>
   {
      struct fee_parameters_type { uint64_t fee = GRAPHENE_BLOCKCHAIN_PRECISION / 100; };

      asset fee;
      account_id_type author; ///<author of the content. If co-authors is not filled, this account will receive full payout

      std::map<account_id_type, uint32_t> co_authors; ///<Optional parameter. If map is not empty, payout will be splitted - the parameter maps co-authors to basis points split, e.g. author1:9000 (bp), auhtor2:1000 (bp)
      std::string URI; ///<URI where the content can be found
      std::vector<regional_price> price; ///<list of regional prices

      uint64_t size; ///<Size of content, including samples, in megabytes
      fc::ripemd160 hash; ///<hash of the content

      std::vector<account_id_type> seeders; ///<List of selected seeders
      std::vector<ciphertext_type> key_parts; ///< Key particles, each assigned to one of the seeders, encrypted with his key
      /// Defines number of seeders needed to restore the encryption key
      uint32_t quorum; ///< How many seeders needs to cooperate to recover the key
      fc::time_point_sec expiration;
      asset publishing_fee; ///< Fee must be greater than the sum of seeders' publishing prices * number of days. Is paid by author
      std::string synopsis; ///<JSON formatted structure containing content information
      fc::optional<custody_data_type> cd; ///< if cd.n == 0 then no custody is submitted, and simplified verification is done.

      account_id_type fee_payer()const { return author; }
      void validate()const;
      share_type      calculate_fee( const fee_parameters_type& k,
                                     const fc::time_point_sec now )const {if(seeders.size()) return 0; return k.fee; };
   };

   /**
    * @ingroup operations
    * @brief This operation is used to cancel submitted content.
    */
   struct content_cancellation_operation : public base_operation<false>
   {
      struct fee_parameters_type { uint64_t fee = 0; };

      asset fee;
      account_id_type author;
      std::string URI;

      account_id_type fee_payer()const { return author; }
      void validate()const { FC_ASSERT( URI != "" ); };
   };

#ifdef _MSC_VER
#undef IN
#endif
// ISO 3166-1 alpha-2 two-letter region codes ( num of codes = 249 )
#define COUNTRY_CODES1 (AD)(AE)(AF)(AG)(AI)(AL)(AM)(AO)(AQ)(AR)(AS)(AT)(AU)(AW)(AX)(AZ)(BA)(BB)(BD)(BE) \
                       (BF)(BG)(BH)(BI)(BJ)(BL)(BM)(BN)(BO)(BQ)(BR)(BS)(BT)(BV)(BW)(BY)(BZ)(CA)(CC)(CD) \
                       (CF)(CG)(CH)(CI)(CK)(CL)(CM)(CN)(CO)(CR)(CU)(CV)(CW)(CX)(CY)(CZ)(DE)(DJ)(DK)(DM) \
                       (DO)(DZ)(EC)(EE)(EG)(EH)(ER)(ES)(ET)(FI)(FJ)(FK)(FM)(FO)(FR)(GA)(GB)(GD)(GE)(GF) \
                       (GG)(GH)(GI)(GL)(GM)(GN)(GP)(GQ)(GR)(GS)(GT)(GU)(GW)(GY)(HK)(HM)(HN)(HR)(HT)(HU) \
                       (ID)(IE)(IL)(IM)(IN)(IO)(IQ)(IR)(IS)(IT)(JE)(JM)(JO)(JP)(KE)(KG)(KH)(KI)(KM)(KN) \
                       (KP)(KR)(KW)(KY)(KZ)(LA)(LB)(LC)(LI)(LK)(LR)(LS)(LT)(LU)(LV)(LY)(MA)(MC)(MD)(ME) \
                       (MF)(MG)(MH)(MK)(ML)(MM)(MN)(MO)(MP)(MQ)(MR)(MS)(MT)(MU)(MV)(MW)(MX)(MY)(MZ)(NA) \
                       (NC)(NE)(NF)(NG)(NI)(NL)(NO)(NP)(NR)(NU)(NZ)(OM)(PA)(PE)(PF)(PG)(PH)(PK)(PL)(PM) \
                       (PN)(PR)(PS)(PT)(PW)(PY)(QA)(RE)(RO)(RS)(RU)(RW)(SA)(SB)(SC)(SD)(SE)(SG)(SH)(SI)

#define COUNTRY_CODES2 (SJ)(SK)(SL)(SM)(SN)(SO)(SR)(SS)(ST)(SV)(SX)(SY)(SZ)(TC)(TD)(TF)(TG)(TH)(TJ)(TK) \
                       (TL)(TM)(TN)(TO)(TR)(TT)(TV)(TW)(TZ)(UA)(UG)(UM)(US)(UY)(UZ)(VA)(VC)(VE)(VG)(VI) \
                       (VN)(VU)(WF)(WS)(YE)(YT)(ZA)(ZM)(ZW)

#define INNER_MACRO(r, data, elem) elem,
#define INNER_MACRO2(r, data, elem) std::make_pair(uint32_t(RegionCodes::elem), BOOST_PP_STRINGIZE(elem)),
#define MY_MACRO( ENUM, FIELDS1, FIELDS2 ) \
enum ENUM{ \
 OO_none = 1,\
 OO_all,\
 BOOST_PP_SEQ_FOR_EACH(INNER_MACRO, _, FIELDS1) \
 BOOST_PP_SEQ_FOR_EACH(INNER_MACRO, _, FIELDS2) \
}; \
static bool InitCodeAndName() { \
   std::vector<std::pair<uint32_t, std::string>> arr { \
      std::make_pair(uint32_t(RegionCodes::OO_none), ""), \
      std::make_pair(uint32_t(RegionCodes::OO_all), "default"), \
      BOOST_PP_SEQ_FOR_EACH(INNER_MACRO2, _, FIELDS1) \
      BOOST_PP_SEQ_FOR_EACH(INNER_MACRO2, _, FIELDS2) \
   }; \
   for (auto const& item : arr) \
   { \
      s_mapCodeToName.insert(std::make_pair(item.first, item.second)); \
      s_mapNameToCode.insert(std::make_pair(item.second, item.first)); \
   } \
   return true; \
}

   class RegionCodes
   {
   public:
      MY_MACRO( RegionCode, COUNTRY_CODES1, COUNTRY_CODES2 ) // enum + InitCodeAndName
      static bool bAuxillary;
      static std::map<uint32_t, std::string> s_mapCodeToName;
      static std::map<std::string, uint32_t> s_mapNameToCode;
   };

   /**
    * @ingroup operations
    * @brief This operation is used to send a request to buy a content.
    */
   struct request_to_buy_operation : public base_operation<false>
   {
      struct fee_parameters_type { uint64_t fee = 0; };

      asset fee;
      std::string URI; ///<Reference to the content beuing bought
      account_id_type consumer; ///< Who is buying (and paying)
      asset price; ///< Has to be equal or greater than the price defined in content
      uint32_t region_code_from = RegionCodes::OO_none; ///< Location of the consumer

      /// Consumer's public key
      bigint_type pubKey;

      account_id_type fee_payer()const { return consumer; }
      void validate()const;
   };

   /**
    * @ingroup operations
    * @brief Rates a content.
    */
   struct leave_rating_and_comment_operation : public base_operation<false>
   {
      struct fee_parameters_type { uint64_t fee = 0; };

      asset fee;
      std::string URI;
      account_id_type consumer;
      uint64_t rating; ///<1-5 stars
      std::string comment; /// DECENT_MAX_COMMENT_SIZE

      account_id_type fee_payer()const { return consumer; }
      void validate()const;
   };

   /**
    * @ingroup operations
    * @brief This operation is used to register a new seeder, modify the existing seeder or to extend seeder's lifetime.
    * @warning Obsolete operation. Use \c ready_to_publish_operation instead.
    */
   struct ready_to_publish_obsolete_operation : public base_operation<false>
   {
      struct fee_parameters_type { uint64_t fee = 0; };

      asset fee;
      account_id_type seeder;
      bigint_type pubKey;
      /// Available space on seeder's disc dedicated to contents, in MBs
      uint64_t space;
      /// The price charged to author for seeding 1 MB per day
      uint32_t price_per_MByte;
      std::string ipfs_ID;

      account_id_type fee_payer()const { return seeder; }
      void validate()const;
   };

   /**
    * @ingroup operations
    * @brief This operation is used to register a new seeder, modify the existing seeder or to extend seeder's lifetime.
    * @note can't be broadcast more than once in 30 minutes.
    */
   struct ready_to_publish_operation : public base_operation<false>
   {
      struct fee_parameters_type { uint64_t fee = 0; };

      asset fee;
      account_id_type seeder;
      bigint_type pubKey;
      /// Available space on seeder's disc dedicated to contents, in MBs
      uint64_t space;
      /// The price charged to author for seeding 1 MB per day
      uint32_t price_per_MByte;
      std::string ipfs_ID;
      /// Optional ISO 3166-1 alpha-2 two-letter region code
      fc::optional<std::string> region_code;
      extensions_type extensions;

      account_id_type fee_payer()const { return seeder; }
      void validate()const;
   };

   /**
    * @ingroup operations
    * @brief Seeders have to periodically prove that they hold the content.
    */
   struct proof_of_custody_operation : public base_operation<false>
   {
      struct fee_parameters_type { uint64_t fee = 0; };

      asset fee;
      account_id_type seeder;
      std::string URI;
      fc::optional<custody_proof_type> proof;

      account_id_type fee_payer()const { return seeder; }
      void validate()const;
   };

   /**
    * @ingroup operations
    * @brief This operation is used to send encrypted share of a content and proof of delivery to consumer.
    */
   struct deliver_keys_operation : public base_operation<false>
   {
      struct fee_parameters_type { uint64_t fee = 0; };

      asset fee;
      account_id_type seeder;
      buying_id_type buying;

      delivery_proof_type proof;
      ciphertext_type key;

      account_id_type fee_payer()const { return seeder; }
      void validate()const;
   };

   /**
    * @ingroup operations
    * @brief This is a virtual operation emitted for the purpose of returning escrow to author
    */
   struct return_escrow_submission_operation : public base_operation<true>
   {
      struct fee_parameters_type { uint64_t fee = 0; };

      asset fee;
      account_id_type author;
      asset escrow;
      content_id_type content;

      account_id_type fee_payer()const { return author; }
      void            validate()const { FC_ASSERT( !"virtual operation" ); }
   };

   /**
    * @ingroup operations
    * @brief This is a virtual operation emitted for the purpose of returning escrow to consumer
    */
   struct return_escrow_buying_operation : public base_operation<true>
   {
      struct fee_parameters_type { uint64_t fee = 0; };

      asset fee;
      account_id_type consumer;
      asset escrow;
      buying_id_type buying;

      account_id_type fee_payer()const { return consumer; }
      void            validate()const { FC_ASSERT( !"virtual operation" ); }
   };

   /**
    * @ingroup operations
    * @brief This operation is used to report stats. These stats are later used to rate seeders.
    */
   struct report_stats_operation : public base_operation<false>
   {
      struct fee_parameters_type { uint64_t fee = 0; };

      asset fee;
      /// Map of seeders to amount they uploaded
      std::map<account_id_type,uint64_t> stats;
      account_id_type consumer;

      account_id_type fee_payer()const { return consumer; }
      void validate()const;
   };

   /**
    * @ingroup operations
    * @brief This is a virtual operation
    */
   struct pay_seeder_operation : public base_operation<true>
   {
      struct fee_parameters_type { uint64_t fee = 0; };

      asset fee;

      asset payout;
      account_id_type author;
      account_id_type seeder;

      account_id_type fee_payer()const { return author; }
      void            validate()const { FC_ASSERT( !"virtual operation" ); }
   };

   /**
    * @ingroup operations
    * @brief This is a virtual operation
    */
   struct finish_buying_operation : public base_operation<true>
   {
      struct fee_parameters_type { uint64_t fee = 0; };

      asset fee;

      asset payout;
      // do we need here region_code_from?
      account_id_type author;
      std::map<account_id_type, uint32_t> co_authors;
      account_id_type consumer;
      buying_id_type buying;

      account_id_type fee_payer()const { return author; }
      void            validate()const { FC_ASSERT( !"virtual operation" ); }
   };

} } // graphene::chain

FC_REFLECT(graphene::chain::regional_price,(region)(price))

FC_REFLECT(graphene::chain::content_submit_operation,(fee)(size)(author)(co_authors)(URI)(quorum)(price)(hash)(seeders)(key_parts)(expiration)(publishing_fee)(synopsis)(cd))
FC_REFLECT(graphene::chain::set_publishing_manager_operation,(fee)(from)(to)(can_create_publishers))
FC_REFLECT(graphene::chain::set_publishing_right_operation,(fee)(from)(to)(is_publisher))
FC_REFLECT(graphene::chain::content_cancellation_operation,(fee)(author)(URI))
FC_REFLECT(graphene::chain::request_to_buy_operation,(fee)(URI)(consumer)(price)(region_code_from)(pubKey))
FC_REFLECT(graphene::chain::leave_rating_and_comment_operation,(fee)(URI)(consumer)(comment)(rating))
FC_REFLECT(graphene::chain::ready_to_publish_obsolete_operation,(fee)(seeder)(space)(pubKey)(price_per_MByte)(ipfs_ID))
FC_REFLECT(graphene::chain::ready_to_publish_operation,(fee)(seeder)(space)(pubKey)(price_per_MByte)(ipfs_ID)(region_code))
FC_REFLECT(graphene::chain::proof_of_custody_operation,(fee)(seeder)(URI)(proof))
FC_REFLECT(graphene::chain::deliver_keys_operation,(fee)(seeder)(proof)(key)(buying))
FC_REFLECT(graphene::chain::return_escrow_submission_operation,(fee)(author)(escrow)(content))
FC_REFLECT(graphene::chain::return_escrow_buying_operation,(fee)(consumer)(escrow)(buying))
FC_REFLECT(graphene::chain::report_stats_operation,(fee)(consumer)(stats))
FC_REFLECT(graphene::chain::pay_seeder_operation,(fee)(payout)(author)(seeder))
FC_REFLECT(graphene::chain::finish_buying_operation,(fee)(payout)(author)(co_authors)(buying)(consumer))

FC_REFLECT( graphene::chain::set_publishing_manager_operation::fee_parameters_type, ( fee ) )
FC_REFLECT( graphene::chain::set_publishing_right_operation::fee_parameters_type, ( fee ) )
FC_REFLECT( graphene::chain::content_submit_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::content_cancellation_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::request_to_buy_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::leave_rating_and_comment_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::ready_to_publish_obsolete_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::ready_to_publish_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::proof_of_custody_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::deliver_keys_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::return_escrow_submission_operation::fee_parameters_type, ( fee ) )
FC_REFLECT( graphene::chain::return_escrow_buying_operation::fee_parameters_type, ( fee ) )
FC_REFLECT( graphene::chain::report_stats_operation::fee_parameters_type, ( fee ) )
FC_REFLECT( graphene::chain::pay_seeder_operation::fee_parameters_type, ( fee ) )
FC_REFLECT( graphene::chain::finish_buying_operation::fee_parameters_type, ( fee ) )
