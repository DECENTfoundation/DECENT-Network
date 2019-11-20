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
#include <fc/io/varint.hpp>
#include <fc/crypto/elliptic.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/safe.hpp>
#include <fc/io/raw.hpp>
#include <decent/encrypt/crypto_types.hpp>

#include <memory>
#include <graphene/db/object_id.hpp>
#include <graphene/chain/config.hpp>

namespace graphene { namespace chain {

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
      local_seeding_object_type  = 0,
      // insert new here
      local_object_type_count    // added due to know count
   };

   typedef db::object_id<local_ids, local_seeding_object_type, seeding_object> seeding_id_type;

   /**
    *  List all object types from all namespaces here so they can
    *  be easily reflected and displayed in debug output.  If a 3rd party
    *  wants to extend the core code then they will have to change the
    *  packed_object::type field from enum_type to uint16 to avoid
    *  warnings when converting packed_objects to/from json.
    */
   enum protocol_object_type
   {
      //null_object_type                   = 0,
      //base_object_type                   = 1,
      account_object_type                  = 2,
      asset_object_type                    = 3,
      miner_object_type                    = 4,
      //custom_object_type                 = 5,
      proposal_object_type                 = 6,
      operation_history_object_type        = 7,
      withdraw_permission_object_type      = 8,
      vesting_balance_object_type          = 9,
      non_fungible_token_object_type       = 10,
      non_fungible_token_data_object_type  = 11,
      // insert new here
      protocol_object_type_count       // added due to need to know count
   };

   enum impl_object_type
   {
      impl_global_property_object_type              = 0,
      impl_dynamic_global_property_object_type      = 1,
      //impl_index_meta_object_type                 = 2,
      impl_asset_dynamic_data_type                  = 3,
      impl_account_balance_object_type              = 4,
      impl_account_statistics_object_type           = 5,
      impl_transaction_object_type                  = 6,
      impl_block_summary_object_type                = 7,
      impl_account_transaction_history_object_type  = 8,
      impl_chain_property_object_type               = 9,
      impl_miner_schedule_object_type               = 10,
      impl_budget_record_object_type                = 11,
      impl_buying_object_type                       = 12,
      impl_content_object_type                      = 13,
      impl_publisher_object_type                    = 14,
      impl_subscription_object_type                 = 15,
      impl_seeding_statistics_object_type           = 16,
      impl_transaction_detail_object_type           = 17,
      impl_messaging_object_type                    = 18,
      impl_transaction_history_object_type          = 19,
      // insert new here
      impl_object_type_count       // added due to need to know count
   };

   class account_object;
   class miner_object;
   class asset_object;
   class proposal_object;
   class operation_history_object;
   class withdraw_permission_object;
   class vesting_balance_object;
   class non_fungible_token_object;
   class non_fungible_token_data_object;

   typedef db::object_id<protocol_ids, account_object_type, account_object>                         account_id_type;
   typedef db::object_id<protocol_ids, asset_object_type, asset_object>                             asset_id_type;
   typedef db::object_id<protocol_ids, miner_object_type, miner_object>                             miner_id_type;
   typedef db::object_id<protocol_ids, proposal_object_type, proposal_object>                       proposal_id_type;
   typedef db::object_id<protocol_ids, operation_history_object_type, operation_history_object>     operation_history_id_type;
   typedef db::object_id<protocol_ids, withdraw_permission_object_type,withdraw_permission_object>  withdraw_permission_id_type;
   typedef db::object_id<protocol_ids, vesting_balance_object_type, vesting_balance_object>         vesting_balance_id_type;
   typedef db::object_id<protocol_ids, non_fungible_token_object_type, non_fungible_token_object>   non_fungible_token_id_type;
   typedef db::object_id<protocol_ids, non_fungible_token_data_object_type, non_fungible_token_data_object> non_fungible_token_data_id_type;

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
   class message_object;
   class transaction_history_object;

