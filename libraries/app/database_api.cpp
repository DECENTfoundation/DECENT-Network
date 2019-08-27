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

#include <cctype>
#include <boost/algorithm/string.hpp>
#include <fc/bloom_filter.hpp>
#include <fc/crypto/hex.hpp>
#include <fc/thread/thread.hpp>
#include <graphene/app/database_api.hpp>
#include <graphene/chain/get_config.hpp>
#include <graphene/chain/seeding_statistics_object.hpp>
#include <graphene/chain/transaction_history_object.hpp>
#include <decent/encrypt/encryptionutils.hpp>
#include <graphene/app/database_api.hpp>
#include <graphene/app/exceptions.hpp>

#define GET_REQUIRED_FEES_MAX_RECURSION 4

namespace {
      CryptoPP::AutoSeededRandomPool randomGenerator;
}

namespace {

   template <bool is_ascending>
   struct return_one {

      template <class T1, class T2>
      static auto choose(const T1& t1, const T2& t2) -> typename std::conditional<is_ascending, T1, T2 >::type {
         return t1;
      }
   };

   template <>
   struct return_one<false> {

      template <class T1, class T2>
      static auto choose(const T1& t1, const T2& t2) -> typename std::conditional<false, T1, T2 >::type {
         return t2;
      }
   };
}

namespace graphene { namespace app {

   class database_api_impl;
   const int CURRENT_OUTPUT_LIMIT_1000 = 1000;
   const int CURRENT_OUTPUT_LIMIT_100 = 100;

   class database_api_impl : public std::enable_shared_from_this<database_api_impl>
   {
   public:
      database_api_impl( graphene::chain::database& db );
      ~database_api_impl();

      // Objects
      fc::variants get_objects(const vector<object_id_type>& ids)const;

      // Subscriptions
      void set_subscribe_callback( std::function<void(const variant&)> cb, bool clear_filter );
      void set_content_update_callback( const string & URI, std::function<void()> cb );
      void set_pending_transaction_callback( std::function<void(const variant&)> cb );
      void set_block_applied_callback( std::function<void(const variant& block_id)> cb );
      void cancel_all_subscriptions();

      // Blocks and transactions
      optional<block_header> get_block_header(uint32_t block_num)const;
      vector<optional<block_header>> get_block_headers(uint32_t block_num, uint32_t count)const;
      optional<signed_block> get_block(uint32_t block_num)const;
      vector<optional<signed_block>> get_blocks(uint32_t block_num, uint32_t count)const;
      processed_transaction get_transaction( uint32_t block_num, uint32_t trx_in_block )const;
      fc::time_point_sec head_block_time()const;
      miner_reward_input get_time_to_maint_by_block_time(fc::time_point_sec block_time) const;
      share_type get_miner_pay_from_fees_by_block_time(fc::time_point_sec block_time) const;
      optional<processed_transaction> get_transaction_by_id(const transaction_id_type& id) const;
      vector<proposal_object> get_proposed_transactions( account_id_type id )const;

      // Globals
      chain_property_object get_chain_properties()const;
      global_property_object get_global_properties()const;
      chain_id_type get_chain_id()const;
      dynamic_global_property_object get_dynamic_global_properties()const;
      vector<operation_info> list_operations()const;

      // Keys
      vector<vector<account_id_type>> get_key_references( vector<public_key_type> key )const;

      // Accounts
      vector<optional<account_object>> get_accounts(const vector<account_id_type>& account_ids)const;
      std::map<string,full_account> get_full_accounts( const vector<string>& names_or_ids, bool subscribe );
      optional<account_object> get_account_by_name( string name )const;
      vector<account_id_type> get_account_references( account_id_type account_id )const;
      vector<optional<account_object>> lookup_account_names(const vector<string>& account_names)const;
      map<string,account_id_type> lookup_accounts(const string& lower_bound_name, uint32_t limit)const;
      vector<account_object> search_accounts(const string& search_term, const string order, const object_id_type& id, uint32_t limit)const;
      vector<optional<account_statistics_object>> get_account_statistics(const vector<account_statistics_id_type>& account_statistics_ids)const;
      vector<transaction_detail_object> search_account_history(account_id_type const& account,
                                                               string const& order,
                                                               object_id_type const& id,
                                                               int limit) const;
      uint64_t get_account_count()const;

      // Balances
      vector<asset> get_account_balances(account_id_type id, const flat_set<asset_id_type>& assets)const;
      vector<asset> get_named_account_balances(const std::string& name, const flat_set<asset_id_type>& assets)const;
      vector<vesting_balance_object> get_vesting_balances( account_id_type account_id )const;
      map<non_fungible_token_id_type,uint32_t> get_non_fungible_token_summary(account_id_type account_id)const;
      vector<non_fungible_token_data_object> get_non_fungible_token_balances(account_id_type account_id, const set<non_fungible_token_id_type>& ids)const;

      // Assets
      uint64_t get_asset_count()const;
      vector<optional<asset_object>> get_assets(const vector<asset_id_type>& asset_ids)const;
      vector<asset_object>           list_assets(const string& lower_bound_symbol, uint32_t limit)const;
      vector<optional<asset_object>> lookup_asset_symbols(const vector<string>& symbols_or_ids)const;
      share_type get_new_asset_per_block() const;
      share_type get_asset_per_block_by_block_num(uint32_t block_num)const;
      vector<optional<asset_dynamic_data_object>> get_asset_dynamic_data(const vector<asset_dynamic_data_id_type>& asset_dynamic_data_ids)const;
      asset price_to_dct( asset price )const;

      // Non Fungible Tokens
      uint64_t get_non_fungible_token_count()const;
      vector<optional<non_fungible_token_object>> get_non_fungible_tokens(const vector<non_fungible_token_id_type>& nft_ids)const;
      vector<non_fungible_token_object> list_non_fungible_tokens(const string& lower_bound_symbol, uint32_t limit)const;
      vector<optional<non_fungible_token_object>> get_non_fungible_tokens_by_symbols(const vector<string>& symbols)const;
      uint64_t get_non_fungible_token_data_count()const;
      vector<optional<non_fungible_token_data_object>> get_non_fungible_token_data(const vector<non_fungible_token_data_id_type>& nft_data_ids)const;
      vector<non_fungible_token_data_object> list_non_fungible_token_data(non_fungible_token_id_type nft_id)const;
      void_t burn_non_fungible_token_data(non_fungible_token_data_id_type nft_data_id)const;
      vector<transaction_detail_object> search_non_fungible_token_history(non_fungible_token_data_id_type nft_data_id)const;

      // Miners
      vector<optional<miner_object>> get_miners(const vector<miner_id_type>& miner_ids)const;
      fc::optional<miner_object> get_miner_by_account(account_id_type account)const;
      map<string, miner_id_type> lookup_miner_accounts(const string& lower_bound_name, uint32_t limit)const;
      uint64_t get_miner_count()const;
      multimap< time_point_sec, price_feed> get_feeds_by_miner( const account_id_type account_id, uint32_t count)const;

      // Votes
      vector<optional<miner_object>> lookup_vote_ids( const vector<vote_id_type>& votes )const;
      vector<miner_voting_info> search_miner_voting(const string& account_id,
                                                    const string& term,
                                                    bool only_my_votes,
                                                    const string& order,
                                                    const string& id,
                                                    uint32_t count ) const;

      // Authority / validation
      std::string get_transaction_hex(const signed_transaction& trx)const;
      set<public_key_type> get_required_signatures( const signed_transaction& trx, const flat_set<public_key_type>& available_keys )const;
      set<public_key_type> get_potential_signatures( const signed_transaction& trx )const;
      bool verify_authority( const signed_transaction& trx )const;
      bool verify_account_authority( const string& name_or_id, const flat_set<public_key_type>& signers )const;
      processed_transaction validate_transaction( const signed_transaction& trx )const;
      fc::variants get_required_fees( vector<operation> ops, asset_id_type id )const;

      // Content
      vector<account_id_type> list_publishing_managers( const string& lower_bound_name, uint32_t limit )const;
      vector<buying_object> get_open_buyings()const;
      vector<buying_object> get_open_buyings_by_URI(const string& URI)const;
      vector<buying_object> get_open_buyings_by_consumer(const account_id_type& consumer)const;
      optional<buying_object> get_buying_by_consumer_URI( const account_id_type& consumer, const string& URI) const;
      vector<buying_object> get_buying_history_objects_by_consumer( const account_id_type& consumer )const;
      vector<buying_object> get_buying_objects_by_consumer( const account_id_type& consumer, const string& order, const object_id_type& id, const string& term, uint32_t count)const;
      vector<buying_object> search_feedback(const string& user, const string& URI, const object_id_type& id, uint32_t count) const;
      optional<content_object> get_content( const string& URI )const;
      vector<content_summary> search_content(const string& term,
                                             const string& order,
                                             const string& user,
                                             const string& region_code,
                                             const object_id_type& id,
                                             const string& type,
                                             uint32_t count)const;
      vector<seeder_object> list_seeders_by_price( const uint32_t count )const;
      optional<seeder_object> get_seeder( const account_id_type& account )const;
      vector<seeder_object> list_seeders_by_upload( const uint32_t count )const;
      vector<seeder_object> list_seeders_by_region( const string region_code )const;
      vector<seeder_object> list_seeders_by_rating( const uint32_t count )const;
      vector<subscription_object> list_active_subscriptions_by_consumer( const account_id_type& account, const uint32_t count )const;
      vector<subscription_object> list_subscriptions_by_consumer( const account_id_type& account, const uint32_t count )const;
      vector<subscription_object> list_active_subscriptions_by_author( const account_id_type& account, const uint32_t count )const;
      vector<subscription_object> list_subscriptions_by_author( const account_id_type& account, const uint32_t count )const;
      optional<subscription_object> get_subscription( const subscription_id_type& sid) const;

      //private:
      template<typename T>
      void subscribe_to_item( const T& i )const
      {
         auto vec = fc::raw::pack(i);
         if( !_subscribe_callback )
            return;

         if( !is_subscribed_to_item(i) )
         {
            idump((i));
            _subscribe_filter.insert( vec.data(), vec.size() );//(vecconst char*)&i, sizeof(i) );
         }
      }

      template<typename T>
      bool is_subscribed_to_item( const T& i )const
      {
         if( !_subscribe_callback )
            return false;
         return true;
      }

      /** called every time a block is applied to report the objects that were changed */
      void on_objects_changed(const vector<object_id_type>& ids);
      void on_applied_block();

      mutable fc::bloom_filter                               _subscribe_filter;
      std::function<void(const fc::variant&)> _subscribe_callback;
      std::function<void(const fc::variant&)> _pending_trx_callback;
      std::function<void(const fc::variant&)> _block_applied_callback;

      boost::signals2::scoped_connection                                                                                           _change_connection;
      boost::signals2::scoped_connection                                                                                           _applied_block_connection;
      boost::signals2::scoped_connection                                                                                           _pending_trx_connection;
      map< string, std::function<void()> >                              _content_subscriptions;
      graphene::chain::database&                                                                                                   _db;
   };

   //////////////////////////////////////////////////////////////////////
   //                                                                  //
   // Constructors                                                     //
   //                                                                  //
   //////////////////////////////////////////////////////////////////////

   database_api::database_api( graphene::chain::database& db )
   : my( new database_api_impl( db ) ) {}

   database_api::~database_api() {}

   database_api_impl::database_api_impl( graphene::chain::database& db ):_db(db)
   {
      dlog("creating database api ${x}", ("x",int64_t(this)) );
      _change_connection = _db.changed_objects.connect([this](const vector<object_id_type>& ids) {
         on_objects_changed(ids);
      });
      _applied_block_connection = _db.applied_block.connect([this](const signed_block&){ on_applied_block(); });

      _pending_trx_connection = _db.on_pending_transaction.connect([this](const signed_transaction& trx ){
         if( _pending_trx_callback ) _pending_trx_callback( fc::variant(trx) );
      });
   }

   database_api_impl::~database_api_impl()
   {
      dlog("freeing database api ${x}", ("x",int64_t(this)) );
   }

   //////////////////////////////////////////////////////////////////////
   //                                                                  //
   // Objects                                                          //
   //                                                                  //
   //////////////////////////////////////////////////////////////////////

   fc::variants database_api::get_objects(const vector<object_id_type>& ids)const
   {
      return my->get_objects( ids );
   }

   fc::variants database_api_impl::get_objects(const vector<object_id_type>& ids)const
   {
      if( _subscribe_callback )
      {
         for( auto id : ids )
         {
            if( (id.type() != operation_history_object_type || id.space() != protocol_ids) && (id.type() != impl_account_transaction_history_object_type || id.space() != implementation_ids) )
            {
               this->subscribe_to_item( id );
            }
         }
      }
      else
      {
         elog( "getObjects without subscribe callback??" );
      }

      fc::variants result;
      result.reserve(ids.size());

      std::transform(ids.begin(), ids.end(), std::back_inserter(result),
                     [this](object_id_type id) -> fc::variant {
                        if(auto obj = _db.find_object(id))
                           return obj->to_variant();
                        return {};
                     });

      return result;
   }

   //////////////////////////////////////////////////////////////////////
   //                                                                  //
   // Subscriptions                                                    //
   //                                                                  //
   //////////////////////////////////////////////////////////////////////

   void database_api::set_subscribe_callback( std::function<void(const variant&)> cb, bool clear_filter )
   {
      my->set_subscribe_callback( cb, clear_filter );
   }

   void database_api_impl::set_subscribe_callback( std::function<void(const variant&)> cb, bool clear_filter )
   {
      ddump((clear_filter));
      _subscribe_callback = cb;
      if( clear_filter || !cb )
      {
         static fc::bloom_parameters param;
         param.projected_element_count    = 10000;
         param.false_positive_probability = 1.0/10000;
         param.maximum_size = 1024*8*8*2;
         param.compute_optimal_parameters();
         _subscribe_filter = fc::bloom_filter(param);
      }
   }


   void database_api::set_content_update_callback( std::function<void()> cb, const string & URI )
   {
      my->set_content_update_callback(URI, cb);
   }

