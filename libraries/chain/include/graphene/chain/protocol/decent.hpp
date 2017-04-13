#pragma once
#include <graphene/chain/protocol/base.hpp>
#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/protocol/asset.hpp>
#include <graphene/chain/content_object.hpp>
#include <boost/preprocessor/seq/seq.hpp>


#include <fc/reflect/reflect.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/time.hpp>

#include <stdint.h>
#include <vector>

#include <decent/encrypt/crypto_types.hpp>

namespace graphene { namespace chain {

   /**
    * @ingroup transactions
    * @brief Submits content to the blockchain.
    */
   struct content_submit_operation : public base_operation
   {
      struct fee_parameters_type { uint64_t fee = 0; };

      asset fee;
      account_id_type author;
#ifdef DECENT_TESTNET2
      // optional parameter. If map is not empty, payout will be splitted
      // maps co-authors to split based on basis points
      // author can be included in co_authors map
      // max num of co-authors = 10
      map<account_id_type, uint32_t> co_authors;
#endif
      string URI;
      asset price;
      uint64_t size; //<Size of content, including samples, in megabytes
      fc::ripemd160 hash;

      vector<account_id_type> seeders; //<List of selected seeders
      vector<decent::encrypt::CiphertextString> key_parts; //<Key particles, each assigned to one of the seeders, encrypted with his key
      /// Defines number of seeders needed to restore the encryption key
      uint32_t quorum;
      fc::time_point_sec expiration;
      asset publishing_fee; //< Fee must be greater than the sum of seeders' publishing prices * number of days. Is paid by author
      string synopsis;
      decent::encrypt::CustodyData cd;
      
      account_id_type fee_payer()const { return author; }
      void validate()const;
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
      /// Consumer's public key
      decent::encrypt::DIntegerString pubKey;
      
      account_id_type fee_payer()const { return consumer; }
      void validate()const;
   };

   /**
    * @ingroup transactions
    * @brief Rates a content.
    */
   struct leave_rating_operation : public base_operation
   {
      struct fee_parameters_type { uint64_t fee = 0; };

      asset fee;
      string URI;
      account_id_type consumer;
      uint64_t rating; //<1-5
      
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
      decent::encrypt::DIntegerString pubKey;
      /// Available space on seeder's disc dedicated to contents, in MBs
      uint64_t space;
      /// The price charged to author for seeding 1 MB per day
      uint32_t price_per_MByte;
      vector<string> ipfs_IDs;
      
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

      decent::encrypt::CustodyProof proof;

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

      decent::encrypt::DeliveryProofString proof;
      decent::encrypt::CiphertextString key;
      
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
      /// The final stats about single download process. Map of seeders to amount they uploaded
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
      account_id_type author;
      map<account_id_type, uint32_t> co_authors;
      buying_id_type buying;

      account_id_type fee_payer()const { return author; }
      void            validate()const { FC_ASSERT( !"virtual operation" ); }
   };

} } // graphene::chain

#ifdef DECENT_TESTNET2
FC_REFLECT(graphene::chain::content_submit_operation,(fee)(size)(author)(co_authors)(URI)(quorum)(price)(hash)(seeders)(key_parts)(expiration)(publishing_fee)(synopsis)(cd))
#else
FC_REFLECT(graphene::chain::content_submit_operation,(fee)(size)(author)(URI)(quorum)(price)(hash)(seeders)(key_parts)(expiration)(publishing_fee)(synopsis)(cd))
#endif
FC_REFLECT(graphene::chain::request_to_buy_operation,(fee)(URI)(consumer)(price)(pubKey))
FC_REFLECT(graphene::chain::leave_rating_operation,(fee)(URI)(consumer)(rating))
FC_REFLECT(graphene::chain::ready_to_publish_operation,(fee)(seeder)(space)(pubKey)(price_per_MByte)(ipfs_IDs))
FC_REFLECT(graphene::chain::proof_of_custody_operation,(fee)(seeder)(URI)(proof))
FC_REFLECT(graphene::chain::deliver_keys_operation,(fee)(seeder)(proof)(key)(buying))
FC_REFLECT(graphene::chain::return_escrow_submission_operation,(fee)(author)(escrow)(content))
FC_REFLECT(graphene::chain::return_escrow_buying_operation,(fee)(consumer)(escrow)(buying))
FC_REFLECT(graphene::chain::report_stats_operation,(fee)(consumer)(stats))
FC_REFLECT(graphene::chain::pay_seeder_operation,(fee)(payout)(author)(seeder));
FC_REFLECT(graphene::chain::finish_buying_operation,(fee)(payout)(author)(co_authors)(buying));


FC_REFLECT( graphene::chain::content_submit_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::request_to_buy_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::leave_rating_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::ready_to_publish_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::proof_of_custody_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::deliver_keys_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::return_escrow_submission_operation::fee_parameters_type, ( fee ) )
FC_REFLECT( graphene::chain::return_escrow_buying_operation::fee_parameters_type, ( fee ) )
FC_REFLECT( graphene::chain::report_stats_operation::fee_parameters_type, ( fee ) )
FC_REFLECT( graphene::chain::pay_seeder_operation::fee_parameters_type, ( fee ) )
FC_REFLECT( graphene::chain::finish_buying_operation::fee_parameters_type, ( fee ) )
