/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once
#include <fc/container/flat_fwd.hpp>
#include <fc/io/varint.hpp>
#include <fc/io/enum_type.hpp>
#include <fc/crypto/sha224.hpp>
#include <fc/crypto/elliptic.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/optional.hpp>
#include <fc/safe.hpp>
#include <fc/container/flat.hpp>
#include <fc/string.hpp>
#include <fc/io/raw.hpp>
#include <fc/uint128.hpp>
#include <fc/static_variant.hpp>
#include <fc/smart_ref_fwd.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <decent/encrypt/crypto_types.hpp>

#include <memory>
#include <vector>
#include <deque>
#include <cstdint>
#include <graphene/db/object_id.hpp>
#include <graphene/chain/protocol/config.hpp>

namespace graphene { namespace chain {

   using                               std::map;
   using                               std::vector;
   using                               std::unordered_map;
   using                               std::string;
   using                               std::deque;
   using                               std::shared_ptr;
   using                               std::weak_ptr;
   using                               std::unique_ptr;
   using                               std::set;
   using                               std::pair;
   using                               std::enable_shared_from_this;
   using                               std::tie;
   using                               std::make_pair;

   using                               fc::smart_ref;
   using                               fc::variant_object;
   using                               fc::variant;
   using                               fc::enum_type;
   using                               fc::optional;
   using                               fc::unsigned_int;
   using                               fc::signed_int;
   using                               fc::time_point_sec;
   using                               fc::time_point;
   using                               fc::safe;
   using                               fc::flat_map;
   using                               fc::flat_set;
   using                               fc::static_variant;
   using                               fc::ecc::range_proof_type;
   using                               fc::ecc::range_proof_info;
   using                               fc::ecc::commitment_type;
   struct void_t{};

   typedef fc::ecc::private_key        private_key_type;
   typedef fc::sha256 chain_id_type;

   typedef decent::encrypt::CustodyData custody_data_type;
   typedef decent::encrypt::CustodyProof custody_proof_type;
   typedef decent::encrypt::DIntegerString bigint_type;
   typedef decent::encrypt::CiphertextString ciphertext_type;
   typedef decent::encrypt::DeliveryProofString delivery_proof_type;

   enum reserved_spaces
   {
      local_ids             = 0,
      protocol_ids          = 1,
      implementation_ids    = 2,
      // insert new here
      reserved_spaces_count,     // added due to know count
   };

   // local types
   class seeding_object;

   enum local_object_type
   {
      local_seeding_object_type,                     // 0
      // insert new here
      local_object_type_count    // added due to know count
   };

   typedef graphene::db::object_id<local_ids, local_seeding_object_type, seeding_object> seeding_id_type;

   /**
    *  List all object types from all namespaces here so they can
    *  be easily reflected and displayed in debug output.  If a 3rd party
    *  wants to extend the core code then they will have to change the
    *  packed_object::type field from enum_type to uint16 to avoid
    *  warnings when converting packed_objects to/from json.
    */
   enum protocol_object_type
   {
      null_object_type,                // 0
      base_object_type,
      account_object_type,
      asset_object_type,
      miner_object_type,
      custom_object_type,              // 5
      proposal_object_type,
      operation_history_object_type,
      withdraw_permission_object_type,
      vesting_balance_object_type,
      non_fungible_token_object_type,  // 10
      non_fungible_token_data_object_type,
      // insert new here
      protocol_object_type_count       // added due to need to know count
   };

   enum impl_object_type
   {
      impl_global_property_object_type,
      impl_dynamic_global_property_object_type,
      impl_reserved0_object_type,      // formerly index_meta_object_type, TODO: delete me
      impl_asset_dynamic_data_type,
      impl_account_balance_object_type,
      impl_account_statistics_object_type,         // 5
      impl_transaction_object_type,
      impl_block_summary_object_type,
      impl_account_transaction_history_object_type,
      impl_chain_property_object_type,
      impl_miner_schedule_object_type,             // 10
      impl_budget_record_object_type,
      impl_buying_object_type,
      impl_content_object_type,
      impl_publisher_object_type,
      impl_subscription_object_type,               // 15
      impl_seeding_statistics_object_type,
      impl_transaction_detail_object_type,
      impl_messaging_object_type,
      impl_transaction_history_object_type,
      // insert new here
      impl_object_type_count       // added due to need to know count
   };