   void database_api_impl::set_content_update_callback( const string & URI, std::function<void()> cb )
   {
      _content_subscriptions[ URI ] = cb;
   }

   void database_api::set_pending_transaction_callback( std::function<void(const variant&)> cb )
   {
      my->set_pending_transaction_callback( cb );
   }

   void database_api_impl::set_pending_transaction_callback( std::function<void(const variant&)> cb )
   {
      _pending_trx_callback = cb;
   }

   void database_api::set_block_applied_callback( std::function<void(const variant& block_id)> cb )
   {
      my->set_block_applied_callback( cb );
   }

   void database_api_impl::set_block_applied_callback( std::function<void(const variant& block_id)> cb )
   {
      _block_applied_callback = cb;
   }

   void database_api::cancel_all_subscriptions()
   {
      my->cancel_all_subscriptions();
   }

   void database_api_impl::cancel_all_subscriptions()
   {
      set_subscribe_callback( std::function<void(const fc::variant&)>(), true);
   }

   //////////////////////////////////////////////////////////////////////
   //                                                                  //
   // Blocks and transactions                                          //
   //                                                                  //
   //////////////////////////////////////////////////////////////////////

   static signed_block_with_info signed_block_with_info_from_block(const signed_block& block, share_type miner_reward)
   {
      signed_block_with_info result;
      reinterpret_cast<signed_block&>(result) = block;
      result.block_id = block.id();
      result.signing_key = block.signee();
      result.transaction_ids.reserve( block.transactions.size() );
      for( const processed_transaction& tx : block.transactions )
         result.transaction_ids.push_back( tx.id() );

      result.miner_reward = miner_reward;
      return result;
   }

   optional<block_header> database_api::get_block_header(uint32_t block_num)const
   {
      return my->get_block_header(block_num);
   }

   optional<block_header> database_api_impl::get_block_header(uint32_t block_num) const
   {
      auto result = _db.fetch_block_by_number(block_num);
      if(result)
         return *result;
      return {};
   }

   vector<optional<block_header>> database_api::get_block_headers(uint32_t block_num, uint32_t count)const
   {
      return my->get_block_headers(block_num, count);
   }

   vector<optional<block_header>> database_api_impl::get_block_headers(uint32_t block_num, uint32_t count)const
   {
      vector<optional<block_header>> headers;
      headers.reserve(count);
      uint32_t end_block = block_num + count;
      while(block_num < end_block)
         headers.push_back(get_block_header(block_num++));
      return headers;
   }

   optional<signed_block_with_info> database_api::get_block(uint32_t block_num)const
   {
      auto block = my->get_block(block_num);
      if( !block )
         return {};

      share_type miner_pay_from_fees = get_miner_pay_from_fees_by_block_time(block->timestamp);
      share_type miner_pay_from_reward = get_asset_per_block_by_block_num(block_num);

      //this should never happen, but better check.
      if (miner_pay_from_fees < share_type(0))
         miner_pay_from_fees = share_type(0);

      return signed_block_with_info_from_block(*block, miner_pay_from_fees + miner_pay_from_reward);
   }

   optional<signed_block> database_api_impl::get_block(uint32_t block_num)const
   {
      return _db.fetch_block_by_number(block_num);
   }

   vector<optional<signed_block_with_info>> database_api::get_blocks(uint32_t block_num, uint32_t count)const
   {
      auto blocks = my->get_blocks(block_num, count);
      vector<optional<signed_block_with_info>> result;
      result.reserve(blocks.size());
      for( const auto& block : blocks )
      {
         if( block )
         {
            share_type miner_pay_from_fees = get_miner_pay_from_fees_by_block_time(block->timestamp);
            share_type miner_pay_from_reward = get_asset_per_block_by_block_num(block->block_num());

            //this should never happen, but better check.
            if (miner_pay_from_fees < share_type(0))
               miner_pay_from_fees = share_type(0);

            result.emplace_back(signed_block_with_info_from_block(*block, miner_pay_from_fees + miner_pay_from_reward));
         }
         else
         {
            result.push_back({});
         }
      }

      return result;
   }

   vector<optional<signed_block>> database_api_impl::get_blocks(uint32_t block_num, uint32_t count)const
   {
      vector<optional<signed_block>> blocks;
      blocks.reserve(count);
      uint32_t end_block = block_num + count;
      while(block_num < end_block)
         blocks.push_back(get_block(block_num++));
      return blocks;
   }

   processed_transaction database_api::get_transaction( uint32_t block_num, uint32_t trx_in_block )const
   {
      return my->get_transaction( block_num, trx_in_block );
   }

   fc::time_point_sec database_api::head_block_time() const
   {
      return my->head_block_time();
   }

   optional<signed_transaction> database_api::get_recent_transaction_by_id( const transaction_id_type& id )const
   {
      try {
         return my->_db.get_recent_transaction( id );
      } catch ( ... ) {
         return optional<signed_transaction>();
      }
   }

   processed_transaction database_api_impl::get_transaction(uint32_t block_num, uint32_t trx_num)const
   {
      auto opt_block = _db.fetch_block_by_number(block_num);
      if(!opt_block)
         FC_THROW_EXCEPTION(block_not_found_exception, "Block number: ${bn}", ("bn", block_num));
      if(opt_block->transactions.size() <= trx_num)
         FC_THROW_EXCEPTION(block_does_not_contain_requested_trx_exception, "Block number: ${bn} transaction index: ${ti}", ("bn", block_num)("ti", trx_num));
      return opt_block->transactions[trx_num];
   }

   fc::time_point_sec database_api_impl::head_block_time() const
   {
      return _db.head_block_time();
   }

   optional<processed_transaction> database_api::get_transaction_by_id( const transaction_id_type& id )const
   {
      return my->get_transaction_by_id( id );
   }

   optional<processed_transaction> database_api_impl::get_transaction_by_id( const transaction_id_type& id )const
   {
      const auto& idx = _db.get_index_type<transaction_history_index>().indices().get<by_tx_id>();
      auto itr = idx.find(id);
      if (itr != idx.end())
      {
         return get_transaction( itr->block_num, itr->trx_in_block );
      }

      return optional<processed_transaction>();
   }

   transaction_id_type database_api::get_transaction_id( const signed_transaction& trx )const
   {
      return trx.id();
   }

   //////////////////////////////////////////////////////////////////////
   //                                                                  //
   // Globals                                                          //
   //                                                                  //
   //////////////////////////////////////////////////////////////////////

   std::string database_api::info()const
   {
      return get_api_name();
   }

   chain_property_object database_api::get_chain_properties()const
   {
      return my->get_chain_properties();
   }

   chain_property_object database_api_impl::get_chain_properties()const
   {
      return _db.get(chain_property_id_type());
   }

   global_property_object database_api::get_global_properties()const
   {
      return my->get_global_properties();
   }

   global_property_object database_api_impl::get_global_properties()const
   {
      return _db.get(global_property_id_type());
   }

   fc::variant_object database_api::get_config()const
   {
      fc::mutable_variant_object result;
      result[ "GRAPHENE_SYMBOL" ] = GRAPHENE_SYMBOL;
      result[ "GRAPHENE_ADDRESS_PREFIX" ] = GRAPHENE_ADDRESS_PREFIX;
      result[ "GRAPHENE_MIN_ACCOUNT_NAME_LENGTH" ] = GRAPHENE_MIN_ACCOUNT_NAME_LENGTH;
      result[ "GRAPHENE_MAX_ACCOUNT_NAME_LENGTH" ] = GRAPHENE_MAX_ACCOUNT_NAME_LENGTH;
      result[ "GRAPHENE_MIN_ASSET_SYMBOL_LENGTH" ] = GRAPHENE_MIN_ASSET_SYMBOL_LENGTH;
      result[ "GRAPHENE_MAX_ASSET_SYMBOL_LENGTH" ] = GRAPHENE_MAX_ASSET_SYMBOL_LENGTH;
      result[ "GRAPHENE_MAX_SHARE_SUPPLY" ] = GRAPHENE_MAX_SHARE_SUPPLY;
      result[ "GRAPHENE_MAX_PAY_RATE" ] = GRAPHENE_MAX_PAY_RATE;
      result[ "GRAPHENE_MAX_SIG_CHECK_DEPTH" ] = GRAPHENE_MAX_SIG_CHECK_DEPTH;
      result[ "GRAPHENE_MIN_TRANSACTION_SIZE_LIMIT" ] = GRAPHENE_MIN_TRANSACTION_SIZE_LIMIT;
      result[ "GRAPHENE_MIN_BLOCK_INTERVAL" ] = GRAPHENE_MIN_BLOCK_INTERVAL;
      result[ "GRAPHENE_MAX_BLOCK_INTERVAL" ] = GRAPHENE_MAX_BLOCK_INTERVAL;
      result[ "GRAPHENE_DEFAULT_BLOCK_INTERVAL" ] = GRAPHENE_DEFAULT_BLOCK_INTERVAL;
      result[ "GRAPHENE_DEFAULT_MAX_TRANSACTION_SIZE" ] = GRAPHENE_DEFAULT_MAX_TRANSACTION_SIZE;
      result[ "GRAPHENE_DEFAULT_MAX_BLOCK_SIZE" ] = GRAPHENE_DEFAULT_MAX_BLOCK_SIZE;
      result[ "GRAPHENE_DEFAULT_MAX_TIME_UNTIL_EXPIRATION" ] = GRAPHENE_DEFAULT_MAX_TIME_UNTIL_EXPIRATION;
      result[ "GRAPHENE_DEFAULT_MAINTENANCE_INTERVAL" ] = GRAPHENE_DEFAULT_MAINTENANCE_INTERVAL;
      result[ "GRAPHENE_DEFAULT_MAINTENANCE_SKIP_SLOTS" ] = GRAPHENE_DEFAULT_MAINTENANCE_SKIP_SLOTS;
      result[ "GRAPHENE_MIN_UNDO_HISTORY" ] = GRAPHENE_MIN_UNDO_HISTORY;
      result[ "GRAPHENE_MAX_UNDO_HISTORY" ] = GRAPHENE_MAX_UNDO_HISTORY;
      result[ "GRAPHENE_MIN_BLOCK_SIZE_LIMIT" ] = GRAPHENE_MIN_BLOCK_SIZE_LIMIT;
      result[ "GRAPHENE_MIN_TRANSACTION_EXPIRATION_LIMIT" ] = GRAPHENE_MIN_TRANSACTION_EXPIRATION_LIMIT;
      result[ "GRAPHENE_BLOCKCHAIN_PRECISION" ] = GRAPHENE_BLOCKCHAIN_PRECISION;
      result[ "GRAPHENE_BLOCKCHAIN_PRECISION_DIGITS" ] = GRAPHENE_BLOCKCHAIN_PRECISION_DIGITS;
      result[ "GRAPHENE_DEFAULT_TRANSFER_FEE" ] = GRAPHENE_DEFAULT_TRANSFER_FEE;
      result[ "GRAPHENE_MAX_INSTANCE_ID" ] = GRAPHENE_MAX_INSTANCE_ID;
      result[ "GRAPHENE_100_PERCENT" ] = GRAPHENE_100_PERCENT;
      result[ "GRAPHENE_1_PERCENT" ] = GRAPHENE_1_PERCENT;
      result[ "GRAPHENE_MAX_MARKET_FEE_PERCENT" ] = GRAPHENE_MAX_MARKET_FEE_PERCENT;
      result[ "GRAPHENE_DEFAULT_FORCE_SETTLEMENT_DELAY" ] = GRAPHENE_DEFAULT_FORCE_SETTLEMENT_DELAY;
      result[ "GRAPHENE_DEFAULT_FORCE_SETTLEMENT_OFFSET" ] = GRAPHENE_DEFAULT_FORCE_SETTLEMENT_OFFSET;
      result[ "GRAPHENE_DEFAULT_FORCE_SETTLEMENT_MAX_VOLUME" ] = GRAPHENE_DEFAULT_FORCE_SETTLEMENT_MAX_VOLUME;
      result[ "GRAPHENE_DEFAULT_PRICE_FEED_LIFETIME" ] = GRAPHENE_DEFAULT_PRICE_FEED_LIFETIME;
      result[ "GRAPHENE_MAX_FEED_PRODUCERS" ] = GRAPHENE_MAX_FEED_PRODUCERS;
      result[ "GRAPHENE_DEFAULT_MAX_AUTHORITY_MEMBERSHIP" ] = GRAPHENE_DEFAULT_MAX_AUTHORITY_MEMBERSHIP;
      result[ "GRAPHENE_DEFAULT_MAX_ASSET_WHITELIST_AUTHORITIES" ] = GRAPHENE_DEFAULT_MAX_ASSET_WHITELIST_AUTHORITIES;
      result[ "GRAPHENE_DEFAULT_MAX_ASSET_FEED_PUBLISHERS" ] = GRAPHENE_DEFAULT_MAX_ASSET_FEED_PUBLISHERS;
      result[ "GRAPHENE_COLLATERAL_RATIO_DENOM" ] = GRAPHENE_COLLATERAL_RATIO_DENOM;
      result[ "GRAPHENE_MIN_COLLATERAL_RATIO" ] = GRAPHENE_MIN_COLLATERAL_RATIO;
      result[ "GRAPHENE_MAX_COLLATERAL_RATIO" ] = GRAPHENE_MAX_COLLATERAL_RATIO;
      result[ "GRAPHENE_DEFAULT_MAINTENANCE_COLLATERAL_RATIO" ] = GRAPHENE_DEFAULT_MAINTENANCE_COLLATERAL_RATIO;
      result[ "GRAPHENE_DEFAULT_MAX_SHORT_SQUEEZE_RATIO" ] = GRAPHENE_DEFAULT_MAX_SHORT_SQUEEZE_RATIO;
      result[ "GRAPHENE_DEFAULT_MARGIN_PERIOD_SEC" ] = GRAPHENE_DEFAULT_MARGIN_PERIOD_SEC;
      result[ "GRAPHENE_DEFAULT_MAX_MINERS" ] = GRAPHENE_DEFAULT_MAX_MINERS;
      result[ "GRAPHENE_DEFAULT_MAX_PROPOSAL_LIFETIME_SEC" ] = GRAPHENE_DEFAULT_MAX_PROPOSAL_LIFETIME_SEC;
      result[ "GRAPHENE_DEFAULT_MINER_PROPOSAL_REVIEW_PERIOD_SEC" ] = GRAPHENE_DEFAULT_MINER_PROPOSAL_REVIEW_PERIOD_SEC;
      result[ "GRAPHENE_DEFAULT_NETWORK_PERCENT_OF_FEE" ] = GRAPHENE_DEFAULT_NETWORK_PERCENT_OF_FEE;
      result[ "GRAPHENE_DEFAULT_LIFETIME_REFERRER_PERCENT_OF_FEE" ] = GRAPHENE_DEFAULT_LIFETIME_REFERRER_PERCENT_OF_FEE;
      result[ "GRAPHENE_DEFAULT_MAX_BULK_DISCOUNT_PERCENT" ] = GRAPHENE_DEFAULT_MAX_BULK_DISCOUNT_PERCENT;
      result[ "GRAPHENE_DEFAULT_BULK_DISCOUNT_THRESHOLD_MIN" ] = GRAPHENE_DEFAULT_BULK_DISCOUNT_THRESHOLD_MIN;
      result[ "GRAPHENE_DEFAULT_BULK_DISCOUNT_THRESHOLD_MAX" ] = GRAPHENE_DEFAULT_BULK_DISCOUNT_THRESHOLD_MAX;
      result[ "GRAPHENE_DEFAULT_CASHBACK_VESTING_PERIOD_SEC" ] = GRAPHENE_DEFAULT_CASHBACK_VESTING_PERIOD_SEC;
      result[ "GRAPHENE_DEFAULT_CASHBACK_VESTING_THRESHOLD" ] = GRAPHENE_DEFAULT_CASHBACK_VESTING_THRESHOLD;
      result[ "GRAPHENE_DEFAULT_BURN_PERCENT_OF_FEE" ] = GRAPHENE_DEFAULT_BURN_PERCENT_OF_FEE;
      result[ "GRAPHENE_MINER_PAY_PERCENT_PRECISION" ] = GRAPHENE_MINER_PAY_PERCENT_PRECISION;
      result[ "GRAPHENE_DEFAULT_MAX_ASSERT_OPCODE" ] = GRAPHENE_DEFAULT_MAX_ASSERT_OPCODE;
      result[ "GRAPHENE_DEFAULT_FEE_LIQUIDATION_THRESHOLD" ] = GRAPHENE_DEFAULT_FEE_LIQUIDATION_THRESHOLD;
      result[ "GRAPHENE_DEFAULT_ACCOUNTS_PER_FEE_SCALE" ] = GRAPHENE_DEFAULT_ACCOUNTS_PER_FEE_SCALE;
      result[ "GRAPHENE_DEFAULT_ACCOUNT_FEE_SCALE_BITSHIFTS" ] = GRAPHENE_DEFAULT_ACCOUNT_FEE_SCALE_BITSHIFTS;
      result[ "GRAPHENE_MAX_WORKER_NAME_LENGTH" ] = GRAPHENE_MAX_WORKER_NAME_LENGTH;
      result[ "GRAPHENE_MAX_URL_LENGTH" ] = GRAPHENE_MAX_URL_LENGTH;
      result[ "GRAPHENE_NEAR_SCHEDULE_CTR_IV" ] = GRAPHENE_NEAR_SCHEDULE_CTR_IV;
      result[ "GRAPHENE_FAR_SCHEDULE_CTR_IV" ] = GRAPHENE_FAR_SCHEDULE_CTR_IV;
      result[ "GRAPHENE_CORE_ASSET_CYCLE_RATE" ] = GRAPHENE_CORE_ASSET_CYCLE_RATE;
      result[ "GRAPHENE_CORE_ASSET_CYCLE_RATE_BITS" ] = GRAPHENE_CORE_ASSET_CYCLE_RATE_BITS;
      result[ "GRAPHENE_DEFAULT_MINER_PAY_PER_BLOCK" ] = GRAPHENE_DEFAULT_MINER_PAY_PER_BLOCK;
      result[ "GRAPHENE_DEFAULT_MINER_PAY_VESTING_SECONDS" ] = GRAPHENE_DEFAULT_MINER_PAY_VESTING_SECONDS;
      result[ "GRAPHENE_MAX_INTEREST_APR" ] = GRAPHENE_MAX_INTEREST_APR;
      result[ "GRAPHENE_MINER_ACCOUNT" ] = GRAPHENE_MINER_ACCOUNT;
      result[ "GRAPHENE_NULL_ACCOUNT" ] = GRAPHENE_NULL_ACCOUNT;
      result[ "GRAPHENE_TEMP_ACCOUNT" ] = GRAPHENE_TEMP_ACCOUNT;
      return result;
   }

