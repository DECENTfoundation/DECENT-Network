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

#include <graphene/chain/database.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/block_summary_object.hpp>
#include <graphene/chain/budget_record_object.hpp>
#include <graphene/chain/buying_object.hpp>
#include <graphene/chain/chain_property_object.hpp>
#include <graphene/chain/content_object.hpp>
#include <graphene/chain/global_property_object.hpp>
#include <graphene/chain/operation_history_object.hpp>
#include <graphene/chain/proposal_object.hpp>
#include <graphene/chain/seeder_object.hpp>
#include <graphene/chain/transaction_object.hpp>
#include <graphene/chain/vesting_balance_object.hpp>
#include <graphene/chain/withdraw_permission_object.hpp>
#include <graphene/chain/miner_object.hpp>
#include <graphene/chain/miner_schedule_object.hpp>
#include <graphene/chain/transaction_detail_object.hpp>

#include <graphene/chain/account_evaluator.hpp>
#include <graphene/chain/asset_evaluator.hpp>
#include <graphene/chain/assert_evaluator.hpp>
#include <graphene/chain/custom_evaluator.hpp>
#include <graphene/chain/decent_evaluator.hpp>
#include <graphene/chain/proposal_evaluator.hpp>
#include <graphene/chain/transfer_evaluator.hpp>
#include <graphene/chain/vesting_balance_evaluator.hpp>
#include <graphene/chain/withdraw_permission_evaluator.hpp>
#include <graphene/chain/miner_evaluator.hpp>
#include <graphene/chain/subscription_evaluator.hpp>

#include <graphene/chain/protocol/fee_schedule.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/uint128.hpp>
#include <fc/crypto/digest.hpp>

#include <boost/algorithm/string.hpp>

