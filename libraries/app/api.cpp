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

#include <graphene/utilities/key_conversion.hpp>
#include <graphene/app/api.hpp>
#include <graphene/app/application.hpp>
#include <graphene/app/balance.hpp>
#include <graphene/app/impacted.hpp>

using namespace monitoring;
namespace graphene { namespace app {

   login_api::login_api(application& a)
   :_app(a)
   {
   }

   login_api::~login_api()
   {
   }

   bool login_api::login(const string& user, const string& password)
   {
      optional< api_access_info > acc = _app.get_api_access_info( user );
      if( !acc.valid() )
         return false;
      if( acc->password_hash_b64 != "*" )
      {
         std::string password_salt = fc::base64_decode( acc->password_salt_b64 );
         std::string acc_password_hash = fc::base64_decode( acc->password_hash_b64 );

         fc::sha256 hash_obj = fc::sha256::hash( password + password_salt );
         if( hash_obj.data_size() != acc_password_hash.length() )
            return false;
         if( memcmp( hash_obj.data(), acc_password_hash.c_str(), hash_obj.data_size() ) != 0 )
            return false;
      }

      for( const std::string& api_name : acc->allowed_apis )
         enable_api( api_name );
      return true;
   }

   void login_api::enable_api( const std::string& api_name )
   {
      if( api_name == database_api::get_api_name() )
      {
         _database_api = std::make_shared< database_api >( std::ref( *_app.chain_database() ) );
      }
      else if( api_name == network_broadcast_api::get_api_name() )
      {
         _network_broadcast_api = std::make_shared< network_broadcast_api >( std::ref( _app ) );
      }
      else if( api_name == history_api::get_api_name() )
      {
         _history_api = std::make_shared< history_api >( _app );
      }
      else if( api_name == network_node_api::get_api_name() )
      {
         _network_node_api = std::make_shared< network_node_api >( std::ref(_app) );
      }
      else if( api_name == crypto_api::get_api_name() )
      {
         _crypto_api = std::make_shared< crypto_api >( std::ref(_app) );
      }
      else if (api_name == messaging_api::get_api_name() )
      {
         _messaging_api = std::make_shared< messaging_api >( std::ref(_app) );
      }
      else if (api_name == monitoring_api::get_api_name() )
      {
         _monitoring_api = std::make_shared< monitoring_api >();
      }
   }

   fc::api<network_broadcast_api> login_api::network_broadcast()const
      {
      FC_ASSERT(_network_broadcast_api);
      return *_network_broadcast_api;
      }

   fc::api<network_node_api> login_api::network_node()const
      {
      FC_ASSERT(_network_node_api);
      return *_network_node_api;
      }

   fc::api<database_api> login_api::database()const
      {
      FC_ASSERT(_database_api);
      return *_database_api;
      }

   fc::api<history_api> login_api::history() const
      {
      FC_ASSERT(_history_api);
      return *_history_api;
      }

   fc::api<crypto_api> login_api::crypto() const
      {
      FC_ASSERT(_crypto_api);
      return *_crypto_api;
      }

   fc::api<graphene::app::messaging_api> login_api::messaging() const
      {
      FC_ASSERT(_messaging_api);
      return *_messaging_api;
      }

   fc::api<monitoring_api> login_api::monitoring() const
      {
      FC_ASSERT(_monitoring_api);
      return *_monitoring_api;
      }


   network_broadcast_api::network_broadcast_api(application& a):_app(a)
   {
      _applied_block_connection = _app.chain_database()->applied_block.connect([this](const signed_block& b){ on_applied_block(b); });
   }

   void network_broadcast_api::on_applied_block( const signed_block& b )
   {
      if( _callbacks.size() )
      {
         /// we need to ensure the database_api is not deleted for the life of the async operation
         auto capture_this = shared_from_this();
         for( uint32_t trx_num = 0; trx_num < b.transactions.size(); ++trx_num )
         {
            const auto& trx = b.transactions[trx_num];
            auto id = trx.id();
            auto itr = _callbacks.find(id);
            if( itr != _callbacks.end() )
            {
               auto block_num = b.block_num();
               auto& callback = _callbacks.find(id)->second;
               transaction_confirmation conf{ id, block_num, trx_num, trx};
               fc::variant confv(conf);

               fc::async( [capture_this,confv, callback](){ callback( confv ); } );
            }
         }
      }
   }

