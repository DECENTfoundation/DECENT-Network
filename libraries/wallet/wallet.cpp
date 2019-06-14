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
#include <functional>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <utility>
#include <string>
#include <list>
#include <map>

#include <nlohmann/json.hpp>
#include <ipfs/client.h>

#include <boost/version.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string.hpp>

#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/range/algorithm/unique.hpp>
#include <boost/range/algorithm/sort.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/hashed_index.hpp>

#include <fc/git_revision.hpp>
#include <fc/monitoring.hpp>
#include <fc/io/fstream.hpp>
#include <fc/io/json.hpp>
#include <fc/io/stdio.hpp>
#include <fc/io/console.hpp>
#include <fc/network/http/websocket.hpp>
#include <fc/rpc/cli.hpp>
#include <fc/rpc/websocket_api.hpp>
#include <fc/crypto/aes.hpp>
#include <fc/crypto/hex.hpp>
#include <fc/thread/mutex.hpp>
#include <fc/thread/scoped_lock.hpp>
#include <fc/rpc/api_connection.hpp>

#include <graphene/app/api.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/hardfork.hpp>
#include <graphene/chain/protocol/fee_schedule.hpp>
#include <graphene/utilities/git_revision.hpp>
#include <graphene/utilities/key_conversion.hpp>
#include <graphene/utilities/keys_generator.hpp>
#include <graphene/utilities/string_escape.hpp>
#include <graphene/utilities/words.hpp>
#include <graphene/wallet/wallet.hpp>
#include <graphene/wallet/wallet_utility.hpp>
#include <graphene/wallet/api_documentation.hpp>
#include <graphene/wallet/reflect_util.hpp>
#include <graphene/chain/custom_evaluator.hpp>

#include <decent/encrypt/encryptionutils.hpp>
#include <decent/package/package.hpp>
#include <decent/package/package_config.hpp>
#include <decent/ipfs_check.hpp>
#include <decent/about.hpp>

#ifndef WIN32
# include <sys/types.h>
# include <sys/stat.h>
#endif

CryptoPP::AutoSeededRandomPool randomGenerator;

using namespace decent::package;



namespace graphene { namespace wallet {

namespace detail {

// this class helps to gather seeding statistics. Tracks seeders currently in use.
class seeders_tracker{
public:
   seeders_tracker(wallet_api_impl& wallet):_wallet(wallet) {};
   vector<account_id_type> track_content(const string& URI);
   vector<account_id_type> untrack_content(const string& URI);
   bool is_empty() { return seeder_to_content.empty(); };
   vector<account_id_type> get_unfinished_seeders();
   void set_initial_stats( const account_id_type& seeder, const uint64_t amount );
   uint64_t get_final_stats( const account_id_type& seeder, const uint64_t amount );
   void remove_stats( const account_id_type& seeder );
private:
   wallet_api_impl& _wallet;
   multimap<account_id_type, content_id_type> seeder_to_content;
   map<account_id_type, uint64_t> initial_stats;
};

struct ipfs_stats_listener : public EventListenerInterface{

   ipfs_stats_listener(const string& URI, wallet_api_impl& api, account_id_type consumer) : _URI(URI), _wallet(api), _consumer(consumer),
      _ipfs_client(PackageManagerConfigurator::instance().get_ipfs_host(), PackageManagerConfigurator::instance().get_ipfs_port()){}

   virtual void package_download_start();
   virtual void package_download_complete();
   virtual void package_download_error(const std::string&);

private:
   string            _URI;
   wallet_api_impl&  _wallet;
   account_id_type   _consumer;
   ipfs::Client      _ipfs_client;
};

struct submit_transfer_listener : public EventListenerInterface {
   
   submit_transfer_listener(wallet_api_impl& wallet, shared_ptr<PackageInfo> info, const content_submit_operation& op, const std::string& protocol)
      : _wallet(wallet), _info(info), _op(op), _protocol(protocol), _is_finished(false) {
   }
   
   virtual void package_seed_complete();
   virtual void package_creation_complete();
   
   fc::ripemd160 get_hash() const { return _info->get_hash(); }
   const content_submit_operation& op() const { return _op; }
   bool is_finished() { return _is_finished; }
   
private:
   wallet_api_impl&          _wallet;
   shared_ptr<PackageInfo>   _info;
   content_submit_operation  _op;
   std::string               _protocol;
   bool                      _is_finished;
};

struct operation_result_printer
{
public:
   operation_result_printer( const wallet_api_impl& w )
      : _wallet(w) {}
   const wallet_api_impl& _wallet;
   typedef std::string result_type;

   std::string operator()(const void_result& x) const;
   std::string operator()(const object_id_type& oid);
   std::string operator()(const asset& a);
};

// BLOCK  TRX  OP  VOP
struct operation_printer
{
private:
   ostream& out;
   const wallet_api_impl& wallet;
   operation_result result;

   void fee(const asset& a) const;
   std::string memo(const optional<memo_data>& data, const account_object& from, const account_object& to) const;

public:
   operation_printer( ostream& out, const wallet_api_impl& wallet, const operation_result& r = operation_result() )
      : out(out),
        wallet(wallet),
        result(r)
   {}
   typedef std::string result_type;

   template<typename T>
   std::string operator()(const T& op)const;

   std::string operator()(const transfer_obsolete_operation& op)const;
   std::string operator()(const transfer_operation& op)const;
   std::string operator()(const non_fungible_token_issue_operation& op) const;
   std::string operator()(const non_fungible_token_transfer_operation& op) const;
   std::string operator()(const account_create_operation& op)const;
   std::string operator()(const account_update_operation& op)const;
   std::string operator()(const asset_create_operation& op)const;
   std::string operator()(const content_submit_operation& op)const;
   std::string operator()(const request_to_buy_operation& op)const;
   std::string operator()(const leave_rating_and_comment_operation& op)const;
   std::string operator()(const ready_to_publish_operation& op)const;
   std::string operator()(const custom_operation& op)const;
};


template<class T>
optional<T> maybe_id( const string& name_or_id )
{
   if( std::isdigit( name_or_id.front() ) )
   {
      try
      {
         return fc::variant(name_or_id).as<T>();
      }
      catch (const fc::exception&)
      {
      }
   }
   return optional<T>();
}

struct op_prototype_visitor
{
   typedef void result_type;

   int t = 0;
   flat_map< std::string, operation >& name2op;

   op_prototype_visitor(
      int _t,
      flat_map< std::string, operation >& _prototype_ops
      ):t(_t), name2op(_prototype_ops) {}

   template<typename Type>
   result_type operator()( const Type& op )const
   {
      string name = fc::get_typename<Type>::name();
      size_t p = name.rfind(':');
      if( p != string::npos )
         name = name.substr( p+1 );
      name2op[ name ] = Type();
   }
};

class wallet_api_impl
{
public:
   api_documentation method_documentation;
private:
   void claim_registered_account(const account_object& account)
   {
      auto it = _wallet.pending_account_registrations.find( account.name );
      FC_ASSERT( it != _wallet.pending_account_registrations.end() );
      for (const std::string& wif_key : it->second)
         if( !import_key( account.name, wif_key ) )
         {
            // somebody else beat our pending registration, there is
            //    nothing we can do except log it and move on
            elog( "account ${name} registered by someone else first!",
                  ("name", account.name) );
            // might as well remove it from pending regs,
            //    because there is now no way this registration
            //    can become valid (even in the extremely rare
            //    possibility of migrating to a fork where the
            //    name is available, the user can always
            //    manually re-register)
         }
      _wallet.pending_account_registrations.erase( it );
   }

   // after a miner registration succeeds, this saves the private key in the wallet permanently
   //
   void claim_registered_miner(const std::string& miner_name)
   {
      auto iter = _wallet.pending_miner_registrations.find(miner_name);
      FC_ASSERT(iter != _wallet.pending_miner_registrations.end());
      std::string wif_key = iter->second;

      // get the list key id this key is registered with in the chain
      fc::optional<fc::ecc::private_key> miner_private_key = wif_to_key(wif_key);
      FC_ASSERT(miner_private_key);

      auto pub_key = miner_private_key->get_public_key();
      _keys[pub_key] = wif_key;
      _wallet.pending_miner_registrations.erase(iter);
   }

   fc::mutex _resync_mutex;
   void resync()
   {
      fc::scoped_lock<fc::mutex> lock(_resync_mutex);
      // this method is used to update wallet_data annotations
      //   e.g. wallet has been restarted and was not notified
      //   of events while it was down
      //
      // everything that is done "incremental style" when a push
      //   notification is received, should also be done here
      //   "batch style" by querying the blockchain

      if( !_wallet.pending_account_registrations.empty() )
      {
         // make a vector of the account names pending registration
         std::vector<string> pending_account_names = boost::copy_range<std::vector<string> >(boost::adaptors::keys(_wallet.pending_account_registrations));

         // look those up on the blockchain
         std::vector<fc::optional<graphene::chain::account_object >>
               pending_account_objects = _remote_db->lookup_account_names( pending_account_names );

         // if any of them exist, claim them
         for( const fc::optional<graphene::chain::account_object>& optional_account : pending_account_objects )
            if( optional_account )
               claim_registered_account(*optional_account);
      }

      if (!_wallet.pending_miner_registrations.empty())
      {
         // make a vector of the owner accounts for miners pending registration
         std::vector<string> pending_miner_names = boost::copy_range<std::vector<string> >(boost::adaptors::keys(_wallet.pending_miner_registrations));

         // look up the owners on the blockchain
         std::vector<fc::optional<graphene::chain::account_object>> owner_account_objects = _remote_db->lookup_account_names(pending_miner_names);

         // if any of them have registered miners, claim them
         for( const fc::optional<graphene::chain::account_object>& optional_account : owner_account_objects )
            if (optional_account)
            {
               fc::optional<miner_object> miner_obj = _remote_db->get_miner_by_account(optional_account->id);
               if (miner_obj)
                  claim_registered_miner(optional_account->name);
            }
      }
   }

   void enable_umask_protection()
   {
#ifdef __unix__
      _old_umask = umask( S_IRWXG | S_IRWXO );
#endif
   }

   void disable_umask_protection()
   {
#ifdef __unix__
      umask( _old_umask );
#endif
   }

   void init_prototype_ops()
   {
      operation op;
      for( int t=0; t<op.count(); t++ )
      {
         op.set_which( t );
         op.visit( op_prototype_visitor(t, _prototype_ops) );
      }
      return;
   }

   map<transaction_handle_type, signed_transaction> _builder_transactions;

   // if the user executes the same command twice in quick succession,
   // we might generate the same transaction id, and cause the second
   // transaction to be rejected.  This can be avoided by altering the
   // second transaction slightly (bumping up the expiration time by
   // a second).  Keep track of recent transaction ids we've generated
   // so we can know if we need to do this
   struct recently_generated_transaction_record
   {
      fc::time_point_sec generation_time;
      graphene::chain::transaction_id_type transaction_id;
   };
   struct timestamp_index{};
   typedef boost::multi_index_container<recently_generated_transaction_record,
                                        boost::multi_index::indexed_by<boost::multi_index::hashed_unique<boost::multi_index::member<recently_generated_transaction_record,
                                                                                                                                    graphene::chain::transaction_id_type,
                                                                                                                                    &recently_generated_transaction_record::transaction_id>,
                                                                                                         std::hash<graphene::chain::transaction_id_type> >,
                                                                       boost::multi_index::ordered_non_unique<boost::multi_index::tag<timestamp_index>,
                                                                                                              boost::multi_index::member<recently_generated_transaction_record, fc::time_point_sec, &recently_generated_transaction_record::generation_time> > > > recently_generated_transaction_set_type;
   recently_generated_transaction_set_type _recently_generated_transactions;

public:
   wallet_api& self;
   wallet_api_impl( wallet_api& s, const fc::api<login_api> &rapi, const chain_id_type &chain_id, const server_data &ws )
      : self(s),
        _chain_id(chain_id),
        _remote_api(rapi),
        _remote_db(rapi->database()),
        _remote_net_broadcast(rapi->network_broadcast()),
        _remote_hist(rapi->history()),
        _seeders_tracker(*this)
   {
      chain_id_type remote_chain_id = _remote_db->get_chain_id();
      if( remote_chain_id != _chain_id )
      {
         FC_THROW( "Remote server gave us an unexpected chain_id",
            ("remote_chain_id", remote_chain_id)
            ("chain_id", _chain_id) );
      }
      init_prototype_ops();

      _remote_db->set_block_applied_callback( [this](const variant& block_id )
      {
         on_block_applied( block_id );
      } );

      _wallet.chain_id = _chain_id;
      _wallet.ws_server = ws.server;
      _wallet.ws_user = ws.user;
      _wallet.ws_password = ws.password;
      decent::package::PackageManager::instance().recover_all_packages();
   }
   virtual ~wallet_api_impl()
   {
      try
      {
         _remote_db->cancel_all_subscriptions();
      }
      catch (const fc::exception&)
      {
         // Right now the wallet_api has no way of knowing if the connection to the
         // miner has already disconnected (via the miner node exiting first).
         // If it has exited, cancel_all_subscriptsions() will throw and there's
         // nothing we can do about it.
         // dlog("Caught exception ${e} while canceling database subscriptions", ("e", e));
      }
   }

   void encrypt_keys()
   {

      plain_ec_and_el_gamal_keys data;
      data.ec_keys = _keys;
      std::transform( _el_gamal_keys.begin(), _el_gamal_keys.end(), std::back_inserter( data.el_gamal_keys ),
         [](const std::pair<DInteger,DInteger> el_gamal_pair) {
            return el_gamal_key_pair_str {el_gamal_pair.second, el_gamal_pair.first}; });
      data.checksum = _checksum;
      auto plain_txt = fc::raw::pack(data);
      _wallet.cipher_keys = fc::aes_encrypt( data.checksum, plain_txt );
   }

   void encrypt_keys2()
   {
      plain_ec_and_el_gamal_keys data;
      data.ec_keys = _keys;
      std::transform( _el_gamal_keys.begin(), _el_gamal_keys.end(), std::back_inserter( data.el_gamal_keys ),
                      [](const std::pair<DInteger,DInteger> el_gamal_pair) {
                          return el_gamal_key_pair_str {el_gamal_pair.second, el_gamal_pair.first}; });
      data.checksum = _checksum;
      auto data_string = fc::json::to_string(data);
      vector<char> plain_txt;
      plain_txt.resize(data_string.length());
      memcpy(plain_txt.data(), data_string.data(), data_string.length());
      _wallet.cipher_keys = fc::aes_encrypt( data.checksum, plain_txt );
   }

   void on_block_applied( const variant& block_id )
   {
      fc::async([this]{resync();}, "Resync after block");
   }

   bool is_locked()const
   {
      return _checksum == fc::sha512();
   }

   template<typename T>
   T get_object(graphene::db::object_id<T::space_id, T::type_id, T> id)const
   {
      auto ob = _remote_db->get_objects({id}).front();
      return ob.template as<T>();
   }

   void set_operation_fees( signed_transaction& tx, const fee_schedule& s  ) const
   {
      for( auto& op : tx.operations )
         s.set_fee(op);
   }

   variant info() const
   {
      auto chain_props = get_chain_properties();
      auto global_props = get_global_properties();
      auto dynamic_props = get_dynamic_global_properties();
      fc::mutable_variant_object result;
      result["head_block_num"] = dynamic_props.head_block_number;
      result["head_block_id"] = dynamic_props.head_block_id;
      result["head_block_age"] = fc::get_approximate_relative_time_string(dynamic_props.time,
                                                                          time_point_sec(time_point::now()),
                                                                          " old");
      result["next_maintenance_time"] = fc::get_approximate_relative_time_string(dynamic_props.next_maintenance_time);
      result["chain_id"] = chain_props.chain_id;
      result["participation"] = (100*dynamic_props.recent_slots_filled.popcount()) / 128.0;
      result["active_miners"] = global_props.active_miners;
      return result;
   }

   variant_object about() const
   {
      fc::mutable_variant_object result;
      result["about_cli_wallet"] = decent::get_about_wallet();
      result["about_decentd"] = _remote_db->about();
      return result;
   }

   chain_property_object get_chain_properties() const
   {
      return _remote_db->get_chain_properties();
   }

   global_property_object get_global_properties() const
   {
      return _remote_db->get_global_properties();
   }

   dynamic_global_property_object get_dynamic_global_properties() const
   {
      return _remote_db->get_dynamic_global_properties();
   }

   optional<account_object> find_account(account_id_type account_id) const
   {
      auto rec = _remote_db->get_accounts({account_id}).front();
      
      if(!rec)
         FC_THROW_EXCEPTION(fc::account_does_not_exist_exception, "Account: ${account}", ("account", account_id));

      return *rec;
   }
 
