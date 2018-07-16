/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
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

#include <graphene/app/api.hpp>
#include <graphene/app/api_access.hpp>
#include <graphene/app/application.hpp>
#include <graphene/app/impacted.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/get_config.hpp>
#include <graphene/utilities/key_conversion.hpp>
#include <graphene/chain/protocol/fee_schedule.hpp>
#include <graphene/chain/transaction_object.hpp>
#include <graphene/chain/withdraw_permission_object.hpp>
#include <graphene/seeding/seeding_utility.hpp>
#include <graphene/app/balance.hpp>


#include <fc/crypto/hex.hpp>
#include <fc/smart_ref_impl.hpp>

#include <boost/spirit/home/support/container.hpp>
#include <boost/multiprecision/cpp_int.hpp>

namespace decent { namespace seeding {
      fc::promise<decent::seeding::seeding_plugin_startup_options>::ptr seeding_promise;
}}

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
          _crypto_api = std::make_shared< crypto_api >();
       }
       else if (api_name == "messaging_api")
       {
          _messaging_api = std::make_shared< messaging_api >( std::ref(_app) );
       }
       else if( api_name == "debug_api" )
       {
          // can only enable this API if the plugin was loaded
          if( _app.get_plugin( "debug_miner" ) )
             _debug_api = std::make_shared< graphene::debug_miner::debug_api >( std::ref(_app) );
       }
       return;
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
                
                fc::async( [capture_this,this,confv, callback](){ callback( confv ); } );
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

    fc::api<graphene::debug_miner::debug_api> login_api::debug() const
    {
       FC_ASSERT(_debug_api);
       return *_debug_api;
    }

    fc::api<graphene::app::messaging_api> login_api::messaging() const
    {
       FC_ASSERT(_messaging_api);
       return *_messaging_api;
    }

    vector<account_id_type> get_relevant_accounts( const object* obj )
    {
       vector<account_id_type> result;
       if( obj->id.space() == protocol_ids )
       {
          switch( (object_type)obj->id.type() )
          {
            case null_object_type:
            case base_object_type:
            case OBJECT_TYPE_COUNT:
               return result;
            case account_object_type:{
               result.push_back( obj->id );
               break;
            } case asset_object_type:{
               const auto& aobj = dynamic_cast<const asset_object*>(obj);
               assert( aobj != nullptr );
               result.push_back( aobj->issuer );
               break;
           } case miner_object_type:{
               const auto& aobj = dynamic_cast<const miner_object*>(obj);
               assert( aobj != nullptr );
               result.push_back( aobj->miner_account );
               break;
            } case custom_object_type:{
              break;
            } case proposal_object_type:{
               const auto& aobj = dynamic_cast<const proposal_object*>(obj);
               assert( aobj != nullptr );
               flat_set<account_id_type> impacted;
               transaction_get_impacted_accounts( aobj->proposed_transaction, impacted );
               result.reserve( impacted.size() );
               for( auto& item : impacted ) result.emplace_back(item);
               break;
            } case operation_history_object_type:{
               const auto& aobj = dynamic_cast<const operation_history_object*>(obj);
               assert( aobj != nullptr );
               flat_set<account_id_type> impacted;
               operation_get_impacted_accounts( aobj->op, impacted );
               result.reserve( impacted.size() );
               for( auto& item : impacted ) result.emplace_back(item);
               break;
            } case withdraw_permission_object_type:{
               const auto& aobj = dynamic_cast<const withdraw_permission_object*>(obj);
               assert( aobj != nullptr );
               result.push_back( aobj->withdraw_from_account );
               result.push_back( aobj->authorized_account );
               break;
            } case vesting_balance_object_type:{
               const auto& aobj = dynamic_cast<const vesting_balance_object*>(obj);
               assert( aobj != nullptr );
               result.push_back( aobj->owner );
               break;
            }
          }
       }
       else if( obj->id.space() == implementation_ids )
       {
          switch( (impl_object_type)obj->id.type() )
          {
                 case impl_global_property_object_type:
                  break;
                 case impl_dynamic_global_property_object_type:
                  break;
                 case impl_reserved0_object_type:
                  break;
                 case impl_asset_dynamic_data_type:
                  break;
                 case impl_account_balance_object_type:{
                  const auto& aobj = dynamic_cast<const account_balance_object*>(obj);
                  assert( aobj != nullptr );
                  result.push_back( aobj->owner );
                  break;
               } case impl_account_statistics_object_type:{
                  const auto& aobj = dynamic_cast<const account_statistics_object*>(obj);
                  assert( aobj != nullptr );
                  result.push_back( aobj->owner );
                  break;
               } case impl_transaction_object_type:{
                  const auto& aobj = dynamic_cast<const transaction_object*>(obj);
                  assert( aobj != nullptr );
                  flat_set<account_id_type> impacted;
                  transaction_get_impacted_accounts( aobj->trx, impacted );
                  result.reserve( impacted.size() );
                  for( auto& item : impacted ) result.emplace_back(item);
                  break;
               } case impl_block_summary_object_type:
                  break;
                 case impl_account_transaction_history_object_type:
                  break;
                 case impl_chain_property_object_type:
                  break;
                 case impl_miner_schedule_object_type:
                  break;
                 case impl_budget_record_object_type:
                  break;
                 case impl_buying_object_type:{
                  const auto& bobj = dynamic_cast<const buying_object*>(obj);
                  assert( bobj != nullptr );
                  result.push_back( bobj->consumer );
                  break;
                 }
                 case impl_content_object_type:{
                    const auto& cobj = dynamic_cast<const content_object*>(obj);
                    assert( cobj != nullptr );
                    result.push_back( cobj->author );
                    break;
                 }
                 case impl_publisher_object_type:{
                    const auto& sobj = dynamic_cast<const seeder_object*>(obj);
                    assert( sobj != nullptr );
                    result.push_back( sobj->seeder );
                    break;
                 }
                case impl_subscription_object_type:{
                   const auto& sobj = dynamic_cast<const subscription_object*>(obj);
                   assert( sobj != nullptr );
                   result.push_back( sobj->from );
                   result.push_back( sobj->to );
                   break;
                }
                case impl_seeding_statistics_object_type:{
                   const auto& ssobj = dynamic_cast<const seeding_statistics_object*>(obj);
                   assert( ssobj != nullptr );
                   result.push_back( ssobj->seeder );
                   break;
                }
                case impl_transaction_detail_object_type:
                   break;
          }
       }
       return result;
    } // end get_relevant_accounts( obj )



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
       if( start == operation_history_id_type() )
          start = node->operation_id;
          
       while(node && node->operation_id.instance.value > stop.instance.value && result.size() < limit)
       {
          if( node->operation_id.instance.value <= start.instance.value )
             result.push_back( node->operation_id(db) );
          if( node->next == account_transaction_history_id_type() )
             node = nullptr;
          else node = &node->next(db);
       }
       
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
       const auto& hist_idx = db.get_index_type<account_transaction_history_index>();
       const auto& by_seq_idx = hist_idx.indices().get<by_seq>();
       
       auto itr = by_seq_idx.upper_bound( boost::make_tuple( account, start ) );
       auto itr_stop = by_seq_idx.lower_bound( boost::make_tuple( account, stop ) );
       --itr;
       
       while ( itr != itr_stop && result.size() < limit )
       {
          result.push_back( itr->operation_id(db) );
          --itr;
       }
       
       return result;
    }

    vector<balance_change_result> history_api::search_account_balance_history(account_id_type account_id,
                                                                              const flat_set<asset_id_type>& assets_list,
                                                                              fc::optional<account_id_type> partner_account_id,
                                                                              uint32_t from_block, uint32_t to_block,
                                                                              const string& order,
                                                                              uint32_t offset,
                                                                              int limit) const
    {
        vector<balance_change_result> result;
        operation_history_id_type start;

        do {

            vector<operation_history_object> current = this->get_account_history(account_id, operation_history_id_type(), std::min(100,limit), start);
            if (current.empty())
                break;

            for( auto& o : current ) {

                balance_change_result info;
                info.hist_object = o;
                graphene::app::operation_get_balance_history(o.op, account_id, info.balance, info.fee);

                if (info.balance.a0.amount == 0ll && info.balance.a1.amount == 0ll && info.fee.amount == 0ll)
                    continue;

                if (assets_list.empty() ||
                    ((info.balance.a0.amount != 0ll && assets_list.find(info.balance.a0.asset_id) != assets_list.end()) ||
                     (info.balance.a1.amount != 0ll && assets_list.find(info.balance.a0.asset_id) != assets_list.end()) ))
                {
                    if (partner_account_id) {
                        if (o.op.which() == operation::tag<transfer_operation>::value) {
                            const transfer_operation& top = o.op.get<transfer_operation>();
                            if (!top.is_partner_account_id(*partner_account_id))
                                continue;
                        }
                        if (o.op.which() == operation::tag<transfer2_operation>::value) {
                            const transfer2_operation& top = o.op.get<transfer2_operation>();
                            if (!top.is_partner_account_id(*partner_account_id))
                                continue;
                        }
                    }

                    result.push_back( info );
                }

                if (result.size() >= limit)
                   break;
            }

            start = current.back().id;
            start = start + (-1);

        } while(result.size() < limit);

        //TODO: ordering...


        return result;
    }

    fc::optional<balance_change_result> history_api::get_account_balance_for_transaction(account_id_type account_id,
                                                                                     operation_history_id_type transaction_id)
    {
        vector<operation_history_object> operation_list = this->get_account_history(account_id,
                                                                            operation_history_id_type(),
                                                                            1,
                                                                            transaction_id);
        if (operation_list.empty())
            return fc::optional<balance_change_result>();

        balance_change_result result;
        result.hist_object = operation_list.front();

        graphene::app::operation_get_balance_history(result.hist_object.op, account_id, result.balance, result.fee);

        return result;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    crypto_api::crypto_api(){};
    
    blind_signature crypto_api::blind_sign( const extended_private_key_type& key, const blinded_hash& hash, int i )
    {
       return fc::ecc::extended_private_key( key ).blind_sign( hash, i );
    }
         
    signature_type crypto_api::unblind_signature( const extended_private_key_type& key,
                                                     const extended_public_key_type& bob,
                                                     const blind_signature& sig,
                                                     const fc::sha256& hash,
                                                     int i )
    {
       return fc::ecc::extended_private_key( key ).unblind_signature( extended_public_key( bob ), sig, hash, i );
    }
                                                               
    commitment_type crypto_api::blind( const blind_factor_type& blind, uint64_t value )
    {
       return fc::ecc::blind( blind, value );
    }
   
    blind_factor_type crypto_api::blind_sum( const std::vector<blind_factor_type>& blinds_in, uint32_t non_neg )
    {
       return fc::ecc::blind_sum( blinds_in, non_neg );
    }
   
    bool crypto_api::verify_sum( const std::vector<commitment_type>& commits_in, const std::vector<commitment_type>& neg_commits_in, int64_t excess )
    {
       return fc::ecc::verify_sum( commits_in, neg_commits_in, excess );
    }
    
    verify_range_result crypto_api::verify_range( const commitment_type& commit, const std::vector<char>& proof )
    {
       verify_range_result result;
       result.success = fc::ecc::verify_range( result.min_val, result.max_val, commit, proof );
       return result;
    }
    
    std::vector<char> crypto_api::range_proof_sign( uint64_t min_value, 
                                                    const commitment_type& commit, 
                                                    const blind_factor_type& commit_blind, 
                                                    const blind_factor_type& nonce,
                                                    int8_t base10_exp,
                                                    uint8_t min_bits,
                                                    uint64_t actual_value )
    {
       return fc::ecc::range_proof_sign( min_value, commit, commit_blind, nonce, base10_exp, min_bits, actual_value );
    }
                               
    verify_range_proof_rewind_result crypto_api::verify_range_proof_rewind( const blind_factor_type& nonce,
                                                                            const commitment_type& commit, 
                                                                            const std::vector<char>& proof )
    {
       verify_range_proof_rewind_result result;
       result.success = fc::ecc::verify_range_proof_rewind( result.blind_out, 
                                                            result.value_out, 
                                                            result.message_out, 
                                                            nonce, 
                                                            result.min_val, 
                                                            result.max_val, 
                                                            const_cast< commitment_type& >( commit ), 
                                                            proof );
       return result;
    }
                                    
    range_proof_info crypto_api::range_get_info( const std::vector<char>& proof )
    {
       return fc::ecc::range_get_info( proof );
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
             int count = itr->second.size();
             int counter = 0;
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

          int count = distance(range.first, range.second);
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
} } // graphene::app