   void network_broadcast_api::broadcast_transaction(const signed_transaction& trx)
   {
      trx.validate();
      _app.chain_database()->push_transaction(trx);
      _app.p2p_node()->broadcast_transaction(trx);
   }

   fc::variant network_broadcast_api::broadcast_transaction_synchronous(const signed_transaction& trx)
   {
      fc::promise<fc::variant>::ptr pr( new fc::promise<fc::variant>() );
      broadcast_transaction_with_callback( [=]( const fc::variant& v ){
           pr->set_value(v);
      }, trx );
      return fc::future<fc::variant>(pr).wait();
   }

   void network_broadcast_api::broadcast_block( const signed_block& b )
   {
      _app.chain_database()->push_block(b, 0, false);
      _app.p2p_node()->broadcast( net::block_message( b ));
   }

   void network_broadcast_api::broadcast_transaction_with_callback(confirmation_callback cb, const signed_transaction& trx)
   {
      trx.validate();
      _callbacks[trx.id()] = cb;
      _app.chain_database()->push_transaction(trx);
      _app.p2p_node()->broadcast_transaction(trx);
   }

   network_node_api::network_node_api( application& a ) : _app( a )
   {
   }

   fc::variant_object network_node_api::get_info() const
   {
      fc::mutable_variant_object result = _app.p2p_node()->network_get_info();
      result["connection_count"] = _app.p2p_node()->get_connection_count();
      return result;
   }

   void network_node_api::add_node(const fc::ip::endpoint& ep)
   {
      _app.p2p_node()->add_node(ep);
   }

   std::vector<net::peer_status> network_node_api::get_connected_peers() const
   {
      return _app.p2p_node()->get_connected_peers();
   }

   std::vector<net::potential_peer_record> network_node_api::get_potential_peers() const
   {
      return _app.p2p_node()->get_potential_peers();
   }

   fc::variant_object network_node_api::get_advanced_node_parameters() const
   {
      return _app.p2p_node()->get_advanced_node_parameters();
   }

   void network_node_api::set_advanced_node_parameters(const fc::variant_object& params)
   {
      return _app.p2p_node()->set_advanced_node_parameters(params);
   }

   vector<operation_history_object> history_api::get_account_history( account_id_type account,
                                                                      operation_history_id_type stop,
                                                                      unsigned limit,
                                                                      operation_history_id_type start ) const
   {
      FC_ASSERT( _app.chain_database() );
      const auto& db = *_app.chain_database();
      FC_ASSERT( limit <= 100 );
      vector<operation_history_object> result;
      const auto& stats = account(db).statistics(db);
      const account_transaction_history_object* node = db.find(stats.most_recent_op);
      if( stats.most_recent_op == account_transaction_history_id_type() && nullptr == node) { return result; }
      if( start == operation_history_id_type() || start.instance.value > node->operation_id.instance.value )
         start = node->operation_id;

      const auto& hist_idx = db.get_index_type<account_transaction_history_index>();
      const auto& by_op_idx = hist_idx.indices().get<by_op>();
      auto index_start = by_op_idx.begin();
      auto itr = by_op_idx.lower_bound(boost::make_tuple(account, start));

      while( itr != index_start && itr->account == account && itr->operation_id.instance.value > stop.instance.value && result.size() < limit )
      {
         if( itr->operation_id.instance.value <= start.instance.value )
            result.push_back(itr->operation_id(db));
         --itr;
      }

      if( stop.instance.value == 0 && itr->account == account && itr->operation_id.instance.value <= start.instance.value && result.size() < limit )
         result.push_back(itr->operation_id(db));

      return result;
   }

   vector<operation_history_object> history_api::get_relative_account_history( account_id_type account,
                                                                               uint32_t stop,
                                                                               unsigned limit,
                                                                               uint32_t start) const
   {
      FC_ASSERT( _app.chain_database() );
      const auto& db = *_app.chain_database();
      FC_ASSERT(limit <= 100);
      vector<operation_history_object> result;
      if( start == 0 )
        start = account(db).statistics(db).total_ops;
      else start = min( account(db).statistics(db).total_ops, start );

      if( start >= stop )
      {
         const auto& hist_idx = db.get_index_type<account_transaction_history_index>();
         const auto& by_seq_idx = hist_idx.indices().get<by_seq>();
         auto itr = by_seq_idx.upper_bound( boost::make_tuple( account, start ) );
         auto itr_stop = by_seq_idx.lower_bound( boost::make_tuple( account, stop ) );

         do
         {
            --itr;
            result.push_back( itr->operation_id(db) );
         }
         while( itr != itr_stop && result.size() < limit );
      }

      return result;
   }