   optional<account_object> find_account(const string& account_name_or_id) const
   {
      if(account_name_or_id.size() == 0)
         FC_THROW_EXCEPTION(fc::account_name_or_id_cannot_be_empty_exception, "Account: ${acc}", ("acc", account_name_or_id));

      if( auto id = maybe_id<account_id_type>(account_name_or_id) )
      {
         // It's an ID
         return find_account(*id);
      } else {
         auto rec = _remote_db->lookup_account_names({ account_name_or_id }).front();
         if(_wallet.my_accounts.get<by_name>().count(account_name_or_id))
         {
            auto local_account = *_wallet.my_accounts.get<by_name>().find(account_name_or_id);
            if(!rec)
               FC_THROW_EXCEPTION(fc::account_in_wallet_not_on_blockchain_exception, "Account: ${acc}", ("acc", account_name_or_id));
            if(local_account.id != rec->id)
               elog("my account id ${id} different from blockchain id ${id2}", ("id", local_account.id)("id2", rec->id));
            if(local_account.name != rec->name)
               elog("my account name ${id} different from blockchain name ${id2}", ("id", local_account.name)("id2", rec->name));

            //return *_wallet.my_accounts.get<by_name>().find(account_name_or_id);
            return rec;
         }
         if(rec && rec->name != account_name_or_id)
            return optional<account_object>();
         return rec;
      }
   }

   account_object get_account(account_id_type account_id) const
   {
      auto rec = find_account(account_id);
      if(!rec)
         FC_THROW_EXCEPTION(fc::account_does_not_exist_exception, "Account: ${acc}", ("acc", account_id));
      return *rec;
   }

   account_object get_account(const string& account_name_or_id) const
   {
      auto rec = find_account(account_name_or_id);
      if(!rec)
         FC_THROW_EXCEPTION(fc::account_does_not_exist_exception, "Account: ${acc}", ("acc", account_name_or_id));
      return *rec;
   }

   optional<asset_object> find_asset(asset_id_type asset_id)const
   {
      return _remote_db->get_assets({asset_id}).front();
   }

   optional<asset_object> find_asset(const string& asset_symbol_or_id) const
   {
      FC_ASSERT( asset_symbol_or_id.size() > 0 );

      if( auto id = maybe_id<asset_id_type>(asset_symbol_or_id) )
      {
         // It's an ID
         return find_asset(*id);
      } else {
         // It's a symbol
         auto rec = _remote_db->lookup_asset_symbols({asset_symbol_or_id}).front();
         if( rec )
         {
            if( rec->symbol != asset_symbol_or_id )
               return optional<asset_object>();
         }
         return rec;
      }
   }

   asset_object get_asset(asset_id_type asset_id)const
   {
      auto opt = find_asset(asset_id);
      FC_ASSERT(opt, "Asset ${asset} does not exist", ("asset", asset_id));
      return *opt;
   }

   asset_object get_asset(const string& asset_symbol_or_id) const
   {
      auto opt = find_asset(asset_symbol_or_id);
      FC_ASSERT(opt, "Asset ${asset} does not exist", ("asset", asset_symbol_or_id));
      return *opt;
   }

   optional<non_fungible_token_object> find_non_fungible_token(non_fungible_token_id_type nft_id)const
   {
      return _remote_db->get_non_fungible_tokens({nft_id}).front();
   }

   optional<non_fungible_token_object> find_non_fungible_token(const string& nft_symbol_or_id) const
   {
      FC_ASSERT( nft_symbol_or_id.size() > 0 );
      if( auto id = maybe_id<non_fungible_token_id_type>(nft_symbol_or_id) )
      {
         // It's an ID
         return find_non_fungible_token(*id);
      } else {
         // It's a symbol
         auto rec = _remote_db->get_non_fungible_tokens_by_symbols({nft_symbol_or_id}).front();
         if( rec )
         {
            if( rec->symbol != nft_symbol_or_id )
               return optional<non_fungible_token_object>();
         }
         return rec;
      }
   }

   non_fungible_token_object get_non_fungible_token(non_fungible_token_id_type nft_id)const
   {
      auto opt = find_non_fungible_token(nft_id);
      FC_ASSERT(opt, "Non fungible token ${nft} does not exist", ("nft", nft_id));
      return *opt;
   }

   non_fungible_token_object get_non_fungible_token(const string& nft_symbol_or_id) const
   {
      auto opt = find_non_fungible_token(nft_symbol_or_id);
      FC_ASSERT(opt, "Non fungible token ${nft} does not exist", ("nft", nft_symbol_or_id));
      return *opt;
   }

   optional<non_fungible_token_data_object> find_non_fungible_token_data(non_fungible_token_data_id_type nft_data_id)const
   {
      return _remote_db->get_non_fungible_token_data({nft_data_id}).front();
   }

   non_fungible_token_data_object get_non_fungible_token_data(non_fungible_token_data_id_type nft_data_id)const
   {
      auto opt = find_non_fungible_token_data(nft_data_id);
      FC_ASSERT(opt, "Non fungible token data ${nft} does not exist", ("nft", nft_data_id));
      return *opt;
   }

   string get_wallet_filename() const
   {
      return _wallet_filename;
   }

   void set_wallet_filename(const string &wallet_filename)
   {
      FC_ASSERT( !wallet_filename.empty() );
      _wallet_filename = wallet_filename;
   }

   fc::ecc::private_key get_private_key(const public_key_type& id)const
   {
      auto it = _keys.find(id);
      FC_ASSERT( it != _keys.end() );

      fc::optional< fc::ecc::private_key > privkey = wif_to_key( it->second );
      FC_ASSERT( privkey );
      return *privkey;
   }

   fc::ecc::private_key get_private_key_for_account(const account_object& account)const
   {
      vector<public_key_type> active_keys = account.active.get_keys();
      if (active_keys.size() != 1)
         FC_THROW("Expecting a simple authority with one active key");
      return get_private_key(active_keys.front());
   }

   // imports the private key into the wallet, and associate it in some way (?) with the
   // given account name.
   // @returns true if the key matches a current active/owner/memo key for the named
   //          account, false otherwise (but it is stored either way)
   bool import_key(const string& account_name_or_id, const string& wif_key)
   {
      FC_ASSERT( !is_locked() );
      fc::optional<fc::ecc::private_key> optional_private_key = wif_to_key(wif_key);
      if (!optional_private_key)
         FC_THROW("Invalid private key");
      graphene::chain::public_key_type wif_pub_key = optional_private_key->get_public_key();

      account_object account = get_account( account_name_or_id );


      // make a list of all current public keys for the named account
      flat_set<public_key_type> all_keys_for_account;
      std::vector<public_key_type> active_keys = account.active.get_keys();
      std::vector<public_key_type> owner_keys = account.owner.get_keys();

      if( std::find( owner_keys.begin(), owner_keys.end(), wif_pub_key ) != owner_keys.end() )
      {
         //we have the owner keys
         int active_key_index = find_first_unused_derived_key_index( *optional_private_key );
         fc::ecc::private_key active_privkey = derive_private_key( wif_key, active_key_index);

         int memo_key_index = find_first_unused_derived_key_index(active_privkey);
         fc::ecc::private_key memo_privkey = derive_private_key( key_to_wif(active_privkey), memo_key_index);

         graphene::chain::public_key_type active_pubkey = active_privkey.get_public_key();
         graphene::chain::public_key_type memo_pubkey = memo_privkey.get_public_key();
         _keys[active_pubkey] = key_to_wif( active_privkey );
         _keys[memo_pubkey] = key_to_wif( memo_privkey );

         DInteger active_el_gamal_priv_key = generate_private_el_gamal_key_from_secret( active_privkey.get_secret() );
         _el_gamal_keys[get_public_el_gamal_key( active_el_gamal_priv_key )] = active_el_gamal_priv_key;
         DInteger memo_el_gamal_priv_key = generate_private_el_gamal_key_from_secret( memo_privkey.get_secret() );
         _el_gamal_keys[get_public_el_gamal_key( memo_el_gamal_priv_key )] = memo_el_gamal_priv_key;

         _wallet.extra_keys[account.id].insert( active_pubkey );
         _wallet.extra_keys[account.id].insert( memo_pubkey );
      }

      std::copy(active_keys.begin(), active_keys.end(), std::inserter(all_keys_for_account, all_keys_for_account.end()));
      std::copy(owner_keys.begin(), owner_keys.end(), std::inserter(all_keys_for_account, all_keys_for_account.end()));
      all_keys_for_account.insert(account.options.memo_key);

      _keys[wif_pub_key] = wif_key;
      DInteger el_gamal_priv_key = generate_private_el_gamal_key_from_secret( optional_private_key->get_secret() );
      _el_gamal_keys[get_public_el_gamal_key( el_gamal_priv_key )] = el_gamal_priv_key;
      _wallet.update_account(account);
      _wallet.extra_keys[account.id].insert(wif_pub_key);

      return all_keys_for_account.find(wif_pub_key) != all_keys_for_account.end();
   }

   // @returns true if the key matches a current active/owner/memo key for the named
   //          account, false otherwise (but it is stored either way)
   bool import_single_key(const string& account_name_or_id, const string& wif_key)
   {
      FC_ASSERT( !is_locked() );
      fc::optional<fc::ecc::private_key> optional_private_key = wif_to_key(wif_key);
      if (!optional_private_key)
         FC_THROW("Invalid private key");
      graphene::chain::public_key_type wif_pub_key = optional_private_key->get_public_key();

      account_object account = get_account( account_name_or_id );

      // make a list of all current public keys for the named account
      flat_set<public_key_type> all_keys_for_account;
      std::vector<public_key_type> active_keys = account.active.get_keys();
      std::vector<public_key_type> owner_keys = account.owner.get_keys();
      std::copy(active_keys.begin(), active_keys.end(), std::inserter(all_keys_for_account, all_keys_for_account.end()));
      std::copy(owner_keys.begin(), owner_keys.end(), std::inserter(all_keys_for_account, all_keys_for_account.end()));
      all_keys_for_account.insert(account.options.memo_key);

      _keys[wif_pub_key] = wif_key;
      DInteger el_gamal_priv_key = generate_private_el_gamal_key_from_secret( optional_private_key->get_secret() );
      _el_gamal_keys[get_public_el_gamal_key( el_gamal_priv_key )] = el_gamal_priv_key;
      _wallet.update_account(account);
      _wallet.extra_keys[account.id].insert(wif_pub_key);

      return all_keys_for_account.find(wif_pub_key) != all_keys_for_account.end();
   }

   int get_wallet_file_version(const fc::variant& data)
   {
      variant_object vo;
      fc::from_variant( data, vo );
      if (vo.find("version") == vo.end()) {
         return 0;
      }

      return vo["version"].as<int>();
   }

   bool load_old_wallet_file(const fc::variant& data, wallet_data& result)
   {
       bool ret;
       try {
           result = data.as<wallet_data>();
           ret = true;
       }
       catch (const fc::exception& ex) {
           elog("Parsing wallet data error: ${ex}", ("ex", ex.what()) );
           ret = false;
       }
       return ret;
   }

   bool load_new_wallet_file(const fc::variant& data, wallet_data& result)
   {
      bool ret;
      try {
         result = data.as<wallet_data>();

         const variant_object& vo = data.get_object();
         result.version = vo["version"].as<int>();
         result.update_time = vo["update_time"].as_string();

         ret = true;
      }
      catch (const fc::exception& ex) {
         elog("Parsing wallet data error: ${ex}", ("ex", ex.what()) );
         ret = false;
      }
      return ret;
   }


   bool load_wallet_file(string wallet_filename = string())
   {
      dlog("load_wallet_file() begin");

      // TODO:  Merge imported wallet with existing wallet,
      //        instead of replacing it
      if( wallet_filename.empty() )
         wallet_filename = _wallet_filename;

      bool result = false;
      wallet_data load_data;
      try {

         fc::variant v = fc::json::from_file(wallet_filename);
         int version = get_wallet_file_version(v);

         if (version == 0) {
            result = load_old_wallet_file(v, load_data);
         }
         else {
            result = load_new_wallet_file(v, load_data);
         }
      }
      catch(const fc::exception& ex) {
         elog("Error loading wallet file: ${ex}", ("ex",ex.what()) );
         result = false;
      }
      catch(const std::exception& ex) {
         elog("Error loading wallet file: ${ex}", ("ex",ex.what()) );
         result = false;
      }

      if (!result) {
         dlog("load_wallet_file() end (failed)");
         return false;
      }

      _wallet = load_data;
      if( _wallet.chain_id != _chain_id ) {
         FC_THROW("Wallet chain ID does not match",
                  ("wallet.chain_id", _wallet.chain_id)
                        ("chain_id", _chain_id));
      }

      size_t account_pagination = 100;
      vector< account_id_type > account_ids_to_send;
      size_t n = _wallet.my_accounts.size();
      account_ids_to_send.reserve( std::min( account_pagination, n ) );
      auto it = _wallet.my_accounts.begin();

      for( size_t start=0; start<n; start+=account_pagination )
      {
         size_t end = std::min( start+account_pagination, n );
         assert( end > start );
         account_ids_to_send.clear();
         std::vector< account_object > old_accounts;
         for( size_t i=start; i<end; i++ )
         {
            assert( it != _wallet.my_accounts.end() );
            old_accounts.push_back( *it );
            account_ids_to_send.push_back( old_accounts.back().id );
            ++it;
         }
         std::vector< optional< account_object > > accounts = _remote_db->get_accounts(account_ids_to_send);
         // server response should be same length as request
         FC_ASSERT( accounts.size() == account_ids_to_send.size() );
         size_t i = 0;
         for( const optional< account_object >& acct : accounts )
         {
            account_object& old_acct = old_accounts[i];
            if( !acct.valid() )
            {
               elog( "Could not find account ${id} : \"${name}\" does not exist on the chain!", ("id", old_acct.id)("name", old_acct.name) );
               i++;
               continue;
            }
            // this check makes sure the server didn't send results
            // in a different order, or accounts we didn't request
            FC_ASSERT( acct->id == old_acct.id );
            if( fc::json::to_string(*acct) != fc::json::to_string(old_acct) )
            {
               wlog( "Account ${id} : \"${name}\" updated on chain", ("id", acct->id)("name", acct->name) );
            }
            _wallet.update_account( *acct );
            i++;
         }
      }

      dlog("load_wallet_file() end");
      return true;
   }

   string save_old_wallet(const wallet_data& data)
   {
      return fc::json::to_pretty_string( _wallet );
   }

   string save_new_wallet(const wallet_data& data)
   {
      fc::mutable_variant_object mvo;
      mvo["version"] = fc::variant(data.version);
      mvo["update_time"] = fc::variant(data.update_time);
      mvo["chain_id"] = fc::variant(data.chain_id);
      mvo["my_accounts"] = fc::variant(data.my_accounts);
      mvo["cipher_keys"] = fc::variant(data.cipher_keys);
      mvo["extra_keys"] = fc::variant(data.extra_keys);
      mvo["pending_account_registrations"] = fc::variant(data.pending_account_registrations);
      mvo["pending_miner_registrations"] = fc::variant(data.pending_miner_registrations);
      mvo["ws_server"] = fc::variant(data.ws_server);
      mvo["ws_user"] = fc::variant(data.ws_user);
      mvo["ws_password"] = fc::variant(data.ws_password);

      fc::variant v(mvo);
      return fc::json::to_pretty_string( v );
   }

   void save_wallet_file(string wallet_filename = string() )
   {
      dlog("save_wallet_file() begin");
      FC_ASSERT( !is_locked() );

      //
      // Serialize in memory, then save to disk
      //
      // This approach lessens the risk of a partially written wallet
      // if exceptions are thrown in serialization
      //

      //allways save in new format
      _wallet.version = 1;
      _wallet.update_time = fc::time_point::now();
      encrypt_keys2();

      if( wallet_filename.empty() )
         wallet_filename = _wallet_filename;

      wlog( "saving wallet to file ${fn}", ("fn", wallet_filename) );

      string data;
      if (_wallet.version == 0) {
         data = save_old_wallet( _wallet );
      }
      else {
         data = save_new_wallet( _wallet );
      }

      try
      {
         enable_umask_protection();
         //
         // Parentheses on the following declaration fails to compile,
         // due to the Most Vexing Parse.  Thanks, C++
         //
         // http://en.wikipedia.org/wiki/Most_vexing_parse
         //
         fc::ofstream outfile{ boost::filesystem::path( wallet_filename ) };
         outfile.write( data.c_str(), data.length() );
         outfile.flush();
         outfile.close();
         disable_umask_protection();
      }
      catch(const fc::exception& ex) {
         elog("Error save wallet file: ${ex}", ("ex", ex.what()));

         disable_umask_protection();
         throw;
      }
      catch(const std::exception& ex) {
         elog("Error save wallet file: ${ex}", ("ex", ex.what()));

         disable_umask_protection();
         throw;
      }

      dlog("save_wallet_file() end");
   }

