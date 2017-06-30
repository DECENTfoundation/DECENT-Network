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
    * @ingroup transaction
    * @brief This operation is used to promote account to publishing manager.
    * Such an account can grant or remove right to publish a content. Only DECENT account has permission to use this method.
    */
   struct set_publishing_manager_operation : public base_operation
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
    * @ingroup transaction
    * @brief Allows account to publish a content. Only account with publishing manager status has permission to use this method.
    */
   struct set_publishing_right_operation : public base_operation
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
    * @ingroup transactions
    * @brief Submits content to the blockchain.
    */
   struct content_submit_operation : public base_operation
   {
      struct fee_parameters_type { uint64_t fee = 0; };

      asset fee;
      account_id_type author;
      // optional parameter. If map is not empty, payout will be splitted
      // maps co-authors to split based on basis points
      // author can be included in co_authors map
      // max num of co-authors = 10
      map<account_id_type, uint32_t> co_authors;
      string URI;
      vector<regional_price> price;

      uint64_t size; //<Size of content, including samples, in megabytes
      fc::ripemd160 hash;

      vector<account_id_type> seeders; //<List of selected seeders
      vector<ciphertext_type> key_parts; //<Key particles, each assigned to one of the seeders, encrypted with his key
      /// Defines number of seeders needed to restore the encryption key
      uint32_t quorum;
      fc::time_point_sec expiration;
      asset publishing_fee; //< Fee must be greater than the sum of seeders' publishing prices * number of days. Is paid by author
      string synopsis;
      optional<custody_data_type> cd; //< if cd.n == 0 then no custody is submitted, and simplified verification is done.

      account_id_type fee_payer()const { return author; }
      void validate()const;
   };

   /**
    * @ingroup transactions
    * @brief This operation is used to cancel submitted content.
    */
   struct content_cancellation_operation : public base_operation
   {
      struct fee_parameters_type { uint64_t fee = 0; };

      asset fee;
      account_id_type author;
      string URI;

      account_id_type fee_payer()const { return author; }
      void validate()const { FC_ASSERT( URI != "" ); };
   };


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

   /**
    * @ingroup transactions
    * @brief This operation is used to send a request to buy a content.
    */
   struct request_to_buy_operation : public base_operation
   {
      struct fee_parameters_type { uint64_t fee = 0; };
      
      asset fee;
      string URI;
      account_id_type consumer;
      asset price;
      uint32_t region_code_from = RegionCodes::OO_none;

      /// Consumer's public key
      bigint_type pubKey;
      
      account_id_type fee_payer()const { return consumer; }
      void validate()const;
   };

   /**
    * @ingroup transactions
    * @brief Rates a content.
    */
   struct leave_rating_and_comment_operation : public base_operation
   {
      struct fee_parameters_type { uint64_t fee = 0; };

      asset fee;
      string URI;
      account_id_type consumer;
      uint64_t rating; //<1-5
      string comment; // up to 1000 characters
      
      account_id_type fee_payer()const { return consumer; }
      void validate()const;
   };

   /**
    * @ingroup transactions
    * @brief This operation is used to register a new seeder, modify the existing seeder or to extend seeder's lifetime.
    */
   struct ready_to_publish_operation : public base_operation
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
    * @ingroup transactions
    * @brief Seeders have to periodically prove that they hold the content.
    */
   struct proof_of_custody_operation : public base_operation
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
    * @ingroup transactions
    * @brief This operation is used to send encrypted share of a content and proof of delivery to consumer.
    */
   struct deliver_keys_operation : public base_operation
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
    * @ingroup transactions
    * @brief This is a virtual operation emitted for the purpose of returning escrow to author
    */
   struct return_escrow_submission_operation : public base_operation
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
    * @ingroup transactions
    * @brief This is a virtual operation emitted for the purpose of returning escrow to consumer
    */
   struct return_escrow_buying_operation : public base_operation
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
    * @ingroup transactions
    * @brief This operation is used to report stats. These stats are later used to rate seeders.
    */
   struct report_stats_operation : public base_operation
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
    * @ingroup transactions
    * @brief
    */
   struct pay_seeder_operation : public base_operation
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
    * @ingroup transactions
    * @brief
    */
   struct finish_buying_operation : public base_operation
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
FC_REFLECT(graphene::chain::proof_of_custody_operation,(fee)(seeder)(URI)(proof))
FC_REFLECT(graphene::chain::deliver_keys_operation,(fee)(seeder)(proof)(key)(buying))
FC_REFLECT(graphene::chain::return_escrow_submission_operation,(fee)(author)(escrow)(content))
FC_REFLECT(graphene::chain::return_escrow_buying_operation,(fee)(consumer)(escrow)(buying))
FC_REFLECT(graphene::chain::report_stats_operation,(fee)(consumer)(stats))
FC_REFLECT(graphene::chain::pay_seeder_operation,(fee)(payout)(author)(seeder));
FC_REFLECT(graphene::chain::finish_buying_operation,(fee)(payout)(author)(co_authors)(buying)(consumer));

FC_REFLECT( graphene::chain::set_publishing_manager_operation::fee_parameters_type, ( fee ) )
FC_REFLECT( graphene::chain::set_publishing_right_operation::fee_parameters_type, ( fee ) )
FC_REFLECT( graphene::chain::content_submit_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::content_cancellation_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::request_to_buy_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::leave_rating_and_comment_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::ready_to_publish_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::proof_of_custody_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::deliver_keys_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::return_escrow_submission_operation::fee_parameters_type, ( fee ) )
FC_REFLECT( graphene::chain::return_escrow_buying_operation::fee_parameters_type, ( fee ) )
FC_REFLECT( graphene::chain::report_stats_operation::fee_parameters_type, ( fee ) )
FC_REFLECT( graphene::chain::pay_seeder_operation::fee_parameters_type, ( fee ) )
FC_REFLECT( graphene::chain::finish_buying_operation::fee_parameters_type, ( fee ) )