   typedef db::object_id<implementation_ids, impl_global_property_object_type, global_property_object>                   global_property_id_type;
   typedef db::object_id<implementation_ids, impl_dynamic_global_property_object_type, dynamic_global_property_object>   dynamic_global_property_id_type;
   typedef db::object_id<implementation_ids, impl_asset_dynamic_data_type, asset_dynamic_data_object>                    asset_dynamic_data_id_type;
   typedef db::object_id<implementation_ids, impl_account_balance_object_type, account_balance_object>                   account_balance_id_type;
   typedef db::object_id<implementation_ids, impl_account_statistics_object_type, account_statistics_object>             account_statistics_id_type;
   typedef db::object_id<implementation_ids, impl_transaction_object_type, transaction_object>                           transaction_obj_id_type;
   typedef db::object_id<implementation_ids, impl_block_summary_object_type, block_summary_object>                       block_summary_id_type;
   typedef db::object_id<implementation_ids, impl_account_transaction_history_object_type, account_transaction_history_object> account_transaction_history_id_type;
   typedef db::object_id<implementation_ids, impl_chain_property_object_type, chain_property_object>                     chain_property_id_type;
   typedef db::object_id<implementation_ids, impl_miner_schedule_object_type, miner_schedule_object>                     miner_schedule_id_type;
   typedef db::object_id<implementation_ids, impl_budget_record_object_type, budget_record_object>                       budget_record_id_type;
   typedef db::object_id<implementation_ids, impl_buying_object_type, buying_object>                                     buying_id_type;
   typedef db::object_id<implementation_ids, impl_content_object_type, content_object>                                   content_id_type;
   typedef db::object_id<implementation_ids, impl_publisher_object_type, seeder_object>                                  publisher_id_type;
   typedef db::object_id<implementation_ids, impl_subscription_object_type, subscription_object>                         subscription_id_type;
   typedef db::object_id<implementation_ids, impl_seeding_statistics_object_type, seeding_statistics_object>             seeding_statistics_id_type;
   typedef db::object_id<implementation_ids, impl_transaction_detail_object_type, transaction_detail_object>             transaction_detail_id_type;
   typedef db::object_id<implementation_ids, impl_messaging_object_type, message_object>                                 message_id_type;
   typedef db::object_id<implementation_ids, impl_transaction_history_object_type, transaction_history_object>           transaction_history_id_type;

   typedef fc::array<char, GRAPHENE_MAX_ASSET_SYMBOL_LENGTH>    symbol_type;
   typedef fc::sha256                                           chain_id_type;
   typedef fc::ripemd160                                        block_id_type;
   typedef fc::ripemd160                                        checksum_type;
   typedef fc::ripemd160                                        transaction_id_type;
   typedef fc::sha256                                           digest_type;
   typedef fc::ecc::compact_signature                           signature_type;
   typedef fc::safe<int64_t>                                    share_type;
   typedef uint16_t                                             weight_type;
   typedef fc::ecc::private_key                                 private_key_type;
   typedef fc::ecc::public_key                                  public_key_type;

   typedef decent::encrypt::CustodyData custody_data_type;
   typedef decent::encrypt::CustodyProof custody_proof_type;
   typedef decent::encrypt::DIntegerString bigint_type;
   typedef decent::encrypt::CiphertextString ciphertext_type;
   typedef decent::encrypt::DeliveryProofString delivery_proof_type;

   struct void_t {};

} }  // graphene::chain

FC_REFLECT_TYPENAME( graphene::chain::share_type )
FC_REFLECT_TYPENAME( graphene::chain::account_id_type )
FC_REFLECT_TYPENAME( graphene::chain::asset_id_type )
FC_REFLECT_TYPENAME( graphene::chain::miner_id_type )
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
FC_REFLECT_TYPENAME( graphene::chain::message_id_type )
FC_REFLECT_TYPENAME( graphene::chain::transaction_history_id_type )

FC_REFLECT_EMPTY( graphene::chain::void_t )