   transaction_handle_type begin_builder_transaction()
   {
      transaction_handle_type trx_handle = _builder_transactions.empty()? 0
                                                    : (--_builder_transactions.end())->first + 1;
      _builder_transactions[trx_handle];
      return trx_handle;
   }
   void add_operation_to_builder_transaction(transaction_handle_type transaction_handle, const operation& op)
   {
      FC_ASSERT(_builder_transactions.count(transaction_handle));
      _builder_transactions[transaction_handle].operations.emplace_back(op);
   }
   void replace_operation_in_builder_transaction(transaction_handle_type handle,
                                                 uint32_t operation_index,
                                                 const operation& new_op)
   {
      FC_ASSERT(_builder_transactions.count(handle));
      signed_transaction& trx = _builder_transactions[handle];
      FC_ASSERT( operation_index < trx.operations.size());
      trx.operations[operation_index] = new_op;
   }
   asset set_fees_on_builder_transaction(transaction_handle_type handle, const string& fee_asset = string(GRAPHENE_SYMBOL))
   {
      FC_ASSERT(_builder_transactions.count(handle));
      FC_ASSERT( fee_asset == GRAPHENE_SYMBOL, "fees can be paid in core asset");

      auto fee_asset_obj = get_asset(fee_asset);
      asset total_fee = fee_asset_obj.amount(0);

      auto gprops = _remote_db->get_global_properties().parameters;
      for( auto& op : _builder_transactions[handle].operations )
         total_fee += gprops.current_fees->set_fee( op );

      return total_fee;
   }
   transaction preview_builder_transaction(transaction_handle_type handle)
   {
      FC_ASSERT(_builder_transactions.count(handle));
      return _builder_transactions[handle];
   }
   signed_transaction sign_builder_transaction(transaction_handle_type transaction_handle, bool broadcast = true)
   {
      FC_ASSERT(_builder_transactions.count(transaction_handle));

      return _builder_transactions[transaction_handle] = sign_transaction(_builder_transactions[transaction_handle], broadcast);
   }
   signed_transaction propose_builder_transaction(
      transaction_handle_type handle,
      time_point_sec expiration = time_point::now() + fc::minutes(1),
      uint32_t review_period_seconds = 0, bool broadcast = true)
   {
      FC_ASSERT(_builder_transactions.count(handle));
      proposal_create_operation op;
      op.expiration_time = expiration;
      signed_transaction& trx = _builder_transactions[handle];
      std::transform(trx.operations.begin(), trx.operations.end(), std::back_inserter(op.proposed_ops),
                     [](const operation& op) -> op_wrapper { return op; });
      if( review_period_seconds )
         op.review_period_seconds = review_period_seconds;
      trx.operations = {op};
      _remote_db->get_global_properties().parameters.current_fees->set_fee( trx.operations.front() );

      return trx = sign_transaction(trx, broadcast);
   }

   signed_transaction propose_builder_transaction2(
      transaction_handle_type handle,
      const string& account_name_or_id,
      time_point_sec expiration = time_point::now() + fc::minutes(1),
      uint32_t review_period_seconds = 0, bool broadcast = true)
   {
      FC_ASSERT(_builder_transactions.count(handle));
      proposal_create_operation op;
      op.fee_paying_account = get_account(account_name_or_id).get_id();
      op.expiration_time = expiration;
      signed_transaction& trx = _builder_transactions[handle];
      std::transform(trx.operations.begin(), trx.operations.end(), std::back_inserter(op.proposed_ops),
                     [](const operation& op) -> op_wrapper { return op; });
      if( review_period_seconds )
         op.review_period_seconds = review_period_seconds;
      trx.operations = {op};
      _remote_db->get_global_properties().parameters.current_fees->set_fee( trx.operations.front() );

      return trx = sign_transaction(trx, broadcast);
   }

   void remove_builder_transaction(transaction_handle_type handle)
   {
      _builder_transactions.erase(handle);
   }


   signed_transaction register_account(const string& name,
                                       public_key_type owner,
                                       public_key_type active,
                                       public_key_type memo,
                                       const string&  registrar_account,
                                       bool broadcast = false)
   { try {
      FC_ASSERT(!find_account(name).valid(), "Account with that name already exists!");
      account_create_operation account_create_op;

      // TODO:  process when pay_from_account is ID

      account_object registrar_account_object = get_account( registrar_account );
      account_id_type registrar_account_id = registrar_account_object.id;

      account_create_op.registrar = registrar_account_id;
      account_create_op.name = name;
      account_create_op.owner = authority(1, owner, 1);
      account_create_op.active = authority(1, active, 1);
      account_create_op.options.memo_key = memo;

      signed_transaction tx;

      tx.operations.push_back( account_create_op );

      auto current_fees = _remote_db->get_global_properties().parameters.current_fees;
      set_operation_fees( tx, current_fees );

      vector<public_key_type> paying_keys = registrar_account_object.active.get_keys();

      auto dyn_props = get_dynamic_global_properties();
      tx.set_reference_block( dyn_props.head_block_id );
      tx.set_expiration( dyn_props.time + fc::seconds(30) );
      tx.validate();

      for( public_key_type& key : paying_keys )
      {
         auto it = _keys.find(key);
         if( it != _keys.end() )
         {
            fc::optional< fc::ecc::private_key > privkey = wif_to_key( it->second );
            if( !privkey.valid() )
            {
               FC_ASSERT( false, "Malformed private key in _keys" );
            }
            tx.sign( *privkey, _chain_id );
         }
      }

      if( broadcast )
         _remote_net_broadcast->broadcast_transaction( tx );
      return tx;
   } FC_CAPTURE_AND_RETHROW( (name)(owner)(active)(memo)(registrar_account)(broadcast) ) }

   signed_transaction register_multisig_account(const string& name,
                                                authority owner_authority,
                                                authority active_authority,
                                                public_key_type memo_pubkey,
                                                const string&  registrar_account,
                                                bool broadcast = false)
   {
      try
      {
         FC_ASSERT(!find_account(name).valid(), "Account with that name already exists!");
         account_create_operation account_create_op;
         account_object acc = get_account( registrar_account );

         account_create_op.registrar = acc.id;
         account_create_op.name = name;
         account_create_op.owner = owner_authority;
         account_create_op.active = active_authority;
         account_create_op.options.memo_key = memo_pubkey;

         signed_transaction tx;
         tx.operations.push_back( account_create_op );
         set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees );
         tx.validate();

         return sign_transaction( tx, broadcast );
      } FC_CAPTURE_AND_RETHROW( (name)(owner_authority)(active_authority)(memo_pubkey)(registrar_account)(broadcast) )
   }

   // This function generates derived keys starting with index 0 and keeps incrementing
   // the index until it finds a key that isn't registered in the block chain.  To be
   // safer, it continues checking for a few more keys to make sure there wasn't a short gap
   // caused by a failed registration or the like.
   int find_first_unused_derived_key_index(const fc::ecc::private_key& parent_key)
   {
      int first_unused_index = 0;
      int number_of_consecutive_unused_keys = 0;
      for (int key_index = 0; ; ++key_index)
      {
         fc::ecc::private_key derived_private_key = derive_private_key(key_to_wif(parent_key), key_index);
         graphene::chain::public_key_type derived_public_key = derived_private_key.get_public_key();
         if( _keys.find(derived_public_key) == _keys.end() )
         {
            if (number_of_consecutive_unused_keys)
            {
               ++number_of_consecutive_unused_keys;
               if (number_of_consecutive_unused_keys > 5)
                  return first_unused_index;
            }
            else
            {
               first_unused_index = key_index;
               number_of_consecutive_unused_keys = 1;
            }
         }
         else
         {
            // key_index is used
            first_unused_index = 0;
            number_of_consecutive_unused_keys = 0;
         }
      }
   }

   signed_transaction create_account_with_private_key(fc::ecc::private_key owner_privkey,
                                                      const string& account_name,
                                                      const string& registrar_account,
                                                      bool import,
                                                      bool broadcast = false,
                                                      bool save_wallet = true)
   { try {

         FC_ASSERT(!find_account(account_name).valid(), "Account with that name already exists!");

         int active_key_index = find_first_unused_derived_key_index(owner_privkey);
         fc::ecc::private_key active_privkey = derive_private_key( key_to_wif(owner_privkey), active_key_index);

         int memo_key_index = find_first_unused_derived_key_index(active_privkey);
         fc::ecc::private_key memo_privkey = derive_private_key( key_to_wif(active_privkey), memo_key_index);

         graphene::chain::public_key_type owner_pubkey = owner_privkey.get_public_key();
         graphene::chain::public_key_type active_pubkey = active_privkey.get_public_key();
         graphene::chain::public_key_type memo_pubkey = memo_privkey.get_public_key();

         account_create_operation account_create_op;

         // TODO:  process when pay_from_account is ID

         account_object registrar_account_object = get_account( registrar_account );
         account_id_type registrar_account_id = registrar_account_object.id;

         account_create_op.registrar = registrar_account_id;
         account_create_op.name = account_name;
         account_create_op.owner = authority(1, owner_pubkey, 1);
         account_create_op.active = authority(1, active_pubkey, 1);
         account_create_op.options.memo_key = memo_pubkey;

         // current_fee_schedule()
         // find_account(pay_from_account)

         // account_create_op.fee = account_create_op.calculate_fee(db.current_fee_schedule());

         signed_transaction tx;
         tx.operations.push_back( account_create_op );
         set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);

         vector<public_key_type> paying_keys = registrar_account_object.active.get_keys();

         auto dyn_props = get_dynamic_global_properties();
         tx.set_reference_block( dyn_props.head_block_id );
         tx.set_expiration( dyn_props.time + fc::seconds(30) );
         tx.validate();

         for( public_key_type& key : paying_keys )
         {
            auto it = _keys.find(key);
            if( it != _keys.end() )
            {
               fc::optional< fc::ecc::private_key > privkey = wif_to_key( it->second );
               FC_ASSERT( privkey.valid(), "Malformed private key in _keys" );
               tx.sign( *privkey, _chain_id );
            }
         }

         // we do not insert owner_privkey here because
         //    it is intended to only be used for key recovery
         if (import)
         {
            _wallet.pending_account_registrations[account_name].push_back(key_to_wif( active_privkey ));
            _wallet.pending_account_registrations[account_name].push_back(key_to_wif( memo_privkey ));
         }
         if( save_wallet )
            save_wallet_file();
         if( broadcast )
            _remote_net_broadcast->broadcast_transaction( tx );
         return tx;
   } FC_CAPTURE_AND_RETHROW( (account_name)(registrar_account)(broadcast) ) }

   signed_transaction create_account_with_brain_key(const string& brain_key,
                                                    const string& account_name,
                                                    const string& registrar_account,
                                                    bool import,
                                                    bool broadcast = false,
                                                    bool save_wallet = true)
   { try {
      string normalized_brain_key = graphene::utilities::normalize_brain_key( brain_key );
      // TODO:  scan blockchain for accounts that exist with same brain key
      fc::ecc::private_key owner_privkey = graphene::utilities::derive_private_key( normalized_brain_key );
      return create_account_with_private_key(owner_privkey, account_name, registrar_account, import, broadcast, save_wallet);
   } FC_CAPTURE_AND_RETHROW( (account_name)(registrar_account) ) }

   signed_transaction update_account_keys( const string& name,
                                           fc::optional<authority> owner_auth,
                                           fc::optional<authority> active_auth,
                                           fc::optional<public_key_type> memo_pubkey,
                                           bool broadcast )
   { try {
         FC_ASSERT( owner_auth || active_auth || memo_pubkey, "at least one authority/public key needs to be specified");

         account_object acc = get_account( name );
         account_update_operation account_update_op;

         // TODO: move the following asserts into evaluator.
         if( owner_auth )
         {
            FC_ASSERT( acc.owner != *owner_auth, "new authority needs to be different from the existing one" );
         }

         if( active_auth )
         {
            FC_ASSERT( acc.active != *active_auth, "new authority needs to be different from the existing one" );
         }

         account_update_op.account = acc.id;
         account_update_op.owner = owner_auth;
         account_update_op.active = active_auth;

         if( memo_pubkey )
         {
            FC_ASSERT( acc.options.memo_key != *memo_pubkey, "new authority needs to be different from the existing one" );
            acc.options.memo_key = *memo_pubkey;
            account_update_op.new_options = acc.options;
         }

         signed_transaction tx;
         tx.operations.push_back( account_update_op );
         set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
         tx.validate();

         return sign_transaction( tx, broadcast );
      } FC_CAPTURE_AND_RETHROW( (name)(owner_auth)(active_auth)(memo_pubkey)(broadcast) ) }