   configuration database_api::get_configuration()const
   {
      return graphene::chain::get_configuration();
   }

   chain_id_type database_api::get_chain_id()const
   {
      return my->get_chain_id();
   }

   chain_id_type database_api_impl::get_chain_id()const
   {
      return _db.get_chain_id();
   }

   dynamic_global_property_object database_api::get_dynamic_global_properties()const
   {
      return my->get_dynamic_global_properties();
   }

   dynamic_global_property_object database_api_impl::get_dynamic_global_properties()const
   {
      return _db.get(dynamic_global_property_id_type());
   }

   decent::about_info database_api::about()const
   {
      return decent::get_about_daemon();
   }

   //////////////////////////////////////////////////////////////////////
   //                                                                  //
   // Keys                                                             //
   //                                                                  //
   //////////////////////////////////////////////////////////////////////

   vector<vector<account_id_type>> database_api::get_key_references( vector<public_key_type> key )const
   {
      return my->get_key_references( key );
   }

   /**
    *  @return all accounts that refer to the key or account id in their owner or active authorities.
    */
   vector<vector<account_id_type>> database_api_impl::get_key_references( vector<public_key_type> keys )const
   {
      ddump( (keys) );
      vector< vector<account_id_type> > final_result;
      final_result.reserve(keys.size());

      for( auto& key : keys )
      {
         subscribe_to_item( key );

         const auto& idx = _db.get_index_type<account_index>();
         const auto& aidx = dynamic_cast<const graphene::db::primary_index<account_index>&>(idx);
         const auto& refs = aidx.get_secondary_index<graphene::chain::account_member_index>();
         auto itr = refs.account_to_key_memberships.find(key);
         vector<account_id_type> result;

         if( itr != refs.account_to_key_memberships.end() )
         {
            result.reserve( itr->second.size() );
            for( auto item : itr->second ) result.push_back(item);
         }
         final_result.emplace_back( std::move(result) );
      }

      for( auto i : final_result )
         subscribe_to_item(i);

      return final_result;
   }

   //////////////////////////////////////////////////////////////////////
   //                                                                  //
   // Accounts                                                         //
   //                                                                  //
   //////////////////////////////////////////////////////////////////////

   vector<optional<account_object>> database_api::get_accounts(const vector<account_id_type>& account_ids)const
   {
      return my->get_accounts( account_ids );
   }

   vector<optional<account_object>> database_api_impl::get_accounts(const vector<account_id_type>& account_ids)const
   {
      return _db.get_objects(account_ids);
   }

   std::map<string,full_account> database_api::get_full_accounts( const vector<string>& names_or_ids, bool subscribe )
   {
      return my->get_full_accounts( names_or_ids, subscribe );
   }

   std::map<std::string, full_account> database_api_impl::get_full_accounts( const vector<std::string>& names_or_ids, bool subscribe)
   {
      idump((names_or_ids));
      std::map<std::string, full_account> results;

      for (const std::string& account_name_or_id : names_or_ids)
      {
         const account_object* account = nullptr;
         if (std::isdigit(account_name_or_id[0]))
            account = _db.find(fc::variant(account_name_or_id).as<account_id_type>());
         else
         {
            const auto& idx = _db.get_index_type<account_index>().indices().get<by_name>();
            auto itr = idx.find(account_name_or_id);
            if (itr != idx.end())
               account = &*itr;
         }
         if (account != nullptr)
         {
            if( subscribe )
            {
               ilog( "subscribe to ${id}", ("id",account->name) );
               subscribe_to_item( account->id );
            }

            // fc::mutable_variant_object full_account;
            full_account acnt;
            acnt.account = *account;
            acnt.statistics = account->statistics(_db);
            acnt.registrar_name = account->registrar(_db).name;
            acnt.votes = lookup_vote_ids( vector<vote_id_type>(account->options.votes.begin(),account->options.votes.end()) );

            // Add the account itself, its statistics object, cashback balance, and referral account names
            /*
             full_account("account", *account)("statistics", account->statistics(_db))
             ("registrar_name", account->registrar(_db).name)("referrer_name", account->referrer(_db).name)
             ("lifetime_referrer_name", account->lifetime_referrer(_db).name);
             */
            if (account->cashback_vb)
            {
               acnt.cashback_balance = account->cashback_balance(_db);
            }
            // Add the account's proposals
            const auto& proposal_idx = _db.get_index_type<proposal_index>();
            const auto& pidx = dynamic_cast<const graphene::db::primary_index<proposal_index>&>(proposal_idx);
            const auto& proposals_by_account = pidx.get_secondary_index<graphene::chain::required_approval_index>();
            auto  required_approvals_itr = proposals_by_account._account_to_proposals.find( account->id );
            if( required_approvals_itr != proposals_by_account._account_to_proposals.end() )
            {
               acnt.proposals.reserve( required_approvals_itr->second.size() );
               for( auto proposal_id : required_approvals_itr->second )
                  acnt.proposals.push_back( proposal_id(_db) );
            }


            // Add the account's balances
            auto balance_range = _db.get_index_type<account_balance_index>().indices().get<by_account_asset>().equal_range(boost::make_tuple(account->id));
            //vector<account_balance_object> balances;
            std::for_each(balance_range.first, balance_range.second,
                          [&acnt](const account_balance_object& balance) {
                             acnt.balances.emplace_back(balance);
                          });

            // Add the account's vesting balances
            auto vesting_range = _db.get_index_type<vesting_balance_index>().indices().get<by_account>().equal_range(account->id);
            std::for_each(vesting_range.first, vesting_range.second,
                          [&acnt](const vesting_balance_object& balance) {
                             acnt.vesting_balances.emplace_back(balance);
                          });

            results[account_name_or_id] = acnt;

         }
      }
      return results;
   }

   optional<account_object> database_api::get_account_by_name( string name )const
   {
      return my->get_account_by_name( name );
   }

   optional<account_object> database_api_impl::get_account_by_name( string name )const
   {
      const auto& idx = _db.get_index_type<account_index>().indices().get<by_name>();
      auto itr = idx.find(name);
      if (itr != idx.end())
         return *itr;
      return optional<account_object>();
   }

   vector<account_id_type> database_api::get_account_references( account_id_type account_id )const
   {
      return my->get_account_references( account_id );
   }

   vector<account_id_type> database_api_impl::get_account_references( account_id_type account_id )const
   {
      const auto& idx = _db.get_index_type<account_index>();
      const auto& aidx = dynamic_cast<const graphene::db::primary_index<account_index>&>(idx);
      const auto& refs = aidx.get_secondary_index<graphene::chain::account_member_index>();
      auto itr = refs.account_to_account_memberships.find(account_id);
      vector<account_id_type> result;

      if( itr != refs.account_to_account_memberships.end() )
      {
         result.reserve( itr->second.size() );
         for( auto item : itr->second ) result.push_back(item);
      }
      return result;
   }

   vector<optional<account_object>> database_api::lookup_account_names(const vector<string>& account_names)const
   {
      return my->lookup_account_names( account_names );
   }

   vector<optional<account_object>> database_api_impl::lookup_account_names(const vector<string>& account_names)const
   {
      const auto& accounts_by_name = _db.get_index_type<account_index>().indices().get<by_name>();
      vector<optional<account_object> > result;
      result.reserve(account_names.size());
      std::transform(account_names.begin(), account_names.end(), std::back_inserter(result),
                     [&accounts_by_name](const string& name) -> optional<account_object> {
                        auto itr = accounts_by_name.find(name);
                        return itr == accounts_by_name.end()? optional<account_object>() : *itr;
                     });
      return result;
   }


   vector<account_object> database_api::search_accounts(const string& search_term, const string order, const object_id_type& id, uint32_t limit) const {
      return my->search_accounts( search_term, order, id, limit );
   }

   vector<transaction_detail_object> database_api::search_account_history(account_id_type const& account,
                                                                          string const& order,
                                                                          object_id_type const& id,
                                                                          int limit) const
   {
      return my->search_account_history(account, order, id, limit);
   }


   map<string,account_id_type> database_api::lookup_accounts(const string& lower_bound_name, uint32_t limit)const
   {
      return my->lookup_accounts( lower_bound_name, limit );
   }

   namespace
   {
      template <  typename _t_object_index,
      typename _t_object,
      typename _t_sort_tag,
      typename _t_iterator,
      bool is_ascending>
      void correct_iterator(graphene::chain::database& db,
                            const object_id_type& id,
                            _t_iterator& itr_begin)
      {
         const auto& idx_by_id = db.get_index_type<_t_object_index>().indices().template get<graphene::db::by_id>();
         auto itr_id = idx_by_id.find(id);

         const auto& idx_by_sort_tag = db.get_index_type<_t_object_index>().indices().template get<_t_sort_tag>();

         auto itr_find = idx_by_sort_tag.end();
         if (itr_id != idx_by_id.end())
            itr_find = idx_by_sort_tag.find(key_extractor<_t_sort_tag, _t_object>::get(*itr_id));

         // itr_find has the same keys as the object with id
         // scan to next items until exactly the object with id is found
         auto itr_scan = itr_find;
         while (itr_find != idx_by_sort_tag.end() &&
                itr_id != idx_by_id.end() &&
                ++itr_scan != idx_by_sort_tag.end() &&
                itr_find->id != itr_id->id &&
                key_extractor<_t_sort_tag, _t_object>::get(*itr_scan) == key_extractor<_t_sort_tag, _t_object>::get(*itr_id))
            itr_find = itr_scan;

         if (itr_find != idx_by_sort_tag.end())
         {
            itr_begin = return_one<is_ascending>::choose(itr_find, boost::reverse_iterator<decltype(itr_find)>(itr_find));
            if (false == is_ascending)
               --itr_begin;
         }
      }