   //typedef fc::unsigned_int            object_id_type;
   //typedef uint64_t                    object_id_type;
   class account_object;
   class committee_member_object;
   class miner_object;
   class asset_object;
   class custom_object;
   class proposal_object;
   class operation_history_object;
   class withdraw_permission_object;
   class vesting_balance_object;
   class non_fungible_token_object;
   class non_fungible_token_data_object;

   typedef graphene::db::object_id<protocol_ids, account_object_type, account_object>                         account_id_type;
   typedef graphene::db::object_id<protocol_ids, asset_object_type, asset_object>                             asset_id_type;
   typedef graphene::db::object_id<protocol_ids, miner_object_type, miner_object>                             miner_id_type;
   typedef graphene::db::object_id<protocol_ids, custom_object_type, custom_object>                           custom_id_type;
   typedef graphene::db::object_id<protocol_ids, proposal_object_type, proposal_object>                       proposal_id_type;
   typedef graphene::db::object_id<protocol_ids, operation_history_object_type, operation_history_object>     operation_history_id_type;
   typedef graphene::db::object_id<protocol_ids, withdraw_permission_object_type,withdraw_permission_object>  withdraw_permission_id_type;
   typedef graphene::db::object_id<protocol_ids, vesting_balance_object_type, vesting_balance_object>         vesting_balance_id_type;
   typedef graphene::db::object_id<protocol_ids, non_fungible_token_object_type, non_fungible_token_object>   non_fungible_token_id_type;
   typedef graphene::db::object_id<protocol_ids, non_fungible_token_data_object_type, non_fungible_token_data_object> non_fungible_token_data_id_type;

   // implementation types
   class global_property_object;
   class dynamic_global_property_object;
   class asset_dynamic_data_object;
   class account_balance_object;
   class account_statistics_object;
   class transaction_object;
   class block_summary_object;
   class account_transaction_history_object;
   class chain_property_object;
   class miner_schedule_object;
   class budget_record_object;
   class buying_object;
   class content_object;
   class seeder_object;
   class subscription_object;
   class seeding_statistics_object;
   class transaction_detail_object;
   class transaction_history_object;

   typedef graphene::db::object_id<implementation_ids, impl_global_property_object_type, global_property_object>                   global_property_id_type;
   typedef graphene::db::object_id<implementation_ids, impl_dynamic_global_property_object_type, dynamic_global_property_object>   dynamic_global_property_id_type;
   typedef graphene::db::object_id<implementation_ids, impl_asset_dynamic_data_type, asset_dynamic_data_object>                    asset_dynamic_data_id_type;
   typedef graphene::db::object_id<implementation_ids, impl_account_balance_object_type, account_balance_object>                   account_balance_id_type;
   typedef graphene::db::object_id<implementation_ids, impl_account_statistics_object_type, account_statistics_object>             account_statistics_id_type;
   typedef graphene::db::object_id<implementation_ids, impl_transaction_object_type, transaction_object>                           transaction_obj_id_type;
   typedef graphene::db::object_id<implementation_ids, impl_block_summary_object_type, block_summary_object>                       block_summary_id_type;
   typedef graphene::db::object_id<implementation_ids, impl_account_transaction_history_object_type, account_transaction_history_object> account_transaction_history_id_type;
   typedef graphene::db::object_id<implementation_ids, impl_chain_property_object_type, chain_property_object>                     chain_property_id_type;
   typedef graphene::db::object_id<implementation_ids, impl_miner_schedule_object_type, miner_schedule_object>                     miner_schedule_id_type;
   typedef graphene::db::object_id<implementation_ids, impl_budget_record_object_type, budget_record_object>                       budget_record_id_type;
   typedef graphene::db::object_id<implementation_ids, impl_buying_object_type, buying_object>                                     buying_id_type;
   typedef graphene::db::object_id<implementation_ids, impl_content_object_type, content_object>                                   content_id_type;
   typedef graphene::db::object_id<implementation_ids, impl_publisher_object_type, seeder_object>                                  publisher_id_type;
   typedef graphene::db::object_id<implementation_ids, impl_subscription_object_type, subscription_object>                         subscription_id_type;
   typedef graphene::db::object_id<implementation_ids, impl_seeding_statistics_object_type, seeding_statistics_object>             seeding_statistics_id_type;
   typedef graphene::db::object_id<implementation_ids, impl_transaction_detail_object_type, transaction_detail_object>             transaction_detail_id_type;
   typedef graphene::db::object_id<implementation_ids, impl_transaction_history_object_type, transaction_history_object>           transaction_history_id_type;