   signed_transaction create_user_issued_asset(const string& issuer,
                                               const string& symbol,
                                               uint8_t precision,
                                               const string& description,
                                               uint64_t max_supply,
                                               price core_exchange_rate,
                                               bool is_exchangeable,
                                               bool is_fixed_max_supply,
                                               bool broadcast = false)
   { try {
      account_object issuer_account = get_account( issuer );
      FC_ASSERT(!find_asset(symbol).valid(), "Asset with that symbol already exists!");

      asset_create_operation create_op;
      create_op.issuer = issuer_account.id;
      create_op.symbol = symbol;
      create_op.precision = precision;
      create_op.description = description;
      asset_options opts;
      opts.max_supply = max_supply;
      opts.core_exchange_rate = core_exchange_rate;
      opts.is_exchangeable = is_exchangeable;
      opts.extensions.insert(asset_options::fixed_max_supply_struct(is_fixed_max_supply));

      create_op.options = opts;
      create_op.monitored_asset_opts = optional<monitored_asset_options>();

      signed_transaction tx;
      tx.operations.push_back( create_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (issuer)(symbol)(precision)(description)(max_supply)(is_exchangeable)(broadcast) ) }

   signed_transaction issue_asset(const string& to_account,
                                  const string& amount,
                                  const string& symbol,
                                  const string& memo,
                                  bool broadcast = false)
   {
      auto asset_obj = get_asset(symbol);

      account_object to = get_account(to_account);
      account_object issuer = get_account(asset_obj.issuer);

      asset_issue_operation issue_op;
      issue_op.issuer           = asset_obj.issuer;
      issue_op.asset_to_issue   = asset_obj.amount_from_string(amount);
      issue_op.issue_to_account = to.id;

      if( !memo.empty() )
      {
         issue_op.memo = memo_data(memo, get_private_key(issuer.options.memo_key), to.options.memo_key);
      }

      signed_transaction tx;
      tx.operations.push_back(issue_op);
      set_operation_fees(tx,_remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction(tx, broadcast);
   }

   signed_transaction update_user_issued_asset(const string& symbol,
                                               const string& new_issuer,
                                               const string& description,
                                               uint64_t max_supply,
                                               price core_exchange_rate,
                                               bool is_exchangeable,
                                               bool broadcast = false)
   { try {
         asset_object asset_to_update = get_asset(symbol);

         update_user_issued_asset_operation update_op;
         update_op.issuer = asset_to_update.issuer;
         update_op.asset_to_update = asset_to_update.id;
         update_op.new_description = description;
         update_op.max_supply = max_supply;
         update_op.core_exchange_rate = core_exchange_rate;
         update_op.is_exchangeable = is_exchangeable;

         optional<account_id_type> new_issuer_account_id;
         if( !new_issuer.empty() )
         {
            account_object new_issuer_account = get_account(new_issuer);
            new_issuer_account_id = new_issuer_account.id;
         }
         update_op.new_issuer = new_issuer_account_id;

         signed_transaction tx;
         tx.operations.push_back( update_op );
         set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
         tx.validate();

         return sign_transaction( tx, broadcast );
      } FC_CAPTURE_AND_RETHROW( (symbol)(new_issuer)(description)(max_supply)(core_exchange_rate)(is_exchangeable)(broadcast) ) }

      signed_transaction fund_asset_pools(const string& from,
                                          const string& uia_amount,
                                          const string& uia_symbol,
                                          const string& DCT_amount,
                                          const string& DCT_symbol,
                                          bool broadcast /* = false */)
      { try {
            account_object from_account = get_account(from);
            asset_object uia_asset_to_fund = get_asset(uia_symbol);
            asset_object dct_asset_to_fund = get_asset(DCT_symbol);

            asset_fund_pools_operation fund_op;
            fund_op.from_account = from_account.id;
            fund_op.uia_asset = uia_asset_to_fund.amount_from_string(uia_amount);
            fund_op.dct_asset = dct_asset_to_fund.amount_from_string(DCT_amount);

            signed_transaction tx;
            tx.operations.push_back( fund_op );
            set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
            tx.validate();

            return sign_transaction( tx, broadcast );
         } FC_CAPTURE_AND_RETHROW( (from)(uia_amount)(uia_symbol)(DCT_amount)(DCT_symbol)(broadcast) ) }

   signed_transaction reserve_asset(const string& from,
                                    const string& amount,
                                    const string& symbol,
                                    bool broadcast /* = false */)
   { try {
         account_object from_account = get_account(from);
         asset_object asset_to_reserve = get_asset(symbol);

         asset_reserve_operation reserve_op;
         reserve_op.payer = from_account.id;
         reserve_op.amount_to_reserve = asset_to_reserve.amount_from_string(amount);

         signed_transaction tx;
         tx.operations.push_back( reserve_op );
         set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
         tx.validate();

         return sign_transaction( tx, broadcast );
      } FC_CAPTURE_AND_RETHROW( (from)(amount)(symbol)(broadcast) ) }

   signed_transaction claim_fees(const string& uia_amount,
                                 const string& uia_symbol,
                                 const string& dct_amount,
                                 const string& dct_symbol,
                                 bool broadcast /* = false */)
   { try {
         asset_object uia_asset_to_claim = get_asset(uia_symbol);
         asset_object dct_asset_to_claim = get_asset(dct_symbol);

         FC_ASSERT( dct_asset_to_claim.id == asset_id_type() );

         asset_claim_fees_operation claim_fees_op;
         claim_fees_op.issuer = uia_asset_to_claim.issuer;
         claim_fees_op.uia_asset = uia_asset_to_claim.amount_from_string(uia_amount);
         claim_fees_op.dct_asset = dct_asset_to_claim.amount_from_string(dct_amount);

         signed_transaction tx;
         tx.operations.push_back( claim_fees_op );
         set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
         tx.validate();

         return sign_transaction( tx, broadcast );
      } FC_CAPTURE_AND_RETHROW( (uia_amount)(uia_symbol)(dct_amount)(dct_symbol)(broadcast) ) }

   signed_transaction create_monitored_asset(const string& issuer,
                                             const string& symbol,
                                             uint8_t precision,
                                             const string& description,
                                             uint32_t feed_lifetime_sec,
                                             uint8_t minimum_feeds,
                                             bool broadcast = false)
   { try {
      account_object issuer_account = get_account( issuer );
      FC_ASSERT(!find_asset(symbol).valid(), "Asset with that symbol already exists!");

      asset_create_operation create_op;
      create_op.issuer = issuer_account.id;
      create_op.symbol = symbol;
      create_op.precision = precision;
      create_op.description = description;

      asset_options opts;
      opts.max_supply = 0;
      create_op.options = opts;

      monitored_asset_options m_opts;
      m_opts.feed_lifetime_sec = feed_lifetime_sec;
      m_opts.minimum_feeds = minimum_feeds;
      create_op.monitored_asset_opts = m_opts;

      signed_transaction tx;
      tx.operations.push_back( create_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (issuer)(symbol)(precision)(description)(feed_lifetime_sec)(minimum_feeds)(broadcast) ) }

   signed_transaction update_monitored_asset(const string& symbol,
                                             const string& description,
                                             uint32_t feed_lifetime_sec,
                                             uint8_t minimum_feeds,
                                             bool broadcast /* = false */)
   { try {
      asset_object asset_to_update = get_asset(symbol);

      update_monitored_asset_operation update_op;
      update_op.issuer = asset_to_update.issuer;
      update_op.asset_to_update = asset_to_update.id;
      update_op.new_description = description;
      update_op.new_feed_lifetime_sec = feed_lifetime_sec;
      update_op.new_minimum_feeds = minimum_feeds;

      signed_transaction tx;
      tx.operations.push_back( update_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (symbol)(description)(feed_lifetime_sec)(minimum_feeds)(broadcast) ) }

   signed_transaction publish_asset_feed(const string& publishing_account,
                                         const string& symbol,
                                         price_feed feed,
                                         bool broadcast /* = false */)
   { try {
      asset_object asset_to_update = get_asset(symbol);

      asset_publish_feed_operation publish_op;
      publish_op.publisher = get_account(publishing_account).get_id();
      publish_op.asset_id = asset_to_update.id;
      publish_op.feed = feed;

      signed_transaction tx;
      tx.operations.push_back( publish_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (publishing_account)(symbol)(feed)(broadcast) ) }

   //////////////////////////////////////////////////////////////////////
   //                                                                  //
   // Non Fungible Tokens                                              //
   //                                                                  //
   //////////////////////////////////////////////////////////////////////

   signed_transaction create_non_fungible_token(const string& issuer,
                                                const string& symbol,
                                                const string& description,
                                                const non_fungible_token_data_definitions& definitions,
                                                uint32_t max_supply,
                                                bool fixed_max_supply,
                                                bool transferable,
                                                bool broadcast /* = false */)
   {
      FC_ASSERT(!find_non_fungible_token(symbol).valid(), "Non fungible token with that symbol already exists!");

      non_fungible_token_create_definition_operation create_op;
      create_op.symbol = symbol;
      create_op.transferable = transferable;
      create_op.options.issuer = get_account(issuer).get_id();
      create_op.options.max_supply = max_supply;
      create_op.options.fixed_max_supply = fixed_max_supply;
      create_op.options.description = description;
      create_op.definitions = definitions;

      create_op.validate();

      signed_transaction tx;
      tx.operations.push_back( create_op );
      set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction( tx, broadcast );
   }

   signed_transaction update_non_fungible_token(const string& issuer,
                                                const string& symbol,
                                                const string& description,
                                                uint32_t max_supply,
                                                bool fixed_max_supply,
                                                bool broadcast /* = false */)
   {
      non_fungible_token_object nft_obj = get_non_fungible_token(symbol);
      non_fungible_token_update_definition_operation update_op;
      update_op.current_issuer = nft_obj.options.issuer;
      update_op.nft_id = nft_obj.get_id();
      update_op.options.issuer = issuer.empty() ? nft_obj.options.issuer : get_account(issuer).get_id();
      update_op.options.max_supply = max_supply;
      update_op.options.fixed_max_supply = fixed_max_supply;
      update_op.options.description = description;

      update_op.validate();

      signed_transaction tx;
      tx.operations.push_back( update_op );
      set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction( tx, broadcast );
   }

   signed_transaction issue_non_fungible_token(const string& to_account,
                                               const string& symbol,
                                               const fc::variants& data,
                                               const string& memo,
                                               bool broadcast /* = false */)
   {
      non_fungible_token_object nft_obj = get_non_fungible_token(symbol);
      account_object to = get_account(to_account);
      account_object issuer = get_account(nft_obj.options.issuer);

      non_fungible_token_issue_operation issue_op;
      issue_op.issuer = nft_obj.options.issuer;
      issue_op.to = to.id;
      issue_op.nft_id = nft_obj.get_id();
      issue_op.data = data;

      if(!memo.empty())
         issue_op.memo = memo_data(memo, get_private_key(issuer.options.memo_key), to.options.memo_key);

      issue_op.validate();

      signed_transaction tx;
      tx.operations.push_back( issue_op );
      set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction( tx, broadcast );
   }

   signed_transaction transfer_non_fungible_token_data(const string& to_account,
                                                       const non_fungible_token_data_id_type nft_data_id,
                                                       const string& memo,
                                                       bool broadcast /* = false */)
   {
      non_fungible_token_data_object nft_data = get_non_fungible_token_data(nft_data_id);
      account_object from = get_account(nft_data.owner);
      account_object to = get_account(to_account);

      non_fungible_token_transfer_operation transfer_op;
      transfer_op.from = nft_data.owner;
      transfer_op.to = to.id;
      transfer_op.nft_data_id = nft_data.get_id();

      if(!memo.empty())
         transfer_op.memo = memo_data(memo, get_private_key(from.options.memo_key), to.options.memo_key);

      transfer_op.validate();

      signed_transaction tx;
      tx.operations.push_back( transfer_op );
      set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction( tx, broadcast );
   }

   signed_transaction burn_non_fungible_token_data(const non_fungible_token_data_id_type nft_data_id, bool broadcast /* = false */)
   {
      return transfer_non_fungible_token_data(static_cast<std::string>(static_cast<object_id_type>(GRAPHENE_NULL_ACCOUNT)), nft_data_id, "", broadcast);
   }

   signed_transaction update_non_fungible_token_data(const string& modifier,
                                                     const non_fungible_token_data_id_type nft_data_id,
                                                     const std::unordered_map<string, fc::variant>& data,
                                                     bool broadcast /* = false */)
   {
      non_fungible_token_data_object nft_data = get_non_fungible_token_data(nft_data_id);
      non_fungible_token_update_data_operation data_op;
      data_op.modifier = get_account(modifier).get_id();
      data_op.nft_data_id = nft_data.get_id();
      data_op.data = data;

      data_op.validate();

      signed_transaction tx;
      tx.operations.push_back( data_op );
      set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction( tx, broadcast );
   }

   miner_object get_miner(const string& owner_account)
   {
      try
      {
         fc::optional<miner_id_type> miner_id = maybe_id<miner_id_type>(owner_account);
         if (miner_id)
         {
            std::vector<miner_id_type> ids_to_get;
            ids_to_get.push_back(*miner_id);
            std::vector<fc::optional<miner_object>> miner_objects = _remote_db->get_miners(ids_to_get);
            if (miner_objects.front())
               return *miner_objects.front();
            FC_THROW("No miner is registered for id ${id}", ("id", owner_account));
         }
         else
         {
            // then maybe it's the owner account
            try
            {
               account_id_type owner_account_id = get_account(owner_account).get_id();
               fc::optional<miner_object> miner = _remote_db->get_miner_by_account(owner_account_id);
               if (miner)
                  return *miner;
               else
                  FC_THROW("No miner is registered for account ${account}", ("account", owner_account));
            }
            catch (const fc::exception&)
            {
               FC_THROW("No account or miner named ${account}", ("account", owner_account));
            }
         }
      }
      FC_CAPTURE_AND_RETHROW( (owner_account) )
   }

   signed_transaction create_miner(const string& owner_account,
                                   const string& url,
                                   bool broadcast /* = false */)
   { try {
      account_object miner_account = get_account(owner_account);
      fc::ecc::private_key active_private_key = get_private_key_for_account(miner_account);
      int miner_key_index = find_first_unused_derived_key_index(active_private_key);
      fc::ecc::private_key miner_private_key = derive_private_key(key_to_wif(active_private_key), miner_key_index);
      graphene::chain::public_key_type miner_public_key = miner_private_key.get_public_key();

      miner_create_operation miner_create_op;
      miner_create_op.miner_account = miner_account.id;
      miner_create_op.block_signing_key = miner_public_key;
      miner_create_op.url = url;

      if (_remote_db->get_miner_by_account(miner_create_op.miner_account))
         FC_THROW("Account ${owner_account} is already a miner", ("owner_account", owner_account));

      signed_transaction tx;
      tx.operations.push_back( miner_create_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      _wallet.pending_miner_registrations[owner_account] = key_to_wif(miner_private_key);

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (owner_account)(broadcast) ) }

   signed_transaction update_miner(const string& miner_name,
                                   const string& url,
                                   const string& block_signing_key,
                                   bool broadcast /* = false */)
   { try {
      miner_object miner = get_miner(miner_name);
      account_object miner_account = get_account( miner.miner_account );
      fc::ecc::private_key active_private_key = get_private_key_for_account(miner_account);

      miner_update_operation miner_update_op;
      miner_update_op.miner = miner.id;
      miner_update_op.miner_account = miner_account.id;
      if( !url.empty() )
         miner_update_op.new_url = url;
      if( !block_signing_key.empty() )
         miner_update_op.new_signing_key = public_key_type( block_signing_key );

      signed_transaction tx;
      tx.operations.push_back( miner_update_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees );
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (miner_name)(url)(block_signing_key)(broadcast) ) }

   vector< vesting_balance_object_with_info > get_vesting_balances( const string& account_name )
   { try {
      fc::optional<vesting_balance_id_type> vbid = maybe_id<vesting_balance_id_type>( account_name );
      std::vector<vesting_balance_object_with_info> result;
      fc::time_point_sec now = _remote_db->get_dynamic_global_properties().time;

      if( vbid )
      {
         result.emplace_back( get_object<vesting_balance_object>(*vbid), now );
         return result;
      }

      // try casting to avoid a round-trip if we were given an account ID
      fc::optional<account_id_type> acct_id = maybe_id<account_id_type>( account_name );
      if( !acct_id )
         acct_id = get_account( account_name ).id;

      vector< vesting_balance_object > vbos = _remote_db->get_vesting_balances( *acct_id );
      if( vbos.size() == 0 )
         return result;

      for( const vesting_balance_object& vbo : vbos )
         result.emplace_back( vbo, now );

      return result;
   } FC_CAPTURE_AND_RETHROW( (account_name) )
   }

   signed_transaction withdraw_vesting(const string& miner_name,
                                       const string& amount,
                                       const string& asset_symbol,
                                       bool broadcast = false )
   { try {
      asset_object asset_obj = get_asset( asset_symbol );
      fc::optional<vesting_balance_id_type> vbid = maybe_id<vesting_balance_id_type>(miner_name);
      if( !vbid )
      {
         miner_object wit = get_miner( miner_name );
         FC_ASSERT( wit.pay_vb );
         vbid = wit.pay_vb;
      }

      vesting_balance_object vbo = get_object< vesting_balance_object >( *vbid );
      vesting_balance_withdraw_operation vesting_balance_withdraw_op;

      vesting_balance_withdraw_op.vesting_balance = *vbid;
      vesting_balance_withdraw_op.owner = vbo.owner;
      vesting_balance_withdraw_op.amount = asset_obj.amount_from_string(amount);

      signed_transaction tx;
      tx.operations.push_back( vesting_balance_withdraw_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees );
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (miner_name)(amount) )
   }

   signed_transaction vote_for_miner(const string& voting_account,
                                     const string& miner,
                                     bool approve,
                                     bool broadcast /* = false */)
   { try {
      account_object voting_account_object = get_account(voting_account);
      account_id_type miner_owner_account_id = get_account(miner).get_id();
      fc::optional<miner_object> miner_obj = _remote_db->get_miner_by_account(miner_owner_account_id);
      if (!miner_obj)
         FC_THROW("Account ${miner} is not registered as a miner", ("miner", miner));
      if (approve)
      {
         auto insert_result = voting_account_object.options.votes.insert(miner_obj->vote_id);
         if (!insert_result.second)
            FC_THROW("Account ${account} was already voting for miner ${miner}", ("account", voting_account)("miner", miner));
      }
      else
      {
         auto votes_removed = voting_account_object.options.votes.erase(miner_obj->vote_id);
         if (!votes_removed)
            FC_THROW("Account ${account} is already not voting for miner ${miner}", ("account", voting_account)("miner", miner));
      }
      account_update_operation account_update_op;
      account_update_op.account = voting_account_object.id;
      account_update_op.new_options = voting_account_object.options;

      signed_transaction tx;
      tx.operations.push_back( account_update_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (voting_account)(miner)(approve)(broadcast) ) }

   signed_transaction set_voting_proxy(const string& account_to_modify,
                                       optional<string> voting_account,
                                       bool broadcast /* = false */)
   { try {
      account_object account_object_to_modify = get_account(account_to_modify);
      if (voting_account)
      {
         account_id_type new_voting_account_id = get_account(*voting_account).get_id();
         if (account_object_to_modify.options.voting_account == new_voting_account_id)
            FC_THROW("Voting proxy for ${account} is already set to ${voter}", ("account", account_to_modify)("voter", *voting_account));
         account_object_to_modify.options.voting_account = new_voting_account_id;
      }
      else
      {
         if (account_object_to_modify.options.voting_account == GRAPHENE_PROXY_TO_SELF_ACCOUNT)
            FC_THROW("Account ${account} is already voting for itself", ("account", account_to_modify));
         account_object_to_modify.options.voting_account = GRAPHENE_PROXY_TO_SELF_ACCOUNT;
      }

      account_update_operation account_update_op;
      account_update_op.account = account_object_to_modify.id;
      account_update_op.new_options = account_object_to_modify.options;

      signed_transaction tx;
      tx.operations.push_back( account_update_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (account_to_modify)(voting_account)(broadcast) ) }

   signed_transaction set_desired_miner_count(const string& account_to_modify,
                                              uint16_t desired_number_of_miners,
                                              bool broadcast /* = false */)
   { try {
      account_object account_object_to_modify = get_account(account_to_modify);

      if (account_object_to_modify.options.num_miner == desired_number_of_miners)
         FC_THROW("Account ${account} is already voting for ${miners} miners",
                  ("account", account_to_modify)("miners", desired_number_of_miners));
      account_object_to_modify.options.num_miner = desired_number_of_miners;

      account_update_operation account_update_op;
      account_update_op.account = account_object_to_modify.id;
      account_update_op.new_options = account_object_to_modify.options;

      signed_transaction tx;
      tx.operations.push_back( account_update_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (account_to_modify)(desired_number_of_miners)(broadcast) ) }

   vector<miner_voting_info> search_miner_voting(const string& account_id,
                                                 const string& filter,
                                                 bool only_my_votes,
                                                 const string& order,
                                                 const string& id,
                                                 uint32_t count ) const
   {
      vector<graphene::app::miner_voting_info> tmp_result =
            _remote_db->search_miner_voting(account_id, filter, only_my_votes, order, id, count);

      vector<miner_voting_info> result;
      result.reserve(tmp_result.size());

      for (const auto& item : tmp_result) {
         miner_voting_info info;
         info.id    = item.id;
         info.name  = item.name;
         info.url   = item.url;
         info.total_votes = item.total_votes;
         info.voted = item.voted;

         result.push_back(info);
      }

      return result;
   }

   signed_transaction sign_transaction(signed_transaction tx, bool broadcast = false)
   {
      flat_set<account_id_type> req_active_approvals;
      flat_set<account_id_type> req_owner_approvals;
      vector<authority>         other_auths;

      tx.get_required_authorities( req_active_approvals, req_owner_approvals, other_auths );

      for( const auto& auth : other_auths )
         for( const auto& a : auth.account_auths )
            req_active_approvals.insert(a.first);

      // std::merge lets us de-duplicate account_id's that occur in both
      //   sets, and dump them into a vector (as required by remote_db api)
      //   at the same time
      vector<account_id_type> v_approving_account_ids;
      std::merge(req_active_approvals.begin(), req_active_approvals.end(),
                 req_owner_approvals.begin() , req_owner_approvals.end(),
                 std::back_inserter(v_approving_account_ids));

      /// TODO: fetch the accounts specified via other_auths as well.

      vector< optional<account_object> > approving_account_objects =
            _remote_db->get_accounts( v_approving_account_ids );

      /// TODO: recursively check one layer deeper in the authority tree for keys

      FC_ASSERT( approving_account_objects.size() == v_approving_account_ids.size() );

      flat_map<account_id_type, account_object*> approving_account_lut;
      size_t i = 0;
      for( optional<account_object>& approving_acct : approving_account_objects )
      {
         if( !approving_acct.valid() )
         {
            wlog( "operation_get_required_auths said approval of non-existing account ${id} was needed",
                  ("id", v_approving_account_ids[i]) );
            i++;
            continue;
         }
         approving_account_lut[ approving_acct->id ] = &(*approving_acct);
         i++;
      }

      flat_set<public_key_type> approving_key_set;
      for( account_id_type& acct_id : req_active_approvals )
      {
         const auto it = approving_account_lut.find( acct_id );
         if( it == approving_account_lut.end() )
            continue;
         const account_object* acct = it->second;
         vector<public_key_type> v_approving_keys = acct->active.get_keys();
         for( const public_key_type& approving_key : v_approving_keys )
            approving_key_set.insert( approving_key );
      }
      for( account_id_type& acct_id : req_owner_approvals )
      {
         const auto it = approving_account_lut.find( acct_id );
         if( it == approving_account_lut.end() )
            continue;
         const account_object* acct = it->second;
         vector<public_key_type> v_approving_keys = acct->owner.get_keys();
         for( const public_key_type& approving_key : v_approving_keys )
            approving_key_set.insert( approving_key );
      }
      for( const authority& a : other_auths )
      {
         for( const auto& k : a.key_auths )
            approving_key_set.insert( k.first );
      }

      auto dyn_props = get_dynamic_global_properties();
      tx.set_reference_block( dyn_props.head_block_id );

      // first, some bookkeeping, expire old items from _recently_generated_transactions
      // since transactions include the head block id, we just need the index for keeping transactions unique
      // when there are multiple transactions in the same block.  choose a time period that should be at
      // least one block long, even in the worst case.  2 minutes ought to be plenty.
      fc::time_point_sec oldest_transaction_ids_to_track(dyn_props.time - fc::minutes(2));
      auto oldest_transaction_record_iter = _recently_generated_transactions.get<timestamp_index>().lower_bound(oldest_transaction_ids_to_track);
      auto begin_iter = _recently_generated_transactions.get<timestamp_index>().begin();
      _recently_generated_transactions.get<timestamp_index>().erase(begin_iter, oldest_transaction_record_iter);

      uint32_t expiration_time_offset = 0;
      for (;;)
      {
         tx.set_expiration( dyn_props.time + fc::seconds(30 + expiration_time_offset) );
         tx.signatures.clear();

         for( public_key_type& key : approving_key_set )
         {
            auto it = _keys.find(key);
            if( it != _keys.end() )
            {
               fc::optional<fc::ecc::private_key> privkey = wif_to_key( it->second );
               FC_ASSERT( privkey.valid(), "Malformed private key in _keys" );
               tx.sign( *privkey, _chain_id );
            }
            /// TODO: if transaction has enough signatures to be "valid" don't add any more,
            /// there are cases where the wallet may have more keys than strictly necessary and
            /// the transaction will be rejected if the transaction validates without requiring
            /// all signatures provided
         }

         graphene::chain::transaction_id_type this_transaction_id = tx.id();
         auto iter = _recently_generated_transactions.find(this_transaction_id);
         if (iter == _recently_generated_transactions.end())
         {
            // we haven't generated this transaction before, the usual case
            recently_generated_transaction_record this_transaction_record;
            this_transaction_record.generation_time = dyn_props.time;
            this_transaction_record.transaction_id = this_transaction_id;
            _recently_generated_transactions.insert(this_transaction_record);
            break;
         }

         // else we've generated a dupe, increment expiration time and re-sign it
         ++expiration_time_offset;
      }

      if( broadcast )
      {
         try
         {
            dlog("about to broadcast tx: ${t}", ("t", tx));
            _remote_net_broadcast->broadcast_transaction( tx );
         }
         catch (const fc::exception& e)
         {
            elog("Caught exception while broadcasting tx ${id}:  ${e}", ("id", tx.id().str())("e", e.to_detail_string()) );
            throw;
         }
      }

      return tx;
   }


   signed_transaction transfer(const string& from,
                               const string& to,
                               const string& amount,
                               const string& asset_symbol,
                               const string& memo,
                               bool broadcast = false)
   { try {
      account_object from_account = get_account(from);
      account_id_type from_id = from_account.id;

      asset_object asset_obj = get_asset(asset_symbol);

      account_object to_account;
      object_id_type to_obj_id;

      bool is_account = true;

      try {
         to_account = get_account(to);
         to_obj_id = object_id_type(to_account.id);
      }
      catch ( const fc::exception& )
      {
         is_account = false;
      }

      if( !is_account )
      {
         to_obj_id = object_id_type(to);
         content_id_type content_id = to_obj_id.as<content_id_type>();
         const content_object content_obj = get_object<content_object>(content_id);
         to_account = get_account( content_obj.author);
      }

      signed_transaction tx;
      transfer_operation xfer_op;

      xfer_op.from = from_id;
      xfer_op.to = to_obj_id;
      xfer_op.amount = asset_obj.amount_from_string(amount);

      if( !memo.empty() )
      {
         xfer_op.memo = memo_data(memo, get_private_key(from_account.options.memo_key), to_account.options.memo_key);
      }

      tx.operations.push_back(xfer_op);

      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction(tx, broadcast);
   } FC_CAPTURE_AND_RETHROW( (from)(to)(amount)(asset_symbol)(memo)(broadcast) ) }

   signed_transaction propose_transfer(const string& proposer,
                         const string& from,
                         const string& to,
                         const string& amount,
                         const string& asset_symbol,
                         const string& memo,
                         time_point_sec expiration)
   {
      transaction_handle_type propose_num = begin_builder_transaction();
      transfer_obsolete_operation op;
      account_object from_account = get_account(from);
      account_object to_account = get_account(to);
      op.from = from_account.id;
      op.to = to_account.id;
      asset_object asset_obj = get_asset(asset_symbol);
      op.amount = asset_obj.amount_from_string(amount);
      if( !memo.empty() )
      {
         op.memo = memo_data(memo, get_private_key(from_account.options.memo_key), to_account.options.memo_key);
      }

      add_operation_to_builder_transaction(propose_num, op);
      set_fees_on_builder_transaction(propose_num);

      return propose_builder_transaction2(propose_num, proposer, expiration);
   }

   std::string decrypt_memo( const memo_data& memo, const account_object& from_account, const account_object& to_account ) const
   {
      FC_ASSERT(_keys.count(memo.to) || _keys.count(memo.from), "Memo is encrypted to a key ${to} or ${from} not in this wallet.", ("to", memo.to)("from",memo.from));
      std::string memo_result;
      if( _keys.count(memo.to) ) {
         vector<fc::ecc::private_key> keys_to_try_to;
         auto my_memo_key = wif_to_key(_keys.at(memo.to));

         FC_ASSERT(my_memo_key, "Unable to recover private key to decrypt memo. Wallet may be corrupted.");
         keys_to_try_to.push_back(*my_memo_key);
         for( auto k: to_account.active.key_auths ) {
            auto key_itr = _keys.find(k.first);
            if( key_itr == _keys.end() )
               continue;
            auto my_key = wif_to_key(key_itr->second);
            if(my_key)
               keys_to_try_to.push_back(*my_key);
         }
         for( auto k: to_account.owner.key_auths ) {
            auto key_itr = _keys.find(k.first);
            if( key_itr == _keys.end() )
               continue;
            auto my_key = wif_to_key(key_itr->second);
            if(my_key)
               keys_to_try_to.push_back(*my_key);
         }

         for( auto k : keys_to_try_to ){
            try{
               memo_result = memo.get_message(k, memo.from);
               return memo_result;
            }catch(...){}
         }

         vector<public_key_type> keys_to_try_from;
         for( auto k : from_account.active.key_auths ){
            keys_to_try_from.push_back(k.first);
         }
         for( auto k : from_account.owner.key_auths ){
            keys_to_try_from.push_back(k.first);
         }
         for( auto k : keys_to_try_from ){
            try{
               memo_result = memo.get_message(*my_memo_key, k);
               return memo_result;
            }catch(...){}
         }

      } else {
         vector<fc::ecc::private_key> keys_to_try_from;
         auto my_memo_key = wif_to_key(_keys.at(memo.from));

         FC_ASSERT(my_memo_key, "Unable to recover private key to decrypt memo. Wallet may be corrupted.");
         keys_to_try_from.push_back(*my_memo_key);
         for( auto k: from_account.active.key_auths ) {
            auto key_itr = _keys.find(k.first);
            if( key_itr == _keys.end() )
               continue;
            auto my_key = wif_to_key(key_itr->second);
            if(my_key)
               keys_to_try_from.push_back(*my_key);
         }
         for( auto k: from_account.owner.key_auths ) {
            auto key_itr = _keys.find(k.first);
            if( key_itr == _keys.end() )
               continue;
            auto my_key = wif_to_key(key_itr->second);
            if(my_key)
               keys_to_try_from.push_back(*my_key);
         }

         for( auto k : keys_to_try_from ){
            try{
               memo_result = memo.get_message(k, memo.to);
               return memo_result;
            }catch(...){}
         }

         vector<public_key_type> keys_to_try_to;
         for( auto k : to_account.active.key_auths ) {
            keys_to_try_to.push_back(k.first);
         }
         for( auto k : to_account.owner.key_auths ) {
            keys_to_try_to.push_back(k.first);
         }
         for( auto k : keys_to_try_to ) {
            try {
               memo_result = memo.get_message(*my_memo_key, k);
               return memo_result;
            } catch( ... ) {}
         }

      }

      FC_ASSERT(false);
   }

   std::map<string,std::function<string(fc::variant,const fc::variants&)>> get_result_formatters() const
   {
      std::map<string,std::function<string(fc::variant,const fc::variants&)> > m;
      m["help"] = m["get_help"] = m["from_command_file"] = [](variant result, const fc::variants& a)
      {
         return result.get_string();
      };

      return m;
   }

   signed_transaction propose_parameter_change(
      const string& proposing_account,
      fc::time_point_sec expiration_time,
      const variant_object& changed_values,
      bool broadcast = false)
   {
      FC_ASSERT( !changed_values.contains("current_fees") );

      const chain_parameters& current_params = get_global_properties().parameters;
      chain_parameters new_params = current_params;
      fc::reflector<chain_parameters>::visit(
         fc::from_variant_visitor<chain_parameters>( changed_values, new_params )
         );

      miner_update_global_parameters_operation update_op;
      update_op.new_parameters = new_params;

      proposal_create_operation prop_op;

      prop_op.expiration_time = expiration_time;
      prop_op.review_period_seconds = current_params.miner_proposal_review_period;
      prop_op.fee_paying_account = get_account(proposing_account).id;

      prop_op.proposed_ops.emplace_back( update_op );
      current_params.current_fees->set_fee( prop_op.proposed_ops.back().op );

      signed_transaction tx;
      tx.operations.push_back(prop_op);
      set_operation_fees(tx, current_params.current_fees);
      tx.validate();

      return sign_transaction(tx, broadcast);
   }

   signed_transaction propose_fee_change(
      const string& proposing_account,
      fc::time_point_sec expiration_time,
      const variant_object& changed_fees,
      bool broadcast = false)
   {
      const chain_parameters& current_params = get_global_properties().parameters;
      const fee_schedule_type& current_fees = *(current_params.current_fees);

      flat_map< int, fee_parameters > fee_map;
      fee_map.reserve( current_fees.parameters.size() );
      for( const fee_parameters& op_fee : current_fees.parameters )
         fee_map[ op_fee.which() ] = op_fee;
      uint32_t scale = current_fees.scale;

      for( const auto& item : changed_fees )
      {
         const string& key = item.key();
         if( key == "scale" )
         {
            int64_t _scale = item.value().as_int64();
            FC_ASSERT( _scale >= 0 );
            FC_ASSERT( _scale <= std::numeric_limits<uint32_t>::max() );
            scale = uint32_t( _scale );
            continue;
         }
         // is key a number?
         auto is_numeric = [&]() -> bool
         {
            size_t n = key.size();
            for( size_t i=0; i<n; i++ )
            {
               if( !isdigit( key[i] ) )
                  return false;
            }
            return true;
         };

         int which;
         if( is_numeric() )
            which = std::stoi( key );
         else
         {
            const auto& n2w = _operation_which_map.name_to_which;
            auto it = n2w.find( key );
            FC_ASSERT( it != n2w.end(), "unknown operation" );
            which = it->second;
         }

         fee_parameters fp = from_which_variant< fee_parameters >( which, item.value() );
         fee_map[ which ] = fp;
      }

      fee_schedule_type new_fees;

      for( const std::pair< int, fee_parameters >& item : fee_map )
         new_fees.parameters.insert( item.second );
      new_fees.scale = scale;

      chain_parameters new_params = current_params;
      new_params.current_fees = new_fees;

      miner_update_global_parameters_operation update_op;
      update_op.new_parameters = new_params;

      proposal_create_operation prop_op;

      prop_op.expiration_time = expiration_time;
      prop_op.review_period_seconds = current_params.miner_proposal_review_period;
      prop_op.fee_paying_account = get_account(proposing_account).id;

      prop_op.proposed_ops.emplace_back( update_op );
      current_params.current_fees->set_fee( prop_op.proposed_ops.back().op );

      signed_transaction tx;
      tx.operations.push_back(prop_op);
      set_operation_fees(tx, current_params.current_fees);
      tx.validate();

      return sign_transaction(tx, broadcast);
   }

   signed_transaction approve_proposal(
      const string& fee_paying_account,
      const string& proposal_id,
      const approval_delta& delta,
      bool broadcast = false)
   {
      proposal_update_operation update_op;

      update_op.fee_paying_account = get_account(fee_paying_account).id;
      update_op.proposal = fc::variant(proposal_id).as<proposal_id_type>();
      // make sure the proposal exists
      get_object( update_op.proposal );

      for( const std::string& name : delta.active_approvals_to_add )
         update_op.active_approvals_to_add.insert( get_account( name ).id );
      for( const std::string& name : delta.active_approvals_to_remove )
         update_op.active_approvals_to_remove.insert( get_account( name ).id );
      for( const std::string& name : delta.owner_approvals_to_add )
         update_op.owner_approvals_to_add.insert( get_account( name ).id );
      for( const std::string& name : delta.owner_approvals_to_remove )
         update_op.owner_approvals_to_remove.insert( get_account( name ).id );
      for( const std::string& k : delta.key_approvals_to_add )
         update_op.key_approvals_to_add.insert( public_key_type( k ) );
      for( const std::string& k : delta.key_approvals_to_remove )
         update_op.key_approvals_to_remove.insert( public_key_type( k ) );

      signed_transaction tx;
      tx.operations.push_back(update_op);
      set_operation_fees(tx, get_global_properties().parameters.current_fees);
      tx.validate();
      return sign_transaction(tx, broadcast);
   }


   void submit_content_utility(content_submit_operation& submit_op,
                               vector<regional_price_info> const& price_amounts)
   {
      vector<regional_price> arr_prices;

      for (auto const& item : price_amounts)
      {

         uint32_t region_code_for = RegionCodes::OO_none;

         auto it = RegionCodes::s_mapNameToCode.find(item.region);
         if (it != RegionCodes::s_mapNameToCode.end())
            region_code_for = it->second;
         else
            FC_ASSERT(false, "Invalid region code");

         asset_object currency = get_asset(item.asset_symbol);

         arr_prices.push_back({region_code_for, currency.amount_from_string(item.amount)});
      }

      submit_op.price = arr_prices;
   }

   signed_transaction submit_content(string const& author,
                                     vector< pair< string, uint32_t>> co_authors,
                                     string const& URI,
                                     vector<regional_price_info> const& price_amounts,
                                     fc::ripemd160 const& hash,
                                     uint64_t size,
                                     vector<account_id_type> const& seeders,
                                     uint32_t quorum,
                                     fc::time_point_sec const& expiration,
                                     string const& publishing_fee_symbol_name,
                                     string const& publishing_fee_amount,
                                     string const& synopsis,
                                     DInteger const& secret,
                                     decent::encrypt::CustodyData const& cd,
                                     bool broadcast/* = false */)
   {
      try
      {
         auto& pmc = decent::package::PackageManagerConfigurator::instance();
         decent::check_ipfs_minimal_version(pmc.get_ipfs_host(), pmc.get_ipfs_port());
         account_id_type author_account = get_account( author ).get_id();

         map<account_id_type, uint32_t> co_authors_id_to_split;
         if( !co_authors.empty() )
         {
            for( auto const& element : co_authors )
            {
               account_id_type co_author = get_account( element.first ).get_id();
               co_authors_id_to_split[ co_author ] = element.second;
            }
         }

         // checking for duplicates
         FC_ASSERT( co_authors.size() == co_authors_id_to_split.size(), "Duplicity in the list of co-authors is not allowed." );

         asset_object fee_asset_obj = get_asset(publishing_fee_symbol_name);

         ShamirSecret ss(static_cast<uint16_t>(quorum), static_cast<uint16_t>(seeders.size()), secret);
         ss.calculate_split();
         content_submit_operation submit_op;
         for( int i =0; i<(int)seeders.size(); i++ )
         {
            const auto& s = _remote_db->get_seeder( seeders[i] );
            Ciphertext cp;
            point p = ss.split[i];
            decent::encrypt::el_gamal_encrypt( p ,s->pubKey ,cp );
            submit_op.key_parts.push_back(cp);
         }

         submit_op.author = author_account;
         submit_op.co_authors = co_authors_id_to_split;
         submit_op.URI = URI;
         submit_content_utility(submit_op, price_amounts);
         submit_op.hash = hash;
         submit_op.size = size;
         submit_op.seeders = seeders;
         submit_op.quorum = quorum;
         submit_op.expiration = expiration;
         submit_op.publishing_fee = fee_asset_obj.amount_from_string(publishing_fee_amount);
         submit_op.synopsis = synopsis;
         submit_op.cd = cd;

         signed_transaction tx;
         tx.operations.push_back( submit_op );
         set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
         tx.validate();

         return sign_transaction( tx, broadcast );
      } FC_CAPTURE_AND_RETHROW( (author)(URI)(price_amounts)(hash)(seeders)(quorum)(expiration)(publishing_fee_symbol_name)(publishing_fee_amount)(synopsis)(secret)(broadcast) )
   }

   content_keys submit_content_async( string const& author,
                                       vector< pair< string, uint32_t>> co_authors,
                                       string const& content_dir,
                                       string const& samples_dir,
                                       string const& protocol,
                                       vector<regional_price_info> const& price_amounts,
                                       vector<account_id_type> const& seeders,
                                       fc::time_point_sec const& expiration,
                                       string const& synopsis)
   {
      try
      {
         auto& pmc = decent::package::PackageManagerConfigurator::instance();
         decent::check_ipfs_minimal_version(pmc.get_ipfs_host(), pmc.get_ipfs_port());
         account_object author_account = get_account( author );

         map<account_id_type, uint32_t> co_authors_id_to_split;
         if( !co_authors.empty() )
         {
            for( auto const &element : co_authors )
            {
               account_id_type co_author = get_account( element.first ).get_id();
               co_authors_id_to_split[ co_author ] = element.second;
            }
         }

         // checking for duplicates
         FC_ASSERT( co_authors.size() == co_authors_id_to_split.size(), "Duplicity in the list of co-authors is not allowed." );

         FC_ASSERT(false == price_amounts.empty());
         FC_ASSERT(time_point_sec(fc::time_point::now()) <= expiration);

         CryptoPP::Integer secret(randomGenerator, 256);
         while( secret >= Params::instance().DECENT_SHAMIR_ORDER ){
            CryptoPP::Integer tmp(randomGenerator, 256);
            secret = tmp;
         }

         content_keys keys;
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
            const auto& s = _remote_db->get_seeder( seeders[i] );
            if(!s)
               FC_THROW_EXCEPTION(fc::seeder_not_found_exception, "Seeder: ${s}", ("s", seeders[i]));
            Ciphertext cp;
            point p = ss.split[i];

            decent::encrypt::DIntegerString a(p.first);
            decent::encrypt::DIntegerString b(p.second);

            elog("Split ${i} = ${a} / ${b}",("i",i)("a",a)("b",b));

            decent::encrypt::el_gamal_encrypt( p, s->pubKey ,cp );
            keys.parts.push_back(cp);
         }

         content_submit_operation submit_op;
         submit_op.key_parts = keys.parts;
         submit_op.author = author_account.id;
         submit_op.co_authors = co_authors_id_to_split;
         submit_content_utility(submit_op, price_amounts);

         submit_op.seeders = seeders;
         submit_op.quorum = keys.quorum;
         submit_op.expiration = expiration;
         submit_op.synopsis = synopsis;

         auto& package_manager = decent::package::PackageManager::instance();
         auto package_handle = package_manager.get_package(content_dir, samples_dir, keys.key);
         shared_ptr<submit_transfer_listener> listener_ptr = std::make_shared<submit_transfer_listener>(*this, package_handle, submit_op, protocol);
         _package_manager_listeners.push_back(listener_ptr);
         
         package_handle->add_event_listener(listener_ptr);
         package_handle->create(false);

         //We end up here and return the  to the upper layer. The create method will continue in the background, and once finished, it will call the respective callback of submit_transfer_listener class
         return keys;
      }
      FC_CAPTURE_AND_RETHROW( (author)(content_dir)(samples_dir)(protocol)(price_amounts)(seeders)(expiration)(synopsis) )
   }

signed_transaction content_cancellation(const string& author,
                                        const string& URI,
                                        bool broadcast)
{
   try
   {
      content_cancellation_operation cc_op;
      cc_op.author = get_account( author ).get_id();
      cc_op.URI = URI;

      signed_transaction tx;
      tx.operations.push_back( cc_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();
      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (author)(URI)(broadcast) )
}

   content_download_status get_download_status(const string& consumer, const string& URI) const
   {
      try {

         account_id_type acc = get_account(consumer).id;
         optional<buying_object> bobj = _remote_db->get_buying_by_consumer_URI( acc, URI );
         if (!bobj) {
            FC_THROW("Can not find download object");
         }

         optional<content_object> content = _remote_db->get_content( URI );

         if (!content) {
             FC_THROW("Invalid content URI");
         }

         content_download_status status;
         status.received_key_parts = static_cast<uint32_t>(bobj->key_particles.size());
         status.total_key_parts = static_cast<uint32_t>(content->key_parts.size());

         auto pack = PackageManager::instance().find_package(URI);

         if (!pack) {
             status.total_download_bytes = 0;
             status.received_download_bytes = 0;
             status.status_text = "Unknown";
         } else {
            if (pack->get_data_state() == PackageInfo::CHECKED) {

                status.total_download_bytes = static_cast<int>(pack->get_size());
                status.received_download_bytes = static_cast<int>(pack->get_size());
                status.status_text = "Downloaded";
                
            } else {
                status.total_download_bytes = static_cast<int>(pack->get_total_size());
                status.received_download_bytes = static_cast<int>(pack->get_downloaded_size());
                status.status_text = "Downloading...";                
            }
         }

         return status;
      } FC_CAPTURE_AND_RETHROW( (consumer)(URI) )
   }

   string price_to_dct(const string& amount, const string& asset_symbol_or_id)
   {
      asset_object price_o = get_asset(asset_symbol_or_id);
      asset price = price_o.amount_from_string(amount);
      asset result = _remote_db->price_to_dct(price);
      return to_string(result.amount.value);
   }

   vector<operation_info> list_operations()
   {
       return _remote_db->list_operations();
   }

   string from_command_file( const std::string& command_file_name ) const
   {
       string result = "";
       std::atomic_bool cancel_token(false);
       decent::wallet_utility::WalletAPI my_api(get_wallet_filename(), { _wallet.ws_server, _wallet.ws_user, _wallet.ws_password });
       bool contains_submit_content_async = false;

       try
       {
           std::ifstream cf_in(command_file_name);
           std::string current_line;

           if (! cf_in.good())
           {
               FC_THROW("File not found or an I/O error");
           }

           my_api.Connect(cancel_token);

           while (std::getline(cf_in, current_line))
           {
               if (current_line.size() > 0)
               {
                   if (current_line == "unlock" || current_line == "set_password")
                   {
                       current_line = fc::get_password_hidden(current_line);
                   }

                   fc::variants args = fc::json::variants_from_string(current_line + char(EOF));
                   if( args.size() == 0 )
                      continue;

                   const string& method = args.front().get_string();

                   if (method == "from_command_file")
                   {
                       FC_THROW("Method from_command_file cannot be called from within a command file");
                   }

                   result += my_api.RunTask(current_line) + "\n";

                   if (method == "submit_content_async")
                   {
                       contains_submit_content_async = true;
                   }
               }
           }

           if (contains_submit_content_async)
           {
               bool still_waiting_for_package_manager = true;
               // hold on and periodically check if all package manager listeners are in a final state
               while (still_waiting_for_package_manager)
               {
                   still_waiting_for_package_manager = my_api.IsPackageManagerTaskWaiting();
                   std::this_thread::sleep_for(std::chrono::milliseconds(100));
               }
           }

       } FC_CAPTURE_AND_RETHROW( (command_file_name) )

       cancel_token = true;

       return result;
   }

   void download_content(string const& consumer, string const& URI, string const& str_region_code_from, bool broadcast)
   {
      try
      {
         optional<content_object> content = _remote_db->get_content( URI );
         account_object consumer_account = get_account( consumer );

         if (!content)
         {
            FC_THROW("Invalid content URI");
         }

         uint32_t region_code_from = RegionCodes::OO_none;

         auto it = RegionCodes::s_mapNameToCode.find(str_region_code_from);
         if (it != RegionCodes::s_mapNameToCode.end())
            region_code_from = it->second;
         //
         // may want to throw here to forbid purchase from unknown region
         // but seems can also try to allow purchase if the content has default price
         //

         optional<asset> op_price = content->price.GetPrice(region_code_from);
         if (!op_price)
            FC_THROW("content not available for this region");


         request_to_buy_operation request_op;
         request_op.consumer = consumer_account.id;
         request_op.URI = URI;

         DInteger el_gamal_priv_key = generate_private_el_gamal_key_from_secret ( get_private_key_for_account(consumer_account).get_secret() );

         request_op.pubKey = decent::encrypt::get_public_el_gamal_key( el_gamal_priv_key );
         request_op.price = _remote_db->price_to_dct(*op_price);
         request_op.region_code_from = region_code_from;

         signed_transaction tx;
         tx.operations.push_back( request_op );
         set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
         tx.validate();
         sign_transaction( tx, broadcast );
         auto& package_manager = decent::package::PackageManager::instance();
         auto package = package_manager.get_package( URI, content->_hash );
         shared_ptr<ipfs_stats_listener> listener_ptr = std::make_shared<ipfs_stats_listener>( URI, *this, consumer_account.id );
         package->add_event_listener( listener_ptr );
         package->download();
         
      } FC_CAPTURE_AND_RETHROW( (consumer)(URI)(broadcast) )
   }


   signed_transaction request_to_buy(const string& consumer,
                                     const string& URI,
                                     const string& price_asset_symbol,
                                     const string& price_amount,
                                     const string& str_region_code_from,
                                     bool broadcast/* = false */)
   { try {
      account_object consumer_account = get_account( consumer );
      asset_object asset_obj = get_asset(price_asset_symbol);

      request_to_buy_operation request_op;
      request_op.consumer = consumer_account.id;
      request_op.URI = URI;

      DInteger el_gamal_priv_key = generate_private_el_gamal_key_from_secret ( get_private_key_for_account(consumer_account).get_secret() );
      request_op.pubKey = decent::encrypt::get_public_el_gamal_key( el_gamal_priv_key );

      optional<content_object> content = _remote_db->get_content( URI );
      uint32_t region_code_from = RegionCodes::OO_none;

      auto it = RegionCodes::s_mapNameToCode.find(str_region_code_from);
      if (it != RegionCodes::s_mapNameToCode.end())
         region_code_from = it->second;
      //
      // may want to throw here to forbid purchase from unknown region
      // but seems can also try to allow purchase if the content has default price
      //

      request_op.region_code_from = region_code_from;
      request_op.price = asset_obj.amount_from_string(price_amount);

      signed_transaction tx;
      tx.operations.push_back( request_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction( tx, broadcast );
      } FC_CAPTURE_AND_RETHROW( (consumer)(URI)(price_asset_symbol)(price_amount)(broadcast) )
   }

   signed_transaction leave_rating_and_comment(const string& consumer,
                                 const string& URI,
                                 uint64_t rating,
                                 const string& comment,
                                 bool broadcast/* = false */)
   {
      try
      {
         FC_ASSERT( rating >0 && rating <=5, "Rating shall be 1-5 stars");

         account_object consumer_account = get_account( consumer );

         leave_rating_and_comment_operation leave_rating_op;
         leave_rating_op.consumer = consumer_account.id;
         leave_rating_op.URI = URI;
         leave_rating_op.rating = rating;
         leave_rating_op.comment = comment;

         signed_transaction tx;
         tx.operations.push_back( leave_rating_op );
         set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
         tx.validate();

         return sign_transaction( tx, broadcast );

      } FC_CAPTURE_AND_RETHROW( (consumer)(URI)(rating)(comment)(broadcast) )
   }

   signed_transaction subscribe_to_author( const string& from,
                                           const string& to,
                                           const string& price_amount,
                                           const string& price_asset_symbol,
                                           bool broadcast/* = false */)
   { try {
         asset_object asset_obj = get_asset(price_asset_symbol);
         FC_ASSERT( asset_obj.id == asset_id_type() );

         subscribe_operation subscribe_op;
         subscribe_op.from = get_account( from ).get_id();
         subscribe_op.to = get_account( to ).get_id();
         subscribe_op.price = asset_obj.amount_from_string(price_amount);

         signed_transaction tx;
         tx.operations.push_back( subscribe_op );
         set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
         tx.validate();

         return sign_transaction( tx, broadcast );
      } FC_CAPTURE_AND_RETHROW( (from)(to)(price_amount)(price_asset_symbol)(broadcast) ) }

   signed_transaction subscribe_by_author( const string& from,
                                           const string& to,
                                           bool broadcast/* = false */)
   { try {
         subscribe_by_author_operation subscribe_op;
         subscribe_op.from = get_account( from ).get_id();
         subscribe_op.to = get_account( to ).get_id();

         signed_transaction tx;
         tx.operations.push_back( subscribe_op );
         set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
         tx.validate();

         return sign_transaction( tx, broadcast );
      } FC_CAPTURE_AND_RETHROW( (from)(to)(broadcast)) }

   signed_transaction set_subscription( const string& account,
                                        bool allow_subscription,
                                        uint32_t subscription_period,
                                        const string& price_amount,
                                        const string& price_asset_symbol,
                                        bool broadcast/* = false */)
   { try {
         asset_object asset_obj = get_asset(price_asset_symbol);

         account_object account_object_to_modify = get_account( account );
         account_object_to_modify.options.allow_subscription = allow_subscription;
         account_object_to_modify.options.subscription_period = subscription_period;
         account_object_to_modify.options.price_per_subscribe = asset_obj.amount_from_string(price_amount);

         account_update_operation account_update_op;
         account_update_op.account = account_object_to_modify.id;
         account_update_op.new_options = account_object_to_modify.options;

         signed_transaction tx;
         tx.operations.push_back( account_update_op );
         set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
         tx.validate();

         return sign_transaction( tx, broadcast );
      } FC_CAPTURE_AND_RETHROW( (account)(allow_subscription)(subscription_period)(price_amount)(price_asset_symbol)(broadcast) ) }

   signed_transaction set_automatic_renewal_of_subscription( const string& account_id_or_name,
                                                             subscription_id_type subscription_id,
                                                             bool automatic_renewal,
                                                             bool broadcast/* = false */)
   {
      try {

         account_id_type account = get_account( account_id_or_name ).get_id();
         fc::optional<subscription_object> subscription_obj = _remote_db->get_subscription(subscription_id);
         FC_ASSERT(subscription_obj, "Could not find subscription matching ${subscription}", ("subscription", subscription_id));

         automatic_renewal_of_subscription_operation aros_op;
         aros_op.consumer = account;
         aros_op.subscription = subscription_id;
         aros_op.automatic_renewal = automatic_renewal;

         signed_transaction tx;
         tx.operations.push_back( aros_op );
         set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
         tx.validate();

         return sign_transaction( tx, broadcast );
      } FC_CAPTURE_AND_RETHROW( (account_id_or_name)(subscription_id)(automatic_renewal)(broadcast) ) }

   DInteger restore_encryption_key(const string& account, buying_id_type buying )
   {
      account_object buyer_account = get_account( account );
      const buying_object bo = get_object<buying_object>(buying);
      const content_object co = *(_remote_db->get_content(bo.URI));

      decent::encrypt::ShamirSecret ss(static_cast<uint16_t>(co.quorum), static_cast<uint16_t>(co.key_parts.size()) );
      decent::encrypt::point message;

      const auto& el_gamal_priv_key = _el_gamal_keys.find( bo.pubKey );
      FC_ASSERT( el_gamal_priv_key != _el_gamal_keys.end(), "Wallet does not contain required ElGamal key" );

      int i=0;
      for( const auto key_particle : bo.key_particles )
      {
         auto result = decent::encrypt::el_gamal_decrypt( decent::encrypt::Ciphertext( key_particle ), el_gamal_priv_key->second, message );
         FC_ASSERT(result == decent::encrypt::ok);

         decent::encrypt::DIntegerString a(message.first);
         decent::encrypt::DIntegerString b(message.second);

         elog("Split ${i} = ${a} / ${b}",("i",i)("a",a)("b",b));
         i++;
         ss.add_point( message );
      }

      FC_ASSERT( ss.resolvable() );
      ss.calculate_secret();
      return ss.secret;
   }

   pair<account_id_type, vector<account_id_type>> get_author_and_co_authors_by_URI( const string& URI )const
   {
      fc::optional<content_object> co = _remote_db->get_content( URI );
      FC_ASSERT( co.valid(), "Content does not exist.");
      pair<account_id_type, vector<account_id_type>> result;
      result.first = co->author;
      for( auto const& co_author : co->co_authors )
         result.second.emplace_back( co_author.first );

      return result;
   };

   bool is_package_manager_task_waiting()const
   {
      for (const auto& listener : _package_manager_listeners)
      {
         if (! listener->is_finished())
         {
            return true;
         }
      }

      return false;
   }

   vector<message_object> get_message_objects(optional<account_id_type> sender, optional<account_id_type> receiver, uint32_t max_count)const
   {
      try {
         const auto& mapi = _remote_api->messaging();
         vector<message_object> objects = mapi->get_message_objects(sender, receiver, max_count);
         
         for (message_object& obj : objects) {

            for (const auto& receivers_data_item : obj.receivers_data) {

               try {

                  if( obj.sender_pubkey == public_key_type() || receivers_data_item.receiver_pubkey == public_key_type() )
                  {
                     obj.text = receivers_data_item.get_message(private_key_type(), public_key_type());
                     break;
                  }

                  auto it = _keys.find(receivers_data_item.receiver_pubkey);
                  if (it != _keys.end()) {
                     fc::optional< fc::ecc::private_key > privkey = wif_to_key(it->second);
                     if (privkey)
                        obj.text = receivers_data_item.get_message(*privkey, obj.sender_pubkey );
                     else
                        std::cout << "Cannot decrypt message." << std::endl;
                  }
                  else {
                     it = _keys.find(obj.sender_pubkey);
                     if (it != _keys.end()) {
                        fc::optional< fc::ecc::private_key > privkey = wif_to_key(it->second);
                        if (privkey)
                           obj.text = receivers_data_item.get_message(*privkey, receivers_data_item.receiver_pubkey);
                        else
                           std::cout << "Cannot decrypt message." << std::endl;
                     }
                     else {
                        std::cout << "Cannot decrypt message." << std::endl;
                     }
                  }

               }
               catch (fc::exception& e)
               {
                  std::cout << "Cannot decrypt message." << std::endl;
                  std::cout << "Error: " << e.what() << std::endl;
               }
               catch (...) {
                  std::cout << "Unknown exception in decrypting message" << std::endl;
               }
            }
         }
         return objects;
      } FC_CAPTURE_AND_RETHROW((sender)(receiver))
   }

   vector<text_message> get_messages(const std::string& receiver, uint32_t max_count) const
   {
         const auto& receiver_id = get_account(receiver).get_id();
         auto itr = _wallet.my_accounts.get<graphene::db::by_id>().find(receiver_id);
         if (itr == _wallet.my_accounts.get<graphene::db::by_id>().end()) 
            return vector<text_message>();
         
         optional<account_id_type> sender_id;
         vector<message_object> objects = get_message_objects(sender_id, optional<account_id_type>(receiver_id), max_count);
         vector<text_message> messages;

         for (message_object& obj : objects) {
            graphene::chain::text_message msg;
            
            msg.created = obj.created;
            account_object account_sender = get_account(obj.sender);
            msg.from = account_sender.name;
            for (message_object_receivers_data& item : obj.receivers_data)
            {
               msg.to.push_back(get_account(item.receiver).name);
            }
            msg.text = obj.text;

            messages.push_back(msg);
         }
         return messages;
   }

   vector<text_message> get_sent_messages(const std::string& sender, uint32_t max_count)const
   {
      const auto& sender_id = get_account(sender).get_id();
      auto itr = _wallet.my_accounts.get<graphene::db::by_id>().find(sender_id);
      if (itr == _wallet.my_accounts.get<graphene::db::by_id>().end())
         return vector<text_message>();

      optional<account_id_type> receiver_id;
      vector<message_object> objects = get_message_objects(optional<account_id_type>(sender_id), receiver_id, max_count);
      vector<text_message> messages;
      
      for (message_object& obj : objects) {
         graphene::chain::text_message msg;

         msg.created = obj.created;
         account_object account_sender = get_account(obj.sender);
         msg.from = account_sender.name;
         for (message_object_receivers_data& item : obj.receivers_data)
         {
            msg.to.push_back(get_account(item.receiver).name);
         }
         msg.text = obj.text;

         messages.push_back(msg);
      }
      return messages;
   }

   signed_transaction send_message(const string& from,
                                   const std::vector<string>& to,
                                   const string& text,
                                   bool broadcast = false)
   {
      try {
         std::set<string> unique_to( to.begin(), to.end() );
         if( to.size() != unique_to.size() )
         {
            ilog( "duplicate entries has been removed" );
            std::cout<<"duplicate entries has been removed"<<std::endl;
         }

         account_object from_account = get_account(from);
         account_id_type from_id = from_account.id;

         message_payload pl;
         pl.from = from_id;
         pl.pub_from = from_account.options.memo_key;

         for (const auto& receiver : unique_to) {
            account_object to_account = get_account(receiver);
            pl.receivers_data.emplace_back(text, get_private_key(from_account.options.memo_key), to_account.options.memo_key, to_account.get_id());
         }

         custom_operation cust_op;
         cust_op.id = graphene::chain::custom_operation_subtype_messaging;
         cust_op.payer = from_id;
         cust_op.set_messaging_payload(pl);

         signed_transaction tx;
         tx.operations.push_back(cust_op);

         set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees);
         tx.validate();

         return sign_transaction(tx, broadcast);

      } FC_CAPTURE_AND_RETHROW((from)(to)(text)(broadcast))
   }

   signed_transaction send_unencrypted_message(const string& from,
                                               const std::vector<string>& to,
                                               const string& text,
                                               bool broadcast = false)
   {
      try {
         std::set<string> unique_to( to.begin(), to.end() );
         if( to.size() != unique_to.size() )
         {
            ilog( "duplicate entries has been removed" );
            std::cout<<"duplicate entries has been removed"<<std::endl;
         }

         account_object from_account = get_account(from);
         account_id_type from_id = from_account.id;
         message_payload pl;
         pl.from = from_id;
         pl.pub_from = public_key_type();

         for (const auto &receiver : unique_to) {
            account_object to_account = get_account(receiver);
            pl.receivers_data.emplace_back(text, private_key_type(), public_key_type(), to_account.get_id());
         }

         custom_operation cust_op;
         cust_op.id = graphene::chain::custom_operation_subtype_messaging;
         cust_op.payer = from_id;
         cust_op.set_messaging_payload(pl);

         signed_transaction tx;
         tx.operations.push_back(cust_op);

         set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees);
         tx.validate();

         return sign_transaction(tx, broadcast);

      } FC_CAPTURE_AND_RETHROW((from)(to)(text)(broadcast))
   }

   void reset_counters(const std::vector<std::string>& names)
   {
      const auto& monapi = _remote_api->monitoring();
      monapi->reset_counters(names);
   }

   std::vector<monitoring::counter_item_cli> get_counters(const std::vector<std::string>& names)
   {
      const auto& monapi = _remote_api->monitoring();
      std::vector<monitoring::counter_item_cli> cli_result;  
      std::vector<monitoring::counter_item> result;
      result = monapi->get_counters(names);
      std::for_each(result.begin(), result.end(), [&](monitoring::counter_item& item) {
         monitoring::counter_item_cli item_cli;
         item_cli.name = item.name;
         item_cli.value = item.value;
         item_cli.last_reset = fc::time_point_sec(item.last_reset);
         item_cli.persistent = item.persistent;
         
         cli_result.push_back(item_cli);
      });

      std::sort(cli_result.begin(), cli_result.end(), [&](monitoring::counter_item_cli& item1, monitoring::counter_item_cli& item2) {return item1.name < item2.name; });
      
      return cli_result;
   }

   void use_network_node_api()
   {
      if( _remote_net_node )
         return;
      try
      {
         _remote_net_node = _remote_api->network_node();
      }
      catch( const fc::exception& e )
      {
         std::cerr << "\nCouldn't get network node API.  You probably are not configured\n"
         "to access the network API on the decentd you are\n"
         "connecting to.  Please follow the instructions in README.md to set up an apiaccess file.\n"
         "\n";
         throw(e);
      }
   }

   void network_add_nodes( const vector<string>& nodes )
   {
      use_network_node_api();
      for( const string& node_address : nodes )
      {
         (*_remote_net_node)->add_node( fc::ip::endpoint::resolve_string( node_address ).back() );
      }
   }

   vector< variant > network_get_connected_peers()
   {
      use_network_node_api();
      const auto peers = (*_remote_net_node)->get_connected_peers();
      vector< variant > result;
      result.reserve( peers.size() );
      for( const auto& peer : peers )
      {
         variant v;
         fc::to_variant( peer, v );
         result.push_back( v );
      }
      return result;
   }

   operation get_prototype_operation( const string& operation_name )
   {
      auto it = _prototype_ops.find( operation_name );
      if( it == _prototype_ops.end() )
         FC_THROW("Unsupported operation: \"${operation_name}\"", ("operation_name", operation_name));
      return it->second;
   }

   fc::time_point_sec head_block_time() const
   {
      return _remote_db->head_block_time();
   }

   string                  _wallet_filename;
   wallet_data             _wallet;

   map<public_key_type,string> _keys;
   map<DInteger, DInteger>     _el_gamal_keys;   // public_key/private_key
   fc::sha512                  _checksum;

   chain_id_type           _chain_id;
   fc::api<login_api>      _remote_api;
   fc::api<database_api>   _remote_db;
   fc::api<network_broadcast_api>   _remote_net_broadcast;
   fc::api<history_api>    _remote_hist;
   optional< fc::api<network_node_api> > _remote_net_node;

   flat_map<string, operation> _prototype_ops;

   static_variant_map _operation_which_map = create_static_variant_map< operation >();

#ifdef __unix__
   mode_t                  _old_umask;
#endif
   const string _wallet_filename_extension = ".wallet";

   vector<shared_ptr<graphene::wallet::detail::submit_transfer_listener>> _package_manager_listeners;
   seeders_tracker _seeders_tracker;
};

   void operation_printer::fee(const asset& a) const
   {
      out << " (Fee: " << wallet.get_asset(a.asset_id).amount_to_pretty_string(a) << ")";
   }

   std::string operation_printer::memo(const optional<memo_data>& data, const account_object& from, const account_object& to) const
   {
      std::string memo;
      if( data )
      {
         if( wallet.is_locked() )
         {
            out << " -- Unlock wallet to see memo.";
         }
         else
         {
            try
            {
               memo = wallet.decrypt_memo( *data, from, to );
               out << " -- Memo: " << memo;
            }
            catch (const fc::exception& e)
            {
               out << " -- could not decrypt memo";
               elog("Error when decrypting memo: ${e}", ("e", e.to_detail_string()));
            }
         }
      }
      return memo;
   }

   template<typename T> std::string operation_printer::operator()(const T& op)const
   {
      //balance_accumulator acc;
      //op.get_balance_delta( acc, result );
      auto a = wallet.get_asset( op.fee.asset_id );
      auto payer = wallet.get_account( op.fee_payer() );

      string op_name = fc::get_typename<T>::name();
      if( op_name.find_last_of(':') != string::npos )
         op_name.erase(0, op_name.find_last_of(':')+1);
      out << op_name <<" ";
     // out << "balance delta: " << fc::json::to_string(acc.balance) <<"   ";
      out << payer.name << " fee: " << a.amount_to_pretty_string( op.fee );
      operation_result_printer rprinter(wallet);
      std::string str_result = result.visit(rprinter);
      if( !str_result.empty() )
      {
         out << "   result: " << str_result;
      }
      return std::string();
   }

   string operation_printer::operator()(const transfer_obsolete_operation& op) const
   {
      const auto& from_account = wallet.get_account(op.from);
      const auto& to_account = wallet.get_account(op.to);
      out << "Transfer " << wallet.get_asset(op.amount.asset_id).amount_to_pretty_string(op.amount)
          << " from " << from_account.name << " to " << to_account.name;
      fee(op.fee);
      return memo(op.memo, from_account, to_account);
   }

   string operation_printer::operator()(const transfer_operation& op) const
   {
      const auto& from_account = wallet.get_account(op.from);
      account_object to_account;
      string receiver;

      if(  op.to.is<account_id_type>() )
      {
         to_account = wallet.get_account(op.to);
         receiver = to_account.name;
      }
      else
      {
         content_id_type content_id = op.to.as<content_id_type>();
         const content_object content_obj = wallet.get_object<content_object>(content_id);
         to_account = wallet.get_account(content_obj.author);
         receiver = std::string(op.to);
      }

      out << "Transfer " << wallet.get_asset(op.amount.asset_id).amount_to_pretty_string(op.amount)
          << " from " << from_account.name << " to " << receiver;
      fee(op.fee);
      return memo(op.memo, from_account, to_account);
   }

   string operation_printer::operator()(const non_fungible_token_issue_operation& op) const
   {
      const auto& from_account = wallet.get_account(op.issuer);
      const auto& to_account = wallet.get_account(op.to);
      out << "Issue non fungible token " << wallet.get_non_fungible_token(op.nft_id).symbol << " from " << from_account.name << " to " << to_account.name;
      fee(op.fee);
      return memo(op.memo, from_account, to_account);
   }

   string operation_printer::operator()(const non_fungible_token_transfer_operation& op) const
   {
      const auto& from_account = wallet.get_account(op.from);
      const auto& to_account = wallet.get_account(op.to);
      const auto& nft_data = wallet.get_non_fungible_token_data(op.nft_data_id);
      out << "Transfer non fungible token " << wallet.get_non_fungible_token(nft_data.nft_id).symbol <<
         " (" << std::string(object_id_type(nft_data.get_id())) << ") from " << from_account.name << " to " << to_account.name;
      fee(op.fee);
      return memo(op.memo, from_account, to_account);
   }

   std::string operation_printer::operator()(const leave_rating_and_comment_operation& op) const
   {
      out << wallet.get_account(op.consumer).name;
      if( op.comment.empty() )
         out << " rated " << op.URI << " -- Rating: " << op.rating;
      else if( op.rating == 0 )
         out << " commented " << op.URI << " -- Comment: " << op.comment;
      else
         out << " rated and commented " << op.URI << " -- Rating: " << op.rating << " -- Comment: " << op.comment;
      fee(op.fee);
      return std::string();
   }


   std::string operation_printer::operator()(const account_create_operation& op) const
   {
      out << "Create Account '" << op.name << "'";
      fee(op.fee);
      return std::string();
   }

   std::string operation_printer::operator()(const account_update_operation& op) const
   {
      out << "Update Account '" << wallet.get_account(op.account).name << "'";
      fee(op.fee);
      return std::string();
   }

   std::string operation_printer::operator()(const asset_create_operation& op) const
   {
      out << "Create ";
      out << "Monitored Asset ";

      out << "'" << op.symbol << "' with issuer " << wallet.get_account(op.issuer).name;
      fee(op.fee);
      return std::string();
   }

   std::string operation_printer::operator()(const content_submit_operation& op) const
   {
      out << "Submit content by " << wallet.get_account(op.author).name << " -- URI: " << op.URI;
      fee(op.fee);
      return std::string();
   }

   std::string operation_printer::operator()(const request_to_buy_operation& op) const
   {
      out << "Request to buy by " << wallet.get_account(op.consumer).name << " -- URI: " << op.URI << " -- Price: " << op.price.amount.value;
      fee(op.fee);
      return std::string();
   }

   std::string operation_printer::operator()(const ready_to_publish_operation& op) const
   {
      out << "Ready to publish -- Seeder: " << wallet.get_account(op.seeder).name << " -- space: " << op.space << " -- Price per MB: " << op.price_per_MByte;
      fee(op.fee);
      return std::string();
   }

   std::string operation_printer::operator()(const custom_operation& op) const
   {
      if (op.id == custom_operation_subtype_messaging) {
         message_payload pl;
         op.get_messaging_payload(pl);

         const auto& from_account = wallet.get_account(pl.from);
         account_object to_account;
         std::string receivers;

         for (int i = 0; i < (int)pl.receivers_data.size(); i++) {
            const auto& to_account = wallet.get_account(pl.receivers_data[i].to);
            receivers += to_account.name;
            receivers += " ";
         }
         
         out << "Send message from " << from_account.name << " to " << receivers;
         
         std::string memo;
         if (wallet.is_locked())
         {
            out << " -- Unlock wallet to see memo.";
         }
         else
         {
            for (const auto& receivers_data_item : pl.receivers_data) {
               try
               {
                  try {

                     if (pl.pub_from == public_key_type() || receivers_data_item.pub_to == public_key_type())
                     {
                        memo = receivers_data_item.get_message(private_key_type(), public_key_type());
                        break;
                     }

                     auto it = wallet._keys.find(receivers_data_item.pub_to);
                     if (it != wallet._keys.end()) {
                        fc::optional< fc::ecc::private_key > privkey = wif_to_key(it->second);
                        if (privkey)
                           memo = receivers_data_item.get_message(*privkey, pl.pub_from);
                        else
                           std::cout << "Cannot decrypt message." << std::endl;
                     }
                     else {
                        it = wallet._keys.find(pl.pub_from);
                        if (it != wallet._keys.end()) {
                           fc::optional< fc::ecc::private_key > privkey = wif_to_key(it->second);
                           if (privkey)
                              memo = receivers_data_item.get_message(*privkey, receivers_data_item.pub_to);
                           else
                              std::cout << "Cannot decrypt message." << std::endl;
                        }
                        else {
                           std::cout << "Cannot decrypt message." << std::endl;
                        }
                     }

                  }
                  catch (fc::exception& e)
                  {
                     std::cout << "Cannot decrypt message." << std::endl;
                     std::cout << "Error: " << e.what() << std::endl;
                     throw;
                  }
                  catch (...) {
                     std::cout << "Unknown exception in decrypting message" << std::endl;
                     throw;
                  }
                  // memo = wallet.decrypt_memo(*op.memo, from_account, to_account);
                  out << " -- Memo: " << memo;
               }
               catch (const fc::exception& e)
               {
                  out << " -- could not decrypt memo";
                  elog("Error when decrypting memo: ${e}", ("e", e.to_detail_string()));
               }
            }
         }
         
         fee(op.fee);
         return memo;
      }
      return std::string();
   }

   std::string operation_result_printer::operator()(const void_result& x) const
   {
      return std::string();
   }

   std::string operation_result_printer::operator()(const object_id_type& oid)
   {
      return std::string(oid);
   }

   std::string operation_result_printer::operator()(const asset& a)
   {
      return _wallet.get_asset(a.asset_id).amount_to_pretty_string(a);
   }

   vector<account_id_type> seeders_tracker::track_content(const string& URI)
   {
      optional<content_object> content = _wallet._remote_db->get_content( URI );
      FC_ASSERT( content );
      vector<account_id_type> new_seeders;
      for( const auto& element : content->key_parts )
      {
         auto range = seeder_to_content.equal_range( element.first );
         auto& itr = range.first;
         if( range.first == range.second)
            new_seeders.push_back( element.first );

         while( itr != range.second )
         {
            if( itr->second == content->id )
               break;
            itr++;
         }
         if( itr == range.second )
            seeder_to_content.insert(pair<account_id_type, content_id_type>( element.first, content->id ));
      }
      return new_seeders;
   }

   vector<account_id_type> seeders_tracker::untrack_content(const string& URI)
   {
      optional<content_object> content = _wallet._remote_db->get_content( URI );
      FC_ASSERT( content );
      vector<account_id_type> finished_seeders;
      for( auto& element : content->key_parts )
      {
         auto range = seeder_to_content.equal_range( element.first );
         auto& itr = range.first;
         while( itr != range.second )
         {
            if( itr->second == content->id )
            {
               seeder_to_content.erase(itr);
               break;
            }
            itr++;
         }
         if( seeder_to_content.count( element.first ) == 0 )
            finished_seeders.push_back( element.first );
      }
      return finished_seeders;
   }

   vector<account_id_type> seeders_tracker::get_unfinished_seeders()
   {
      vector<account_id_type> result;
      for( const auto& element : initial_stats )
      {
         result.push_back( element.first );
      }
      return result;
   }

   void seeders_tracker::set_initial_stats( const account_id_type& seeder, const uint64_t amount)
   {
      FC_ASSERT( initial_stats.count(seeder) == 0 );
      initial_stats.emplace( seeder, amount );
   }

   uint64_t seeders_tracker::get_final_stats( const account_id_type& seeder, const uint64_t amount )
   {
      FC_ASSERT( initial_stats.count(seeder) == 1 );
      return amount - initial_stats[seeder];
   }

   void seeders_tracker::remove_stats( const account_id_type& seeder )
   {
      FC_ASSERT( initial_stats.erase( seeder ) == 1 );
   }

   void ipfs_stats_listener::package_download_start()
   {
      vector<account_id_type> new_seeders = _wallet._seeders_tracker.track_content( _URI);
      if( !new_seeders.empty() )
      {
         for( const auto& element : new_seeders )
         {
            const string ipfs_ID = _wallet._remote_db->get_seeder( element )->ipfs_ID;
            ipfs::Json json;
            _ipfs_client.BitswapLedger( ipfs_ID, &json );
            uint64_t received = json["Recv"];
            wlog( "IPFS stats: package download start: ${r} bytes from ${id}", ("r", received)("id", ipfs_ID));
            _wallet._seeders_tracker.set_initial_stats( element, received );
         }
      }
   }

   void ipfs_stats_listener::package_download_complete()
   {
      vector<account_id_type> finished_seeders = _wallet._seeders_tracker.untrack_content( _URI);
      if( !finished_seeders.empty() )
      {
         uint64_t difference;
         map<account_id_type,uint64_t> stats;
         for( const auto& element : finished_seeders )
         {
            const string ipfs_ID = _wallet._remote_db->get_seeder( element )->ipfs_ID;
            ipfs::Json json;
            _ipfs_client.BitswapLedger( ipfs_ID, &json );
            uint64_t received = json["Recv"];
            difference = _wallet._seeders_tracker.get_final_stats( element, received );
            wlog( "IPFS stats: package download complete: received ${r} bytes from ${id}, difference: ${diff}", ("r", received)("id", ipfs_ID)("diff", difference));
            stats.emplace( element, difference );

            _wallet._seeders_tracker.remove_stats( element );
         }

         report_stats_operation op;
         op.consumer = _consumer;
         op.stats = stats;
         signed_transaction tx;
         tx.operations.push_back( op );
         _wallet.set_operation_fees( tx, _wallet._remote_db->get_global_properties().parameters.current_fees);
         tx.validate();
         _wallet.sign_transaction(tx, true);
      }
   }

   void ipfs_stats_listener:: package_download_error(const std::string& )
   {
      vector<account_id_type> finished_seeders = _wallet._seeders_tracker.untrack_content( _URI);
      for( const auto& element : finished_seeders )
      {
         _wallet._seeders_tracker.remove_stats( element );
      }
      wlog( "IPFS stats: package download error: URI: ${uri}", ("uri", _URI));
   }

   }}}



   namespace graphene { namespace wallet {

   wallet_api::wallet_api( const fc::api<login_api> &rapi, const chain_id_type &chain_id, const server_data &ws )
      : my(new detail::wallet_api_impl(*this, rapi, chain_id, ws))
   {
   }

   wallet_api::~wallet_api()
   {
   }

#include "wallet_general.inl"
#include "wallet_wallet_file.inl"
#include "wallet_account.inl"
#include "wallet_assets.inl"
#include "wallet_transaction_builder.inl"
#include "wallet_mining.inl"
#include "wallet_seeding.inl"
#include "wallet_proposals.inl"
#include "wallet_content.inl"
#include "wallet_subscription.inl"
#include "wallet_messaging.inl"
#include "wallet_monitoring.inl"

   vector<non_fungible_token_object> wallet_api::list_non_fungible_tokens(const string& lowerbound, uint32_t limit) const
   {
      return my->_remote_db->list_non_fungible_tokens(lowerbound, limit);
   }

   non_fungible_token_object wallet_api::get_non_fungible_token(const string& nft_symbol_or_id) const
   {
      return my->get_non_fungible_token(nft_symbol_or_id);
   }

   signed_transaction_info wallet_api::create_non_fungible_token(const string& issuer,
                                                                 const string& symbol,
                                                                 const string& description,
                                                                 const non_fungible_token_data_definitions& definitions,
                                                                 uint32_t max_supply,
                                                                 bool fixed_max_supply,
                                                                 bool transferable,
                                                                 bool broadcast /* = false */)
   {
      return my->create_non_fungible_token(issuer, symbol, description, definitions, max_supply, fixed_max_supply, transferable, broadcast);
   }

   signed_transaction_info wallet_api::update_non_fungible_token(const string& issuer,
                                                                 const string& symbol,
                                                                 const string& description,
                                                                 uint32_t max_supply,
                                                                 bool fixed_max_supply,
                                                                 bool broadcast /* = false */)
   {
      return my->update_non_fungible_token(issuer, symbol, description, max_supply, fixed_max_supply, broadcast);
   }

   signed_transaction_info wallet_api::issue_non_fungible_token(const string& to_account,
                                                                const string& symbol,
                                                                const fc::variants& data,
                                                                const string& memo,
                                                                bool broadcast /* = false */)
   {
      return my->issue_non_fungible_token(to_account, symbol, data, memo, broadcast);
   }

   vector<non_fungible_token_data_object> wallet_api::list_non_fungible_token_data(const string& nft_symbol_or_id) const
   {
      return my->_remote_db->list_non_fungible_token_data(get_non_fungible_token(nft_symbol_or_id).get_id());
   }

   vector<non_fungible_token_data_object> wallet_api::get_non_fungible_token_balances(const string& account,
                                                                                      const set<string>& symbols_or_ids) const
   {
      std::set<non_fungible_token_id_type> ids;
      std::for_each(symbols_or_ids.begin(), symbols_or_ids.end(), [&](const string& symbol) { ids.insert(get_non_fungible_token(symbol).get_id()); } );
      return my->_remote_db->get_non_fungible_token_balances(get_account(account).get_id(), ids);
   }

   vector<transaction_detail_object> wallet_api::search_non_fungible_token_history(non_fungible_token_data_id_type nft_data_id) const
   {
      return my->_remote_db->search_non_fungible_token_history(nft_data_id);
   }

   signed_transaction_info wallet_api::transfer_non_fungible_token_data(const string& to_account,
                                                                        const non_fungible_token_data_id_type nft_data_id,
                                                                        const string& memo,
                                                                        bool broadcast /* = false */)
   {
      return my->transfer_non_fungible_token_data(to_account, nft_data_id, memo, broadcast);
   }

   signed_transaction_info wallet_api::burn_non_fungible_token_data(const non_fungible_token_data_id_type nft_data_id,
                                                                    bool broadcast /* = false */)
   {
      return my->burn_non_fungible_token_data(nft_data_id, broadcast);
   }

   signed_transaction_info wallet_api::update_non_fungible_token_data(const string& modifier,
                                                                      const non_fungible_token_data_id_type nft_data_id,
                                                                      const std::unordered_map<string, fc::variant>& data,
                                                                      bool broadcast /* = false */)
   {
      return my->update_non_fungible_token_data(modifier, nft_data_id, data, broadcast);
   }

   std::map<string,std::function<string(fc::variant,const fc::variants&)> > wallet_api::get_result_formatters() const
   {
      return my->get_result_formatters();
   }

   vesting_balance_object_with_info::vesting_balance_object_with_info(const vesting_balance_object& vbo,
                                                                      fc::time_point_sec now )
      : vesting_balance_object( vbo )
   {
      allowed_withdraw = get_allowed_withdraw( now );
      allowed_withdraw_time = now;
   }

   void graphene::wallet::detail::submit_transfer_listener::package_creation_complete()
   {
      uint64_t size = std::max((uint64_t)1, ( _info->get_size() + (1024 * 1024) -1 ) / (1024 * 1024));

      asset total_price_per_day;
      for( const account_id_type &account : _op.seeders )
      {
         optional<seeder_object> s = _wallet._remote_db->get_seeder( account );
         total_price_per_day += s->price.amount * size;
      }

      fc::microseconds duration = (_op.expiration - fc::time_point::now());
      uint32_t days = static_cast<uint32_t>((duration.to_seconds()+3600*24-1) / 3600 / 24);

      _op.hash = _info->get_hash();
      _op.size = size;
      _op.publishing_fee = days * total_price_per_day;
      _op.cd = _info->get_custody_data();

      _info->start_seeding(_protocol, false);
   }

   void graphene::wallet::detail::submit_transfer_listener::package_seed_complete()
   {

      _op.URI = _info->get_url();

      signed_transaction tx;
      tx.operations.push_back( _op );
      _wallet.set_operation_fees( tx, _wallet._remote_db->get_global_properties().parameters.current_fees);
      tx.validate();
       _wallet.sign_transaction(tx, true);

       _is_finished = true;
   }

} } // graphene::wallet


void fc::to_variant(const account_multi_index_type& accts, fc::variant& vo)
{
   vo = vector<account_object>(accts.begin(), accts.end());
}

void fc::from_variant(const fc::variant& var, account_multi_index_type& vo)
{
   const vector<account_object>& v = var.as<vector<account_object>>();
   vo = account_multi_index_type(v.begin(), v.end());
}