      template <bool is_ascending, class sort_tag>
      void search_accounts_template(graphene::chain::database& db,
                                    const string& term,
                                    uint32_t count,
                                    const object_id_type& id,
                                    vector<account_object>& result)
      {
         const auto& idx_by_sort_tag = db.get_index_type<account_index>().indices().get<sort_tag>();

         auto itr_begin = return_one<is_ascending>::choose(idx_by_sort_tag.cbegin(), idx_by_sort_tag.crbegin());
         auto itr_end = return_one<is_ascending>::choose(idx_by_sort_tag.end(), idx_by_sort_tag.rend());

         correct_iterator<account_index, account_object, sort_tag, decltype(itr_begin), is_ascending>(db, id, itr_begin);

         while(count &&
               itr_begin != itr_end)
         {
            account_object const& element = *itr_begin;
            ++itr_begin;


            std::string account_id_str = fc::variant(element.get_id()).as<std::string>();
            std::string account_name = element.name;
            std::string search_term = term;

            boost::algorithm::to_lower(account_id_str);
            boost::algorithm::to_lower(account_name);
            boost::algorithm::to_lower(search_term);

            if (search_term.empty() ||
                account_name.find(search_term) != std::string::npos ||
                account_id_str.find(search_term) != std::string::npos)
            {
               result.emplace_back(element);
               --count;
            }
         }
      }
   }

   vector<account_object> database_api_impl::search_accounts(const string& term, const string order, const object_id_type& id, uint32_t limit)const
   {
      if(limit > CURRENT_OUTPUT_LIMIT_1000)
         FC_THROW_EXCEPTION(limit_exceeded_exception, "Current limit: ${l}", ("l", CURRENT_OUTPUT_LIMIT_1000));
      vector<account_object> result;

      if (order == "+id")
         search_accounts_template<true, graphene::db::by_id>(_db, term, limit, id, result);
      else if (order == "-id")
         search_accounts_template<false, graphene::db::by_id>(_db, term, limit, id, result);
      else if (order == "-name")
         search_accounts_template<false, by_name>(_db, term, limit, id, result);
      else
         search_accounts_template<true, by_name>(_db, term, limit, id, result);


      return result;
   }

   namespace
   {
      template <bool is_ascending, class sort_tag>
      void search_account_history_template(graphene::chain::database& db,
                                           const account_id_type& account,
                                           uint32_t count,
                                           const object_id_type& id,
                                           vector<transaction_detail_object>& result)
      {
         const auto& idx_by_sort_tag = db.get_index_type<transaction_detail_index>().indices().get<sort_tag>();

         auto itr_begin = return_one<is_ascending>::choose(idx_by_sort_tag.cbegin(), idx_by_sort_tag.crbegin());
         auto itr_end = return_one<is_ascending>::choose(idx_by_sort_tag.end(), idx_by_sort_tag.rend());

         correct_iterator<transaction_detail_index, transaction_detail_object, sort_tag, decltype(itr_begin), is_ascending>(db, id, itr_begin);

         while(count &&
               itr_begin != itr_end)
         {
            transaction_detail_object const& element = *itr_begin;
            ++itr_begin;

            if (account == element.m_from_account || account == element.m_to_account)
            {
               result.emplace_back(element);
               --count;
            }
         }
      }
   }

   vector<optional<account_statistics_object>> database_api::get_account_statistics(const vector<account_statistics_id_type>& account_statistics_ids)const
   {
      return my->get_account_statistics( account_statistics_ids );
   }

   vector<optional<account_statistics_object>> database_api_impl::get_account_statistics(const vector<account_statistics_id_type>& account_statistics_ids)const
   {
      return _db.get_objects(account_statistics_ids);
   }

   vector<transaction_detail_object> database_api_impl::search_account_history(account_id_type const& account,
                                                                               string const& order,
                                                                               object_id_type const& id,
                                                                               int limit) const
   {
      vector<transaction_detail_object> result;

      if (order == "+type")
         search_account_history_template<true, by_operation_type>(_db, account, limit, id, result);
      else if (order == "-type")
         search_account_history_template<false, by_operation_type>(_db, account, limit, id, result);
      else if (order == "+to")
         search_account_history_template<true, by_to_account>(_db, account, limit, id, result);
      else if (order == "-to")
         search_account_history_template<false, by_to_account>(_db, account, limit, id, result);
      else if (order == "+from")
         search_account_history_template<true, by_from_account>(_db, account, limit, id, result);
      else if (order == "-from")
         search_account_history_template<false, by_from_account>(_db, account, limit, id, result);
      else if (order == "+price")
         search_account_history_template<true, by_transaction_amount>(_db, account, limit, id, result);
      else if (order == "-price")
         search_account_history_template<false, by_transaction_amount>(_db, account, limit, id, result);
      else if (order == "+fee")
         search_account_history_template<true, by_transaction_fee>(_db, account, limit, id, result);
      else if (order == "-fee")
         search_account_history_template<false, by_transaction_fee>(_db, account, limit, id, result);
      else if (order == "+nft")
         search_account_history_template<true, by_nft>(_db, account, limit, id, result);
      else if (order == "-nft")
         search_account_history_template<false, by_nft>(_db, account, limit, id, result);
      else if (order == "+description")
         search_account_history_template<true, by_description>(_db, account, limit, id, result);
      else if (order == "-description")
         search_account_history_template<false, by_description>(_db, account, limit, id, result);
      else if (order == "+time")
         search_account_history_template<true, by_time>(_db, account, limit, id, result);
      else// if (order == "-time")
         search_account_history_template<false, by_time>(_db, account, limit, id, result);

      return result;
   }

   map<string,account_id_type> database_api_impl::lookup_accounts(const string& lower_bound_name, uint32_t limit)const
   {
      if(limit > CURRENT_OUTPUT_LIMIT_1000)
         FC_THROW_EXCEPTION(limit_exceeded_exception, "Current limit: ${l}", ("l", CURRENT_OUTPUT_LIMIT_1000));
      const auto& accounts_by_name = _db.get_index_type<account_index>().indices().get<by_name>();
      map<string,account_id_type> result;

      for( auto itr = accounts_by_name.lower_bound(lower_bound_name);
          limit-- && itr != accounts_by_name.end();
          ++itr )
      {
         result.insert(make_pair(itr->name, itr->get_id()));
         if( limit == 1 )
            subscribe_to_item( itr->get_id() );
      }

      return result;
   }

   uint64_t database_api::get_account_count()const
   {
      return my->get_account_count();
   }

   uint64_t database_api_impl::get_account_count()const
   {
      return _db.get_index_type<account_index>().indices().size();
   }

   //////////////////////////////////////////////////////////////////////
   //                                                                  //
   // Balances                                                         //
   //                                                                  //
   //////////////////////////////////////////////////////////////////////

   vector<asset> database_api::get_account_balances(account_id_type id, const flat_set<asset_id_type>& assets)const
   {
      return my->get_account_balances( id, assets );
   }

   vector<asset> database_api_impl::get_account_balances(account_id_type acnt, const flat_set<asset_id_type>& assets)const
   {
      vector<asset> result;
      if (assets.empty())
      {
         // if the caller passes in an empty list of assets, return balances for all assets the account owns
         const account_balance_index& balance_index = _db.get_index_type<account_balance_index>();
         auto range = balance_index.indices().get<by_account_asset>().equal_range(boost::make_tuple(acnt));
         for (const account_balance_object& balance : boost::make_iterator_range(range.first, range.second))
            result.push_back(asset(balance.get_balance()));
      }
      else
      {
         result.reserve(assets.size());

         std::transform(assets.begin(), assets.end(), std::back_inserter(result),
                        [this, acnt](asset_id_type id) { return _db.get_balance(acnt, id); });
      }

      return result;
   }

   vector<asset> database_api::get_named_account_balances(const std::string& name, const flat_set<asset_id_type>& assets)const
   {
      return my->get_named_account_balances( name, assets );
   }

   vector<asset> database_api_impl::get_named_account_balances(const std::string& name, const flat_set<asset_id_type>& assets) const
   {
      const auto& accounts_by_name = _db.get_index_type<account_index>().indices().get<by_name>();
      auto itr = accounts_by_name.find(name);
      if(itr == accounts_by_name.end())
         FC_THROW_EXCEPTION(app::account_does_not_exist_exception, "Account: ${account}", ("account", name));
      
      return get_account_balances(itr->get_id(), assets);
   }

   vector<vesting_balance_object> database_api::get_vesting_balances( account_id_type account_id )const
   {
      return my->get_vesting_balances( account_id );
   }

   vector<vesting_balance_object> database_api_impl::get_vesting_balances( account_id_type account_id )const
   {
      try
      {
         vector<vesting_balance_object> result;
         auto vesting_range = _db.get_index_type<vesting_balance_index>().indices().get<by_account>().equal_range(account_id);
         std::for_each(vesting_range.first, vesting_range.second,
                       [&result](const vesting_balance_object& balance) {
                          result.emplace_back(balance);
                       });
         return result;
      }
      FC_CAPTURE_AND_RETHROW( (account_id) );
   }

   map<non_fungible_token_id_type,uint32_t> database_api::get_non_fungible_token_summary(account_id_type account_id)const
   {
      return my->get_non_fungible_token_summary(account_id);
   }

   map<non_fungible_token_id_type,uint32_t> database_api_impl::get_non_fungible_token_summary(account_id_type account_id)const
   {
      const auto& nft_data_index = _db.get_index_type<non_fungible_token_data_index>();
      auto nft_data_range = nft_data_index.indices().get<by_account>().equal_range(account_id);

      map<non_fungible_token_id_type,uint32_t> result;
      std::for_each(nft_data_range.first, nft_data_range.second, [&](const non_fungible_token_data_object& nft_data) {
         auto ret = result.insert(std::make_pair(nft_data.nft_id, 1u));
         if(!ret.second)
            ++ret.first->second;
      });
      return result;
   }

   vector<non_fungible_token_data_object> database_api::get_non_fungible_token_balances(account_id_type account_id,
                                                                                        const set<non_fungible_token_id_type>& ids)const
   {
      return my->get_non_fungible_token_balances(account_id, ids);
   }

   vector<non_fungible_token_data_object> database_api_impl::get_non_fungible_token_balances(account_id_type account_id,
                                                                                             const set<non_fungible_token_id_type>& ids)const
   {
      const auto& nft_data_index = _db.get_index_type<non_fungible_token_data_index>();
      auto nft_data_range = nft_data_index.indices().get<by_account>().equal_range(account_id);

      vector<non_fungible_token_data_object> result;
      std::for_each(nft_data_range.first, nft_data_range.second, [&](const non_fungible_token_data_object& nft_data) {
         if(ids.empty() || ids.count(nft_data.nft_id))
            result.emplace_back(nft_data);
      });
      return result;
   }

   //////////////////////////////////////////////////////////////////////
   //                                                                  //
   // Assets                                                           //
   //                                                                  //
   //////////////////////////////////////////////////////////////////////

   uint64_t database_api::get_asset_count()const
      {
      return my->get_asset_count();
      }

   uint64_t database_api_impl::get_asset_count()const
      {
      return _db.get_index_type<asset_index>().indices().size();
      }

   vector<optional<asset_object>> database_api::get_assets(const vector<asset_id_type>& asset_ids)const
   {
      return my->get_assets( asset_ids );
   }

   vector<optional<asset_object>> database_api_impl::get_assets(const vector<asset_id_type>& asset_ids)const
   {
      return _db.get_objects(asset_ids);
   }

   vector<asset_object> database_api::list_assets(const string& lower_bound_symbol, uint32_t limit)const
   {
      return my->list_assets( lower_bound_symbol, limit );
   }

   vector<asset_object> database_api_impl::list_assets(const string& lower_bound_symbol, uint32_t limit)const
   {
      if(limit > CURRENT_OUTPUT_LIMIT_100)
         FC_THROW_EXCEPTION(limit_exceeded_exception, "Current limit: ${l}", ("l", CURRENT_OUTPUT_LIMIT_100));
      const auto& assets_by_symbol = _db.get_index_type<asset_index>().indices().get<by_symbol>();
      vector<asset_object> result;
      result.reserve(limit);

      auto itr = assets_by_symbol.lower_bound(lower_bound_symbol);

      if( lower_bound_symbol == "" )
         itr = assets_by_symbol.begin();

      while(limit-- && itr != assets_by_symbol.end())
         result.emplace_back(*itr++);

      return result;
   }

   vector<optional<asset_object>> database_api::lookup_asset_symbols(const vector<string>& symbols_or_ids)const
   {
      return my->lookup_asset_symbols( symbols_or_ids );
   }

   vector<optional<asset_object>> database_api_impl::lookup_asset_symbols(const vector<string>& symbols_or_ids)const
   {
      const auto& assets_by_symbol = _db.get_index_type<asset_index>().indices().get<by_symbol>();
      vector<optional<asset_object> > result;
      result.reserve(symbols_or_ids.size());
      std::transform(symbols_or_ids.begin(), symbols_or_ids.end(), std::back_inserter(result),
                     [this, &assets_by_symbol](const string& symbol_or_id) -> optional<asset_object> {
                        if( !symbol_or_id.empty() && std::isdigit(symbol_or_id[0]) )
                        {
                           auto ptr = _db.find(variant(symbol_or_id).as<asset_id_type>());
                           return ptr == nullptr? optional<asset_object>() : *ptr;
                        }
                        auto itr = assets_by_symbol.find(symbol_or_id);
                        return itr == assets_by_symbol.end()? optional<asset_object>() : *itr;
                     });
      return result;
   }

