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

   const int CURRENT_OUTPUT_LIMIT_1000 = 1000;
   const int CURRENT_OUTPUT_LIMIT_100 = 100;

   void content_summary::set(const chain::content_object& co, const chain::account_object& ao, const std::string& region_code)
   {
      id = std::string(co.id);
      author = ao.name;
      auto it = chain::RegionCodes::s_mapNameToCode.find(region_code);
      FC_ASSERT(it != chain::RegionCodes::s_mapNameToCode.end());
      fc::optional<chain::asset> op_price = co.price.GetPrice(it->second);
      FC_ASSERT(op_price.valid());
      price = *op_price;

      synopsis = co.synopsis;
      URI = co.URI;
      AVG_rating = co.AVG_rating;
      _hash = co._hash;
      size = co.size;
      expiration = co.expiration;
      created = co.created;
      times_bought = co.times_bought;
      if(co.last_proof.size() >= co.quorum)
         status = "Uploaded";
      else if( co.last_proof.size() > 0 )
         status = "Partially uploaded";
      else
         status = "Uploading";
      if(co.expiration <= fc::time_point::now() )
         status = "Expired";
   }

   class database_api_impl : public std::enable_shared_from_this<database_api_impl>
   {
   public:
      database_api_impl( chain::database& db );
      ~database_api_impl();

      // Objects
      fc::variants get_objects(const std::vector<db::object_id_type>& ids) const;

      // Subscriptions
      void set_content_update_callback( const std::string & URI, std::function<void()> cb );
      void set_pending_transaction_callback( std::function<void(const fc::variant&)> cb );
      void set_block_applied_callback( std::function<void(const fc::variant& block_id)> cb );

      // Blocks and transactions
      fc::optional<chain::block_header> get_block_header(uint32_t block_num) const;
      std::vector<fc::optional<chain::block_header>> get_block_headers(uint32_t block_num, uint32_t count) const;
      fc::optional<chain::signed_block> get_block(uint32_t block_num) const;
      std::vector<fc::optional<chain::signed_block>> get_blocks(uint32_t block_num, uint32_t count) const;
      chain::processed_transaction get_transaction(uint32_t block_num, uint32_t trx_in_block) const;
      fc::time_point_sec head_block_time() const;
      chain::miner_reward_input get_time_to_maint_by_block_time(fc::time_point_sec block_time) const;
      chain::share_type get_miner_pay_from_fees_by_block_time(fc::time_point_sec block_time) const;
      fc::optional<chain::processed_transaction> get_transaction_by_id(chain::transaction_id_type id) const;
      std::vector<chain::proposal_object> get_proposed_transactions(chain::account_id_type id) const;

      // Globals
      chain::chain_property_object get_chain_properties() const;
      chain::global_property_object get_global_properties() const;
      chain::chain_id_type get_chain_id() const;
      chain::dynamic_global_property_object get_dynamic_global_properties() const;
      std::vector<operation_info> list_operations() const;

      // Keys
      std::vector<std::vector<chain::account_id_type>> get_key_references(const std::vector<chain::public_key_type>& keys) const;

      // Accounts
      std::vector<fc::optional<chain::account_object>> get_accounts(const std::vector<chain::account_id_type>& account_ids) const;
      std::map<std::string, full_account> get_full_accounts(const std::vector<std::string>& names_or_ids, bool subscribe) const;
      fc::optional<chain::account_object> get_account_by_name(const std::string& name) const;
      std::vector<chain::account_id_type> get_account_references(chain::account_id_type account_id) const;
      std::vector<fc::optional<chain::account_object>> lookup_account_names(const std::vector<std::string>& account_names) const;
      std::map<std::string, chain::account_id_type> lookup_accounts(const std::string& lower_bound_name, uint32_t limit) const;
      std::vector<chain::account_object> search_accounts(const std::string& search_term, const std::string& order, db::object_id_type id, uint32_t limit) const;
      std::vector<fc::optional<chain::account_statistics_object>> get_account_statistics(const std::vector<chain::account_statistics_id_type>& account_statistics_ids) const;
      std::vector<chain::transaction_detail_object> search_account_history(chain::account_id_type account, const std::string& order, db::object_id_type id, int limit) const;
      uint64_t get_account_count() const;

      // Balances
      std::vector<chain::asset> get_account_balances(chain::account_id_type id, const boost::container::flat_set<chain::asset_id_type>& assets) const;
      std::vector<chain::asset> get_named_account_balances(const std::string& name, const boost::container::flat_set<chain::asset_id_type>& assets) const;
      std::vector<chain::vesting_balance_object> get_vesting_balances(chain::account_id_type account_id) const;
      std::map<chain::non_fungible_token_id_type, uint32_t> get_non_fungible_token_summary(chain::account_id_type account_id) const;
      std::vector<chain::non_fungible_token_data_object> get_non_fungible_token_balances(chain::account_id_type account_id, const std::set<chain::non_fungible_token_id_type>& ids) const;

      // Assets
      uint64_t get_asset_count()const;
      std::vector<fc::optional<chain::asset_object>> get_assets(const std::vector<chain::asset_id_type>& asset_ids) const;
      std::vector<chain::asset_object> list_assets(const std::string& lower_bound_symbol, uint32_t limit) const;
      std::vector<fc::optional<chain::asset_object>> lookup_asset_symbols(const std::vector<std::string>& symbols_or_ids) const;
      chain::share_type get_new_asset_per_block() const;
      chain::share_type get_asset_per_block_by_block_num(uint32_t block_num) const;
      std::vector<fc::optional<chain::asset_dynamic_data_object>> get_asset_dynamic_data(const std::vector<chain::asset_dynamic_data_id_type>& asset_dynamic_data_ids) const;
      chain::asset price_to_dct(const chain::asset& price) const;

      // Non Fungible Tokens
      uint64_t get_non_fungible_token_count() const;
      std::vector<fc::optional<chain::non_fungible_token_object>> get_non_fungible_tokens(const std::vector<chain::non_fungible_token_id_type>& nft_ids) const;
      std::vector<chain::non_fungible_token_object> list_non_fungible_tokens(const std::string& lower_bound_symbol, uint32_t limit) const;
      std::vector<fc::optional<chain::non_fungible_token_object>> get_non_fungible_tokens_by_symbols(const std::vector<std::string>& symbols) const;
      uint64_t get_non_fungible_token_data_count() const;
      std::vector<fc::optional<chain::non_fungible_token_data_object>> get_non_fungible_token_data(const std::vector<chain::non_fungible_token_data_id_type>& nft_data_ids) const;
      std::vector<chain::non_fungible_token_data_object> list_non_fungible_token_data(chain::non_fungible_token_id_type nft_id) const;
      chain::void_t burn_non_fungible_token_data(chain::non_fungible_token_data_id_type nft_data_id) const;
      std::vector<chain::transaction_detail_object> search_non_fungible_token_history(chain::non_fungible_token_data_id_type nft_data_id) const;

      // Miners
      std::vector<fc::optional<chain::miner_object>> get_miners(const std::vector<chain::miner_id_type>& miner_ids) const;
      fc::optional<chain::miner_object> get_miner_by_account(chain::account_id_type account) const;
      std::map<std::string, chain::miner_id_type> lookup_miner_accounts(const std::string& lower_bound_name, uint32_t limit) const;
      uint64_t get_miner_count() const;
      std::multimap<fc::time_point_sec, chain::price_feed> get_feeds_by_miner(chain::account_id_type account_id, uint32_t count) const;

      // Votes
      std::vector<fc::optional<chain::miner_object>> lookup_vote_ids(const std::vector<chain::vote_id_type>& votes) const;
      std::vector<miner_voting_info> search_miner_voting(const std::string& account_id, const std::string& term, bool only_my_votes, const std::string& order, const std::string& id, uint32_t count) const;

      // Authority / validation
      std::string get_transaction_hex(const chain::signed_transaction& trx) const;
      std::set<chain::public_key_type> get_required_signatures(const chain::signed_transaction& trx, const boost::container::flat_set<chain::public_key_type>& available_keys) const;
      std::set<chain::public_key_type> get_potential_signatures(const chain::signed_transaction& trx) const;
      bool verify_authority(const chain::signed_transaction& trx) const;
      bool verify_account_authority(const std::string& name_or_id, const boost::container::flat_set<chain::public_key_type>& signers) const;
      chain::processed_transaction validate_transaction(const chain::signed_transaction& trx) const;
      fc::variants get_required_fees(std::vector<chain::operation> ops, chain::asset_id_type id) const;

      // Content
      std::vector<chain::account_id_type> list_publishing_managers(const std::string& lower_bound_name, uint32_t limit) const;
      std::vector<chain::buying_object> get_open_buyings() const;
      std::vector<chain::buying_object> get_open_buyings_by_URI(const std::string& URI) const;
      std::vector<chain::buying_object> get_open_buyings_by_consumer(chain::account_id_type consumer) const;
      fc::optional<chain::buying_object> get_buying_by_consumer_URI(chain::account_id_type consumer, const std::string& URI) const;
      std::vector<chain::buying_object> get_buying_history_objects_by_consumer(chain::account_id_type consumer ) const;
      std::vector<chain::buying_object> get_buying_objects_by_consumer(chain::account_id_type consumer, const std::string& order, db::object_id_type id, const std::string& term, uint32_t count) const;
      std::vector<chain::buying_object> search_feedback(const std::string& user, const std::string& URI, db::object_id_type id, uint32_t count) const;
      fc::optional<chain::content_object> get_content(const std::string& URI) const;
      std::vector<content_summary> search_content(const std::string& term, const std::string& order, const std::string& user, const std::string& region_code, db::object_id_type id, const std::string& type, uint32_t count) const;
      std::vector<chain::seeder_object> list_seeders_by_price(uint32_t count) const;
      fc::optional<chain::seeder_object> get_seeder(chain::account_id_type account ) const;
      std::vector<chain::seeder_object> list_seeders_by_region(const std::string& region_code) const;
      std::vector<chain::seeder_object> list_seeders_by_rating(uint32_t count) const;
      std::vector<chain::subscription_object> list_active_subscriptions_by_consumer(chain::account_id_type account, uint32_t count) const;
      std::vector<chain::subscription_object> list_subscriptions_by_consumer(chain::account_id_type account, uint32_t count) const;
      std::vector<chain::subscription_object> list_active_subscriptions_by_author(chain::account_id_type account, uint32_t count) const;
      std::vector<chain::subscription_object> list_subscriptions_by_author(chain::account_id_type account, uint32_t count) const;
      fc::optional<chain::subscription_object> get_subscription(chain::subscription_id_type sid) const;

      //private:

      /** called every time a block is applied to report the objects that were changed */
      void on_objects_changed(const std::vector<db::object_id_type>& ids, bool sync_mode);
      void on_applied_block();

      std::function<void(const fc::variant&)> _pending_trx_callback;
      std::function<void(const fc::variant&)> _block_applied_callback;

      boost::signals2::scoped_connection   _change_connection;
      boost::signals2::scoped_connection   _applied_block_connection;
      boost::signals2::scoped_connection   _pending_trx_connection;
      std::map<std::string, std::function<void()> > _content_subscriptions;
      chain::database& _db;
   };

   //////////////////////////////////////////////////////////////////////
   //                                                                  //
   // Constructors                                                     //
   //                                                                  //
   //////////////////////////////////////////////////////////////////////

   database_api::database_api( chain::database& db )
   : my( new database_api_impl( db ) ) {}

   database_api::~database_api() {}

   database_api_impl::database_api_impl( chain::database& db ):_db(db)
   {
      dlog("creating database api ${x}", ("x",int64_t(this)) );
      _change_connection = _db.changed_objects.connect([this](const std::vector<db::object_id_type>& ids, bool sync_mode){ on_objects_changed(ids, sync_mode); });
      _applied_block_connection = _db.applied_block.connect([this](const chain::signed_block&){ on_applied_block(); });

      _pending_trx_connection = _db.on_pending_transaction.connect([this](const chain::signed_transaction& trx ){
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

   fc::variants database_api::get_objects(const std::vector<db::object_id_type>& ids) const
   {
      return my->get_objects( ids );
   }

   fc::variants database_api_impl::get_objects(const std::vector<db::object_id_type>& ids) const
   {
      fc::variants result;
      result.reserve(ids.size());

      std::transform(ids.begin(), ids.end(), std::back_inserter(result),
                     [this](db::object_id_type id) -> fc::variant {
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

   void database_api::set_content_update_callback( std::function<void()> cb, const std::string & URI )
   {
      my->set_content_update_callback(URI, cb);
   }

   void database_api_impl::set_content_update_callback( const std::string & URI, std::function<void()> cb )
   {
      _content_subscriptions[ URI ] = cb;
   }

   void database_api::set_pending_transaction_callback( std::function<void(const fc::variant&)> cb )
   {
      my->set_pending_transaction_callback( cb );
   }

   void database_api_impl::set_pending_transaction_callback( std::function<void(const fc::variant&)> cb )
   {
      _pending_trx_callback = cb;
   }

   void database_api::set_block_applied_callback( std::function<void(const fc::variant& block_id)> cb )
   {
      my->set_block_applied_callback( cb );
   }

   void database_api_impl::set_block_applied_callback( std::function<void(const fc::variant& block_id)> cb )
   {
      _block_applied_callback = cb;
   }

   //////////////////////////////////////////////////////////////////////
   //                                                                  //
   // Blocks and transactions                                          //
   //                                                                  //
   //////////////////////////////////////////////////////////////////////

   fc::optional<chain::block_header> database_api::get_block_header(uint32_t block_num) const
   {
      return my->get_block_header(block_num);
   }

   fc::optional<chain::block_header> database_api_impl::get_block_header(uint32_t block_num) const
   {
      auto result = _db.fetch_block_by_number(block_num);
      if(result)
         return *result;
      return {};
   }

   std::vector<fc::optional<chain::block_header>> database_api::get_block_headers(uint32_t block_num, uint32_t count) const
   {
      return my->get_block_headers(block_num, count);
   }

   std::vector<fc::optional<chain::block_header>> database_api_impl::get_block_headers(uint32_t block_num, uint32_t count) const
   {
      std::vector<fc::optional<chain::block_header>> headers;
      headers.reserve(count);
      uint32_t end_block = block_num + count;
      while(block_num < end_block)
         headers.push_back(get_block_header(block_num++));
      return headers;
   }

   fc::optional<chain::signed_block_with_info> database_api::get_block(uint32_t block_num) const
   {
      auto block = my->get_block(block_num);
      if( !block )
         return {};

      return my->_db.get_signed_block_with_info(*block);
   }

   fc::optional<chain::signed_block> database_api_impl::get_block(uint32_t block_num) const
   {
      return _db.fetch_block_by_number(block_num);
   }

   std::vector<fc::optional<chain::signed_block_with_info>> database_api::get_blocks(uint32_t block_num, uint32_t count) const
   {
      auto blocks = my->get_blocks(block_num, count);
      std::vector<fc::optional<chain::signed_block_with_info>> result;
      result.reserve(blocks.size());
      for( const auto& block : blocks )
      {
         if( block )
            result.emplace_back(my->_db.get_signed_block_with_info(*block));
         else
            result.push_back({});
      }

      return result;
   }

   std::vector<fc::optional<chain::signed_block>> database_api_impl::get_blocks(uint32_t block_num, uint32_t count) const
   {
      std::vector<fc::optional<chain::signed_block>> blocks;
      blocks.reserve(count);
      uint32_t end_block = block_num + count;
      while(block_num < end_block)
         blocks.push_back(get_block(block_num++));
      return blocks;
   }

   chain::processed_transaction database_api::get_transaction( uint32_t block_num, uint32_t trx_in_block ) const
   {
      return my->get_transaction( block_num, trx_in_block );
   }

   fc::time_point_sec database_api::head_block_time() const
   {
      return my->head_block_time();
   }

   chain::processed_transaction database_api_impl::get_transaction(uint32_t block_num, uint32_t trx_num) const
   {
      auto opt_block = _db.fetch_block_by_number(block_num);
      FC_VERIFY_AND_THROW(opt_block.valid(), block_not_found_exception, "Block number: ${bn}", ("bn", block_num));
      FC_VERIFY_AND_THROW(opt_block->transactions.size() > trx_num, block_does_not_contain_requested_trx_exception, "Block number: ${bn} transaction index: ${ti}", ("bn", block_num)("ti", trx_num));
      return opt_block->transactions[trx_num];
   }

   fc::time_point_sec database_api_impl::head_block_time() const
   {
      return _db.head_block_time();
   }

   fc::optional<chain::processed_transaction> database_api::get_transaction_by_id(chain::transaction_id_type id) const
   {
      return my->get_transaction_by_id( id );
   }

   fc::optional<chain::processed_transaction> database_api_impl::get_transaction_by_id(chain::transaction_id_type id) const
   {
      const auto& idx = _db.get_index_type<chain::transaction_history_index>().indices().get<chain::by_tx_id>();
      auto itr = idx.find(id);
      if (itr != idx.end())
      {
         return get_transaction( itr->block_num, itr->trx_in_block );
      }

      return {};
   }

   chain::transaction_id_type database_api::get_transaction_id(const chain::signed_transaction& trx) const
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

   chain::chain_property_object database_api::get_chain_properties() const
   {
      return my->get_chain_properties();
   }

   chain::chain_property_object database_api_impl::get_chain_properties() const
   {
      return _db.get(chain::chain_property_id_type());
   }

   chain::global_property_object database_api::get_global_properties() const
   {
      return my->get_global_properties();
   }

   chain::global_property_object database_api_impl::get_global_properties() const
   {
      return _db.get(chain::global_property_id_type());
   }

   chain::configuration database_api::get_configuration() const
   {
      return chain::get_configuration();
   }

   chain::chain_id_type database_api::get_chain_id() const
   {
      return my->get_chain_id();
   }

   chain::chain_id_type database_api_impl::get_chain_id() const
   {
      return _db.get_chain_id();
   }

   chain::dynamic_global_property_object database_api::get_dynamic_global_properties() const
   {
      return my->get_dynamic_global_properties();
   }

   chain::dynamic_global_property_object database_api_impl::get_dynamic_global_properties() const
   {
      return _db.get(chain::dynamic_global_property_id_type());
   }

   decent::about_info database_api::about() const
   {
      return decent::get_about_info();
   }

   //////////////////////////////////////////////////////////////////////
   //                                                                  //
   // Keys                                                             //
   //                                                                  //
   //////////////////////////////////////////////////////////////////////

   std::vector<std::vector<chain::account_id_type>> database_api::get_key_references(const std::vector<chain::public_key_type>& keys) const
   {
      return my->get_key_references( keys );
   }

   /**
    *  @return all accounts that refer to the key or account id in their owner or active authorities.
    */
   std::vector<std::vector<chain::account_id_type>> database_api_impl::get_key_references(const std::vector<chain::public_key_type>& keys) const
   {
      ddump( (keys) );
      std::vector<std::vector<chain::account_id_type>> final_result;
      final_result.reserve(keys.size());

      for( auto& key : keys )
      {
         const auto& idx = _db.get_index_type<chain::account_index>();
         const auto& aidx = dynamic_cast<const db::primary_index<chain::account_index>&>(idx);
         const auto& refs = aidx.get_secondary_index<chain::account_member_index>();
         auto itr = refs.account_to_key_memberships.find(key);
         std::vector<chain::account_id_type> result;

         if( itr != refs.account_to_key_memberships.end() )
         {
            result.reserve( itr->second.size() );
            for( auto item : itr->second ) result.push_back(item);
         }
         final_result.emplace_back( std::move(result) );
      }

      return final_result;
   }

   //////////////////////////////////////////////////////////////////////
   //                                                                  //
   // Accounts                                                         //
   //                                                                  //
   //////////////////////////////////////////////////////////////////////

   std::vector<fc::optional<chain::account_object>> database_api::get_accounts(const std::vector<chain::account_id_type>& account_ids) const
   {
      return my->get_accounts( account_ids );
   }

   std::vector<fc::optional<chain::account_object>> database_api_impl::get_accounts(const std::vector<chain::account_id_type>& account_ids) const
   {
      return _db.get_objects(account_ids);
   }

   std::map<std::string, full_account> database_api::get_full_accounts(const std::vector<std::string>& names_or_ids, bool subscribe) const
   {
      return my->get_full_accounts( names_or_ids, subscribe );
   }

   std::map<std::string, full_account> database_api_impl::get_full_accounts(const std::vector<std::string>& names_or_ids, bool subscribe) const
   {
      if( subscribe )
         ilog( "subscribe callback functionality has been removed and 'subscribe' parameter has no effect" );

      ddump((names_or_ids));
      std::map<std::string, full_account> results;

      for (const std::string& account_name_or_id : names_or_ids)
      {
         const chain::account_object* account = nullptr;
         if (std::isdigit(account_name_or_id[0]))
            account = _db.find(fc::variant(account_name_or_id).as<chain::account_id_type>());
         else
         {
            const auto& idx = _db.get_index_type<chain::account_index>().indices().get<chain::by_name>();
            auto itr = idx.find(account_name_or_id);
            if (itr != idx.end())
               account = &*itr;
         }
         if (account != nullptr)
         {
            // fc::mutable_variant_object full_account;
            full_account acnt;
            acnt.account = *account;
            acnt.statistics = account->statistics(_db);
            acnt.registrar_name = account->registrar(_db).name;
            acnt.votes = lookup_vote_ids( std::vector<chain::vote_id_type>(account->options.votes.begin(), account->options.votes.end()) );

            // Add the account's proposals
            const auto& proposal_idx = _db.get_index_type<chain::proposal_index>();
            const auto& pidx = dynamic_cast<const db::primary_index<chain::proposal_index>&>(proposal_idx);
            const auto& proposals_by_account = pidx.get_secondary_index<chain::required_approval_index>();
            auto  required_approvals_itr = proposals_by_account._account_to_proposals.find( account->id );
            if( required_approvals_itr != proposals_by_account._account_to_proposals.end() )
            {
               acnt.proposals.reserve( required_approvals_itr->second.size() );
               for( auto proposal_id : required_approvals_itr->second )
                  acnt.proposals.push_back( proposal_id(_db) );
            }

            // Add the account's balances
            auto balance_range = _db.get_index_type<chain::account_balance_index>().indices().get<chain::by_account_asset>().equal_range(boost::make_tuple(account->id));
            //vector<account_balance_object> balances;
            std::for_each(balance_range.first, balance_range.second,
                          [&acnt](const chain::account_balance_object& balance) {
                             acnt.balances.emplace_back(balance);
                          });

            // Add the account's vesting balances
            auto vesting_range = _db.get_index_type<chain::vesting_balance_index>().indices().get<chain::by_account>().equal_range(account->id);
            std::for_each(vesting_range.first, vesting_range.second,
                          [&acnt](const chain::vesting_balance_object& balance) {
                             acnt.vesting_balances.emplace_back(balance);
                          });

            results[account_name_or_id] = acnt;

         }
      }
      return results;
   }

   fc::optional<chain::account_object> database_api::get_account_by_name(const std::string& name ) const
   {
      return my->get_account_by_name( name );
   }

   fc::optional<chain::account_object> database_api_impl::get_account_by_name(const std::string& name ) const
   {
      const auto& idx = _db.get_index_type<chain::account_index>().indices().get<chain::by_name>();
      auto itr = idx.find(name);
      if (itr != idx.end())
         return *itr;
      return {};
   }

   std::vector<chain::account_id_type> database_api::get_account_references(chain::account_id_type account_id) const
   {
      return my->get_account_references( account_id );
   }

   std::vector<chain::account_id_type> database_api_impl::get_account_references(chain::account_id_type account_id) const
   {
      const auto& idx = _db.get_index_type<chain::account_index>();
      const auto& aidx = dynamic_cast<const db::primary_index<chain::account_index>&>(idx);
      const auto& refs = aidx.get_secondary_index<chain::account_member_index>();
      auto itr = refs.account_to_account_memberships.find(account_id);
      std::vector<chain::account_id_type> result;

      if( itr != refs.account_to_account_memberships.end() )
      {
         result.reserve( itr->second.size() );
         for( auto item : itr->second ) result.push_back(item);
      }
      return result;
   }

   std::vector<fc::optional<chain::account_object>> database_api::lookup_account_names(const std::vector<std::string>& account_names) const
   {
      return my->lookup_account_names( account_names );
   }

   std::vector<fc::optional<chain::account_object>> database_api_impl::lookup_account_names(const std::vector<std::string>& account_names) const
   {
      const auto& accounts_by_name = _db.get_index_type<chain::account_index>().indices().get<chain::by_name>();
      std::vector<fc::optional<chain::account_object> > result;
      result.reserve(account_names.size());
      std::transform(account_names.begin(), account_names.end(), std::back_inserter(result),
                     [&accounts_by_name](const std::string& name) -> fc::optional<chain::account_object> {
                        auto itr = accounts_by_name.find(name);
                        return itr == accounts_by_name.end() ? fc::optional<chain::account_object>() : *itr;
                     });
      return result;
   }

   std::vector<chain::account_object> database_api::search_accounts(const std::string& search_term, const std::string& order, db::object_id_type id, uint32_t limit) const
   {
      return my->search_accounts( search_term, order, id, limit );
   }

   std::vector<chain::transaction_detail_object> database_api::search_account_history(chain::account_id_type account, const std::string& order, db::object_id_type id, int limit) const
   {
      return my->search_account_history(account, order, id, limit);
   }

   std::map<std::string, chain::account_id_type> database_api::lookup_accounts(const std::string& lower_bound_name, uint32_t limit) const
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
      void correct_iterator(chain::database& db,
                            db::object_id_type id,
                            _t_iterator& itr_begin)
      {
         const auto& idx_by_id = db.get_index_type<_t_object_index>().indices().template get<db::by_id>();
         auto itr_id = idx_by_id.find(id);

         const auto& idx_by_sort_tag = db.get_index_type<_t_object_index>().indices().template get<_t_sort_tag>();

         auto itr_find = idx_by_sort_tag.end();
         if (itr_id != idx_by_id.end())
            itr_find = idx_by_sort_tag.find(chain::key_extractor<_t_sort_tag, _t_object>::get(*itr_id));

         // itr_find has the same keys as the object with id
         // scan to next items until exactly the object with id is found
         auto itr_scan = itr_find;
         while (itr_find != idx_by_sort_tag.end() &&
                itr_id != idx_by_id.end() &&
                ++itr_scan != idx_by_sort_tag.end() &&
                itr_find->id != itr_id->id &&
                chain::key_extractor<_t_sort_tag, _t_object>::get(*itr_scan) == chain::key_extractor<_t_sort_tag, _t_object>::get(*itr_id))
            itr_find = itr_scan;

         if (itr_find != idx_by_sort_tag.end())
         {
            itr_begin = return_one<is_ascending>::choose(itr_find, boost::reverse_iterator<decltype(itr_find)>(itr_find));
            if (false == is_ascending)
               --itr_begin;
         }
      }

      template <bool is_ascending, class sort_tag>
      void search_accounts_template(chain::database& db, const std::string& term, uint32_t count, db::object_id_type id, std::vector<chain::account_object>& result)
      {
         const auto& idx_by_sort_tag = db.get_index_type<chain::account_index>().indices().get<sort_tag>();

         auto itr_begin = return_one<is_ascending>::choose(idx_by_sort_tag.cbegin(), idx_by_sort_tag.crbegin());
         auto itr_end = return_one<is_ascending>::choose(idx_by_sort_tag.end(), idx_by_sort_tag.rend());

         correct_iterator<chain::account_index, chain::account_object, sort_tag, decltype(itr_begin), is_ascending>(db, id, itr_begin);

         while(count &&
               itr_begin != itr_end)
         {
            const chain::account_object& element = *itr_begin;
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

   std::vector<chain::account_object> database_api_impl::search_accounts(const std::string& term, const std::string& order, db::object_id_type id, uint32_t limit) const
   {
      FC_VERIFY_AND_THROW(limit <= CURRENT_OUTPUT_LIMIT_1000, limit_exceeded_exception, "Current limit: ${l}", ("l", CURRENT_OUTPUT_LIMIT_1000));
      std::vector<chain::account_object> result;

      if (order == "+id")
         search_accounts_template<true, db::by_id>(_db, term, limit, id, result);
      else if (order == "-id")
         search_accounts_template<false, db::by_id>(_db, term, limit, id, result);
      else if (order == "-name")
         search_accounts_template<false, chain::by_name>(_db, term, limit, id, result);
      else
         search_accounts_template<true, chain::by_name>(_db, term, limit, id, result);

      return result;
   }

   namespace
   {
      template <bool is_ascending, class sort_tag>
      void search_account_history_template(chain::database& db, chain::account_id_type account, uint32_t count, db::object_id_type id, std::vector<chain::transaction_detail_object>& result)
      {
         const auto& idx_by_sort_tag = db.get_index_type<chain::transaction_detail_index>().indices().get<sort_tag>();

         auto itr_begin = return_one<is_ascending>::choose(idx_by_sort_tag.cbegin(), idx_by_sort_tag.crbegin());
         auto itr_end = return_one<is_ascending>::choose(idx_by_sort_tag.end(), idx_by_sort_tag.rend());

         correct_iterator<chain::transaction_detail_index, chain::transaction_detail_object, sort_tag, decltype(itr_begin), is_ascending>(db, id, itr_begin);

         while(count &&
               itr_begin != itr_end)
         {
            const chain::transaction_detail_object& element = *itr_begin;
            ++itr_begin;

            if (account == element.m_from_account || account == element.m_to_account)
            {
               result.emplace_back(element);
               --count;
            }
         }
      }
   }

   std::vector<fc::optional<chain::account_statistics_object>> database_api::get_account_statistics(const std::vector<chain::account_statistics_id_type>& account_statistics_ids) const
   {
      return my->get_account_statistics( account_statistics_ids );
   }

   std::vector<fc::optional<chain::account_statistics_object>> database_api_impl::get_account_statistics(const std::vector<chain::account_statistics_id_type>& account_statistics_ids) const
   {
      return _db.get_objects(account_statistics_ids);
   }

   std::vector<chain::transaction_detail_object> database_api_impl::search_account_history(chain::account_id_type account, const std::string& order, db::object_id_type id, int limit) const
   {
      std::vector<chain::transaction_detail_object> result;

      if (order == "+type")
         search_account_history_template<true, chain::by_operation_type>(_db, account, limit, id, result);
      else if (order == "-type")
         search_account_history_template<false, chain::by_operation_type>(_db, account, limit, id, result);
      else if (order == "+to")
         search_account_history_template<true, chain::by_to_account>(_db, account, limit, id, result);
      else if (order == "-to")
         search_account_history_template<false, chain::by_to_account>(_db, account, limit, id, result);
      else if (order == "+from")
         search_account_history_template<true, chain::by_from_account>(_db, account, limit, id, result);
      else if (order == "-from")
         search_account_history_template<false, chain::by_from_account>(_db, account, limit, id, result);
      else if (order == "+price")
         search_account_history_template<true, chain::by_transaction_amount>(_db, account, limit, id, result);
      else if (order == "-price")
         search_account_history_template<false, chain::by_transaction_amount>(_db, account, limit, id, result);
      else if (order == "+fee")
         search_account_history_template<true, chain::by_transaction_fee>(_db, account, limit, id, result);
      else if (order == "-fee")
         search_account_history_template<false, chain::by_transaction_fee>(_db, account, limit, id, result);
      else if (order == "+nft")
         search_account_history_template<true, chain::by_nft>(_db, account, limit, id, result);
      else if (order == "-nft")
         search_account_history_template<false, chain::by_nft>(_db, account, limit, id, result);
      else if (order == "+description")
         search_account_history_template<true, chain::by_description>(_db, account, limit, id, result);
      else if (order == "-description")
         search_account_history_template<false, chain::by_description>(_db, account, limit, id, result);
      else if (order == "+time")
         search_account_history_template<true, chain::by_time>(_db, account, limit, id, result);
      else// if (order == "-time")
         search_account_history_template<false, chain::by_time>(_db, account, limit, id, result);

      return result;
   }

   std::map<std::string, chain::account_id_type> database_api_impl::lookup_accounts(const std::string& lower_bound_name, uint32_t limit) const
   {
      FC_VERIFY_AND_THROW(limit <= CURRENT_OUTPUT_LIMIT_1000, limit_exceeded_exception, "Current limit: ${l}", ("l", CURRENT_OUTPUT_LIMIT_1000));
      const auto& accounts_by_name = _db.get_index_type<chain::account_index>().indices().get<chain::by_name>();
      std::map<std::string, chain::account_id_type> result;

      for( auto itr = accounts_by_name.lower_bound(lower_bound_name);
          limit-- && itr != accounts_by_name.end();
          ++itr )
      {
         result.insert(std::make_pair(itr->name, itr->get_id()));
      }

      return result;
   }

   uint64_t database_api::get_account_count() const
   {
      return my->get_account_count();
   }

   uint64_t database_api_impl::get_account_count() const
   {
      return _db.get_index_type<chain::account_index>().indices().size();
   }

   //////////////////////////////////////////////////////////////////////
   //                                                                  //
   // Balances                                                         //
   //                                                                  //
   //////////////////////////////////////////////////////////////////////

   std::vector<chain::asset> database_api::get_account_balances(chain::account_id_type id, const boost::container::flat_set<chain::asset_id_type>& assets) const
   {
      return my->get_account_balances( id, assets );
   }

   std::vector<chain::asset> database_api_impl::get_account_balances(chain::account_id_type acnt, const boost::container::flat_set<chain::asset_id_type>& assets) const
   {
      std::vector<chain::asset> result;
      if (assets.empty())
      {
         // if the caller passes in an empty list of assets, return balances for all assets the account owns
         const chain::account_balance_index& balance_index = _db.get_index_type<chain::account_balance_index>();
         auto range = balance_index.indices().get<chain::by_account_asset>().equal_range(boost::make_tuple(acnt));
         for (const chain::account_balance_object& balance : boost::make_iterator_range(range.first, range.second))
            result.push_back(balance.get_balance());
      }
      else
      {
         result.reserve(assets.size());

         std::transform(assets.begin(), assets.end(), std::back_inserter(result),
                        [this, acnt](chain::asset_id_type id) { return _db.get_balance(acnt, id); });
      }

      return result;
   }

   std::vector<chain::asset> database_api::get_named_account_balances(const std::string& name, const boost::container::flat_set<chain::asset_id_type>& assets) const
   {
      return my->get_named_account_balances( name, assets );
   }

   std::vector<chain::asset> database_api_impl::get_named_account_balances(const std::string& name, const boost::container::flat_set<chain::asset_id_type>& assets) const
   {
      const auto& accounts_by_name = _db.get_index_type<chain::account_index>().indices().get<chain::by_name>();
      auto itr = accounts_by_name.find(name);
      FC_VERIFY_AND_THROW(itr != accounts_by_name.end(), account_does_not_exist_exception, "Account: ${account}", ("account", name));
      return get_account_balances(itr->get_id(), assets);
   }

   std::vector<chain::vesting_balance_object> database_api::get_vesting_balances(chain::account_id_type account_id) const
   {
      return my->get_vesting_balances( account_id );
   }

   std::vector<chain::vesting_balance_object> database_api_impl::get_vesting_balances(chain::account_id_type account_id) const
   {
      try
      {
         std::vector<chain::vesting_balance_object> result;
         auto vesting_range = _db.get_index_type<chain::vesting_balance_index>().indices().get<chain::by_account>().equal_range(account_id);
         std::for_each(vesting_range.first, vesting_range.second,
                       [&result](const chain::vesting_balance_object& balance) {
                          result.emplace_back(balance);
                       });
         return result;
      }
      FC_CAPTURE_AND_RETHROW( (account_id) );
   }

   std::map<chain::non_fungible_token_id_type, uint32_t> database_api::get_non_fungible_token_summary(chain::account_id_type account_id) const
   {
      return my->get_non_fungible_token_summary(account_id);
   }

   std::map<chain::non_fungible_token_id_type, uint32_t> database_api_impl::get_non_fungible_token_summary(chain::account_id_type account_id) const
   {
      const auto& nft_data_index = _db.get_index_type<chain::non_fungible_token_data_index>();
      auto nft_data_range = nft_data_index.indices().get<chain::by_account>().equal_range(account_id);

      std::map<chain::non_fungible_token_id_type, uint32_t> result;
      std::for_each(nft_data_range.first, nft_data_range.second, [&](const chain::non_fungible_token_data_object& nft_data) {
         auto ret = result.insert(std::make_pair(nft_data.nft_id, 1u));
         if(!ret.second)
            ++ret.first->second;
      });
      return result;
   }

   std::vector<chain::non_fungible_token_data_object> database_api::get_non_fungible_token_balances(chain::account_id_type account_id, const std::set<chain::non_fungible_token_id_type>& ids) const
   {
      return my->get_non_fungible_token_balances(account_id, ids);
   }

   std::vector<chain::non_fungible_token_data_object> database_api_impl::get_non_fungible_token_balances(chain::account_id_type account_id, const std::set<chain::non_fungible_token_id_type>& ids) const
   {
      const auto& nft_data_index = _db.get_index_type<chain::non_fungible_token_data_index>();
      auto nft_data_range = nft_data_index.indices().get<chain::by_account>().equal_range(account_id);

      std::vector<chain::non_fungible_token_data_object> result;
      std::for_each(nft_data_range.first, nft_data_range.second, [&](const chain::non_fungible_token_data_object& nft_data) {
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

   uint64_t database_api::get_asset_count() const
   {
      return my->get_asset_count();
   }

   uint64_t database_api_impl::get_asset_count() const
   {
      return _db.get_index_type<chain::asset_index>().indices().size();
   }

   std::vector<fc::optional<chain::asset_object>> database_api::get_assets(const std::vector<chain::asset_id_type>& asset_ids) const
   {
      return my->get_assets( asset_ids );
   }

   std::vector<fc::optional<chain::asset_object>> database_api_impl::get_assets(const std::vector<chain::asset_id_type>& asset_ids) const
   {
      return _db.get_objects(asset_ids);
   }

   std::vector<chain::asset_object> database_api::list_assets(const std::string& lower_bound_symbol, uint32_t limit) const
   {
      return my->list_assets( lower_bound_symbol, limit );
   }

   std::vector<chain::asset_object> database_api_impl::list_assets(const std::string& lower_bound_symbol, uint32_t limit) const
   {
      FC_VERIFY_AND_THROW(limit <= CURRENT_OUTPUT_LIMIT_100, limit_exceeded_exception, "Current limit: ${l}", ("l", CURRENT_OUTPUT_LIMIT_100));
      const auto& assets_by_symbol = _db.get_index_type<chain::asset_index>().indices().get<chain::by_symbol>();
      std::vector<chain::asset_object> result;
      result.reserve(limit);

      auto itr = assets_by_symbol.lower_bound(lower_bound_symbol);

      if( lower_bound_symbol == "" )
         itr = assets_by_symbol.begin();

      while(limit-- && itr != assets_by_symbol.end())
         result.emplace_back(*itr++);

      return result;
   }

   std::vector<fc::optional<chain::asset_object>> database_api::lookup_asset_symbols(const std::vector<std::string>& symbols_or_ids) const
   {
      return my->lookup_asset_symbols( symbols_or_ids );
   }

   std::vector<fc::optional<chain::asset_object>> database_api_impl::lookup_asset_symbols(const std::vector<std::string>& symbols_or_ids) const
   {
      const auto& assets_by_symbol = _db.get_index_type<chain::asset_index>().indices().get<chain::by_symbol>();
      std::vector<fc::optional<chain::asset_object> > result;
      result.reserve(symbols_or_ids.size());
      std::transform(symbols_or_ids.begin(), symbols_or_ids.end(), std::back_inserter(result),
                     [this, &assets_by_symbol](const std::string& symbol_or_id) -> fc::optional<chain::asset_object> {
                        if( !symbol_or_id.empty() && std::isdigit(symbol_or_id[0]) )
                        {
                           auto ptr = _db.find(fc::variant(symbol_or_id).as<chain::asset_id_type>());
                           return ptr == nullptr ? fc::optional<chain::asset_object>() : *ptr;
                        }
                        auto itr = assets_by_symbol.find(symbol_or_id);
                        return itr == assets_by_symbol.end() ? fc::optional<chain::asset_object>() : *itr;
                     });
      return result;
   }

   std::vector<fc::optional<chain::asset_dynamic_data_object>> database_api::get_asset_dynamic_data(const std::vector<chain::asset_dynamic_data_id_type>& asset_dynamic_data_ids) const
   {
      return my->get_asset_dynamic_data( asset_dynamic_data_ids );
   }

   std::vector<fc::optional<chain::asset_dynamic_data_object>> database_api_impl::get_asset_dynamic_data(const std::vector<chain::asset_dynamic_data_id_type>& asset_dynamic_data_ids) const
   {
      return _db.get_objects(asset_dynamic_data_ids);
   }

   chain::asset database_api::price_to_dct(const chain::asset& price) const
   {
      return my->price_to_dct( price );
   }

   chain::asset database_api_impl::price_to_dct(const chain::asset& price) const
   {
      return _db.price_to_dct( price );
   }

   //////////////////////////////////////////////////////////////////////
   //                                                                  //
   // Non Fungible Tokens                                              //
   //                                                                  //
   //////////////////////////////////////////////////////////////////////

   uint64_t database_api::get_non_fungible_token_count() const
   {
      return my->get_non_fungible_token_count();
   }

   uint64_t database_api_impl::get_non_fungible_token_count() const
   {
      return _db.get_index_type<chain::non_fungible_token_index>().indices().size();
   }

   std::vector<fc::optional<chain::non_fungible_token_object>> database_api::get_non_fungible_tokens(const std::vector<chain::non_fungible_token_id_type>& nft_ids) const
   {
      return my->get_non_fungible_tokens(nft_ids);
   }

   std::vector<fc::optional<chain::non_fungible_token_object>> database_api_impl::get_non_fungible_tokens(const std::vector<chain::non_fungible_token_id_type>& nft_ids) const
   {
      return _db.get_objects(nft_ids);
   }

   std::vector<chain::non_fungible_token_object> database_api::list_non_fungible_tokens(const std::string& lower_bound_symbol, uint32_t limit) const
   {
      return my->list_non_fungible_tokens(lower_bound_symbol, limit);
   }

   std::vector<chain::non_fungible_token_object> database_api_impl::list_non_fungible_tokens(const std::string& lower_bound_symbol, uint32_t limit) const
   {
      FC_VERIFY_AND_THROW(limit <= CURRENT_OUTPUT_LIMIT_100, limit_exceeded_exception, "Current limit: ${l}", ("l", CURRENT_OUTPUT_LIMIT_100));
      const auto& nfts_by_symbol = _db.get_index_type<chain::non_fungible_token_index>().indices().get<chain::by_symbol>();
      std::vector<chain::non_fungible_token_object> result;
      result.reserve(limit);

      auto itr = nfts_by_symbol.lower_bound(lower_bound_symbol);
      if( lower_bound_symbol == "" )
         itr = nfts_by_symbol.begin();

      while(limit-- && itr != nfts_by_symbol.end())
         result.emplace_back(*itr++);

      return result;
   }

   std::vector<fc::optional<chain::non_fungible_token_object>> database_api::get_non_fungible_tokens_by_symbols(const std::vector<std::string>& symbols) const
   {
      return my->get_non_fungible_tokens_by_symbols(symbols);
   }

   std::vector<fc::optional<chain::non_fungible_token_object>> database_api_impl::get_non_fungible_tokens_by_symbols(const std::vector<std::string>& symbols) const
   {
      const auto& nfts_by_symbol = _db.get_index_type<chain::non_fungible_token_index>().indices().get<chain::by_symbol>();
      std::vector<fc::optional<chain::non_fungible_token_object> > result;
      result.reserve(symbols.size());

      std::transform(symbols.begin(), symbols.end(), std::back_inserter(result),
                     [&nfts_by_symbol](const std::string& symbol) -> fc::optional<chain::non_fungible_token_object> {
                        auto itr = nfts_by_symbol.find(symbol);
                        return itr == nfts_by_symbol.end() ? fc::optional<chain::non_fungible_token_object>() : *itr;
                     });
      return result;
   }

   uint64_t database_api::get_non_fungible_token_data_count() const
   {
      return my->get_non_fungible_token_data_count();
   }

   uint64_t database_api_impl::get_non_fungible_token_data_count() const
   {
      return _db.get_index_type<chain::non_fungible_token_data_index>().indices().size();
   }

   std::vector<fc::optional<chain::non_fungible_token_data_object>> database_api::get_non_fungible_token_data(const std::vector<chain::non_fungible_token_data_id_type>& nft_data_ids) const
   {
      return my->get_non_fungible_token_data(nft_data_ids);
   }

   std::vector<fc::optional<chain::non_fungible_token_data_object>> database_api_impl::get_non_fungible_token_data(const std::vector<chain::non_fungible_token_data_id_type>& nft_data_ids) const
   {
      return _db.get_objects(nft_data_ids);
   }

   std::vector<chain::non_fungible_token_data_object> database_api::list_non_fungible_token_data(chain::non_fungible_token_id_type nft_id) const
   {
      return my->list_non_fungible_token_data(nft_id);
   }

   std::vector<chain::non_fungible_token_data_object> database_api_impl::list_non_fungible_token_data(chain::non_fungible_token_id_type nft_id) const
   {
      const auto& nft_data_range = _db.get_index_type<chain::non_fungible_token_data_index>().indices().get<chain::by_nft>().equal_range(nft_id);

      std::vector<chain::non_fungible_token_data_object> result;
      result.reserve(std::distance(nft_data_range.first, nft_data_range.second));
      std::for_each(nft_data_range.first, nft_data_range.second, [&](const chain::non_fungible_token_data_object& nft_data) {
         if( nft_data.owner != GRAPHENE_NULL_ACCOUNT )
            result.emplace_back(nft_data);
      });

      return result;
   }

   std::vector<chain::transaction_detail_object> database_api::search_non_fungible_token_history(chain::non_fungible_token_data_id_type nft_data_id) const
   {
      return my->search_non_fungible_token_history(nft_data_id);
   }

   std::vector<chain::transaction_detail_object> database_api_impl::search_non_fungible_token_history(chain::non_fungible_token_data_id_type nft_data_id) const
   {
      auto nft_data_range = _db.get_index_type<chain::transaction_detail_index>().indices().get<chain::by_nft>().equal_range(nft_data_id);

      std::vector<chain::transaction_detail_object> result;
      std::for_each(nft_data_range.first, nft_data_range.second, [&](const chain::transaction_detail_object &obj) {
         result.emplace_back(obj);
      });
      return result;
   }

   //////////////////////////////////////////////////////////////////////
   //                                                                  //
   // Miners                                                           //
   //                                                                  //
   //////////////////////////////////////////////////////////////////////

   std::vector<fc::optional<chain::miner_object>> database_api::get_miners(const std::vector<chain::miner_id_type>& miner_ids) const
   {
      return my->get_miners( miner_ids );
   }

   std::vector<fc::optional<chain::miner_object>> database_api_impl::get_miners(const std::vector<chain::miner_id_type>& miner_ids) const
   {
      return _db.get_objects(miner_ids);
   }

   fc::optional<chain::miner_object> database_api::get_miner_by_account(chain::account_id_type account) const
   {
      return my->get_miner_by_account( account );
   }

   fc::optional<chain::miner_object> database_api_impl::get_miner_by_account(chain::account_id_type account) const
   {
      const auto& idx = _db.get_index_type<chain::miner_index>().indices().get<chain::by_account>();
      auto itr = idx.find(account);
      if( itr != idx.end() )
         return *itr;
      return {};
   }

   std::map<std::string, chain::miner_id_type> database_api::lookup_miner_accounts(const std::string& lower_bound_name, uint32_t limit) const
   {
      return my->lookup_miner_accounts( lower_bound_name, limit );
   }

   std::map<std::string, chain::miner_id_type> database_api_impl::lookup_miner_accounts(const std::string& lower_bound_name, uint32_t limit) const
   {
      FC_VERIFY_AND_THROW(limit <= CURRENT_OUTPUT_LIMIT_1000, limit_exceeded_exception, "Current limit: ${l}", ("l", CURRENT_OUTPUT_LIMIT_1000));
      const auto& miners_by_id = _db.get_index_type<chain::miner_index>().indices().get<db::by_id>();

      // we want to order miners by account name, but that name is in the account object
      // so the miner_index doesn't have a quick way to access it.
      // get all the names and look them all up, sort them, then figure out what
      // records to return.  This could be optimized, but we expect the
      // number of miners to be few and the frequency of calls to be rare
      std::map<std::string, chain::miner_id_type> miners_by_account_name;
      for (const chain::miner_object& miner : miners_by_id)
         if (auto account_iter = _db.find(miner.miner_account))
            if (account_iter->name >= lower_bound_name) // we can ignore anything below lower_bound_name
               miners_by_account_name.insert(std::make_pair(account_iter->name, miner.id));

      auto end_iter = miners_by_account_name.begin();
      while (end_iter != miners_by_account_name.end() && limit--)
         ++end_iter;
      miners_by_account_name.erase(end_iter, miners_by_account_name.end());
      return miners_by_account_name;
   }

   uint64_t database_api::get_miner_count() const
   {
      return my->get_miner_count();
   }

   uint64_t database_api_impl::get_miner_count() const
   {
      return _db.get_index_type<chain::miner_index>().indices().size();
   }

   std::multimap<fc::time_point_sec, chain::price_feed> database_api::get_feeds_by_miner(chain::account_id_type account_id, uint32_t count) const
   {
      return my->get_feeds_by_miner( account_id, count);
   }

   std::multimap<fc::time_point_sec, chain::price_feed> database_api_impl::get_feeds_by_miner(chain::account_id_type account_id, uint32_t count) const
   {
      FC_VERIFY_AND_THROW(count <= CURRENT_OUTPUT_LIMIT_100, limit_exceeded_exception, "Current limit: ${l}", ("l", CURRENT_OUTPUT_LIMIT_100));
      auto& asset_idx = _db.get_index_type<chain::asset_index>().indices().get<chain::by_type>();
      auto mia_itr = asset_idx.lower_bound(true);

      std::multimap<fc::time_point_sec, chain::price_feed> result;

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

   std::vector<fc::optional<chain::miner_object>> database_api::lookup_vote_ids(const std::vector<chain::vote_id_type>& votes) const
   {
      return my->lookup_vote_ids( votes );
   }

   std::vector<fc::optional<chain::miner_object>> database_api_impl::lookup_vote_ids(const std::vector<chain::vote_id_type>& votes) const
   {
      FC_VERIFY_AND_THROW(votes.size() <= CURRENT_OUTPUT_LIMIT_1000, limit_exceeded_exception, "Only ${l} votes can be queried at a time", ("l", CURRENT_OUTPUT_LIMIT_1000));

      const auto& miner_idx = _db.get_index_type<chain::miner_index>().indices().get<chain::by_vote_id>();

      std::vector<fc::optional<chain::miner_object>> result;
      result.reserve( votes.size() );
      for( auto id : votes )
      {
         switch( id.type() )
         {
            case chain::vote_id_type::miner:
            {
               auto itr = miner_idx.find( id );
               if( itr != miner_idx.end() )
                  result.emplace_back( *itr );
               else
                  result.emplace_back( );
               break;
            }

            case chain::vote_id_type::VOTE_TYPE_COUNT:
               break; // supress unused enum value warnings
         }
      }
      return result;
   }

   std::vector<miner_voting_info> database_api::search_miner_voting(
      const std::string& account_id, const std::string& term, bool only_my_votes, const std::string& order, const std::string& id, uint32_t count) const
   {
      return my->search_miner_voting(account_id, term, only_my_votes, order, id, count);
   }

   std::vector<miner_voting_info> database_api_impl::search_miner_voting(
      const std::string& account_id, const std::string& term, bool only_my_votes, const std::string& order, const std::string& id, uint32_t count) const
   {
      // miner sorting helper struct
      struct miner_sorter
      {
         std::string sort_;

         miner_sorter(const std::string& sort) : sort_(sort) {}

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
         db::object_id_type search_id_;

         miner_search(const std::string& search_id) : search_id_(search_id) {}

         bool operator()(const miner_voting_info& info) const
         {
            return info.id == search_id_;
         }
      };

      boost::container::flat_set<chain::vote_id_type> acc_votes;

      if (! account_id.empty())
      {
         const chain::account_object* acc_obj = nullptr;
         if (std::isdigit(account_id[0]))
            acc_obj = _db.find(fc::variant(account_id).as<chain::account_id_type>());
         else
         {
            const auto& idx = _db.get_index_type<chain::account_index>().indices().get<chain::by_name>();
            auto itr = idx.find(account_id);
            if (itr != idx.end())
               acc_obj = &*itr;
         }

         FC_VERIFY_AND_THROW(acc_obj, account_does_not_exist_exception, "Account: ${account}", ("account", account_id));
         acc_votes = acc_obj->options.votes;
      }

      std::map<std::string, chain::miner_id_type> miners = this->lookup_miner_accounts("", 1000);

      std::vector<miner_voting_info> miners_info;
      miners_info.reserve(miners.size());

      for (auto item : miners)
      {
         miner_voting_info info;
         info.id = item.second;
         info.name = item.first;

         if (term.empty() || item.first.find(term) != std::string::npos )
         {
            fc::optional<chain::miner_object> ob = this->get_miners({ item.second }).front();
            if (ob)
            {
               chain::miner_object obj = *ob;

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

      std::vector<miner_voting_info> result;
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

   std::string database_api::get_transaction_hex(const chain::signed_transaction& trx) const
   {
      return my->get_transaction_hex( trx );
   }

   std::string database_api_impl::get_transaction_hex(const chain::signed_transaction& trx) const
   {
      return fc::to_hex(fc::raw::pack(trx));
   }

   std::set<chain::public_key_type> database_api::get_required_signatures(const chain::signed_transaction& trx, const boost::container::flat_set<chain::public_key_type>& available_keys) const
   {
      return my->get_required_signatures( trx, available_keys );
   }

   std::set<chain::public_key_type> database_api_impl::get_required_signatures(const chain::signed_transaction& trx, const boost::container::flat_set<chain::public_key_type>& available_keys) const
   {
      ddump((trx)(available_keys));
      auto result = trx.get_required_signatures( _db.get_chain_id(),
                                                available_keys,
                                                [&]( chain::account_id_type id ){ return &id(_db).active; },
                                                [&]( chain::account_id_type id ){ return &id(_db).owner; },
                                                _db.get_global_properties().parameters.max_authority_depth );
      ddump((result));
      return result;
   }

   std::set<chain::public_key_type> database_api::get_potential_signatures(const chain::signed_transaction& trx) const
   {
      return my->get_potential_signatures( trx );
   }

   std::set<chain::public_key_type> database_api_impl::get_potential_signatures(const chain::signed_transaction& trx) const
   {
      ddump((trx));
      std::set<chain::public_key_type> result;
      trx.get_required_signatures(
                                  _db.get_chain_id(),
                                  boost::container::flat_set<chain::public_key_type>(),
                                  [&]( chain::account_id_type id )
                                  {
                                     const auto& auth = id(_db).active;
                                     for( const auto& k : auth.get_keys() )
                                        result.insert(k);
                                     return &auth;
                                  },
                                  [&]( chain::account_id_type id )
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

   bool database_api::verify_authority(const chain::signed_transaction& trx) const
   {
      return my->verify_authority( trx );
   }

   bool database_api_impl::verify_authority(const chain::signed_transaction& trx) const
   {
      try
      {
         trx.verify_authority( trx.get_signature_keys(_db.get_chain_id()),
                              [&]( chain::account_id_type id ){ return &id(_db).active; },
                              [&]( chain::account_id_type id ){ return &id(_db).owner; },
                              _db.get_global_properties().parameters.max_authority_depth );
         return true;
      }
      catch( const chain::transaction_exception &e )
      {
         dlog(e.to_string(fc::log_level::debug));
      }

      return false;
   }

   bool database_api::verify_account_authority(const std::string& name_or_id, const boost::container::flat_set<chain::public_key_type>& signers) const
   {
      return my->verify_account_authority( name_or_id, signers );
   }

   bool database_api_impl::verify_account_authority(const std::string& name_or_id, const boost::container::flat_set<chain::public_key_type>& keys) const
   {
      if(name_or_id.size() == 0)
         FC_THROW("Account name or id cannot be empty string");
      const chain::account_object* account = nullptr;
      if (std::isdigit(name_or_id[0]))
         account = _db.find(fc::variant(name_or_id).as<chain::account_id_type>());
      else
      {
         const auto& idx = _db.get_index_type<chain::account_index>().indices().get<chain::by_name>();
         auto itr = idx.find(name_or_id);
         if (itr != idx.end())
            account = &*itr;
      }

      FC_VERIFY_AND_THROW(account, account_does_not_exist_exception, "Account: ${account}", ("account", name_or_id));

      try
      {
         /// reuse trx.verify_authority by creating a dummy transfer
         chain::signed_transaction trx;
         chain::transfer_obsolete_operation op;
         op.from = account->id;
         trx.operations.emplace_back(op);
         trx.verify_authority( keys,
                               [&]( chain::account_id_type id ){ return &id(_db).active; },
                               [&]( chain::account_id_type id ){ return &id(_db).owner; },
                               _db.get_global_properties().parameters.max_authority_depth );
         return true;
      }
      catch( const chain::transaction_exception &e )
      {
         dlog(e.to_string(fc::log_level::debug));
      }

      return false;
   }

   chain::processed_transaction database_api::validate_transaction(const chain::signed_transaction& trx) const
   {
      return my->validate_transaction( trx );
   }

   chain::processed_transaction database_api_impl::validate_transaction(const chain::signed_transaction& trx) const
   {
      return _db.validate_transaction(trx);
   }

   fc::variants database_api::get_required_fees(const std::vector<chain::operation>& ops, chain::asset_id_type id) const
   {
      return my->get_required_fees( ops, id );
   }

   /**
    * Container method for mutually recursive functions used to
    * implement get_required_fees() with potentially nested proposals.
    */
   struct get_required_fees_helper
   {
      get_required_fees_helper(const chain::fee_schedule& _current_fee_schedule,
                               const chain::price& _core_exchange_rate,
                               uint32_t _max_recursion
                               )
      : current_fee_schedule(_current_fee_schedule),
      core_exchange_rate(_core_exchange_rate),
      max_recursion(_max_recursion)
      {}


      fc::variant set_op_fees( chain::operation& op, fc::time_point_sec now )
      {
         if( op.which() == chain::operation::tag<chain::proposal_create_operation>::value )
         {
            return set_proposal_create_op_fees( op, now );
         }
         else
         {
            chain::asset fee = current_fee_schedule.set_fee( op, now, core_exchange_rate );
            fc::variant result;
            fc::to_variant( fee, result );
            return result;
         }
      }

      fc::variant set_proposal_create_op_fees( chain::operation& proposal_create_op, fc::time_point_sec now )
      {
         chain::proposal_create_operation& op = proposal_create_op.get<chain::proposal_create_operation>();
         std::pair<chain::asset, fc::variants> result;
         for(chain::op_wrapper& prop_op : op.proposed_ops )
         {
            FC_ASSERT( current_recursion < max_recursion );
            ++current_recursion;
            result.second.push_back( set_op_fees( prop_op.op, now ) );
            --current_recursion;
         }
         // we need to do this on the boxed version, which is why we use
         // two mutually recursive functions instead of a visitor
         result.first = current_fee_schedule.set_fee( proposal_create_op, now, core_exchange_rate );
         fc::variant vresult;
         fc::to_variant( result, vresult );
         return vresult;
      }

      const chain::fee_schedule& current_fee_schedule;
      const chain::price& core_exchange_rate;
      uint32_t max_recursion;
      uint32_t current_recursion = 0;
   };

   fc::variants database_api_impl::get_required_fees(std::vector<chain::operation> ops, chain::asset_id_type id) const
   {
      //
      // we copy the ops because we need to mutate an operation to reliably
      // determine its fee, see #435
      //

      const chain::asset_object& a = id(_db);
      get_required_fees_helper helper(
                                      _db.current_fee_schedule(),
                                      a.options.core_exchange_rate,
                                      GET_REQUIRED_FEES_MAX_RECURSION );

      fc::variants result(ops.size());

      std::transform(ops.begin(), ops.end(), result.begin(), [&](chain::operation &op) {
         return helper.set_op_fees( op, _db.head_block_time() ); } );
      return result;
   }

   //////////////////////////////////////////////////////////////////////
   //                                                                  //
   // Proposed transactions                                            //
   //                                                                  //
   //////////////////////////////////////////////////////////////////////

   std::vector<chain::proposal_object> database_api::get_proposed_transactions(chain::account_id_type id) const
   {
      return my->get_proposed_transactions( id );
   }

   std::vector<operation_info> database_api::list_operations() const
   {
      return my->list_operations();
   }

   /** TODO: add secondary index that will accelerate this process */
   std::vector<chain::proposal_object> database_api_impl::get_proposed_transactions(chain::account_id_type id) const
   {
      const auto& idx = _db.get_index_type<chain::proposal_index>();
      std::vector<chain::proposal_object> result;

      idx.inspect_all_objects( [&](const db::object& obj){
         const chain::proposal_object& p = static_cast<const chain::proposal_object&>(obj);
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

        std::shared_ptr<std::vector<std::string>> op_names;
        operation_info_visitor(std::shared_ptr<std::vector<std::string>> _op_names) : op_names(_op_names) { }

        template<typename Type>
        result_type operator()( const Type& op )const
        {
           std::string vo = fc::get_typename<Type>::name();
           op_names->emplace_back( vo );
        }
   };

   std::vector<operation_info> database_api_impl::list_operations( )const
   {
       std::shared_ptr<std::vector<std::string>> op_names_ptr = std::make_shared<std::vector<std::string>>();

       std::vector<operation_info> result;
       std::map<int32_t, bool> op_processed;
       std::map<int32_t, operation_info> op_cached;

       chain::fee_schedule temp_fee_schedule;
       temp_fee_schedule = temp_fee_schedule.get_default();

       chain::fee_schedule global_fee_schedule;
       global_fee_schedule = get_global_properties().parameters.current_fees;

       op_names_ptr->clear();
       result.clear();
       op_processed.clear();

       try
       {
          chain::operation op;

          for( std::size_t i = 0; i < chain::operation::type_info::count; ++i )
          {
             op.set_which(i);
             op.visit( operation_info_visitor(op_names_ptr) );
          }

          for( chain::fee_parameters& params : global_fee_schedule.parameters )
          {
              op_processed[params.which()] = true;
              op_cached[params.which()] = operation_info(params.which(), (*op_names_ptr)[params.which()].replace(0, std::string("graphene::chain::").length(), ""), params);
          }

          for( chain::fee_parameters& params : temp_fee_schedule.parameters )
          {
              if (0 == op_processed.count(params.which()) || ! op_processed[params.which()])
              {
                  op_cached[params.which()] = operation_info(params.which(), (*op_names_ptr)[params.which()].replace(0, std::string("graphene::chain::").length(), ""), params);
              }
          }

          std::map<int32_t, operation_info>::iterator it;

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

   chain::real_supply database_api::get_real_supply() const
   {
      return my->_db.get_real_supply();
   }

   std::vector<chain::account_id_type> database_api::list_publishing_managers(const std::string& lower_bound_name, uint32_t limit) const
   {
      return my->list_publishing_managers( lower_bound_name, limit );
   }

   std::vector<chain::account_id_type> database_api_impl::list_publishing_managers(const std::string& lower_bound_name, uint32_t limit) const
   {
      FC_VERIFY_AND_THROW(limit <= CURRENT_OUTPUT_LIMIT_100, limit_exceeded_exception, "Current limit: ${l}", ("l", CURRENT_OUTPUT_LIMIT_100));
      const auto& idx = _db.get_index_type<chain::account_index>().indices().get<chain::by_publishing_manager_and_name>();
      std::vector<chain::account_id_type> result;

      for( auto itr = idx.lower_bound( boost::make_tuple( true, lower_bound_name ) );
           limit-- && itr->rights_to_publish.is_publishing_manager && itr != idx.end();
           ++itr )
         result.push_back(itr->id);

      return result;
   }

   std::vector<chain::buying_object> database_api::get_open_buyings() const
   {
      return my->get_open_buyings();
   }

   std::vector<chain::buying_object> database_api_impl::get_open_buyings() const
   {
      const auto& range = _db.get_index_type<chain::buying_index>().indices().get<chain::by_open_expiration>().equal_range(true);
      std::vector<chain::buying_object> result;
      result.reserve(distance(range.first, range.second));

      std::for_each(range.first, range.second, [&](const chain::buying_object &element) {
         if( element.expiration_time >= _db.head_block_time() )
            result.emplace_back(element);
      });
      return result;
   }

   std::vector<chain::buying_object> database_api::get_open_buyings_by_URI(const std::string& URI) const
   {
      return my->get_open_buyings_by_URI( URI );
   }

   std::vector<chain::buying_object> database_api_impl::get_open_buyings_by_URI(const std::string& URI) const
   {
      try
      {
         auto range = _db.get_index_type<chain::buying_index>().indices().get<chain::by_URI_open>().equal_range( std::make_tuple( URI, true ) );
         std::vector<chain::buying_object> result;
         result.reserve(distance(range.first, range.second));
         std::for_each(range.first, range.second, [&](const chain::buying_object& element) {
            if( element.expiration_time >= _db.head_block_time() )
               result.emplace_back(element);
         });
         return result;
      }
      FC_CAPTURE_AND_RETHROW( (URI) );
   }

   std::vector<chain::buying_object> database_api::get_open_buyings_by_consumer(chain::account_id_type consumer) const
   {
      return my->get_open_buyings_by_consumer( consumer );
   }

   std::vector<chain::buying_object> database_api_impl::get_open_buyings_by_consumer(chain::account_id_type consumer) const
   {
      try
      {
         auto range = _db.get_index_type<chain::buying_index>().indices().get<chain::by_consumer_open>().equal_range( std::make_tuple( consumer, true ));
         std::vector<chain::buying_object> result;
         result.reserve(distance(range.first, range.second));

         std::for_each(range.first, range.second, [&](const chain::buying_object& element) {
            if( element.expiration_time >= _db.head_block_time() )
               result.emplace_back(element);
         });
         return result;
      }
      FC_CAPTURE_AND_RETHROW( (consumer) );
   }

   std::vector<chain::buying_object> database_api::get_buying_history_objects_by_consumer(chain::account_id_type consumer) const
   {
      return my->get_buying_history_objects_by_consumer( consumer );
   }

   std::vector<chain::buying_object> database_api_impl::get_buying_history_objects_by_consumer(chain::account_id_type consumer) const
   {
      try {
         const auto &range = _db.get_index_type<chain::buying_index>().indices().get<chain::by_consumer_open>().equal_range( std::make_tuple(consumer, false));
         std::vector<chain::buying_object> result;
         result.reserve(distance(range.first, range.second));

         std::for_each(range.first, range.second, [&](const chain::buying_object &element) {
            result.emplace_back(element);
         });

         return result;
      }
      FC_CAPTURE_AND_RETHROW( (consumer) );
   }

   std::vector<chain::buying_object> database_api::get_buying_objects_by_consumer(
      chain::account_id_type consumer, const std::string& order, db::object_id_type id, const std::string& term, uint32_t count) const
   {
      return my->get_buying_objects_by_consumer( consumer, order, id, term, count );
   }

namespace
{
   template <bool is_ascending, class sort_tag>
   void search_buying_template(chain::database& db, chain::account_id_type consumer, const std::string& term, db::object_id_type id, uint32_t count, std::vector<chain::buying_object>& result)
   {
      const auto& idx_by_sort_tag = db.get_index_type<chain::buying_index>().indices().get<sort_tag>();

      auto itr_begin = return_one<is_ascending>::choose(idx_by_sort_tag.cbegin(), idx_by_sort_tag.crbegin());
      auto itr_end = return_one<is_ascending>::choose(idx_by_sort_tag.end(), idx_by_sort_tag.rend());

      correct_iterator<chain::buying_index, chain::buying_object, sort_tag, decltype(itr_begin), is_ascending>(db, id, itr_begin);

      while(count &&
            itr_begin != itr_end)
      {
         chain::buying_object const& element = *itr_begin;
         ++itr_begin;

         if (element.consumer == consumer)
         {
            std::string title;
            std::string description;

            chain::ContentObjectPropertyManager synopsis_parser(element.synopsis);
            title = synopsis_parser.get<chain::ContentObjectTitle>();
            description = synopsis_parser.get<chain::ContentObjectDescription>();

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

   std::vector<chain::buying_object> database_api_impl::get_buying_objects_by_consumer(
      chain::account_id_type consumer, const std::string& order, db::object_id_type id, const std::string& term, uint32_t count) const
   {
      try {
         std::vector<chain::buying_object> result;

         if(order == "+size")
            search_buying_template<true, chain::by_size>(_db, consumer, term, id, count, result);
         else if(order == "-size")
            search_buying_template<false, chain::by_size>(_db, consumer, term, id, count, result);
         else if(order == "+price")
            search_buying_template<true, chain::by_price_before_exchange>(_db, consumer, term, id, count, result);
         else if(order == "-price")
            search_buying_template<false, chain::by_price_before_exchange>(_db, consumer, term, id, count, result);
         else if(order == "+created")
            search_buying_template<true, chain::by_created>(_db, consumer, term, id, count, result);
         else if(order == "-created")
            search_buying_template<false, chain::by_created>(_db, consumer, term, id, count, result);
         else if(order == "+purchased")
            search_buying_template<true, chain::by_purchased>(_db, consumer, term, id, count, result);
         else //if(order == "-purchased")
            search_buying_template<false, chain::by_purchased>(_db, consumer, term, id, count, result);
         return result;
      }
      FC_CAPTURE_AND_RETHROW( (consumer) );
   }

   fc::optional<chain::content_object> database_api::get_content(const std::string& URI) const
   {
      return my->get_content( URI );
   }

   fc::sha256 database_api::restore_encryption_key(decent::encrypt::DIntegerString el_gamal_priv_key_string, chain::buying_id_type buying) const
   {
      auto objects = get_objects({buying});
      FC_VERIFY_AND_THROW(!objects.empty(), buying_object_does_not_exist_exception, "Buying: ${buying}", ("buying", buying));

      const chain::buying_object bo = objects.front().template as<chain::buying_object>();
      auto content = get_content(bo.URI);

      FC_VERIFY_AND_THROW(content.valid(), content_object_does_not_exist_exception, "URI: ${uri}", ("uri", bo.URI));
      const chain::content_object co = *content;

      decent::encrypt::ShamirSecret ss( static_cast<uint16_t>(co.quorum), static_cast<uint16_t>(co.key_parts.size()) );
      decent::encrypt::point message;

      decent::encrypt::DInteger el_gamal_priv_key = el_gamal_priv_key_string;

      for( const auto key_particle : bo.key_particles )
      {
         FC_VERIFY_AND_THROW(decent::encrypt::el_gamal_decrypt(decent::encrypt::Ciphertext(key_particle), el_gamal_priv_key, message) == decent::encrypt::ok, decryption_of_key_particle_failed_exception);
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

   content_keys database_api::generate_content_keys(const std::vector<chain::account_id_type>& seeders) const
   {
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
      decent::encrypt::ShamirSecret ss(static_cast<uint16_t>(keys.quorum), static_cast<uint16_t>(seeders.size()), secret);
      ss.calculate_split();

      for( int i =0; i < (int)seeders.size(); i++ )
      {
         const auto& s = my->get_seeder( seeders[i] );
         FC_VERIFY_AND_THROW(s.valid(), seeder_not_found_exception, "Seeder: ${s}", ("s", seeders[i]));
         decent::encrypt::Ciphertext cp;
         decent::encrypt::point p = ss.split[i];
         decent::encrypt::el_gamal_encrypt( p, s->pubKey, cp );
         keys.parts.push_back(cp);
      }

      return keys;
   }

   fc::optional<chain::seeder_object> database_api::get_seeder(chain::account_id_type account) const
   {
      return my->get_seeder(account);
   }

   fc::optional<chain::content_object> database_api_impl::get_content(const std::string& URI) const
   {
      const auto& idx = _db.get_index_type<chain::content_index>().indices().get<chain::by_URI>();
      auto itr = idx.find(URI);
      if (itr != idx.end())
         return *itr;
      return {};
   }

   std::vector<chain::buying_object> database_api::search_feedback(const std::string& user, const std::string& URI, db::object_id_type id, uint32_t count) const
   {
      return my->search_feedback(user, URI, id, count);
   }

   namespace {

      template <bool is_ascending, class sort_tag>
      void search_rating_template(chain::database& db, uint32_t count, const std::string& URI, db::object_id_type id, std::vector<chain::buying_object>& result)
      {
         const auto& range_equal = db.get_index_type<chain::buying_index>().indices().get<sort_tag>().equal_range(std::make_tuple( URI, true ));
         auto range_begin = range_equal.first;
         auto range_end = range_equal.second;

         auto itr_begin = return_one<is_ascending>::choose(range_begin, boost::reverse_iterator<decltype(range_end)>(range_end));
         auto itr_end = return_one<is_ascending>::choose(range_end, boost::reverse_iterator<decltype(range_begin)>(range_begin));

         correct_iterator<chain::buying_index, chain::buying_object, sort_tag, decltype(itr_begin), is_ascending>(db, id, itr_begin);

         while (count && itr_begin != itr_end)
         {
            const chain::buying_object& rating_item = *itr_begin;
            ++itr_begin;

            result.push_back(rating_item);
            count--;
         }
      }
   }

   std::vector<chain::buying_object> database_api_impl::search_feedback(const std::string& user, const std::string& URI, db::object_id_type id, uint32_t count) const
   {
      std::vector<chain::buying_object> result;

      try
      {
         const auto& idx_account = _db.get_index_type<chain::account_index>().indices().get<chain::by_name>();
         const auto account_itr = idx_account.find(user);

         if (false == user.empty())
         {
            if (account_itr != idx_account.end())
            {
               const auto& idx = _db.get_index_type<chain::buying_index>().indices().get<chain::by_consumer_URI>();
               auto itr = idx.find(std::make_tuple(account_itr->id, URI));
               if(itr != idx.end() && itr->rated_or_commented)
                  result.push_back(*itr);
            }
         }
         else
         {
            search_rating_template<false, chain::by_URI_rated>(_db, count, URI, id, result);
         }
      }FC_CAPTURE_AND_RETHROW( (user)(URI) );

      return result;
   }

   fc::optional<chain::buying_object> database_api::get_buying_by_consumer_URI(chain::account_id_type consumer, const std::string& URI) const
   {
      return my->get_buying_by_consumer_URI( consumer, URI );
   }

   fc::optional<chain::buying_object> database_api_impl::get_buying_by_consumer_URI(chain::account_id_type consumer, const std::string& URI) const
   {
      try{
         const auto & idx = _db.get_index_type<chain::buying_index>().indices().get<chain::by_consumer_URI>();
         auto itr = idx.find(std::make_tuple(consumer, URI));
         std::vector<chain::buying_object> result;
         if(itr!=idx.end()){
            return *itr;
         }
         return {};

      }FC_CAPTURE_AND_RETHROW( (consumer)(URI) );
   }

   fc::optional<chain::seeder_object> database_api_impl::get_seeder(chain::account_id_type account) const
   {
      const auto& idx = _db.get_index_type<chain::seeder_index>().indices().get<chain::by_seeder>();
      auto itr = idx.find(account);
      if (itr != idx.end())
         return *itr;
      return {};
   }

   fc::optional<chain::subscription_object> database_api::get_subscription(chain::subscription_id_type sid) const
   {
      return my->get_subscription(sid);
   }

   fc::optional<chain::subscription_object> database_api_impl::get_subscription(chain::subscription_id_type sid) const
   {
      const auto& idx = _db.get_index_type<chain::subscription_index>().indices().get<db::by_id>();
      auto itr = idx.find(sid);
      if (itr != idx.end())
         return *itr;
      return {};
   }

   std::vector<chain::subscription_object> database_api::list_active_subscriptions_by_consumer(chain::account_id_type account, uint32_t count) const
   {
      return my->list_active_subscriptions_by_consumer( account, count );
   }

   std::vector<chain::subscription_object> database_api_impl::list_active_subscriptions_by_consumer(chain::account_id_type account, uint32_t count) const
   {
      try{
         FC_VERIFY_AND_THROW(count <= CURRENT_OUTPUT_LIMIT_100, limit_exceeded_exception, "Current limit: ${i}", ("l", CURRENT_OUTPUT_LIMIT_100));
         auto range = _db.get_index_type<chain::subscription_index>().indices().get<chain::by_from_expiration>().equal_range(account);
         std::vector<chain::subscription_object> result;
         result.reserve(distance(range.first, range.second));
         std::for_each(range.first, range.second,
                       [&](const chain::subscription_object& element) {
                            if( element.expiration > _db.head_block_time() )
                               result.emplace_back(element);
                       });
         return result;

      }FC_CAPTURE_AND_RETHROW( (account)(count) );
   }

   std::vector<chain::subscription_object> database_api::list_subscriptions_by_consumer(chain::account_id_type account, uint32_t count) const
   {
      return my->list_subscriptions_by_consumer( account, count );
   }

   std::vector<chain::subscription_object> database_api_impl::list_subscriptions_by_consumer(chain::account_id_type account, uint32_t count) const
   {
      try{
         FC_VERIFY_AND_THROW(count <= CURRENT_OUTPUT_LIMIT_100, limit_exceeded_exception, "Current limit: ${i}", ("l", CURRENT_OUTPUT_LIMIT_100));
         uint32_t i = count;
         const auto& range = _db.get_index_type<chain::subscription_index>().indices().get<chain::by_from>().equal_range(account);
         std::vector<chain::subscription_object> result;
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

   std::vector<chain::subscription_object> database_api::list_active_subscriptions_by_author(chain::account_id_type account, uint32_t count) const
   {
      return my->list_active_subscriptions_by_author( account, count );
   }

   std::vector<chain::subscription_object> database_api_impl::list_active_subscriptions_by_author(chain::account_id_type account, uint32_t count) const
   {
      try{
         FC_VERIFY_AND_THROW(count <= CURRENT_OUTPUT_LIMIT_100, limit_exceeded_exception, "Current limit: ${i}", ("l", CURRENT_OUTPUT_LIMIT_100));
         auto range = _db.get_index_type<chain::subscription_index>().indices().get<chain::by_to_expiration>().equal_range(account);
         std::vector<chain::subscription_object> result;
         result.reserve(distance(range.first, range.second));
         std::for_each(range.first, range.second,
                       [&](const chain::subscription_object& element) {
                            if( element.expiration > _db.head_block_time() )
                               result.emplace_back(element);
                       });
         return result;

      }FC_CAPTURE_AND_RETHROW( (account)(count) );
   }

   std::vector<chain::subscription_object> database_api::list_subscriptions_by_author(chain::account_id_type account, uint32_t count) const
   {
      return my->list_subscriptions_by_author( account, count );
   }

   std::vector<chain::subscription_object> database_api_impl::list_subscriptions_by_author(chain::account_id_type account, uint32_t count) const
   {
      try{
         FC_VERIFY_AND_THROW(count <= CURRENT_OUTPUT_LIMIT_100, limit_exceeded_exception, "Current limit: ${i}", ("l", CURRENT_OUTPUT_LIMIT_100));
         uint32_t i = count;
         const auto& range = _db.get_index_type<chain::subscription_index>().indices().get<chain::by_to>().equal_range(account);
         std::vector<chain::subscription_object> result;
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

   std::vector<content_summary> database_api::search_content(const std::string& term, const std::string& order, const std::string& user,
                                                             const std::string& region_code, db::object_id_type id, const std::string& type, uint32_t count) const
   {
      return my->search_content(term, order, user, region_code, id, type, count);
   }

   namespace {

      template <bool is_ascending, class sort_tag>
      void search_content_template(chain::database& db, const std::string& search_term, uint32_t count, const std::string& user, const std::string& region_code,
         db::object_id_type id, const std::string& type, std::vector<content_summary>& result)
      {
         const auto& idx_by_sort_tag = db.get_index_type<chain::content_index>().indices().get<sort_tag>();

         auto itr_begin = return_one<is_ascending>::choose(idx_by_sort_tag.cbegin(), idx_by_sort_tag.crbegin());
         auto itr_end = return_one<is_ascending>::choose(idx_by_sort_tag.cend(), idx_by_sort_tag.crend());

         correct_iterator<chain::content_index, chain::content_object, sort_tag, decltype(itr_begin), is_ascending>(db, id, itr_begin);

         content_summary content;
         const auto& idx_account = db.get_index_type<chain::account_index>().indices().get<db::by_id>();

         chain::ContentObjectTypeValue filter_type;
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
                  chain::ContentObjectTypeValue content_type;

                  try {
                     chain::ContentObjectPropertyManager synopsis_parser(content.synopsis);
                     title = synopsis_parser.get<chain::ContentObjectTitle>();
                     desc = synopsis_parser.get<chain::ContentObjectDescription>();
                     content_type = synopsis_parser.get<chain::ContentObjectType>();
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

   std::vector<content_summary> database_api_impl::search_content(const std::string& search_term, const std::string& order, const std::string& user,
                                                                  const std::string& region_code, db::object_id_type id, const std::string& type, uint32_t count) const
   {
      FC_VERIFY_AND_THROW(count <= CURRENT_OUTPUT_LIMIT_100, limit_exceeded_exception, "Current limit: ${i}", ("l", CURRENT_OUTPUT_LIMIT_100));

      std::vector<content_summary> result;
      result.reserve( count );

      if (order == "+author")
         search_content_template<true, chain::by_author>(_db, search_term, count, user, region_code, id, type, result);
      else if (order == "+rating")
         search_content_template<true, chain::by_AVG_rating>(_db, search_term, count, user, region_code, id, type, result);
      else if (order == "+size")
         search_content_template<true, chain::by_size>(_db, search_term, count, user, region_code, id, type, result);
      else if (order == "+price")
         search_content_template<true, chain::by_price>(_db, search_term, count, user, region_code, id, type, result);
      else if (order == "+created")
         search_content_template<true, chain::by_created>(_db, search_term, count, user, region_code, id, type, result);
      else if (order == "+expiration")
         search_content_template<true, chain::by_expiration>(_db, search_term, count, user, region_code, id, type, result);

      else if (order == "-author")
         search_content_template<false, chain::by_author>(_db, search_term, count, user, region_code, id, type, result);
      else if (order == "-rating")
         search_content_template<false, chain::by_AVG_rating>(_db, search_term, count, user, region_code, id, type, result);
      else if (order == "-size")
         search_content_template<false, chain::by_size>(_db, search_term, count, user, region_code, id, type, result);
      else if (order == "-price")
         search_content_template<false, chain::by_price>(_db, search_term, count, user, region_code, id, type, result);
      else if (order == "-expiration")
         search_content_template<false, chain::by_expiration>(_db, search_term, count, user, region_code, id, type, result);
      else// if (order == "-created")
         search_content_template<false, chain::by_created>(_db, search_term, count, user, region_code, id, type, result);

      return result;
   }

   std::vector<chain::seeder_object> database_api::list_seeders_by_price(uint32_t count) const
   {
      return my->list_seeders_by_price( count );
   }

   std::vector<chain::seeder_object> database_api_impl::list_seeders_by_price(uint32_t count) const
   {
      FC_VERIFY_AND_THROW(count <= CURRENT_OUTPUT_LIMIT_100, limit_exceeded_exception, "Current limit: ${i}", ("l", CURRENT_OUTPUT_LIMIT_100));

      const auto& idx = _db.get_index_type<chain::seeder_index>().indices().get<chain::by_price>();
      fc::time_point_sec now = _db.head_block_time();
      std::vector<chain::seeder_object> result;
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

   std::vector<chain::seeder_object> database_api::list_seeders_by_region(const std::string& region_code) const
   {
      return my->list_seeders_by_region( region_code );
   }

   std::vector<chain::seeder_object> database_api_impl::list_seeders_by_region(const std::string& region_code) const
   {
      const auto& range = _db.get_index_type<chain::seeder_index>().indices().get<chain::by_region>().equal_range( region_code );
      std::vector<chain::seeder_object> result;

      fc::time_point_sec now = head_block_time();
      auto itr = range.first;

      while(itr != range.second )
      {
         if( itr->expiration > now )
            result.emplace_back(*itr);
         ++itr;
      }

      return result;
   }

   std::vector<chain::seeder_object> database_api::list_seeders_by_rating(uint32_t count) const
   {
      return my->list_seeders_by_rating( count );
   }

   std::vector<chain::seeder_object> database_api_impl::list_seeders_by_rating(uint32_t count) const
   {
      FC_VERIFY_AND_THROW(count <= CURRENT_OUTPUT_LIMIT_100, limit_exceeded_exception, "Current limit: ${i}", ("l", CURRENT_OUTPUT_LIMIT_100));

      const auto& idx = _db.get_index_type<chain::seeder_index>().indices().get<chain::by_rating>();
      fc::time_point_sec now = _db.head_block_time();
      std::vector<chain::seeder_object> result;
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

   //////////////////////////////////////////////////////////////////////
   //                                                                  //
   // Private methods                                                  //
   //                                                                  //
   //////////////////////////////////////////////////////////////////////

   void database_api_impl::on_objects_changed(const std::vector<db::object_id_type>& ids, bool sync_mode)
   {
      std::vector<std::string> content_update_queue;

      for(auto id : ids)
      {
         const db::object* obj = nullptr;

         if( _content_subscriptions.size() )
         {
            obj = _db.find_object( id );
            if( obj )
            {
               const chain::content_object* content = dynamic_cast<const chain::content_object*>(obj);
               if(content && _content_subscriptions[ content->URI ] )
                  content_update_queue.emplace_back( content->URI );
            }

         }
      }

      auto capture_this = shared_from_this();

      /// pushing the future back / popping the prior future if it is complete.
      /// if a connection hangs then this could get backed up and result in
      /// a failure to exit cleanly.
      fc::async([capture_this,this, content_update_queue](){
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
         chain::block_id_type block_id = _db.head_block_id();
         fc::async([this,capture_this,block_id](){
            _block_applied_callback(fc::variant(block_id));
         });
      }
   }

   chain::share_type database_api::get_new_asset_per_block() const
   {
      return my->get_new_asset_per_block();
   }

   chain::share_type database_api_impl::get_new_asset_per_block() const
   {
      return _db.get_new_asset_per_block();
   }

   chain::share_type database_api::get_asset_per_block_by_block_num(uint32_t block_num) const
   {
      return my->get_asset_per_block_by_block_num(block_num);
   }

   chain::share_type database_api_impl::get_asset_per_block_by_block_num(uint32_t block_num) const
   {
      return _db.get_asset_per_block_by_block_num(block_num);
   }

   chain::miner_reward_input database_api::get_time_to_maint_by_block_time(fc::time_point_sec block_time) const
   {
      return my->get_time_to_maint_by_block_time(block_time);
   }

   chain::miner_reward_input database_api_impl::get_time_to_maint_by_block_time(fc::time_point_sec block_time) const
   {
      return _db.get_time_to_maint_by_block_time(block_time);
   }

   chain::share_type database_api::get_miner_pay_from_fees_by_block_time(fc::time_point_sec block_time) const
   {
      return my->get_miner_pay_from_fees_by_block_time(block_time);
   }

   chain::share_type database_api_impl::get_miner_pay_from_fees_by_block_time(fc::time_point_sec block_time) const
   {
      return _db.get_miner_pay_from_fees_by_block_time(block_time);
   }

   std::vector<chain::database::votes_gained> database_api::get_actual_votes() const
   {
      return my->_db.get_actual_votes();
   }

} } // graphene::app