   vector<balance_change_result> history_api::search_account_balance_history(account_id_type account_id,
                                                                             const flat_set<asset_id_type>& assets_list,
                                                                             fc::optional<account_id_type> partner_account_id,
                                                                             uint32_t from_block, uint32_t to_block,
                                                                             uint32_t start_offset,
                                                                             unsigned limit) const
   {
      vector<balance_change_result> result;
      vector<operation_history_object> current_history;
      operation_history_id_type start;
      uint32_t current_history_offset = 0;
      uint32_t current_offset = 0;
      bool account_history_query_required = true;
      result.reserve(limit);

      FC_ASSERT( _app.chain_database() );
      const auto& db = *_app.chain_database();

      try
      {
         do
         {
            if (account_history_query_required)
            {
               current_history = this->get_account_history(account_id, operation_history_id_type(), 100, start);
               account_history_query_required = false;
            }

            // access and store the current account history object
            if (current_history_offset < current_history.size())
            {
               auto &o = current_history.at(current_history_offset);

               // if no block range is specified or if the current block is within the block range
               if ((from_block == 0 && to_block == 0) || (o.block_num >= from_block && o.block_num <= to_block))
               {
                  // create the balance change result object
                  balance_change_result info;
                  info.hist_object = o;
                  graphene::app::operation_get_balance_history(o.op, account_id, info.balance, info.fee);
                  info.timestamp = db.fetch_block_by_number(o.block_num)->timestamp;
                  info.transaction_id = db.fetch_block_by_number(o.block_num)->transactions[o.trx_in_block].id();

                  if (info.balance.asset0.amount != 0ll || info.balance.asset1.amount != 0ll || info.fee.amount != 0ll)
                  {
                     bool is_non_zero_balance_for_asset0 = (info.balance.asset0.amount != 0ll && assets_list.find(info.balance.asset0.asset_id) != assets_list.end());
                     bool is_non_zero_balance_for_asset1 = (info.balance.asset1.amount != 0ll && assets_list.find(info.balance.asset1.asset_id) != assets_list.end());
                     bool is_non_zero_fee = (info.fee.amount != 0ll && assets_list.find(info.fee.asset_id) != assets_list.end());

                     if (assets_list.empty() || is_non_zero_balance_for_asset0 || is_non_zero_balance_for_asset1 || is_non_zero_fee)
                     {
                        bool skip_due_to_partner_account_id = false;

                        if (partner_account_id)
                        {
                           if (o.op.which() == operation::tag<transfer_obsolete_operation>::value)
                           {
                              const transfer_obsolete_operation& top = o.op.get<transfer_obsolete_operation>();
                              if (! top.is_partner_account_id(*partner_account_id))
                                 skip_due_to_partner_account_id = true;
                           }
                           else if (o.op.which() == operation::tag<transfer_operation>::value)
                           {
                              const transfer_operation& top = o.op.get<transfer_operation>();
                              if (! top.is_partner_account_id(*partner_account_id))
                                 skip_due_to_partner_account_id = true;
                           }
                        }

                        if (! skip_due_to_partner_account_id)
                        {
                           // store the balance change result object
                           if (current_offset >= start_offset)
                              result.push_back(info);
                           current_offset++;
                        }
                     }
                  }
               }
            }
            // rolling in the account transaction history
            else if (! current_history.empty())
            {
               account_history_query_required = true;
               current_history_offset = 0;
               start = current_history.back().id;
               if (start != operation_history_id_type())
                  start = start + (-1);
            }

            if (! account_history_query_required)
               current_history_offset++;
         }
         // while the limit is not reached and there are potentially more entries to be processed
         while (result.size() < limit && ! current_history.empty() && current_history_offset <= current_history.size() && (current_history_offset != 0 || start != operation_history_id_type()));

         return result;
      }
      FC_CAPTURE_AND_RETHROW((account_id)(assets_list)(partner_account_id)(from_block)(to_block)(start_offset)(limit));
   }