   vector<optional<asset_dynamic_data_object>> database_api::get_asset_dynamic_data(const vector<asset_dynamic_data_id_type>& asset_dynamic_data_ids)const
   {
      return my->get_asset_dynamic_data( asset_dynamic_data_ids );
   }

   vector<optional<asset_dynamic_data_object>> database_api_impl::get_asset_dynamic_data(const vector<asset_dynamic_data_id_type>& asset_dynamic_data_ids)const
   {
      return _db.get_objects(asset_dynamic_data_ids);
   }

   asset database_api::price_to_dct( asset price )const
   {
      return my->price_to_dct( price );
   }

   asset database_api_impl::price_to_dct( asset price )const
   {
      return _db.price_to_dct( price );
   }

   //////////////////////////////////////////////////////////////////////
   //                                                                  //
   // Non Fungible Tokens                                              //
   //                                                                  //
   //////////////////////////////////////////////////////////////////////

   uint64_t database_api::get_non_fungible_token_count()const
   {
      return my->get_non_fungible_token_count();
   }

   uint64_t database_api_impl::get_non_fungible_token_count()const
   {
      return _db.get_index_type<non_fungible_token_index>().indices().size();
   }

   vector<optional<non_fungible_token_object>> database_api::get_non_fungible_tokens(const vector<non_fungible_token_id_type>& nft_ids)const
   {
      return my->get_non_fungible_tokens(nft_ids);
   }

   vector<optional<non_fungible_token_object>> database_api_impl::get_non_fungible_tokens(const vector<non_fungible_token_id_type>& nft_ids)const
   {
      return _db.get_objects(nft_ids);
   }

   vector<non_fungible_token_object> database_api::list_non_fungible_tokens(const string& lower_bound_symbol, uint32_t limit)const
   {
      return my->list_non_fungible_tokens(lower_bound_symbol, limit);
   }

   vector<non_fungible_token_object> database_api_impl::list_non_fungible_tokens(const string& lower_bound_symbol, uint32_t limit)const
   {
      if(limit > CURRENT_OUTPUT_LIMIT_100)
         FC_THROW_EXCEPTION(limit_exceeded_exception, "Current limit: ${l}", ("l", CURRENT_OUTPUT_LIMIT_100));
      const auto& nfts_by_symbol = _db.get_index_type<non_fungible_token_index>().indices().get<by_symbol>();
      vector<non_fungible_token_object> result;
      result.reserve(limit);

      auto itr = nfts_by_symbol.lower_bound(lower_bound_symbol);
      if( lower_bound_symbol == "" )
         itr = nfts_by_symbol.begin();

      while(limit-- && itr != nfts_by_symbol.end())
         result.emplace_back(*itr++);

      return result;
   }

   vector<optional<non_fungible_token_object>> database_api::get_non_fungible_tokens_by_symbols(const vector<string>& symbols)const
   {
      return my->get_non_fungible_tokens_by_symbols(symbols);
   }

   vector<optional<non_fungible_token_object>> database_api_impl::get_non_fungible_tokens_by_symbols(const vector<string>& symbols)const
   {
      const auto& nfts_by_symbol = _db.get_index_type<non_fungible_token_index>().indices().get<by_symbol>();
      vector<optional<non_fungible_token_object> > result;
      result.reserve(symbols.size());

      std::transform(symbols.begin(), symbols.end(), std::back_inserter(result),
                     [&nfts_by_symbol](const string& symbol) -> optional<non_fungible_token_object> {
                        auto itr = nfts_by_symbol.find(symbol);
                        return itr == nfts_by_symbol.end()? optional<non_fungible_token_object>() : *itr;
                     });
      return result;
   }

   uint64_t database_api::get_non_fungible_token_data_count()const
   {
      return my->get_non_fungible_token_data_count();
   }

   uint64_t database_api_impl::get_non_fungible_token_data_count()const
   {
      return _db.get_index_type<non_fungible_token_data_index>().indices().size();
   }

   vector<optional<non_fungible_token_data_object>> database_api::get_non_fungible_token_data(const vector<non_fungible_token_data_id_type>& nft_data_ids)const
   {
      return my->get_non_fungible_token_data(nft_data_ids);
   }

   vector<optional<non_fungible_token_data_object>> database_api_impl::get_non_fungible_token_data(const vector<non_fungible_token_data_id_type>& nft_data_ids)const
   {
      return _db.get_objects(nft_data_ids);
   }

   vector<non_fungible_token_data_object> database_api::list_non_fungible_token_data(non_fungible_token_id_type nft_id)const
   {
      return my->list_non_fungible_token_data(nft_id);
   }

   vector<non_fungible_token_data_object> database_api_impl::list_non_fungible_token_data(non_fungible_token_id_type nft_id)const
   {
      const auto& nft_data_range = _db.get_index_type<non_fungible_token_data_index>().indices().get<by_nft>().equal_range(nft_id);

      vector<non_fungible_token_data_object> result;
      result.reserve(std::distance(nft_data_range.first, nft_data_range.second));
      std::for_each(nft_data_range.first, nft_data_range.second, [&](const non_fungible_token_data_object& nft_data) {
         if( nft_data.owner != GRAPHENE_NULL_ACCOUNT )
            result.emplace_back(nft_data);
      });

      return result;
   }

   vector<transaction_detail_object> database_api::search_non_fungible_token_history(non_fungible_token_data_id_type nft_data_id)const
   {
      return my->search_non_fungible_token_history(nft_data_id);
   }

   vector<transaction_detail_object> database_api_impl::search_non_fungible_token_history(non_fungible_token_data_id_type nft_data_id)const
   {
      auto nft_data_range = _db.get_index_type<transaction_detail_index>().indices().get<by_nft>().equal_range(nft_data_id);

      vector<transaction_detail_object> result;
      std::for_each(nft_data_range.first, nft_data_range.second, [&](const transaction_detail_object &obj) {
         result.emplace_back(obj);
      });
      return result;
   }

   //////////////////////////////////////////////////////////////////////
   //                                                                  //
   // Miners                                                           //
   //                                                                  //
   //////////////////////////////////////////////////////////////////////

   vector<optional<miner_object>> database_api::get_miners(const vector<miner_id_type>& miner_ids)const
   {
      return my->get_miners( miner_ids );
   }


   vector<optional<miner_object>> database_api_impl::get_miners(const vector<miner_id_type>& miner_ids)const
   {
      return _db.get_objects(miner_ids);
   }

   fc::optional<miner_object> database_api::get_miner_by_account(account_id_type account)const
   {
      return my->get_miner_by_account( account );
   }

   fc::optional<miner_object> database_api_impl::get_miner_by_account(account_id_type account) const
   {
      const auto& idx = _db.get_index_type<miner_index>().indices().get<by_account>();
      auto itr = idx.find(account);
      if( itr != idx.end() )
         return *itr;
      return {};
   }

   map<string, miner_id_type> database_api::lookup_miner_accounts(const string& lower_bound_name, uint32_t limit)const
   {
      return my->lookup_miner_accounts( lower_bound_name, limit );
   }

   map<string, miner_id_type> database_api_impl::lookup_miner_accounts(const string& lower_bound_name, uint32_t limit)const
   {
      if(limit > CURRENT_OUTPUT_LIMIT_1000)
         FC_THROW_EXCEPTION(limit_exceeded_exception, "Current limit: ${l}", ("l", CURRENT_OUTPUT_LIMIT_1000));
      const auto& miners_by_id = _db.get_index_type<miner_index>().indices().get<graphene::db::by_id>();

      // we want to order miners by account name, but that name is in the account object
      // so the miner_index doesn't have a quick way to access it.
      // get all the names and look them all up, sort them, then figure out what
      // records to return.  This could be optimized, but we expect the
      // number of miners to be few and the frequency of calls to be rare
      std::map<std::string, miner_id_type> miners_by_account_name;
      for (const miner_object& miner : miners_by_id)
         if (auto account_iter = _db.find(miner.miner_account))
            if (account_iter->name >= lower_bound_name) // we can ignore anything below lower_bound_name
               miners_by_account_name.insert(std::make_pair(account_iter->name, miner.id));

      auto end_iter = miners_by_account_name.begin();
      while (end_iter != miners_by_account_name.end() && limit--)
         ++end_iter;
      miners_by_account_name.erase(end_iter, miners_by_account_name.end());
      return miners_by_account_name;
   }

   uint64_t database_api::get_miner_count()const
   {
      return my->get_miner_count();
   }

   uint64_t database_api_impl::get_miner_count()const
   {
      return _db.get_index_type<miner_index>().indices().size();
   }

   multimap< time_point_sec, price_feed> database_api::get_feeds_by_miner(const account_id_type account_id, const uint32_t count)const
   {
      return my->get_feeds_by_miner( account_id, count);
   }

   multimap< time_point_sec, price_feed> database_api_impl::get_feeds_by_miner(const account_id_type account_id, uint32_t count)const
   {
      if(count > CURRENT_OUTPUT_LIMIT_100)
         FC_THROW_EXCEPTION(limit_exceeded_exception, "Current limit: ${l}", ("l", CURRENT_OUTPUT_LIMIT_100));
      auto& asset_idx = _db.get_index_type<asset_index>().indices().get<by_type>();
      auto mia_itr = asset_idx.lower_bound(true);

      multimap< time_point_sec, price_feed> result;

      while( mia_itr != asset_idx.end() && count-- )
      {
         const auto& itr = mia_itr->monitored_asset_opts->feeds.find(account_id);
         if( itr != mia_itr->monitored_asset_opts->feeds.end() )
            result.emplace( itr->second.first, itr->second.second );
         else
            count++;

         mia_itr++;
      }

      return result;
   }

   //////////////////////////////////////////////////////////////////////
   //                                                                  //
   // Votes                                                            //
   //                                                                  //
   //////////////////////////////////////////////////////////////////////

   vector<optional<miner_object>> database_api::lookup_vote_ids( const vector<vote_id_type>& votes )const
   {
      return my->lookup_vote_ids( votes );
   }

   vector<optional<miner_object>> database_api_impl::lookup_vote_ids( const vector<vote_id_type>& votes )const
   {
      if(votes.size() >= CURRENT_OUTPUT_LIMIT_1000)
         FC_THROW_EXCEPTION(limit_exceeded_exception, "Only ${l} votes can be queried at a time", ("l", CURRENT_OUTPUT_LIMIT_1000));

      const auto& miner_idx = _db.get_index_type<miner_index>().indices().get<by_vote_id>();

      vector<optional<miner_object>> result;
      result.reserve( votes.size() );
      for( auto id : votes )
      {
         switch( id.type() )
         {
            case vote_id_type::miner:
            {
               auto itr = miner_idx.find( id );
               if( itr != miner_idx.end() )
                  result.emplace_back( *itr );
               else
                  result.emplace_back( );
               break;
            }

            case vote_id_type::VOTE_TYPE_COUNT: break; // supress unused enum value warnings
         }
      }
      return result;
   }

   vector<miner_voting_info> database_api::search_miner_voting(const string& account_id,
                                                               const string& term,
                                                               bool only_my_votes,
                                                               const string& order,
                                                               const string& id,
                                                               uint32_t count ) const
   {
      return my->search_miner_voting(account_id, term, only_my_votes, order, id, count);
   }

   vector<miner_voting_info> database_api_impl::search_miner_voting(const string& account_id,
                                                      const string& term,
                                                      bool only_my_votes,
                                                      const string& order,
                                                      const string& id,
                                                      uint32_t count ) const
   {
      // miner sorting helper struct
      struct miner_sorter
      {
         string sort_;

         miner_sorter(const string& sort) : sort_(sort) {}

         bool operator()(const miner_voting_info& lhs, const miner_voting_info& rhs) const
         {
            if (sort_ == "+name") return lhs.name.compare(rhs.name) < 0;
            else if (sort_ == "-name") return rhs.name.compare(lhs.name) < 0;
            else if (sort_ == "+link") return lhs.url.compare(rhs.url) < 0;
            else if (sort_ == "-link") return rhs.url.compare(lhs.url) < 0;
            else if (sort_ == "+votes") return lhs.total_votes < rhs.total_votes;
            else if (sort_ == "-votes") return rhs.total_votes < lhs.total_votes;
            return false;
         }
      };

      // miner searching helper struct
      struct miner_search
      {
         object_id_type search_id_;

         miner_search(const string& search_id) : search_id_(search_id) {}

         bool operator()(const miner_voting_info& info) const
         {
            return info.id == search_id_;
         }
      };

      flat_set<vote_id_type> acc_votes;

      if (! account_id.empty())
      {
         const account_object* acc_obj = nullptr;
         if (std::isdigit(account_id[0]))
            acc_obj = _db.find(fc::variant(account_id).as<account_id_type>());
         else
         {
            const auto& idx = _db.get_index_type<account_index>().indices().get<by_name>();
            auto itr = idx.find(account_id);
            if (itr != idx.end())
               acc_obj = &*itr;
         }

         if (!acc_obj)
            FC_THROW_EXCEPTION(account_does_not_exist_exception, "Account: ${account}", ("account", account_id));

         acc_votes = acc_obj->options.votes;
      }

      map<string,miner_id_type> miners = this->lookup_miner_accounts("", 1000);

      vector<miner_voting_info> miners_info;
      miners_info.reserve(miners.size());

      for (auto item : miners)
      {
         miner_voting_info info;
         info.id = item.second;
         info.name = item.first;

         if (term.empty() || item.first.find(term) != std::string::npos )
         {
            optional<miner_object> ob = this->get_miners({ item.second }).front();
            if (ob)
            {
               miner_object obj = *ob;

               info.url = obj.url;
               info.total_votes = obj.total_votes;
               info.voted = acc_votes.find(obj.vote_id) != acc_votes.end();

               if (! only_my_votes || info.voted)
                  miners_info.push_back(info);
            }
         }
      }

      if (!order.empty())
         std::sort(miners_info.begin(), miners_info.end(), miner_sorter(order));

      vector<miner_voting_info> result;
      result.reserve(count);

      auto it = miners_info.begin();
      if (!id.empty())
         it = std::find_if(miners_info.begin(), miners_info.end(), miner_search(id));

      while (count && it != miners_info.end())
      {
         result.push_back(*it);
         count--;
         it++;
      }

      return result;
   }