   typedef fc::array<char, GRAPHENE_MAX_ASSET_SYMBOL_LENGTH>    symbol_type;
   typedef fc::ripemd160                                        block_id_type;
   typedef fc::ripemd160                                        checksum_type;
   typedef fc::ripemd160                                        transaction_id_type;
   typedef fc::sha256                                           digest_type;
   typedef fc::ecc::compact_signature                           signature_type;
   typedef safe<int64_t>                                        share_type;
   typedef uint16_t                                             weight_type;

   struct public_key_type
   {
       struct binary_key
       {
          binary_key() {}
          uint32_t                 check = 0;
          fc::ecc::public_key_data data;
       };
       fc::ecc::public_key_data key_data;
       public_key_type();
       public_key_type( const fc::ecc::public_key_data& data );
       public_key_type( const fc::ecc::public_key& pubkey );
       explicit public_key_type( const std::string& base58str );
       operator fc::ecc::public_key_data() const;
       operator fc::ecc::public_key() const;
       explicit operator std::string() const;
       friend bool operator == ( const public_key_type& p1, const fc::ecc::public_key& p2);
       friend bool operator == ( const public_key_type& p1, const public_key_type& p2);
       friend bool operator != ( const public_key_type& p1, const public_key_type& p2);
       // TODO: This is temporary for testing
       bool is_valid_v1( const std::string& base58str );
   };
   inline bool operator < ( const public_key_type& a, const public_key_type& b )
   {
        int i=0;
        while (i<33 )
        {
            if(a.key_data.at(i) < b.key_data.at(i) )
                return true;
            if(a.key_data.at(i) > b.key_data.at(i) )
                return false;
            i++;
        }
        return false;
        
   }

   struct extended_public_key_type
   {
      struct binary_key
      {
         binary_key() {}
         uint32_t                   check = 0;
         fc::ecc::extended_key_data data;
      };
      
      fc::ecc::extended_key_data key_data;
       
      extended_public_key_type();
      extended_public_key_type( const fc::ecc::extended_key_data& data );
      extended_public_key_type( const fc::ecc::extended_public_key& extpubkey );
      explicit extended_public_key_type( const std::string& base58str );
      operator fc::ecc::extended_public_key() const;
      explicit operator std::string() const;
      friend bool operator == ( const extended_public_key_type& p1, const fc::ecc::extended_public_key& p2);
      friend bool operator == ( const extended_public_key_type& p1, const extended_public_key_type& p2);
      friend bool operator != ( const extended_public_key_type& p1, const extended_public_key_type& p2);
   };
   
   struct extended_private_key_type
   {
      struct binary_key
      {
         binary_key() {}
         uint32_t                   check = 0;
         fc::ecc::extended_key_data data;
      };
      
      fc::ecc::extended_key_data key_data;
       