namespace graphene { namespace chain {

// C++ requires that static class variables declared and initialized
// in headers must also have a definition in a single source file,
// else linker errors will occur [1].
//
// The purpose of this source file is to collect such definitions in
// a single place.
//
// [1] http://stackoverflow.com/questions/8016780/undefined-reference-to-static-constexpr-char

const uint8_t account_object::space_id;
const uint8_t account_object::type_id;

const uint8_t asset_object::space_id;
const uint8_t asset_object::type_id;

const uint8_t block_summary_object::space_id;
const uint8_t block_summary_object::type_id;

const uint8_t global_property_object::space_id;
const uint8_t global_property_object::type_id;

const uint8_t operation_history_object::space_id;
const uint8_t operation_history_object::type_id;

const uint8_t proposal_object::space_id;
const uint8_t proposal_object::type_id;

const uint8_t transaction_object::space_id;
const uint8_t transaction_object::type_id;

const uint8_t vesting_balance_object::space_id;
const uint8_t vesting_balance_object::type_id;

const uint8_t withdraw_permission_object::space_id;
const uint8_t withdraw_permission_object::type_id;

const uint8_t miner_object::space_id;
const uint8_t miner_object::type_id;


void database::initialize_evaluators()
{
   _operation_evaluators.resize(255);
   register_evaluator<account_create_evaluator>();
   register_evaluator<account_update_evaluator>();
   register_evaluator<custom_evaluator>();
   register_evaluator<asset_create_evaluator>();
   register_evaluator<asset_issue_evaluator>();
   register_evaluator<monitored_asset_update_evaluator>();
   register_evaluator<user_issued_asset_update_evaluator>();
   register_evaluator<asset_fund_pools_evaluator>();
   register_evaluator<asset_reserve_evaluator>();
   register_evaluator<asset_claim_fees_evaluator>();
   register_evaluator<assert_evaluator>();
   register_evaluator<transfer_evaluator>();
   register_evaluator<proposal_create_evaluator>();
   register_evaluator<proposal_update_evaluator>();
   register_evaluator<proposal_delete_evaluator>();
   register_evaluator<vesting_balance_create_evaluator>();
   register_evaluator<vesting_balance_withdraw_evaluator>();
   register_evaluator<miner_create_evaluator>();
   register_evaluator<miner_update_evaluator>();
   register_evaluator<miner_update_global_parameters_evaluator>();
   register_evaluator<withdraw_permission_create_evaluator>();
   register_evaluator<withdraw_permission_claim_evaluator>();
   register_evaluator<withdraw_permission_update_evaluator>();
   register_evaluator<withdraw_permission_delete_evaluator>();
   register_evaluator<content_submit_evaluator>();
   register_evaluator<content_cancellation_evaluator>();
   register_evaluator<request_to_buy_evaluator>();
   register_evaluator<leave_rating_evaluator>();
   register_evaluator<ready_to_publish_evaluator>();
   register_evaluator<deliver_keys_evaluator>();
   register_evaluator<proof_of_custody_evaluator>();
   register_evaluator<subscribe_evaluator>();
   register_evaluator<subscribe_by_author_evaluator>();
   register_evaluator<automatic_renewal_of_subscription_evaluator>();
   register_evaluator<disallow_automatic_renewal_of_subscription_evaluator>();
   register_evaluator<renewal_of_subscription_evaluator>();
   register_evaluator<asset_publish_feeds_evaluator>();
   register_evaluator<return_escrow_submission_evaluator>();
   register_evaluator<return_escrow_buying_evaluator>();
   register_evaluator<report_stats_evaluator>();
   register_evaluator<set_publishing_manager_evaluator>();
   register_evaluator<set_publishing_right_evaluator>();
}

void database::initialize_indexes()
{
   reset_indexes();
   _undo_db.set_max_size( GRAPHENE_MIN_UNDO_HISTORY );

   //Protocol object indexes
   add_index< primary_index<asset_index> >();

   auto acnt_index = add_index< primary_index<account_index> >();
   acnt_index->add_secondary_index<account_member_index>();

   add_index< primary_index<miner_index> >();

   auto prop_index = add_index< primary_index<proposal_index > >();
   prop_index->add_secondary_index<required_approval_index>();

   add_index< primary_index<withdraw_permission_index > >();
   add_index< primary_index<vesting_balance_index> >();

   //Implementation object indexes
   add_index< primary_index<transaction_index                             > >();
   add_index< primary_index<account_balance_index                         > >();
   add_index< primary_index<simple_index<global_property_object          >> >();
   add_index< primary_index<simple_index<dynamic_global_property_object  >> >();
   add_index< primary_index<simple_index<account_statistics_object       >> >();
   add_index< primary_index<simple_index<asset_dynamic_data_object       >> >();
   add_index< primary_index<flat_index<  block_summary_object            >> >();
   add_index< primary_index<simple_index<chain_property_object          > > >();
   add_index< primary_index<simple_index<miner_schedule_object        > > >();
   add_index< primary_index< seeder_index                                 > >();
   add_index< primary_index< content_index                                > >();
   add_index< primary_index< buying_index                                 > >();
   add_index< primary_index< subscription_index                                 > >();
   add_index< primary_index< transaction_detail_index                     > >();
   add_index< primary_index< seeding_statistics_index                     > >();
   add_index< primary_index< budget_record_index                          > >();
}

void database::init_genesis(const genesis_state_type& genesis_state)
{ try {
   FC_ASSERT( genesis_state.initial_timestamp != time_point_sec(), "Must initialize genesis timestamp." );
   FC_ASSERT( genesis_state.initial_timestamp.sec_since_epoch() % GRAPHENE_DEFAULT_BLOCK_INTERVAL == 0,
              "Genesis timestamp must be divisible by GRAPHENE_DEFAULT_BLOCK_INTERVAL." );
   FC_ASSERT(genesis_state.initial_miner_candidates.size() > 0,
             "Cannot start a chain with zero miners.");
   FC_ASSERT(genesis_state.initial_active_miners <= genesis_state.initial_miner_candidates.size(),
             "initial_active_miners is larger than the number of candidate miners.");

   _undo_db.disable();
   struct auth_inhibitor {
      auth_inhibitor(database& db) : db(db), old_flags(db.node_properties().skip_flags)
      { db.node_properties().skip_flags |= skip_authority_check; }
      ~auth_inhibitor()
      { db.node_properties().skip_flags = old_flags; }
   private:
      database& db;
      uint32_t old_flags;
   } inhibitor(*this);

   transaction_evaluation_state genesis_eval_state(this);

   flat_index<block_summary_object>& bsi = get_mutable_index_type< flat_index<block_summary_object> >();
   bsi.resize(0xffff+1);

   // Create blockchain accounts
   fc::ecc::private_key null_private_key = fc::ecc::private_key::regenerate(fc::sha256::hash(string("null_key")));
   create<account_balance_object>([](account_balance_object& b) {
      b.balance = GRAPHENE_INITIAL_SHARE_SUPPLY;
   });

   FC_ASSERT(create<account_object>([this](account_object& a) {
       a.name = "miner-account";
       a.statistics = create<account_statistics_object>([&](account_statistics_object& s){s.owner = a.id;}).id;
       a.owner.weight_threshold = 1;
       a.active.weight_threshold = 1;
       a.registrar = GRAPHENE_MINER_ACCOUNT;

   }).get_id() == GRAPHENE_MINER_ACCOUNT);

   FC_ASSERT(create<account_object>([this](account_object& a) {
       a.name = "null-account";
       a.statistics = create<account_statistics_object>([&](account_statistics_object& s){s.owner = a.id;}).id;
       a.owner.weight_threshold = 1;
       a.active.weight_threshold = 1;
       a.registrar = GRAPHENE_MINER_ACCOUNT;
   }).get_id() == GRAPHENE_NULL_ACCOUNT);

   FC_ASSERT(create<account_object>([this](account_object& a) {
       a.name = "temp-account";
       a.statistics = create<account_statistics_object>([&](account_statistics_object& s){s.owner = a.id;}).id;
       a.owner.weight_threshold = 0;
       a.active.weight_threshold = 0;
       a.registrar = GRAPHENE_MINER_ACCOUNT;
   }).get_id() == GRAPHENE_TEMP_ACCOUNT);

   FC_ASSERT(create<account_object>([this](account_object& a) {
       a.name = "proxy-to-self";
       a.statistics = create<account_statistics_object>([&](account_statistics_object& s){s.owner = a.id;}).id;
       a.owner.weight_threshold = 1;
       a.active.weight_threshold = 1;
       a.registrar = GRAPHENE_MINER_ACCOUNT;
   }).get_id() == GRAPHENE_PROXY_TO_SELF_ACCOUNT);

   // Create more special accounts
   while( true )
   {
      uint64_t id = get_index<account_object>().get_next_id().instance();
      if( id >= genesis_state.immutable_parameters.num_special_accounts )
         break;
      const account_object& acct = create<account_object>([&](account_object& a) {
          a.name = "special-account-" + std::to_string(id);
          a.statistics = create<account_statistics_object>([&](account_statistics_object& s){s.owner = a.id;}).id;
          a.owner.weight_threshold = 1;
          a.active.weight_threshold = 1;
          a.registrar = GRAPHENE_MINER_ACCOUNT;
      });
      FC_ASSERT( acct.get_id() == account_id_type(id) );
      remove( acct );
   }

   // Create core asset
   const asset_dynamic_data_object& dyn_asset =
      create<asset_dynamic_data_object>([&](asset_dynamic_data_object& a) {
         a.current_supply = GRAPHENE_INITIAL_SHARE_SUPPLY;
      });
   const asset_object& core_asset =
     create<asset_object>( [&]( asset_object& a ) {
         a.symbol = GRAPHENE_SYMBOL;
         a.precision = GRAPHENE_BLOCKCHAIN_PRECISION_DIGITS;
         a.issuer = GRAPHENE_NULL_ACCOUNT;
         a.options.max_supply = genesis_state.max_core_supply;
         a.options.core_exchange_rate.base.amount = 1;
         a.options.core_exchange_rate.base.asset_id = asset_id_type(0);
         a.options.core_exchange_rate.quote.amount = 1;
         a.options.core_exchange_rate.quote.asset_id = asset_id_type(0);
         a.dynamic_asset_data_id = dyn_asset.id;
      });
   assert( asset_id_type(core_asset.id) == asset().asset_id );
   assert( get_balance(account_id_type(), asset_id_type()) == asset(dyn_asset.current_supply) );
   // Create more special assets
   while( true )
   {
      uint64_t id = get_index<asset_object>().get_next_id().instance();
      if( id >= genesis_state.immutable_parameters.num_special_assets )
         break;
      const asset_dynamic_data_object& dyn_asset =
         create<asset_dynamic_data_object>([&](asset_dynamic_data_object& a) {
            a.current_supply = 0;
         });
      const asset_object& asset_obj = create<asset_object>( [&]( asset_object& a ) {
         a.symbol = "SPECIAL" + std::to_string( id );
         a.options.max_supply = 0;
         a.precision = GRAPHENE_BLOCKCHAIN_PRECISION_DIGITS;
         a.issuer = GRAPHENE_NULL_ACCOUNT;

         a.dynamic_asset_data_id = dyn_asset.id;
      });
      FC_ASSERT( asset_obj.get_id() == asset_id_type(id) );
      remove( asset_obj );
   }

   chain_id_type chain_id = genesis_state.compute_chain_id();
   idump((chain_id));
   // Create global properties
   create<global_property_object>([&](global_property_object& p) {
       p.parameters = genesis_state.initial_parameters;
       // Set fees to zero initially, so that genesis initialization needs not pay them
       // We'll fix it at the end of the function
       p.parameters.current_fees->zero_all_fees();

   });
   create<dynamic_global_property_object>([&](dynamic_global_property_object& p) {
      p.time = genesis_state.initial_timestamp;
      p.dynamic_flags = 0;
      p.miner_budget_from_fees = 0;
      p.miner_budget_from_rewards = 0;
      p.unspent_fee_budget = 0;
      p.mined_rewards = 0;
      p.recent_slots_filled = fc::uint128::max_value();
   });

   FC_ASSERT( (genesis_state.immutable_parameters.min_miner_count & 1) == 1, "min_miner_count must be odd" );

   create<chain_property_object>([&](chain_property_object& p)
   {
      p.chain_id = chain_id;
      p.immutable_parameters = genesis_state.immutable_parameters;
   } );
   create<block_summary_object>([&](block_summary_object&) {});

   // Create initial accounts
   for( const auto& account : genesis_state.initial_accounts )
   {
      idump((account));
      account_create_operation cop;
      cop.name = account.name;
      cop.registrar = GRAPHENE_TEMP_ACCOUNT;
      uint32_t owner_threshold = 1;
      if(account.owner_threshold)
         owner_threshold = *account.owner_threshold;
      if(account.owner_key2 && account.owner_key3 ){
         FC_ASSERT ( owner_threshold <= 3 );
         cop.owner = authority(owner_threshold, account.owner_key, 1, *account.owner_key2, 1, *account.owner_key3, 1 );
      }else if(account.owner_key2){
         FC_ASSERT ( owner_threshold <= 2 );
         cop.owner = authority(owner_threshold, account.owner_key, 1, *account.owner_key2, 1 );
      }else
         cop.owner = authority(1, account.owner_key, 1);
      if( account.active_key == public_key_type() )
      {
         cop.active = cop.owner;
         cop.options.memo_key = account.owner_key;
      }
      else
      {
         uint32_t active_threshold = 1;
         if(account.active_threshold)
            active_threshold = *account.active_threshold;
         if(account.active_key2 && account.active_key3 ){
            FC_ASSERT ( active_threshold <= 3 );
            cop.active = authority(active_threshold, account.active_key, 1, *account.active_key2, 1, *account.active_key3, 1 );
         }else if(account.active_key2){
            FC_ASSERT ( active_threshold <= 2 );
            cop.active = authority(active_threshold, account.active_key, 1, *account.active_key2, 1 );
         }else
            cop.active = authority(1, account.active_key, 1);
         cop.options.memo_key = account.active_key;
      }
      account_id_type account_id(apply_operation(genesis_eval_state, cop).get<object_id_type>());
   }


   // Helper function to get account ID by name
   const auto& accounts_by_name = get_index_type<account_index>().indices().get<by_name>();
   auto get_account_id = [&accounts_by_name](const string& name) {
      auto itr = accounts_by_name.find(name);
      FC_ASSERT(itr != accounts_by_name.end(),
                "Unable to find account '${acct}'. Did you forget to add a record for it to initial_accounts?",
                ("acct", name));
      return itr->get_id();
   };

   // Helper function to get asset ID by symbol
   const auto& assets_by_symbol = get_index_type<asset_index>().indices().get<by_symbol>();
   const auto get_asset_id = [&assets_by_symbol](const string& symbol) {
      auto itr = assets_by_symbol.find(symbol);

      // TODO: This is temporary for handling BTS snapshot
      if( symbol == "DCT" )
          itr = assets_by_symbol.find(GRAPHENE_SYMBOL);

      FC_ASSERT(itr != assets_by_symbol.end(),
                "Unable to find asset '${sym}'. Did you forget to add a record for it to initial_assets?",
                ("sym", symbol));
      return itr->get_id();
   };

      //create initial balance
   for( const auto& balance: genesis_state.initial_balances )
   {
      transfer_operation top;
      top.from = GRAPHENE_MINER_ACCOUNT;
      top.to = get_account_id( balance.owner );
      asset amount( balance.amount, get_asset_id( balance.asset_symbol ) );
      top.amount = amount;
      ilog("creating balance");
      idump((top));
      idump((balance));
      apply_operation(genesis_eval_state, top);
   }

   map<asset_id_type, share_type> total_supplies;
   map<asset_id_type, share_type> total_debts;

   // Create initial assets. Only monitored assets are supported in decent genesis
   for( const genesis_state_type::initial_asset_type& asset : genesis_state.initial_assets )
   {

      asset_id_type new_asset_id = get_index_type<asset_index>().get_next_id();
      total_supplies[ new_asset_id ] = 0;

      asset_dynamic_data_id_type dynamic_data_id;

      dynamic_data_id = create<asset_dynamic_data_object>([&](asset_dynamic_data_object& d) {
         d.asset_pool = 0;
      }).id;

      create<asset_object>([&](asset_object& a) {
         monitored_asset_options mao;
         a.monitored_asset_opts = mao;
         a.symbol = asset.symbol;
         a.description = asset.description;
         a.precision = asset.precision;
         string issuer_name = asset.issuer_name;
         a.issuer = get_account_id(issuer_name);
         a.options.max_supply = 0;
         a.dynamic_asset_data_id = dynamic_data_id;
      });
   }

   if( total_supplies[ asset_id_type(0) ] > 0 )
   {
       adjust_balance(GRAPHENE_MINER_ACCOUNT, -get_balance(GRAPHENE_MINER_ACCOUNT,{}));
   }
   else
   {
       total_supplies[ asset_id_type(0) ] = GRAPHENE_INITIAL_SHARE_SUPPLY;
   }

   // Save tallied supplies
   for( const auto& item : total_supplies )
   {
       const auto asset_id = item.first;
       const auto total_supply = item.second;

       modify( get( asset_id ), [ & ]( asset_object& asset ) {
           modify( get( asset.dynamic_asset_data_id ), [ & ]( asset_dynamic_data_object& asset_data ) {
               asset_data.current_supply = total_supply;
           } );
       } );
   }

   // Create special miner account
   const miner_object& wit = create<miner_object>([&](miner_object& w) {});
   FC_ASSERT( wit.id == GRAPHENE_NULL_MINER );
   remove(wit);

   // Create initial miners
   std::for_each(genesis_state.initial_miner_candidates.begin(), genesis_state.initial_miner_candidates.end(),
                 [&](const genesis_state_type::initial_miner_type& miner) {
      miner_create_operation op;
      op.miner_account = get_account_id(miner.owner_name);
      op.block_signing_key = miner.block_signing_key;
      apply_operation(genesis_eval_state, op);
   });

   // Set active miners
   modify(get_global_properties(), [&](global_property_object& p) {
      for( uint32_t i = 1; i <= genesis_state.initial_active_miners; ++i )
      {
         p.active_miners.emplace_back(miner_id_type(i));
      }
   });

   // Enable fees
   modify(get_global_properties(), [&genesis_state](global_property_object& p) {
      p.parameters.current_fees = genesis_state.initial_parameters.current_fees;
   });

   // Create miner scheduler
   create<miner_schedule_object>([&]( miner_schedule_object& wso )
   {
      for( const miner_id_type& wid : get_global_properties().active_miners )
         wso.current_shuffled_miners.push_back( wid );
   });


   debug_dump();

   _undo_db.enable();
} FC_CAPTURE_AND_RETHROW() }

} }