   //////////////////////////////////////////////////////////////////////
   //                                                                  //
   // Authority / validation                                           //
   //                                                                  //
   //////////////////////////////////////////////////////////////////////

   std::string database_api::get_transaction_hex(const signed_transaction& trx)const
   {
      return my->get_transaction_hex( trx );
   }

   std::string database_api_impl::get_transaction_hex(const signed_transaction& trx)const
   {
      return fc::to_hex(fc::raw::pack(trx));
   }

   set<public_key_type> database_api::get_required_signatures( const signed_transaction& trx, const flat_set<public_key_type>& available_keys )const
   {
      return my->get_required_signatures( trx, available_keys );
   }

   set<public_key_type> database_api_impl::get_required_signatures( const signed_transaction& trx, const flat_set<public_key_type>& available_keys )const
   {
      ddump((trx)(available_keys));
      auto result = trx.get_required_signatures( _db.get_chain_id(),
                                                available_keys,
                                                [&]( account_id_type id ){ return &id(_db).active; },
                                                [&]( account_id_type id ){ return &id(_db).owner; },
                                                _db.get_global_properties().parameters.max_authority_depth );
      ddump((result));
      return result;
   }

   set<public_key_type> database_api::get_potential_signatures( const signed_transaction& trx )const
   {
      return my->get_potential_signatures( trx );
   }

   set<public_key_type> database_api_impl::get_potential_signatures( const signed_transaction& trx )const
   {
      ddump((trx));
      set<public_key_type> result;
      trx.get_required_signatures(
                                  _db.get_chain_id(),
                                  flat_set<public_key_type>(),
                                  [&]( account_id_type id )
                                  {
                                     const auto& auth = id(_db).active;
                                     for( const auto& k : auth.get_keys() )
                                        result.insert(k);
                                     return &auth;
                                  },
                                  [&]( account_id_type id )
                                  {
                                     const auto& auth = id(_db).owner;
                                     for( const auto& k : auth.get_keys() )
                                        result.insert(k);
                                     return &auth;
                                  },
                                  _db.get_global_properties().parameters.max_authority_depth
                                  );

      ddump((result));
      return result;
   }

   bool database_api::verify_authority( const signed_transaction& trx )const
   {
      return my->verify_authority( trx );
   }

   bool database_api_impl::verify_authority( const signed_transaction& trx )const
   {
      try
      {
         trx.verify_authority( trx.get_signature_keys(_db.get_chain_id()),
                              [&]( account_id_type id ){ return &id(_db).active; },
                              [&]( account_id_type id ){ return &id(_db).owner; },
                              _db.get_global_properties().parameters.max_authority_depth );
         return true;
      }
      catch( const transaction_exception &e )
      {
         dlog(e.to_string(fc::log_level::debug));
      }

      return false;
   }

   bool database_api::verify_account_authority( const string& name_or_id, const flat_set<public_key_type>& signers )const
   {
      return my->verify_account_authority( name_or_id, signers );
   }

   bool database_api_impl::verify_account_authority( const string& name_or_id, const flat_set<public_key_type>& keys )const
   {
      if(name_or_id.size() == 0)
         FC_THROW("Account name or id cannot be empty string");
      const account_object* account = nullptr;
      if (std::isdigit(name_or_id[0]))
         account = _db.find(fc::variant(name_or_id).as<account_id_type>());
      else
      {
         const auto& idx = _db.get_index_type<account_index>().indices().get<by_name>();
         auto itr = idx.find(name_or_id);
         if (itr != idx.end())
            account = &*itr;
      }

      if(!account)
         FC_THROW_EXCEPTION(account_does_not_exist_exception, "Account: ${account}", ("account", name_or_id));

      try
      {
         /// reuse trx.verify_authority by creating a dummy transfer
         signed_transaction trx;
         transfer_obsolete_operation op;
         op.from = account->id;
         trx.operations.emplace_back(op);
         trx.verify_authority( keys,
                               [&]( account_id_type id ){ return &id(_db).active; },
                               [&]( account_id_type id ){ return &id(_db).owner; },
                               _db.get_global_properties().parameters.max_authority_depth );
         return true;
      }
      catch( const transaction_exception &e )
      {
         dlog(e.to_string(fc::log_level::debug));
      }

      return false;
   }

   processed_transaction database_api::validate_transaction( const signed_transaction& trx )const
   {
      return my->validate_transaction( trx );
   }

   processed_transaction database_api_impl::validate_transaction( const signed_transaction& trx )const
   {
      return _db.validate_transaction(trx);
   }

   fc::variants database_api::get_required_fees( const vector<operation>& ops, asset_id_type id )const
   {
      return my->get_required_fees( ops, id );
   }

   /**
    * Container method for mutually recursive functions used to
    * implement get_required_fees() with potentially nested proposals.
    */
   struct get_required_fees_helper
   {
      get_required_fees_helper(
                               const fee_schedule& _current_fee_schedule,
                               const price& _core_exchange_rate,
                               uint32_t _max_recursion
                               )
      : current_fee_schedule(_current_fee_schedule),
      core_exchange_rate(_core_exchange_rate),
      max_recursion(_max_recursion)
      {}

      fc::variant set_op_fees( operation& op )
      {
         if( op.which() == operation::tag<proposal_create_operation>::value )
         {
            return set_proposal_create_op_fees( op );
         }
         else
         {
            asset fee = current_fee_schedule.set_fee( op, core_exchange_rate );
            fc::variant result;
            fc::to_variant( fee, result );
            return result;
         }
      }

      fc::variant set_proposal_create_op_fees( operation& proposal_create_op )
      {
         proposal_create_operation& op = proposal_create_op.get<proposal_create_operation>();
         std::pair< asset, fc::variants > result;
         for( op_wrapper& prop_op : op.proposed_ops )
         {
            FC_ASSERT( current_recursion < max_recursion );
            ++current_recursion;
            result.second.push_back( set_op_fees( prop_op.op ) );
            --current_recursion;
         }
         // we need to do this on the boxed version, which is why we use
         // two mutually recursive functions instead of a visitor
         result.first = current_fee_schedule.set_fee( proposal_create_op, core_exchange_rate );
         fc::variant vresult;
         fc::to_variant( result, vresult );
         return vresult;
      }

      const fee_schedule& current_fee_schedule;
      const price& core_exchange_rate;
      uint32_t max_recursion;
      uint32_t current_recursion = 0;
   };

   fc::variants database_api_impl::get_required_fees( vector<operation> ops, asset_id_type id )const
   {
      //
      // we copy the ops because we need to mutate an operation to reliably
      // determine its fee, see #435
      //

      const asset_object& a = id(_db);
      get_required_fees_helper helper(
                                      _db.current_fee_schedule(),
                                      a.options.core_exchange_rate,
                                      GET_REQUIRED_FEES_MAX_RECURSION );

      fc::variants result(ops.size());
      std::transform(ops.begin(), ops.end(), result.begin(), [&](operation &op) { return helper.set_op_fees( op ); } );
      return result;
   }

   //////////////////////////////////////////////////////////////////////
   //                                                                  //
   // Proposed transactions                                            //
   //                                                                  //
   //////////////////////////////////////////////////////////////////////

   vector<proposal_object> database_api::get_proposed_transactions( account_id_type id )const
   {
      return my->get_proposed_transactions( id );
   }

   vector<operation_info> database_api::list_operations( )const
   {
      return my->list_operations();
   }

   /** TODO: add secondary index that will accelerate this process */
   vector<proposal_object> database_api_impl::get_proposed_transactions( account_id_type id )const
   {
      const auto& idx = _db.get_index_type<proposal_index>();
      vector<proposal_object> result;

      idx.inspect_all_objects( [&](const graphene::db::object& obj){
         const proposal_object& p = static_cast<const proposal_object&>(obj);
         if( p.required_active_approvals.find( id ) != p.required_active_approvals.end() )
            result.push_back(p);
         else if ( p.required_owner_approvals.find( id ) != p.required_owner_approvals.end() )
            result.push_back(p);
         else if ( p.available_active_approvals.find( id ) != p.available_active_approvals.end() )
            result.push_back(p);
      });
      return result;
   }

   struct operation_info_visitor
   {
        typedef void result_type;

        shared_ptr<vector<string>> op_names;
        operation_info_visitor(shared_ptr<vector<string>> _op_names) : op_names(_op_names) { }

        template<typename Type>
        result_type operator()( const Type& op )const
        {
           string vo = fc::get_typename<Type>::name();
           op_names->emplace_back( vo );
        }
   };

   vector<operation_info> database_api_impl::list_operations( )const
   {
       shared_ptr<vector<string>> op_names_ptr = std::make_shared<vector<string>>();

       vector<operation_info> result;
       map<int32_t, bool> op_processed;
       map<int32_t, operation_info> op_cached;

       fee_schedule temp_fee_schedule;
       temp_fee_schedule = temp_fee_schedule.get_default();

       fee_schedule global_fee_schedule;
       global_fee_schedule = get_global_properties().parameters.current_fees;

       op_names_ptr->clear();
       result.clear();
       op_processed.clear();

       try
       {
          graphene::chain::operation op;

          for( std::size_t i = 0; i < graphene::chain::operation::type_info::count; ++i )
          {
             op.set_which(i);
             op.visit( operation_info_visitor(op_names_ptr) );
          }

          for( fee_parameters& params : global_fee_schedule.parameters )
          {
              op_processed[params.which()] = true;
              op_cached[params.which()] = operation_info(params.which(), (*op_names_ptr)[params.which()].replace(0, string("graphene::chain::").length(), ""), params);
          }

          for( fee_parameters& params : temp_fee_schedule.parameters )
          {
              if (0 == op_processed.count(params.which()) || ! op_processed[params.which()])
              {
                  op_cached[params.which()] = operation_info(params.which(), (*op_names_ptr)[params.which()].replace(0, string("graphene::chain::").length(), ""), params);
              }
          }

          map<int32_t, operation_info>::iterator it;

          for( it = op_cached.begin(); it != op_cached.end(); it++ )
          {
              result.emplace_back(it->second);
          }
       }
       catch ( const fc::exception& e ){ edump((e.to_detail_string())); }

       return result;
   }

   //////////////////////////////////////////////////////////////////////
   //                                                                  //
   // Decent                                                           //
   //                                                                  //
   //////////////////////////////////////////////////////////////////////

   real_supply database_api::get_real_supply()const
   {
      return my->_db.get_real_supply();
   }

   vector<account_id_type> database_api::list_publishing_managers( const string& lower_bound_name, uint32_t limit  )const
   {
      return my->list_publishing_managers( lower_bound_name, limit );
   }

   vector<account_id_type> database_api_impl::list_publishing_managers( const string& lower_bound_name, uint32_t limit  )const
   {
      if(limit > CURRENT_OUTPUT_LIMIT_100)
         FC_THROW_EXCEPTION(limit_exceeded_exception, "Current limit: ${l}", ("l", CURRENT_OUTPUT_LIMIT_100));
      const auto& idx = _db.get_index_type<account_index>().indices().get<by_publishing_manager_and_name>();
      vector<account_id_type> result;

      for( auto itr = idx.lower_bound( boost::make_tuple( true, lower_bound_name ) );
           limit-- && itr->rights_to_publish.is_publishing_manager && itr != idx.end();
           ++itr )
         result.push_back(itr->id);

      return result;
   }

      vector<buying_object> database_api::get_open_buyings()const
   {
      return my->get_open_buyings();
   }

   vector<buying_object> database_api_impl::get_open_buyings()const
   {
      const auto& range = _db.get_index_type<buying_index>().indices().get<by_open_expiration>().equal_range( true );
      vector<buying_object> result;
      result.reserve(distance(range.first, range.second));

      std::for_each(range.first, range.second, [&](const buying_object &element) {
         if( element.expiration_time >= _db.head_block_time() )
            result.emplace_back(element);
      });
      return result;
   }

   vector<buying_object> database_api::get_open_buyings_by_URI( const string& URI )const
   {
      return my->get_open_buyings_by_URI( URI );
   }

   vector<buying_object> database_api_impl::get_open_buyings_by_URI( const string& URI )const
   {
      try
      {
         auto range = _db.get_index_type<buying_index>().indices().get<by_URI_open>().equal_range( std::make_tuple( URI, true ) );
         vector<buying_object> result;
         result.reserve(distance(range.first, range.second));
         std::for_each(range.first, range.second, [&](const buying_object& element) {
            if( element.expiration_time >= _db.head_block_time() )
               result.emplace_back(element);
         });
         return result;
      }
      FC_CAPTURE_AND_RETHROW( (URI) );
   }

   vector<buying_object> database_api::get_open_buyings_by_consumer( const account_id_type& consumer )const
   {
      return my->get_open_buyings_by_consumer( consumer );
   }

   vector<buying_object> database_api_impl::get_open_buyings_by_consumer( const account_id_type& consumer )const
   {
      try
      {
         auto range = _db.get_index_type<buying_index>().indices().get<by_consumer_open>().equal_range( std::make_tuple( consumer, true ));
         vector<buying_object> result;
         result.reserve(distance(range.first, range.second));

         std::for_each(range.first, range.second, [&](const buying_object& element) {
            if( element.expiration_time >= _db.head_block_time() )
               result.emplace_back(element);
         });
         return result;
      }
      FC_CAPTURE_AND_RETHROW( (consumer) );
   }

   vector<buying_object> database_api::get_buying_history_objects_by_consumer( const account_id_type& consumer )const
   {
      return my->get_buying_history_objects_by_consumer( consumer );
   }


   vector<buying_object> database_api_impl::get_buying_history_objects_by_consumer ( const account_id_type& consumer )const
   {
      try {
         const auto &range = _db.get_index_type<buying_index>().indices().get<by_consumer_open>().equal_range( std::make_tuple(consumer, false));
         vector<buying_object> result;
         result.reserve(distance(range.first, range.second));

         std::for_each(range.first, range.second, [&](const buying_object &element) {
            result.emplace_back(element);
         });

         return result;
      }
      FC_CAPTURE_AND_RETHROW( (consumer) );
   }


