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

#ifndef STDAFX_APP_H
#include "stdafx.h"
#endif

#include <graphene/app/api.hpp>
#include <graphene/app/application.hpp>
#include <graphene/app/balance.hpp>
#include <graphene/app/impacted.hpp>
#include <graphene/app/seeding_utility.hpp>

namespace decent { namespace seeding {
      fc::promise<decent::seeding::seeding_plugin_startup_options>::ptr seeding_promise;
}}
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
       if( api_name == "database_api" )
       {
          _database_api = std::make_shared< database_api >( std::ref( *_app.chain_database() ) );
       }
       else if( api_name == "network_broadcast_api" )
       {
          _network_broadcast_api = std::make_shared< network_broadcast_api >( std::ref( _app ) );
       }
       else if( api_name == "history_api" )
       {
          _history_api = std::make_shared< history_api >( _app );
       }
       else if( api_name == "network_node_api" )
       {
          _network_node_api = std::make_shared< network_node_api >( std::ref(_app) );
       }
       else if( api_name == "crypto_api" )
       {
          _crypto_api = std::make_shared< crypto_api >( std::ref(_app) );
       }
       else if (api_name == "messaging_api")
       {
          _messaging_api = std::make_shared< messaging_api >( std::ref(_app) );
       }
       else if (api_name == "monitoring_api")
       {
          _monitoring_api = std::make_shared< monitoring_api >();
       }
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

    void network_node_api::seeding_startup(const account_id_type& account_id,
                                           const DInteger& content_private_key,
                                           const fc::ecc::private_key& seeder_private_key,
                                           const uint64_t free_space,
                                           const uint32_t seeding_price,
                                           const string seeding_symbol,
                                           const string packages_path,
                                           const string region_code)
    {
       FC_ASSERT( free_space > 0 );
       FC_ASSERT( seeding_price >= 0 );

       decent::seeding::seeding_plugin_startup_options seeding_options;
       seeding_options.seeder = account_id;
       seeding_options.content_private_key = content_private_key;
       seeding_options.seeder_private_key = seeder_private_key;
       seeding_options.free_space = free_space;
       seeding_options.seeding_price = seeding_price;
       seeding_options.seeding_symbol = seeding_symbol;
       seeding_options.packages_path = packages_path;
       seeding_options.region_code = region_code;
       decent::seeding::seeding_promise->set_value( seeding_options );
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
       if( stats.most_recent_op == account_transaction_history_id_type() ) return result;
       const account_transaction_history_object* node = &stats.most_recent_op(db);
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

       if( stop.instance.value == 0 && result.size() < limit && itr->account == account )
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
                                                                              int limit) const
    {
       FC_ASSERT(limit > 0);
        vector<balance_change_result> tmp_result;
        operation_history_id_type start;
        int32_t offset_counter = -1;

        tmp_result.reserve(limit);

        do {

            vector<operation_history_object> current = this->get_account_history(account_id, operation_history_id_type(), 100, start);
            if (current.empty())
                break;

            for( auto& o : current ) {

               offset_counter++;

                if (from_block != 0 && to_block != 0) {
                    if (o.block_num < from_block || o.block_num > to_block)
                        continue;
                }

                balance_change_result info;
                info.hist_object = o;
                graphene::app::operation_get_balance_history(o.op, account_id, info.balance, info.fee);

                if (info.balance.asset0.amount == 0ll && info.balance.asset1.amount == 0ll && info.fee.amount == 0ll)
                    continue;

                if (assets_list.empty() ||
                    ((info.balance.asset0.amount != 0ll && assets_list.find(info.balance.asset0.asset_id) != assets_list.end()) ||
                     (info.balance.asset1.amount != 0ll && assets_list.find(info.balance.asset0.asset_id) != assets_list.end()) ))
                {
                    if (partner_account_id) {
                        if (o.op.which() == operation::tag<transfer_operation>::value) {
                            const transfer_operation& top = o.op.get<transfer_operation>();
                            if (!top.is_partner_account_id(*partner_account_id))
                                continue;
                        }
                        else if (o.op.which() == operation::tag<transfer2_operation>::value) {
                            const transfer2_operation& top = o.op.get<transfer2_operation>();
                            if (!top.is_partner_account_id(*partner_account_id))
                                continue;
                        }
                    }
                    if(offset_counter >= start_offset)
                       tmp_result.push_back( info );
                }

                if (tmp_result.size() >= (limit))
                   break;
            }

            start = current.back().id;
            start = start + (-1);
            if (start == operation_history_id_type())
               break;

        } while(tmp_result.size() < (limit));

        vector<balance_change_result> result;
        std::copy(tmp_result.begin(), tmp_result.end(), std::back_inserter(result));

        return result;
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

    vector<message_object> messaging_api::get_message_objects(optional<account_id_type> sender, optional<account_id_type> receiver, uint32_t max_count) const
    {
       FC_ASSERT(_app.chain_database());
       const auto& db = *_app.chain_database();
       vector<message_object> result;

       if (receiver) {
        
          const auto& idx = db.get_index_type<message_index>();
          const auto& aidx = dynamic_cast<const primary_index<message_index>&>(idx);
          const auto& refs = aidx.get_secondary_index<graphene::chain::message_receiver_index>();
          auto itr = refs.message_to_receiver_memberships.find(*receiver);

          if (itr != refs.message_to_receiver_memberships.end())
          {
             result.reserve(itr->second.size());
             uint32_t count = itr->second.size();
             uint32_t counter = 0;
             if (sender) {
                for (const object_id_type& item : itr->second) {
                   if (result.size() >= max_count)
                      break;
                   auto msg_itr = db.get_index_type<message_index>().indices().get<by_id>().find(item);
                   if (msg_itr != db.get_index_type<message_index>().indices().get<by_id>().end()) {
                      message_object o = *msg_itr;
                      if (count - counter <= max_count && (*msg_itr).sender == *sender)
                        result.emplace_back(o);
                      counter++;
                   }
                }
             }
             else
             {
                for (const object_id_type& item : itr->second) {
                   if (result.size() >= max_count)
                      break;
                   auto msg_itr = db.get_index_type<message_index>().indices().get<by_id>().find(item);
                   if (msg_itr != db.get_index_type<message_index>().indices().get<by_id>().end()) {
                      message_object o = *msg_itr;
                      if (count - counter <= max_count)
                        result.emplace_back(o);
                      counter++;
                   }
                }
             }
          }
       }
       else 
       if(sender) {
          const auto& range = db.get_index_type<message_index>().indices().get<by_sender>().equal_range(*sender);
          const auto& index_by_sender = db.get_index_type<message_index>().indices().get<by_sender>();
          auto itr = index_by_sender.lower_bound(*sender);
          itr = range.first;

          uint32_t count = distance(range.first, range.second);
          if (count) {
             result.reserve(count);
             int counter = 0;
            
             while (itr != range.second && result.size() < max_count) {
               if (count - counter <= max_count)
                  result.emplace_back(*itr);
               itr++;
               counter++;
             }
          }
       }
       
       return result;
    }

    monitoring_api::monitoring_api()
    {
    }

    std::string monitoring_api::info() const
    {
       return "monitoring_api";
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
