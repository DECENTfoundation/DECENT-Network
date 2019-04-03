/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
#pragma once
#include <graphene/chain/protocol/base.hpp>
#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/protocol/asset.hpp>
#include <boost/preprocessor/seq/seq.hpp>


#include <fc/reflect/reflect.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/time.hpp>

#include <stdint.h>
#include <vector>
#include <utility>

#include <decent/encrypt/crypto_types.hpp>

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
      vector<account_id_type> to;
      bool can_create_publishers;

      account_id_type fee_payer()const { return from; }
      void validate()const;

      void get_required_active_authorities( flat_set<account_id_type>& a )const { a.insert( account_id_type(15) ); }
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
      vector<account_id_type> to;
      bool is_publisher;

      account_id_type fee_payer()const { return from; }
      void validate()const;
   };

   struct regional_price
   {
      uint32_t region;
      asset    price;
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

      map<account_id_type, uint32_t> co_authors; ///<Optional parameter. If map is not empty, payout will be splitted - the parameter maps co-authors to basis points split, e.g. author1:9000 (bp), auhtor2:1000 (bp)
      string URI; ///<URI where the content can be found
      vector<regional_price> price; ///<list of regional prices

      uint64_t size; ///<Size of content, including samples, in megabytes
      fc::ripemd160 hash; ///<hash of the content

      vector<account_id_type> seeders; ///<List of selected seeders
      vector<ciphertext_type> key_parts; ///< Key particles, each assigned to one of the seeders, encrypted with his key
      /// Defines number of seeders needed to restore the encryption key
      uint32_t quorum; ///< How many seeders needs to cooperate to recover the key
      fc::time_point_sec expiration;
      asset publishing_fee; ///< Fee must be greater than the sum of seeders' publishing prices * number of days. Is paid by author
      string synopsis; ///<JSON formatted structure containing content information
      optional<custody_data_type> cd; ///< if cd.n == 0 then no custody is submitted, and simplified verification is done.

      account_id_type fee_payer()const { return author; }
      void validate()const;
      share_type      calculate_fee( const fee_parameters_type& k )const {if(seeders.size()) return 0; return k.fee; };
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
      string URI;

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
                       (PN)(PR)(PS)(PT)(PW)(PY)(QA)(RE)(RO)(RS)(RU)(RW)(SA)(SB)(SC)(SD)(SE)(SG)(SH)(SI) \
                       (SJ)(SK)(SL)(SM)(SN)(SO)(SR)(SS)(ST)(SV)(SX)(SY)(SZ)(TC)(TD)(TF)(TG)(TH)(TJ)(TK) \
                       (TL)(TM)(TN)(TO)(TR)(TT)(TV)(TW)
   
#define COUNTRY_CODES2 (TZ)(UA)(UG)(UM)(US)(UY)(UZ)(VA)(VC)(VE)(VG)(VI) \
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
   vector<pair<uint32_t, string>> arr { \
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
      static map<uint32_t, string> s_mapCodeToName;
      static map<string, uint32_t> s_mapNameToCode;
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

   /**
    * @ingroup operations
    * @brief This operation is used to send a request to buy a content.
    */
   struct request_to_buy_operation : public base_operation<false>
   {
      struct fee_parameters_type { uint64_t fee = 0; };
      
      asset fee;
      string URI; ///<Reference to the content beuing bought
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
      string URI;
      account_id_type consumer;
      uint64_t rating; ///<1-5 stars
      string comment; /// DECENT_MAX_COMMENT_SIZE
      
      account_id_type fee_payer()const { return consumer; }
      void validate()const;
   };

   /**
    * @ingroup operations
    * @brief This operation is used to register a new seeder, modify the existing seeder or to extend seeder's lifetime.
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
      string ipfs_ID;

      
      account_id_type fee_payer()const { return seeder; }
      void validate()const;
   };
/**
    * @ingroup operations
    * @brief This operation is used to register a new seeder, modify the existing seeder or to extend seeder's lifetime.
    */
struct ready_to_publish2_operation : public base_operation<false>
{
   struct fee_parameters_type { uint64_t fee = 0; };

   asset fee;
   account_id_type seeder;
   bigint_type pubKey;
   /// Available space on seeder's disc dedicated to contents, in MBs
   uint64_t space;
   /// The price charged to author for seeding 1 MB per day
   uint32_t price_per_MByte;
   string ipfs_ID;
   /// Optional ISO 3166-1 alpha-2 two-letter region code
   optional<string> region_code;
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
      string URI;
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
      map<account_id_type,uint64_t> stats;
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
      map<account_id_type, uint32_t> co_authors;
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
FC_REFLECT(graphene::chain::ready_to_publish_operation,(fee)(seeder)(space)(pubKey)(price_per_MByte)(ipfs_ID))
FC_REFLECT(graphene::chain::ready_to_publish2_operation,(fee)(seeder)(space)(pubKey)(price_per_MByte)(ipfs_ID)(region_code))
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
FC_REFLECT( graphene::chain::ready_to_publish_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::ready_to_publish2_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::proof_of_custody_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::deliver_keys_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::return_escrow_submission_operation::fee_parameters_type, ( fee ) )
FC_REFLECT( graphene::chain::return_escrow_buying_operation::fee_parameters_type, ( fee ) )
FC_REFLECT( graphene::chain::report_stats_operation::fee_parameters_type, ( fee ) )
FC_REFLECT( graphene::chain::pay_seeder_operation::fee_parameters_type, ( fee ) )
FC_REFLECT( graphene::chain::finish_buying_operation::fee_parameters_type, ( fee ) )