   vector<buying_object> database_api::get_buying_objects_by_consumer(const account_id_type& consumer,
                                                                      const string& order,
                                                                      const object_id_type& id,
                                                                      const string& term,
                                                                      uint32_t count)const
   {
      return my->get_buying_objects_by_consumer( consumer, order, id, term, count );
   }

namespace
{
   template <bool is_ascending, class sort_tag>
   void search_buying_template(graphene::chain::database& db,
                               const account_id_type& consumer,
                               const string& term,
                               const object_id_type& id,
                               uint32_t count,
                               vector<buying_object>& result)
   {
      const auto& idx_by_sort_tag = db.get_index_type<buying_index>().indices().get<sort_tag>();

      auto itr_begin = return_one<is_ascending>::choose(idx_by_sort_tag.cbegin(), idx_by_sort_tag.crbegin());
      auto itr_end = return_one<is_ascending>::choose(idx_by_sort_tag.end(), idx_by_sort_tag.rend());

      correct_iterator<buying_index, buying_object, sort_tag, decltype(itr_begin), is_ascending>(db, id, itr_begin);

      while(count &&
            itr_begin != itr_end)
      {
         buying_object const& element = *itr_begin;
         ++itr_begin;

         if (element.consumer == consumer)
         {
            std::string title;
            std::string description;

            ContentObjectPropertyManager synopsis_parser(element.synopsis);
            title = synopsis_parser.get<ContentObjectTitle>();
            description = synopsis_parser.get<ContentObjectDescription>();

            std::string search_term = term;
            boost::algorithm::to_lower(search_term);
            boost::algorithm::to_lower(title);
            boost::algorithm::to_lower(description);

            if (search_term.empty() ||
                std::string::npos != title.find(search_term) ||
                std::string::npos != description.find(search_term))
            {
               result.emplace_back(element);
               --count;
            }
         }
      }
   }
}

   vector<buying_object> database_api_impl::get_buying_objects_by_consumer(const account_id_type& consumer,
                                                                           const string& order,
                                                                           const object_id_type& id,
                                                                           const string& term,
                                                                           uint32_t count)const
   {
      try {
         vector<buying_object> result;

         if(order == "+size")
            search_buying_template<true, by_size>(_db, consumer, term, id, count, result);
         else if(order == "-size")
            search_buying_template<false, by_size>(_db, consumer, term, id, count, result);
         else if(order == "+price")
            search_buying_template<true, by_price_before_exchange>(_db, consumer, term, id, count, result);
         else if(order == "-price")
            search_buying_template<false, by_price_before_exchange>(_db, consumer, term, id, count, result);
         else if(order == "+created")
            search_buying_template<true, by_created>(_db, consumer, term, id, count, result);
         else if(order == "-created")
            search_buying_template<false, by_created>(_db, consumer, term, id, count, result);
         else if(order == "+purchased")
            search_buying_template<true, by_purchased>(_db, consumer, term, id, count, result);
         else //if(order == "-purchased")
            search_buying_template<false, by_purchased>(_db, consumer, term, id, count, result);
         return result;
      }
      FC_CAPTURE_AND_RETHROW( (consumer) );
   }

   optional<content_object> database_api::get_content(const string& URI)const
   {
      return my->get_content( URI );
   }

   fc::sha256 database_api::restore_encryption_key(DIntegerString el_gamal_priv_key_string, buying_id_type buying ) const {
      auto objects = get_objects({buying});
      if(objects.size() == 0)
         FC_THROW_EXCEPTION(buying_object_does_not_exist_exception, "Buying: ${buying}", ("buying", buying));

      const buying_object bo = objects.front().template as<buying_object>();
      auto content = get_content(bo.URI);

      if(!content)
         FC_THROW_EXCEPTION(content_object_does_not_exist_exception, "URI: ${uri}", ("uri", bo.URI));

      const content_object co = *content;

      decent::encrypt::ShamirSecret ss( static_cast<uint16_t>(co.quorum), static_cast<uint16_t>(co.key_parts.size()) );
      decent::encrypt::point message;

      DInteger el_gamal_priv_key = el_gamal_priv_key_string;

      for( const auto key_particle : bo.key_particles )
      {
         auto result = decent::encrypt::el_gamal_decrypt( decent::encrypt::Ciphertext( key_particle ), el_gamal_priv_key, message );
         if(result != decent::encrypt::ok)
            FC_THROW_EXCEPTION(decryption_of_key_particle_failed_exception, "");
         ss.add_point( message );
      }

      FC_ASSERT( ss.resolvable() );
      ss.calculate_secret();

      fc::sha256 key;
#if CRYPTOPP_VERSION >= 600
      ss.secret.Encode((CryptoPP::byte*)key._hash, 32);
#else
      ss.secret.Encode((byte*)key._hash, 32);
#endif

      return key;
   }



   content_keys database_api::generate_content_keys(vector<account_id_type> const& seeders) const {

         content_keys keys;

         CryptoPP::Integer secret(randomGenerator, 256);
         while( secret >= Params::instance().DECENT_SHAMIR_ORDER ){
            CryptoPP::Integer tmp(randomGenerator, 256);
            secret = tmp;
         }
#if CRYPTOPP_VERSION >= 600
         secret.Encode((CryptoPP::byte*)keys.key._hash, 32);
#else
         secret.Encode((byte*)keys.key._hash, 32);
#endif

         keys.quorum = std::max(2u, static_cast<uint32_t>(seeders.size()/3));
         ShamirSecret ss(static_cast<uint16_t>(keys.quorum), static_cast<uint16_t>(seeders.size()), secret);
         ss.calculate_split();


         for( int i =0; i < (int)seeders.size(); i++ )
         {
            const auto& s = my->get_seeder( seeders[i] );

            if(!s)
              FC_THROW_EXCEPTION(seeder_not_found_exception, "Seeder: ${s}", ("s", seeders[i]));
            Ciphertext cp;
            point p = ss.split[i];
            decent::encrypt::el_gamal_encrypt( p, s->pubKey ,cp );
            keys.parts.push_back(cp);
         }

         return keys;
   }

   optional<seeder_object> database_api::get_seeder( const account_id_type& account ) const
   {
      return my->get_seeder(account);
   }

   optional<content_object> database_api_impl::get_content( const string& URI )const
   {
      const auto& idx = _db.get_index_type<content_index>().indices().get<by_URI>();
      auto itr = idx.find(URI);
      if (itr != idx.end())
         return *itr;
      return optional<content_object>();
   }

   vector<buying_object> database_api::search_feedback(const string& user,
                                                       const string& URI,
                                                       const object_id_type& id,
                                                       uint32_t count) const
   {
      return my->search_feedback(user, URI, id, count);
   }

   namespace {

      template <bool is_ascending, class sort_tag>
      void search_rating_template(graphene::chain::database& db,
                                  uint32_t count,
                                  const string& URI,
                                  const object_id_type& id,
                                  vector<buying_object>& result)
      {
         const auto& range_equal = db.get_index_type<buying_index>().indices().get<sort_tag>().equal_range(std::make_tuple( URI, true ));
         auto range_begin = range_equal.first;
         auto range_end = range_equal.second;

         auto itr_begin = return_one<is_ascending>::choose(range_begin, boost::reverse_iterator<decltype(range_end)>(range_end));
         auto itr_end = return_one<is_ascending>::choose(range_end, boost::reverse_iterator<decltype(range_begin)>(range_begin));

         correct_iterator<buying_index, buying_object, sort_tag, decltype(itr_begin), is_ascending>(db, id, itr_begin);

         while (count && itr_begin != itr_end)
         {
            const buying_object& rating_item = *itr_begin;
            ++itr_begin;

            result.push_back(rating_item);
            count--;
         }
      }
   }

   vector<buying_object> database_api_impl::search_feedback(const string& user,
                                                            const string& URI,
                                                            const object_id_type& id,
                                                            uint32_t count) const
   {
      vector<buying_object> result;

      try
      {
         const auto& idx_account = _db.get_index_type<account_index>().indices().get<by_name>();
         const auto account_itr = idx_account.find(user);

         if (false == user.empty())
         {
            if (account_itr != idx_account.end())
            {
               const auto& idx = _db.get_index_type<buying_index>().indices().get<by_consumer_URI>();
               auto itr = idx.find(std::make_tuple(account_itr->id, URI));
               if(itr != idx.end() && itr->rated_or_commented)
                  result.push_back(*itr);
            }
         }
         else
         {
            search_rating_template<false, by_URI_rated>(_db, count, URI, id, result);
         }
      }FC_CAPTURE_AND_RETHROW( (user)(URI) );

      return result;
   }

   optional<buying_object> database_api::get_buying_by_consumer_URI( const account_id_type& consumer, const string& URI )const
   {
      return my->get_buying_by_consumer_URI( consumer, URI );
   }

   optional <buying_object> database_api_impl::get_buying_by_consumer_URI( const account_id_type& consumer, const string& URI)const
   {
      try{
         const auto & idx = _db.get_index_type<buying_index>().indices().get<by_consumer_URI>();
         auto itr = idx.find(std::make_tuple(consumer, URI));
         vector<buying_object> result;
         if(itr!=idx.end()){
            return *itr;
         }
         return optional<buying_object>();

      }FC_CAPTURE_AND_RETHROW( (consumer)(URI) );
   }

   optional<seeder_object> database_api_impl::get_seeder( const account_id_type& account )const
   {
      const auto& idx = _db.get_index_type<seeder_index>().indices().get<by_seeder>();
      auto itr = idx.find(account);
      if (itr != idx.end())
         return *itr;
      return optional<seeder_object>();
   }

   optional<subscription_object> database_api::get_subscription( const subscription_id_type& sid) const
   {
      return my->get_subscription(sid);
   }

   optional<subscription_object> database_api_impl::get_subscription( const subscription_id_type& sid) const
   {
      const auto& idx = _db.get_index_type<subscription_index>().indices().get<graphene::db::by_id>();
      auto itr = idx.find(sid);
      if (itr != idx.end())
         return *itr;
      return optional<subscription_object>();
   }

   vector< subscription_object > database_api::list_active_subscriptions_by_consumer( const account_id_type& account, const uint32_t count)const
   {
      return my->list_active_subscriptions_by_consumer( account, count );
   }

   vector< subscription_object > database_api_impl::list_active_subscriptions_by_consumer( const account_id_type& account, const uint32_t count)const
   {
      try{
         if(count > CURRENT_OUTPUT_LIMIT_100)
            FC_THROW_EXCEPTION(limit_exceeded_exception, "Current limit: ${i}", ("l", CURRENT_OUTPUT_LIMIT_100));
         auto range = _db.get_index_type<subscription_index>().indices().get<by_from_expiration>().equal_range(account);
         vector<subscription_object> result;
         result.reserve(distance(range.first, range.second));
         std::for_each(range.first, range.second,
                       [&](const subscription_object& element) {
                            if( element.expiration > _db.head_block_time() )
                               result.emplace_back(element);
                       });
         return result;

      }FC_CAPTURE_AND_RETHROW( (account)(count) );
   }

   vector< subscription_object > database_api::list_subscriptions_by_consumer( const account_id_type& account, const uint32_t count)const
   {
      return my->list_subscriptions_by_consumer( account, count );
   }

   vector< subscription_object > database_api_impl::list_subscriptions_by_consumer( const account_id_type& account, const uint32_t count)const
   {
      try{
         if(count > CURRENT_OUTPUT_LIMIT_100)
            FC_THROW_EXCEPTION(limit_exceeded_exception, "Current limit: ${i}", ("l", CURRENT_OUTPUT_LIMIT_100));
         uint32_t i = count;
         const auto& range = _db.get_index_type<subscription_index>().indices().get<by_from>().equal_range(account);
         vector<subscription_object> result;
         result.reserve(count);
         auto itr = range.first;

         while(i-- && itr != range.second)
         {
            result.emplace_back(*itr);
            ++itr;
         }

         return result;

      }FC_CAPTURE_AND_RETHROW( (account)(count) );
   }

   vector< subscription_object > database_api::list_active_subscriptions_by_author( const account_id_type& account, const uint32_t count)const
   {
      return my->list_active_subscriptions_by_author( account, count );
   }

   vector< subscription_object > database_api_impl::list_active_subscriptions_by_author( const account_id_type& account, const uint32_t count)const
   {
      try{
         if(count > CURRENT_OUTPUT_LIMIT_100)
            FC_THROW_EXCEPTION(limit_exceeded_exception, "Current limit: ${i}", ("l", CURRENT_OUTPUT_LIMIT_100));
         auto range = _db.get_index_type<subscription_index>().indices().get<by_to_expiration>().equal_range(account);
         vector<subscription_object> result;
         result.reserve(distance(range.first, range.second));
         std::for_each(range.first, range.second,
                       [&](const subscription_object& element) {
                            if( element.expiration > _db.head_block_time() )
                               result.emplace_back(element);
                       });
         return result;

      }FC_CAPTURE_AND_RETHROW( (account)(count) );
   }



   vector< subscription_object > database_api::list_subscriptions_by_author( const account_id_type& account, const uint32_t count)const
   {
      return my->list_subscriptions_by_author( account, count );
   }

   vector< subscription_object > database_api_impl::list_subscriptions_by_author( const account_id_type& account, const uint32_t count)const
   {
      try{
         if(count > CURRENT_OUTPUT_LIMIT_100)
            FC_THROW_EXCEPTION(limit_exceeded_exception, "Current limit: ${i}", ("l", CURRENT_OUTPUT_LIMIT_100));
         uint32_t i = count;
         const auto& range = _db.get_index_type<subscription_index>().indices().get<by_to>().equal_range(account);
         vector<subscription_object> result;
         result.reserve(count);
         auto itr = range.first;

         while(i-- && itr != range.second)
         {
            result.emplace_back(*itr);
            ++itr;
         }

         return result;

      }FC_CAPTURE_AND_RETHROW( (account)(count) );
   }