   fc::optional<balance_change_result> history_api::get_account_balance_for_transaction(account_id_type account_id,
                                                                                        operation_history_id_type operation_history_id)
   {
       vector<operation_history_object> operation_list = this->get_account_history(account_id,
                                                                           operation_history_id_type(),
                                                                           1,
                                                                           operation_history_id);
       if (operation_list.empty())
           return fc::optional<balance_change_result>();

       balance_change_result result;
       result.hist_object = operation_list.front();

       graphene::app::operation_get_balance_history(result.hist_object.op, account_id, result.balance, result.fee);

       return result;
   }

   crypto_api::crypto_api(application& a) : _app(a)
   {
   }

   public_key_type crypto_api::get_public_key( const string &wif_priv_key )
   {
      return wif_to_private_key( wif_priv_key ).get_public_key();
   }

   private_key_type crypto_api::wif_to_private_key(const string &wif)
   {
       fc::optional<private_key_type> key = graphene::utilities::wif_to_key(wif);
       FC_ASSERT(key.valid(), "Malformed private key");
       return *key;
   }

   signed_transaction crypto_api::sign_transaction(signed_transaction trx, const private_key_type &key)
   {
       trx.sign(key, _app.chain_database()->get_chain_id());
       return trx;
   }

   memo_data crypto_api::encrypt_message(const std::string &message, const private_key_type &key, const public_key_type &pub, uint64_t nonce) const
   {
       return memo_data(message, key, pub, nonce);
   }

   std::string crypto_api::decrypt_message(const memo_data::message_type &message, const private_key_type &key, const public_key_type &pub, uint64_t nonce) const
   {
      return memo_data::decrypt_message(message, key, pub, nonce);
   }

   messaging_api::messaging_api(application& a) : _app(a)
   {
   }

   template<typename ID, typename COMP>
   void find_message_objects(vector<message_object>& result, const ID& ids, const std::set<graphene::db::object_id_type>& objs, uint32_t max_count, const COMP& cmp)
   {
      for (const object_id_type& item : objs) {
         if (max_count == 0)
            return;
         auto msg_itr = ids.find(item);
         if (msg_itr != ids.end()) {
            const message_object& msg_obj = *msg_itr;
            if (cmp(msg_obj.sender)) {
               result.emplace_back(msg_obj);
               --max_count;
            }
         }
      }
   }

   vector<message_object> messaging_api::get_message_objects(optional<account_id_type> sender, optional<account_id_type> receiver, uint32_t max_count) const
   {
      FC_ASSERT(sender.valid() || receiver.valid(), "at least one of the accounts needs to be specified");
      FC_ASSERT(_app.chain_database());
      const auto& db = *_app.chain_database();
      const auto& idx = db.get_index_type<message_index>();

      vector<message_object> result;
      result.reserve(max_count);
      if (receiver) {
         try {
            (*receiver)(db);
         }
         FC_CAPTURE_AND_RETHROW( (receiver) );

         const auto& ids = idx.indices().get<graphene::db::by_id>();
         const auto& midx = dynamic_cast<const graphene::db::primary_index<message_index>&>(idx);
         const auto& refs = midx.get_secondary_index<graphene::chain::message_receiver_index>();
         auto itr = refs.message_to_receiver_memberships.find(*receiver);

         if (itr != refs.message_to_receiver_memberships.end())
         {
            result.reserve(itr->second.size());
            if (sender) {
               try {
                  (*sender)(db);
               }
               FC_CAPTURE_AND_RETHROW( (sender) );

               find_message_objects(result, ids, itr->second, max_count, [&](const account_id_type& s) { return s == *sender; });
            }
            else {
               find_message_objects(result, ids, itr->second, max_count, [](const account_id_type&) { return true; });
            }
         }
      }
      else if (sender) {
         try {
            (*sender)(db);
         }
         FC_CAPTURE_AND_RETHROW( (sender) );

         auto range = idx.indices().get<by_sender>().equal_range(*sender);
         while (range.first != range.second && max_count-- > 0) {
            result.emplace_back(*range.first++);
         }
      }

      return result;
   }

   monitoring_api::monitoring_api()
   {
   }

   std::string monitoring_api::info() const
   {
      return get_api_name();
   }

   void monitoring_api::reset_counters(const std::vector<std::string>& names)
   {
      monitoring_counters_base::reset_counters(names);
   }

   std::vector<counter_item> monitoring_api::get_counters(const std::vector<std::string>& names) const
   {
      std::vector<counter_item> result;
      monitoring_counters_base::get_counters(names, result);
      return result;
   }
} } // graphene::app