      extended_private_key_type();
      extended_private_key_type( const fc::ecc::extended_key_data& data );
      extended_private_key_type( const fc::ecc::extended_private_key& extprivkey );
      explicit extended_private_key_type( const std::string& base58str );
      operator fc::ecc::extended_private_key() const;
      explicit operator std::string() const;
      friend bool operator == ( const extended_private_key_type& p1, const fc::ecc::extended_private_key& p2);
      friend bool operator == ( const extended_private_key_type& p1, const extended_private_key_type& p2);
      friend bool operator != ( const extended_private_key_type& p1, const extended_private_key_type& p2);
   };
} }  // graphene::chain

namespace fc
{
    void to_variant( const graphene::chain::public_key_type& var,  fc::variant& vo );
    void from_variant( const fc::variant& var,  graphene::chain::public_key_type& vo );
    void to_variant( const graphene::chain::extended_public_key_type& var, fc::variant& vo );
    void from_variant( const fc::variant& var, graphene::chain::extended_public_key_type& vo );
    void to_variant( const graphene::chain::extended_private_key_type& var, fc::variant& vo );
    void from_variant( const fc::variant& var, graphene::chain::extended_private_key_type& vo );
}

FC_REFLECT( graphene::chain::public_key_type, (key_data) )
FC_REFLECT( graphene::chain::public_key_type::binary_key, (data)(check) )
FC_REFLECT( graphene::chain::extended_public_key_type, (key_data) )
FC_REFLECT( graphene::chain::extended_public_key_type::binary_key, (check)(data) )
FC_REFLECT( graphene::chain::extended_private_key_type, (key_data) )
FC_REFLECT( graphene::chain::extended_private_key_type::binary_key, (check)(data) )

FC_REFLECT_TYPENAME( graphene::chain::share_type )
FC_REFLECT_TYPENAME( graphene::chain::account_id_type )
FC_REFLECT_TYPENAME( graphene::chain::asset_id_type )
FC_REFLECT_TYPENAME( graphene::chain::miner_id_type )
FC_REFLECT_TYPENAME( graphene::chain::custom_id_type )
FC_REFLECT_TYPENAME( graphene::chain::proposal_id_type )
FC_REFLECT_TYPENAME( graphene::chain::operation_history_id_type )
FC_REFLECT_TYPENAME( graphene::chain::withdraw_permission_id_type )
FC_REFLECT_TYPENAME( graphene::chain::vesting_balance_id_type )
FC_REFLECT_TYPENAME( graphene::chain::non_fungible_token_id_type )
FC_REFLECT_TYPENAME( graphene::chain::non_fungible_token_data_id_type )
FC_REFLECT_TYPENAME( graphene::chain::global_property_id_type )
FC_REFLECT_TYPENAME( graphene::chain::dynamic_global_property_id_type )
FC_REFLECT_TYPENAME( graphene::chain::asset_dynamic_data_id_type )
FC_REFLECT_TYPENAME( graphene::chain::account_balance_id_type )
FC_REFLECT_TYPENAME( graphene::chain::account_statistics_id_type )
FC_REFLECT_TYPENAME( graphene::chain::transaction_obj_id_type )
FC_REFLECT_TYPENAME( graphene::chain::block_summary_id_type )
FC_REFLECT_TYPENAME( graphene::chain::account_transaction_history_id_type )
FC_REFLECT_TYPENAME( graphene::chain::budget_record_id_type )
FC_REFLECT_TYPENAME( graphene::chain::buying_id_type )
FC_REFLECT_TYPENAME( graphene::chain::content_id_type )
FC_REFLECT_TYPENAME( graphene::chain::publisher_id_type )
FC_REFLECT_TYPENAME( graphene::chain::subscription_id_type )
FC_REFLECT_TYPENAME( graphene::chain::seeding_statistics_id_type )
FC_REFLECT_TYPENAME( graphene::chain::transaction_detail_id_type )
FC_REFLECT_TYPENAME( graphene::chain::transaction_history_id_type )

FC_REFLECT_EMPTY( graphene::chain::void_t )