   vector<content_summary> database_api::search_content(const string& term,
                                                        const string& order,
                                                        const string& user,
                                                        const string& region_code,
                                                        const object_id_type& id,
                                                        const string& type,
                                                        uint32_t count)const
   {
      return my->search_content(term, order, user, region_code, id, type, count);
   }


   namespace {

      template <bool is_ascending, class sort_tag>
      void search_content_template(graphene::chain::database& db,
                                   const string& search_term,
                                   uint32_t count,
                                   const string& user,
                                   const string& region_code,
                                   const object_id_type& id,
                                   const string& type,
                                   vector<content_summary>& result)
      {
         const auto& idx_by_sort_tag = db.get_index_type<content_index>().indices().get<sort_tag>();

         auto itr_begin = return_one<is_ascending>::choose(idx_by_sort_tag.cbegin(), idx_by_sort_tag.crbegin());
         auto itr_end = return_one<is_ascending>::choose(idx_by_sort_tag.cend(), idx_by_sort_tag.crend());

         correct_iterator<content_index, content_object, sort_tag, decltype(itr_begin), is_ascending>(db, id, itr_begin);

         content_summary content;
         const auto& idx_account = db.get_index_type<account_index>().indices().get<graphene::db::by_id>();

         ContentObjectTypeValue filter_type;
         filter_type.from_string(type);

         while(count && itr_begin != itr_end)
         {
            const auto account_itr = idx_account.find(itr_begin->author);
            if ( (user.empty() || account_itr->name == user) &&
                 // this is going to be possible if a content object does not have
                 // a price defined for this region
                 // we allow such objects be placed in db index anyway, but simply skip those
                 // during enumeration
                 (itr_begin->price.Valid(region_code)) &&
                 ( !user.empty() || ( itr_begin->seeder_price.empty() || itr_begin->recent_proof(60*60*24) ) ) &&
                 // Content can be cancelled by an author. In such a case content is not available to purchase.
                 ( ! itr_begin->is_blocked )
               )
            {
               content.set( *itr_begin , *account_itr, region_code );
               if (content.expiration > fc::time_point::now())
               {
                  std::string term = search_term;
                  std::string title = content.synopsis;
                  std::string desc;
                  std::string author = content.author;
                  ContentObjectTypeValue content_type;


                  try {
                     ContentObjectPropertyManager synopsis_parser(content.synopsis);
                     title = synopsis_parser.get<ContentObjectTitle>();
                     desc = synopsis_parser.get<ContentObjectDescription>();
                     content_type = synopsis_parser.get<ContentObjectType>();
                  } catch (...) {}

                  boost::algorithm::to_lower(term);
                  boost::algorithm::to_lower(title);
                  boost::algorithm::to_lower(desc);
                  boost::algorithm::to_lower(author);

                  if ( (term.empty() || author.find(term) != std::string::npos || title.find(term) != std::string::npos || desc.find(term) != std::string::npos) &&
                       (content_type.filter(filter_type))
                     )
                  {
                     count--;
                     result.push_back( content );
                  }
               }
            }

            ++itr_begin;
         }

      }
   }


   vector<content_summary> database_api_impl::search_content(const string& search_term,
                                                             const string& order,
                                                             const string& user,
                                                             const string& region_code,
                                                             const object_id_type& id,
                                                             const string& type,
                                                             uint32_t count)const
   {
      if(count > CURRENT_OUTPUT_LIMIT_100)
         FC_THROW_EXCEPTION(limit_exceeded_exception, "Current limit: ${i}", ("l", CURRENT_OUTPUT_LIMIT_100));

      vector<content_summary> result;
      result.reserve( count );

      if (order == "+author")
         search_content_template<true, by_author>(_db, search_term, count, user, region_code, id, type, result);
      else if (order == "+rating")
         search_content_template<true, by_AVG_rating>(_db, search_term, count, user, region_code, id, type, result);
      else if (order == "+size")
         search_content_template<true, by_size>(_db, search_term, count, user, region_code, id, type, result);
      else if (order == "+price")
         search_content_template<true, by_price>(_db, search_term, count, user, region_code, id, type, result);
      else if (order == "+created")
         search_content_template<true, by_created>(_db, search_term, count, user, region_code, id, type, result);
      else if (order == "+expiration")
         search_content_template<true, by_expiration>(_db, search_term, count, user, region_code, id, type, result);

      else if (order == "-author")
         search_content_template<false, by_author>(_db, search_term, count, user, region_code, id, type, result);
      else if (order == "-rating")
         search_content_template<false, by_AVG_rating>(_db, search_term, count, user, region_code, id, type, result);
      else if (order == "-size")
         search_content_template<false, by_size>(_db, search_term, count, user, region_code, id, type, result);
      else if (order == "-price")
         search_content_template<false, by_price>(_db, search_term, count, user, region_code, id, type, result);
      else if (order == "-expiration")
         search_content_template<false, by_expiration>(_db, search_term, count, user, region_code, id, type, result);
      else// if (order == "-created")
         search_content_template<false, by_created>(_db, search_term, count, user, region_code, id, type, result);

      return result;
   }

   vector<seeder_object> database_api::list_seeders_by_price( uint32_t count )const
   {
      return my->list_seeders_by_price( count );
   }
   vector<seeder_object> database_api::list_publishers_by_price( uint32_t count )const
   {
      return my->list_seeders_by_price( count );
   }

   vector<seeder_object> database_api_impl::list_seeders_by_price( uint32_t count )const
   {
      if(count > CURRENT_OUTPUT_LIMIT_100)
         FC_THROW_EXCEPTION(limit_exceeded_exception, "Current limit: ${i}", ("l", CURRENT_OUTPUT_LIMIT_100));

      const auto& idx = _db.get_index_type<seeder_index>().indices().get<by_price>();
      time_point_sec now = _db.head_block_time();
      vector<seeder_object> result;
      result.reserve(count);

      auto itr = idx.begin();

      while(count-- && itr != idx.end())
      {
         if( itr->expiration >= now )
            result.emplace_back(*itr);
         else
            ++count;
         ++itr;
      }

      return result;
   }

   vector<seeder_object> database_api::list_seeders_by_upload( uint32_t count )const
   {
      return my->list_seeders_by_upload( count );
   }

   vector<seeder_object> database_api_impl::list_seeders_by_upload( uint32_t count )const
   {
      if(count > CURRENT_OUTPUT_LIMIT_100)
         FC_THROW_EXCEPTION(limit_exceeded_exception, "Current limit: ${i}", ("l", CURRENT_OUTPUT_LIMIT_100));

      const auto& idx = _db.get_index_type<seeding_statistics_index>().indices().get<by_upload>();
      const auto& idx2 = _db.get_index_type<seeder_index>().indices().get<by_seeder>();
      vector<seeder_object> result;
      result.reserve(count);

      auto itr = idx.begin();

      while(count-- && itr != idx.end())
      {
         const auto& so_itr = idx2.find(itr->seeder);
         result.emplace_back(*so_itr);
         ++itr;
      }

      return result;
   }

   vector<seeder_object> database_api::list_seeders_by_region( const string region_code )const
   {
      return my->list_seeders_by_region( region_code );
   }

   vector<seeder_object> database_api_impl::list_seeders_by_region( const string region_code )const
   {
      const auto& range = _db.get_index_type<seeder_index>().indices().get<by_region>().equal_range( region_code );
      vector<seeder_object> result;

      time_point_sec now = head_block_time();
      auto itr = range.first;

      while(itr != range.second )
      {
         if( itr->expiration > now )
            result.emplace_back(*itr);
         ++itr;
      }

      return result;
   }

   vector<seeder_object> database_api::list_seeders_by_rating( uint32_t count )const
   {
      return my->list_seeders_by_rating( count );
   }

   vector<seeder_object> database_api_impl::list_seeders_by_rating( uint32_t count )const
   {
     if(count > CURRENT_OUTPUT_LIMIT_100)
         FC_THROW_EXCEPTION(limit_exceeded_exception, "Current limit: ${i}", ("l", CURRENT_OUTPUT_LIMIT_100));

      const auto& idx = _db.get_index_type<seeder_index>().indices().get<by_rating>();
      time_point_sec now = _db.head_block_time();
      vector<seeder_object> result;
      result.reserve(count);

      auto itr = idx.begin();

      while(count-- && itr != idx.end())
      {
         if( itr->expiration > now )
            result.emplace_back(*itr);
         else
            ++count;
         ++itr;
      }

      return result;
   }

   miner_reward_input database_api_impl::get_time_to_maint_by_block_time(fc::time_point_sec block_time) const
   {
      const auto& idx = _db.get_index_type<budget_record_index>().indices().get<by_time>();
      FC_ASSERT(idx.crbegin()->record.next_maintenance_time > block_time);
      graphene::chain::miner_reward_input miner_reward_input;

      fc::time_point_sec next_time = (fc::time_point_sec)0;
      for (auto itr = idx.cbegin(), itr_stop = idx.cend(); itr != itr_stop && (next_time == (fc::time_point_sec)0); ++itr )
      {
         if (itr->record.next_maintenance_time > block_time)
         {
            next_time = itr->record.next_maintenance_time;
            miner_reward_input.from_accumulated_fees = itr->record.from_accumulated_fees;
            miner_reward_input.block_interval = itr->record.block_interval;
         }
      }

      FC_ASSERT(next_time != (fc::time_point_sec)0);
      miner_reward_input.time_to_maint = (next_time - block_time).to_seconds();
      return miner_reward_input;
   }

   share_type database_api_impl::get_miner_pay_from_fees_by_block_time(fc::time_point_sec block_time) const
   {
      const auto& idx = _db.get_index_type<budget_record_index>().indices().get<by_time>();
      FC_ASSERT(idx.crbegin()->record.next_maintenance_time > block_time);
      graphene::chain::miner_reward_input miner_reward_input;

      fc::time_point_sec next_time = (fc::time_point_sec)0;
      fc::time_point_sec prev_time = (fc::time_point_sec)0;

      auto itr = idx.cbegin();
      for (auto itr_stop = idx.cend(); itr != itr_stop && (next_time == (fc::time_point_sec)0); ++itr)
      {
         if (itr->record.next_maintenance_time > block_time)
         {
            next_time = itr->record.next_maintenance_time;
            miner_reward_input.from_accumulated_fees = itr->record.from_accumulated_fees;
            miner_reward_input.block_interval = itr->record.block_interval;
         }
      }

      FC_ASSERT(next_time != (fc::time_point_sec)0);

      itr--;

      if (itr == idx.begin())
      {
         fc::optional<signed_block> first_block = get_block(1);
         prev_time = first_block->timestamp;
         miner_reward_input.time_to_maint = (next_time - prev_time).to_seconds();
      }
      else
      {
         itr--;

         prev_time = (*itr).record.next_maintenance_time;
         miner_reward_input.time_to_maint = (next_time - prev_time).to_seconds();
      }

      auto blocks_in_interval = (miner_reward_input.time_to_maint + miner_reward_input.block_interval - 1) / miner_reward_input.block_interval;
      return blocks_in_interval > 0 ? miner_reward_input.from_accumulated_fees / blocks_in_interval : 0;
   }

   //////////////////////////////////////////////////////////////////////
   //                                                                  //
   // Private methods                                                  //
   //                                                                  //
   //////////////////////////////////////////////////////////////////////

   void database_api_impl::on_objects_changed(const vector<object_id_type>& ids)
   {
      vector<variant>    updates;
      vector< string > content_update_queue;

      for(auto id : ids)
      {
         const graphene::db::object* obj = nullptr;
         if( _subscribe_callback )
         {
            obj = _db.find_object( id );
            if( obj )
            {
               updates.emplace_back( obj->to_variant() );
            }
            else
            {
               updates.emplace_back(id); // send just the id to indicate removal
            }
         }

         if( _content_subscriptions.size() )
         {
            if( !_subscribe_callback )
               obj = _db.find_object( id );
            if( obj )
            {
               const content_object* content = dynamic_cast<const content_object*>(obj);
               if(content && _content_subscriptions[ content->URI ] )
                  content_update_queue.emplace_back( content->URI );
            }

         }
      }

      auto capture_this = shared_from_this();

      /// pushing the future back / popping the prior future if it is complete.
      /// if a connection hangs then this could get backed up and result in
      /// a failure to exit cleanly.
      fc::async([capture_this,this,updates, content_update_queue](){
         if( _subscribe_callback ) _subscribe_callback( updates );

         for( const auto& item: content_update_queue )
         {
            _content_subscriptions[ item ]( );
            _content_subscriptions.erase( item );
         }
      });
   }

   /** note: this method cannot yield because it is called in the middle of
    * apply a block.
    */
   void database_api_impl::on_applied_block()
   {
      if (_block_applied_callback)
      {
         auto capture_this = shared_from_this();
         block_id_type block_id = _db.head_block_id();
         fc::async([this,capture_this,block_id](){
            _block_applied_callback(fc::variant(block_id));
         });
      }
   }

   share_type database_api::get_new_asset_per_block()const
   {
      return my->get_new_asset_per_block();
   }

   share_type database_api_impl::get_new_asset_per_block() const
   {
      return _db.get_new_asset_per_block();
   }

   share_type database_api::get_asset_per_block_by_block_num(uint32_t block_num) const
   {
      return my->get_asset_per_block_by_block_num(block_num);
   }

   share_type database_api_impl::get_asset_per_block_by_block_num(uint32_t block_num) const
   {
      return _db.get_asset_per_block_by_block_num(block_num);
   }

   miner_reward_input database_api::get_time_to_maint_by_block_time(fc::time_point_sec block_time) const
   {
      return my->get_time_to_maint_by_block_time(block_time);
   }

   share_type database_api::get_miner_pay_from_fees_by_block_time(fc::time_point_sec block_time) const
   {
      return my->get_miner_pay_from_fees_by_block_time(block_time);
   }

   vector<database::votes_gained> database_api::get_actual_votes() const{
      return my->_db.get_actual_votes();
   }

} } // graphene::app
