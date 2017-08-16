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
#include <fc/io/fstream.hpp>
#include <fc/io/json.hpp>
#include <fc/io/stdio.hpp>
#include <fc/network/http/websocket.hpp>
#include <fc/rpc/cli.hpp>
#include <fc/rpc/websocket_api.hpp>
#include <fc/crypto/aes.hpp>
#include <fc/crypto/hex.hpp>
#include <fc/thread/mutex.hpp>
#include <fc/thread/scoped_lock.hpp>

#include <graphene/app/api.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/protocol/fee_schedule.hpp>
#include <graphene/utilities/git_revision.hpp>
#include <graphene/utilities/key_conversion.hpp>
#include <graphene/utilities/string_escape.hpp>
#include <graphene/utilities/words.hpp>
#include <graphene/wallet/wallet.hpp>
#include <graphene/wallet/api_documentation.hpp>
#include <graphene/wallet/reflect_util.hpp>
#include <graphene/debug_miner/debug_api.hpp>

#include <decent/package/package.hpp>

#include <fc/smart_ref_impl.hpp>
#include "json.hpp"
#include "ipfs/client.h"
#include <decent/package/package_config.hpp>

#ifndef WIN32
# include <sys/types.h>
# include <sys/stat.h>
#endif

#define BRAIN_KEY_WORD_COUNT 16

CryptoPP::AutoSeededRandomPool randomGenerator;

using namespace decent::package;



namespace graphene { namespace wallet {

namespace detail {

// this class helps to gather seeding statistics. Tracks seeders currently in use.
class seeders_tracker{
public:
   seeders_tracker(wallet_api_impl& wallet):_wallet(wallet) {};
   vector<account_id_type> track_content(const string URI);
   vector<account_id_type> untrack_content(const string URI);
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

   ipfs_stats_listener(string URI, wallet_api_impl& api, account_id_type consumer):_URI(URI), _wallet(api), _consumer(consumer),
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
      : _wallet(wallet), _info(info), _op(op), _protocol(protocol) {
   }
   
   virtual void package_seed_complete();
   virtual void package_creation_complete();
   
   fc::ripemd160 get_hash() const { return _info->get_hash(); }
   const content_submit_operation& op() const { return _op; }
   
private:
   wallet_api_impl&          _wallet;
   shared_ptr<PackageInfo>   _info;
   content_submit_operation  _op;
   std::string               _protocol;
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

   std::string fee(const asset& a) const;

public:
   operation_printer( ostream& out, const wallet_api_impl& wallet, const operation_result& r = operation_result() )
      : out(out),
        wallet(wallet),
        result(r)
   {}
   typedef std::string result_type;

   template<typename T>
   std::string operator()(const T& op)const;

   std::string operator()(const transfer_operation& op)const;
   std::string operator()(const account_create_operation& op)const;
   std::string operator()(const account_update_operation& op)const;
   std::string operator()(const asset_create_operation& op)const;
   std::string operator()(const content_submit_operation& op)const;
   std::string operator()(const request_to_buy_operation& op)const;
   std::string operator()(const leave_rating_and_comment_operation& op)const;
   std::string operator()(const ready_to_publish_operation& op)const;
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

fc::ecc::private_key derive_private_key( const std::string& prefix_string,
                                         int sequence_number )
{
   std::string sequence_string = std::to_string(sequence_number);
   fc::sha512 h = fc::sha512::hash(prefix_string + " " + sequence_string);
   fc::ecc::private_key derived_key = fc::ecc::private_key::regenerate(fc::sha256::hash(h));
   return derived_key;
}

string normalize_brain_key( string s )
{
   size_t i = 0, n = s.length();
   std::string result;
   char c;
   result.reserve( n );

   bool preceded_by_whitespace = false;
   bool non_empty = false;
   while( i < n )
   {
      c = s[i++];
      switch( c )
      {
      case ' ':  case '\t': case '\r': case '\n': case '\v': case '\f':
         preceded_by_whitespace = true;
         continue;

      case 'a': c = 'A'; break;
      case 'b': c = 'B'; break;
      case 'c': c = 'C'; break;
      case 'd': c = 'D'; break;
      case 'e': c = 'E'; break;
      case 'f': c = 'F'; break;
      case 'g': c = 'G'; break;
      case 'h': c = 'H'; break;
      case 'i': c = 'I'; break;
      case 'j': c = 'J'; break;
      case 'k': c = 'K'; break;
      case 'l': c = 'L'; break;
      case 'm': c = 'M'; break;
      case 'n': c = 'N'; break;
      case 'o': c = 'O'; break;
      case 'p': c = 'P'; break;
      case 'q': c = 'Q'; break;
      case 'r': c = 'R'; break;
      case 's': c = 'S'; break;
      case 't': c = 'T'; break;
      case 'u': c = 'U'; break;
      case 'v': c = 'V'; break;
      case 'w': c = 'W'; break;
      case 'x': c = 'X'; break;
      case 'y': c = 'Y'; break;
      case 'z': c = 'Z'; break;

      default:
         break;
      }
      if( preceded_by_whitespace && non_empty )
         result.push_back(' ');
      result.push_back(c);
      preceded_by_whitespace = false;
      non_empty = true;
   }
   return result;
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
   wallet_api_impl( wallet_api& s, const wallet_data& initial_data, fc::api<login_api> rapi )
      : self(s),
        _chain_id(initial_data.chain_id),
        _remote_api(rapi),
        _remote_db(rapi->database()),
        _remote_net_broadcast(rapi->network_broadcast()),
        _remote_hist(rapi->history()),
        _seeders_tracker(*this)
   {
      chain_id_type remote_chain_id = _remote_db->get_chain_id();
      if( remote_chain_id != _chain_id && _chain_id != chain_id_type ("0000000000000000000000000000000000000000000000000000000000000000") )
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

      _wallet.chain_id = remote_chain_id;
      _chain_id = _wallet.chain_id;
      _wallet.ws_server = initial_data.ws_server;
      _wallet.ws_user = initial_data.ws_user;
      _wallet.ws_password = initial_data.ws_password;
      decent::package::PackageManager::instance().recover_all_packages();
   }
   virtual ~wallet_api_impl()
   {
      try
      {
         _remote_db->cancel_all_subscriptions();
      }
      catch (const fc::exception& e)
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
      if( !is_locked() )
      {
         plain_keys data;
         data.keys = _keys;
         data.checksum = _checksum;
         auto plain_txt = fc::raw::pack(data);
         _wallet.cipher_keys = fc::aes_encrypt( data.checksum, plain_txt );
      }
   }

   void on_block_applied( const variant& block_id )
   {
      fc::async([this]{resync();}, "Resync after block");
   }

   bool copy_wallet_file( string destination_filename )
   {
      fc::path src_path = get_wallet_filename();
      if( !fc::exists( src_path ) )
         return false;
      fc::path dest_path = destination_filename + _wallet_filename_extension;
      int suffix = 0;
      while( fc::exists(dest_path) )
      {
         ++suffix;
         dest_path = destination_filename + "-" + to_string( suffix ) + _wallet_filename_extension;
      }
      wlog( "backing up wallet ${src} to ${dest}",
            ("src", src_path)
            ("dest", dest_path) );

      fc::path dest_parent = fc::absolute(dest_path).parent_path();
      try
      {
         enable_umask_protection();
         if( !fc::exists( dest_parent ) )
            fc::create_directories( dest_parent );
         fc::copy( src_path, dest_path );
         disable_umask_protection();
      }
      catch(...)
      {
         disable_umask_protection();
         throw;
      }
      return true;
   }

   bool is_locked()const
   {
      return _checksum == fc::sha512();
   }

   template<typename T>
   T get_object(object_id<T::space_id, T::type_id, T> id)const
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
      string client_version( graphene::utilities::git_revision_description );
      const size_t pos = client_version.find( '/' );
      if( pos != string::npos && client_version.size() > pos )
         client_version = client_version.substr( pos + 1 );

      fc::mutable_variant_object result;
      //result["blockchain_name"]        = BLOCKCHAIN_NAME;
      //result["blockchain_description"] = BTS_BLOCKCHAIN_DESCRIPTION;
      result["client_version"]           = client_version;
      result["graphene_revision"]        = graphene::utilities::git_revision_sha;
      result["graphene_revision_age"]    = fc::get_approximate_relative_time_string( fc::time_point_sec( graphene::utilities::git_revision_unix_timestamp ) );
      result["fc_revision"]              = fc::git_revision_sha;
      result["fc_revision_age"]          = fc::get_approximate_relative_time_string( fc::time_point_sec( fc::git_revision_unix_timestamp ) );
      result["compile_date"]             = "compiled on " __DATE__ " at " __TIME__;
      result["boost_version"]            = boost::replace_all_copy(std::string(BOOST_LIB_VERSION), "_", ".");
      result["openssl_version"]          = OPENSSL_VERSION_TEXT;

      std::string bitness = boost::lexical_cast<std::string>(8 * sizeof(int*)) + "-bit";
#if defined(__APPLE__)
      std::string os = "osx";
#elif defined(__linux__)
      std::string os = "linux";
#elif defined(_MSC_VER)
      std::string os = "win32";
#else
      std::string os = "other";
#endif
      result["build"] = os + " " + bitness;

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
   account_object get_account(account_id_type id) const
   {
      //if( _wallet.my_accounts.get<by_id>().count(id) )
      //   return *_wallet.my_accounts.get<by_id>().find(id);
      auto rec = _remote_db->get_accounts({id}).front();
      FC_ASSERT(rec);
      return *rec;
   }
   account_object get_account(string account_name_or_id) const
   {
      FC_ASSERT( account_name_or_id.size() > 0 );

      if( auto id = maybe_id<account_id_type>(account_name_or_id) )
      {
         // It's an ID
         return get_account(*id);
      } else {
         // It's a name
         if( _wallet.my_accounts.get<by_name>().count(account_name_or_id) )
         {
            auto local_account = *_wallet.my_accounts.get<by_name>().find(account_name_or_id);
            auto blockchain_account = _remote_db->lookup_account_names({account_name_or_id}).front();
            FC_ASSERT( blockchain_account );
            if (local_account.id != blockchain_account->id)
               elog("my account id ${id} different from blockchain id ${id2}", ("id", local_account.id)("id2", blockchain_account->id));
            if (local_account.name != blockchain_account->name)
               elog("my account name ${id} different from blockchain name ${id2}", ("id", local_account.name)("id2", blockchain_account->name));

            //return *_wallet.my_accounts.get<by_name>().find(account_name_or_id);
            return *blockchain_account;
         }
         auto rec = _remote_db->lookup_account_names({account_name_or_id}).front();
         FC_ASSERT( rec && rec->name == account_name_or_id );
         return *rec;
      }
   }
   account_id_type get_account_id(string account_name_or_id) const
   {
      return get_account(account_name_or_id).get_id();
   }
   optional<asset_object> find_asset(asset_id_type id)const
   {
      auto rec = _remote_db->get_assets({id}).front();
      if( rec )
         _asset_cache[id] = *rec;
      return rec;
   }
   optional<asset_object> find_asset(string asset_symbol_or_id)const
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

            _asset_cache[rec->get_id()] = *rec;
         }
         return rec;
      }
   }
   asset_object get_asset(asset_id_type id)const
   {
      auto opt = find_asset(id);
      FC_ASSERT(opt);
      return *opt;
   }

   asset_object get_asset(string asset_symbol_or_id)const
   {
      auto opt = find_asset(asset_symbol_or_id);
      FC_ASSERT(opt);
      return *opt;
   }

   asset_id_type get_asset_id(string asset_symbol_or_id) const
   {
      FC_ASSERT( asset_symbol_or_id.size() > 0 );
      vector<optional<asset_object>> opt_asset;
      if( std::isdigit( asset_symbol_or_id.front() ) )
         return fc::variant(asset_symbol_or_id).as<asset_id_type>();
      opt_asset = _remote_db->lookup_asset_symbols( {asset_symbol_or_id} );
      FC_ASSERT( (opt_asset.size() > 0) && (opt_asset[0].valid()) );
      return opt_asset[0]->id;
   }

   string                            get_wallet_filename() const
   {
      return _wallet_filename;
   }

   fc::ecc::private_key              get_private_key(const public_key_type& id)const
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
   bool import_key(string account_name_or_id, string wif_key)
   {
      fc::optional<fc::ecc::private_key> optional_private_key = wif_to_key(wif_key);
      if (!optional_private_key)
         FC_THROW("Invalid private key");
      graphene::chain::public_key_type wif_pub_key = optional_private_key->get_public_key();

      account_object account = get_account( account_name_or_id );


      // make a list of all current public keys for the named account
      flat_set<public_key_type> all_keys_for_account;
      std::vector<public_key_type> active_keys = account.active.get_keys();
      std::vector<public_key_type> owner_keys = account.owner.get_keys();

      if( std::find( owner_keys.begin(), owner_keys.end(), wif_pub_key ) !=owner_keys.end() )
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

         _wallet.extra_keys[account.id].insert( active_pubkey );
         _wallet.extra_keys[account.id].insert( memo_pubkey );

      }

      std::copy(active_keys.begin(), active_keys.end(), std::inserter(all_keys_for_account, all_keys_for_account.end()));
      std::copy(owner_keys.begin(), owner_keys.end(), std::inserter(all_keys_for_account, all_keys_for_account.end()));
      all_keys_for_account.insert(account.options.memo_key);

      _keys[wif_pub_key] = wif_key;

      _wallet.update_account(account);

      _wallet.extra_keys[account.id].insert(wif_pub_key);

      return all_keys_for_account.find(wif_pub_key) != all_keys_for_account.end();
   }

   bool load_wallet_file(string wallet_filename = "")
   {
      // TODO:  Merge imported wallet with existing wallet,
      //        instead of replacing it
      if( wallet_filename == "" )
         wallet_filename = _wallet_filename;

      if( ! fc::exists( wallet_filename ) )
         return false;

      _wallet = fc::json::from_file( wallet_filename ).as< wallet_data >();
      if( _wallet.chain_id != _chain_id )
         FC_THROW( "Wallet chain ID does not match",
            ("wallet.chain_id", _wallet.chain_id)
            ("chain_id", _chain_id) );

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

      return true;
   }
   void save_wallet_file(string wallet_filename = "")
   {
      //
      // Serialize in memory, then save to disk
      //
      // This approach lessens the risk of a partially written wallet
      // if exceptions are thrown in serialization
      //

      encrypt_keys();

      if( wallet_filename == "" )
         wallet_filename = _wallet_filename;

      wlog( "saving wallet to file ${fn}", ("fn", wallet_filename) );

      string data = fc::json::to_pretty_string( _wallet );
      try
      {
         enable_umask_protection();
         //
         // Parentheses on the following declaration fails to compile,
         // due to the Most Vexing Parse.  Thanks, C++
         //
         // http://en.wikipedia.org/wiki/Most_vexing_parse
         //
         fc::ofstream outfile{ fc::path( wallet_filename ) };
         outfile.write( data.c_str(), data.length() );
         outfile.flush();
         outfile.close();
         disable_umask_protection();
      }
      catch(...)
      {
         disable_umask_protection();
         throw;
      }
   }

   transaction_handle_type begin_builder_transaction()
   {
      int trx_handle = _builder_transactions.empty()? 0
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
   asset set_fees_on_builder_transaction(transaction_handle_type handle, string fee_asset = GRAPHENE_SYMBOL)
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
      string account_name_or_id,
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


   signed_transaction register_account(string name,
                                       public_key_type owner,
                                       public_key_type active,
                                       string  registrar_account,
                                       bool broadcast = false)
   { try {
      FC_ASSERT( !self.is_locked() );
      FC_ASSERT( is_valid_name(name) );
      account_create_operation account_create_op;

      // TODO:  process when pay_from_account is ID

      account_object registrar_account_object =
            this->get_account( registrar_account );

      account_id_type registrar_account_id = registrar_account_object.id;

      account_create_op.registrar = registrar_account_id;
      account_create_op.name = name;
      account_create_op.owner = authority(1, owner, 1);
      account_create_op.active = authority(1, active, 1);
      account_create_op.options.memo_key = active;

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
   } FC_CAPTURE_AND_RETHROW( (name)(owner)(active)(registrar_account)(broadcast) ) }


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

//#define DECENTGO

   signed_transaction create_account_with_private_key(fc::ecc::private_key owner_privkey,
                                                      string account_name,
                                                      string registrar_account,
                                                      bool import,
                                                      bool broadcast = false,
                                                      bool save_wallet = true)
   { try {
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
#ifdef DECENTGO
         account_create_op.active = authority(1, owner_pubkey, 1);
         account_create_op.options.memo_key = owner_pubkey;
#else
         account_create_op.active = authority(1, active_pubkey, 1);
         account_create_op.options.memo_key = memo_pubkey;
#endif

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

   signed_transaction create_account_with_brain_key(string brain_key,
                                                    string account_name,
                                                    string registrar_account,
                                                    bool import,
                                                    bool broadcast = false,
                                                    bool save_wallet = true)
   { try {

      FC_ASSERT( !self.is_locked() );
      string normalized_brain_key = normalize_brain_key( brain_key );
      // TODO:  scan blockchain for accounts that exist with same brain key
      fc::ecc::private_key owner_privkey = derive_private_key( normalized_brain_key, 0 );
      return create_account_with_private_key(owner_privkey, account_name, registrar_account, import, broadcast, save_wallet);
   } FC_CAPTURE_AND_RETHROW( (account_name)(registrar_account) ) }


   signed_transaction create_user_issued_asset(string issuer,
                                   string symbol,
                                   uint8_t precision,
                                   string description,
                                   uint64_t max_supply,
                                   price core_exchange_rate,
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
      create_op.options = opts;
      create_op.monitored_asset_opts = optional<monitored_asset_options>();

      signed_transaction tx;
      tx.operations.push_back( create_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (issuer)(symbol)(precision)(description)(max_supply)(broadcast) ) }

   signed_transaction issue_asset(string to_account, string amount, string symbol,
                                  string memo, bool broadcast = false)
   {
      auto asset_obj = get_asset(symbol);

      account_object to = get_account(to_account);
      account_object issuer = get_account(asset_obj.issuer);

      asset_issue_operation issue_op;
      issue_op.issuer           = asset_obj.issuer;
      issue_op.asset_to_issue   = asset_obj.amount_from_string(amount);
      issue_op.issue_to_account = to.id;

      if( memo.size() )
      {
         issue_op.memo = memo_data();
         issue_op.memo->from = issuer.options.memo_key;
         issue_op.memo->to = to.options.memo_key;
         issue_op.memo->set_message(get_private_key(issuer.options.memo_key),
                                    to.options.memo_key, memo);
      }

      signed_transaction tx;
      tx.operations.push_back(issue_op);
      set_operation_fees(tx,_remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction(tx, broadcast);
   }

   signed_transaction update_user_issued_asset(string symbol,
                                               string new_issuer,
                                               string description,
                                               uint64_t max_supply,
                                               price core_exchange_rate,
                                               bool broadcast = false)
   { try {
         optional<asset_object> asset_to_update = find_asset(symbol);
         if (!asset_to_update)
            FC_THROW("No asset with that symbol exists!");

         update_user_issued_asset_operation update_op;
         update_op.issuer = asset_to_update->issuer;
         update_op.asset_to_update = asset_to_update->id;
         update_op.new_description = description;
         update_op.max_supply = max_supply;
         update_op.core_exchange_rate = core_exchange_rate;

         optional<account_id_type> new_issuer_account_id;
         if( new_issuer != "" )
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
      } FC_CAPTURE_AND_RETHROW( (symbol)(new_issuer)(description)(max_supply)(core_exchange_rate)(broadcast) ) }

      signed_transaction fund_asset_fee_pool(string from,
                                             string amount,
                                             string symbol,
                                             bool broadcast /* = false */)
      { try {
            account_object from_account = get_account(from);
            optional<asset_object> asset_to_fund = find_asset(symbol);
            if (!asset_to_fund)
               FC_THROW("No asset with that symbol exists!");
            asset_object core_asset = get_asset(asset_id_type());

            asset_fund_fee_pool_operation fund_op;
            fund_op.from_account = from_account.id;
            fund_op.asset_id = asset_to_fund->id;
            fund_op.amount = core_asset.amount_from_string(amount).amount;

            signed_transaction tx;
            tx.operations.push_back( fund_op );
            set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
            tx.validate();

            return sign_transaction( tx, broadcast );
         } FC_CAPTURE_AND_RETHROW( (from)(symbol)(amount)(broadcast) ) }

   signed_transaction reserve_asset(string from,
                                    string amount,
                                    string symbol,
                                    bool broadcast /* = false */)
   { try {
         account_object from_account = get_account(from);
         optional<asset_object> asset_to_reserve = find_asset(symbol);
         if (!asset_to_reserve)
            FC_THROW("No asset with that symbol exists!");

         asset_reserve_operation reserve_op;
         reserve_op.payer = from_account.id;
         reserve_op.amount_to_reserve = asset_to_reserve->amount_from_string(amount);

         signed_transaction tx;
         tx.operations.push_back( reserve_op );
         set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
         tx.validate();

         return sign_transaction( tx, broadcast );
      } FC_CAPTURE_AND_RETHROW( (from)(amount)(symbol)(broadcast) ) }

   signed_transaction claim_fees(string amount,
                                 string symbol,
                                 bool broadcast /* = false */)
   { try {
         optional<asset_object> asset_to_claim = find_asset(symbol);
         if (!asset_to_claim)
            FC_THROW("No asset with that symbol exists!");

         asset_claim_fees_operation claim_fees_op;
         claim_fees_op.issuer = asset_to_claim->issuer;
         claim_fees_op.amount_to_claim = asset_to_claim->amount_from_string(amount);

         signed_transaction tx;
         tx.operations.push_back( claim_fees_op );
         set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
         tx.validate();

         return sign_transaction( tx, broadcast );
      } FC_CAPTURE_AND_RETHROW( (amount)(symbol)(broadcast) ) }

   signed_transaction create_monitored_asset(string issuer,
                                             string symbol,
                                             uint8_t precision,
                                             string description,
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

   signed_transaction update_monitored_asset(string symbol,
                                             string description,
                                             uint32_t feed_lifetime_sec,
                                             uint8_t minimum_feeds,
                                             bool broadcast /* = false */)
   { try {
      optional<asset_object> asset_to_update = find_asset(symbol);
      if (!asset_to_update)
        FC_THROW("No asset with that symbol exists!");

      update_monitored_asset_operation update_op;
      update_op.issuer = asset_to_update->issuer;
      update_op.asset_to_update = asset_to_update->id;
      update_op.new_description = description;
      update_op.new_feed_lifetime_sec = feed_lifetime_sec;
      update_op.new_minimum_feeds = minimum_feeds;

      signed_transaction tx;
      tx.operations.push_back( update_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (symbol)(description)(feed_lifetime_sec)(minimum_feeds)(broadcast) ) }

   signed_transaction publish_asset_feed(string publishing_account,
                                         string symbol,
                                         price_feed feed,
                                         bool broadcast /* = false */)
   { try {
      optional<asset_object> asset_to_update = find_asset(symbol);
      if (!asset_to_update)
        FC_THROW("No asset with that symbol exists!");

      asset_publish_feed_operation publish_op;
      publish_op.publisher = get_account_id(publishing_account);
      publish_op.asset_id = asset_to_update->id;
      publish_op.feed = feed;

      signed_transaction tx;
      tx.operations.push_back( publish_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (publishing_account)(symbol)(feed)(broadcast) ) }


   miner_object get_miner(string owner_account)
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
               account_id_type owner_account_id = get_account_id(owner_account);
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

   signed_transaction create_miner(string owner_account,
                                     string url,
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

   signed_transaction update_miner(string miner_name,
                                     string url,
                                     string block_signing_key,
                                     bool broadcast /* = false */)
   { try {
      miner_object miner = get_miner(miner_name);
      account_object miner_account = get_account( miner.miner_account );
      fc::ecc::private_key active_private_key = get_private_key_for_account(miner_account);

      miner_update_operation miner_update_op;
      miner_update_op.miner = miner.id;
      miner_update_op.miner_account = miner_account.id;
      if( url != "" )
         miner_update_op.new_url = url;
      if( block_signing_key != "" )
         miner_update_op.new_signing_key = public_key_type( block_signing_key );

      signed_transaction tx;
      tx.operations.push_back( miner_update_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees );
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (miner_name)(url)(block_signing_key)(broadcast) ) }

   vector< vesting_balance_object_with_info > get_vesting_balances( string account_name )
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

   signed_transaction withdraw_vesting(
      string miner_name,
      string amount,
      string asset_symbol,
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

   signed_transaction vote_for_miner(string voting_account,
                                        string miner,
                                        bool approve,
                                        bool broadcast /* = false */)
   { try {
      account_object voting_account_object = get_account(voting_account);
      account_id_type miner_owner_account_id = get_account_id(miner);
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
         unsigned votes_removed = voting_account_object.options.votes.erase(miner_obj->vote_id);
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

   signed_transaction set_voting_proxy(string account_to_modify,
                                       optional<string> voting_account,
                                       bool broadcast /* = false */)
   { try {
      account_object account_object_to_modify = get_account(account_to_modify);
      if (voting_account)
      {
         account_id_type new_voting_account_id = get_account_id(*voting_account);
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

   signed_transaction set_desired_miner_count(string account_to_modify,
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
            elog("about to broadcast tx: ${t}", ("t", tx));
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


   signed_transaction transfer(string from, string to, string amount,
                               string asset_symbol, string memo, bool broadcast = false)
   { try {
      FC_ASSERT( !self.is_locked() );
      fc::optional<asset_object> asset_obj = get_asset(asset_symbol);
      FC_ASSERT(asset_obj, "Could not find asset matching ${asset}", ("asset", asset_symbol));

      account_object from_account = get_account(from);
      account_object to_account = get_account(to);
      account_id_type from_id = from_account.id;
      account_id_type to_id = get_account_id(to);

      transfer_operation xfer_op;

      xfer_op.from = from_id;
      xfer_op.to = to_id;
      xfer_op.amount = asset_obj->amount_from_string(amount);

      if( memo.size() )
         {
            xfer_op.memo = memo_data();
            xfer_op.memo->from = from_account.options.memo_key;
            xfer_op.memo->to = to_account.options.memo_key;
            xfer_op.memo->set_message(get_private_key(from_account.options.memo_key),
                                      to_account.options.memo_key, memo);
         }

      signed_transaction tx;
      tx.operations.push_back(xfer_op);
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction(tx, broadcast);
   } FC_CAPTURE_AND_RETHROW( (from)(to)(amount)(asset_symbol)(memo)(broadcast) ) }

   void propose_transfer(string proposer,
                         string from,
                         string to,
                         string amount,
                         string asset_symbol,
                         string memo,
                         time_point_sec expiration)
   {
      int propose_num = begin_builder_transaction();
      transfer_operation op;
      account_object from_account = get_account(from);
      account_object to_account = get_account(to);
      op.from = from_account.id;
      op.to = to_account.id;
      fc::optional<asset_object> asset_obj = get_asset(asset_symbol);
      FC_ASSERT(asset_obj, "Could not find asset matching ${asset}", ("asset", asset_symbol));
      op.amount = asset_obj->amount_from_string(amount);
      if( memo.size() )
      {
         op.memo = memo_data();
         op.memo->from = from_account.options.memo_key;
         op.memo->to = to_account.options.memo_key;
         op.memo->set_message(get_private_key(from_account.options.memo_key),
                                   to_account.options.memo_key, memo);
      }

      add_operation_to_builder_transaction(propose_num, op);
      set_fees_on_builder_transaction(propose_num);

      propose_builder_transaction2(propose_num, proposer, expiration);
   }

   std::map<string,std::function<string(fc::variant,const fc::variants&)>> get_result_formatters() const
   {
      std::map<string,std::function<string(fc::variant,const fc::variants&)> > m;
      m["help"] = [](variant result, const fc::variants& a)
      {
         return result.get_string();
      };

      m["gethelp"] = [](variant result, const fc::variants& a)
      {
         return result.get_string();
      };
/*
      m["list_account_balances"] = [this](variant result, const fc::variants& a)
      {
         auto r = result.as<vector<asset>>();
         vector<asset_object> asset_recs;
         std::transform(r.begin(), r.end(), std::back_inserter(asset_recs), [this](const asset& a) {
            return get_asset(a.asset_id);
         });

         std::stringstream ss;
         for( unsigned i = 0; i < asset_recs.size(); ++i )
            ss << asset_recs[i].amount_to_pretty_string(r[i]) << "\n";

         return ss.str();
      };
*/
      m["get_order_book"] = [this](variant result, const fc::variants& a)
      {
         auto orders = result.as<order_book>();
         auto bids = orders.bids;
         auto asks = orders.asks;
         std::stringstream ss;
         std::stringstream sum_stream;
         sum_stream << "Sum(" << orders.base << ')';
         double bid_sum = 0;
         double ask_sum = 0;
         const int spacing = 20;

         auto prettify_num = [&]( double n )
         {
            //ss << n;
            if (abs( round( n ) - n ) < 0.00000000001 )
            {
               //ss << setiosflags( !ios::fixed ) << (int) n;     // doesn't compile on Linux with gcc
               ss << (int) n;
            }
            else if (n - floor(n) < 0.000001)
            {
               ss << setiosflags( ios::fixed ) << setprecision(10) << n;
            }
            else
            {
               ss << setiosflags( ios::fixed ) << setprecision(6) << n;
            }
         };

         ss << setprecision( 8 ) << setiosflags( ios::fixed ) << setiosflags( ios::left );

         ss << ' ' << setw( (spacing * 4) + 6 ) << "BUY ORDERS" << "SELL ORDERS\n"
            << ' ' << setw( spacing + 1 ) << "Price" << setw( spacing ) << orders.quote << ' ' << setw( spacing )
            << orders.base << ' ' << setw( spacing ) << sum_stream.str()
            << "   " << setw( spacing + 1 ) << "Price" << setw( spacing ) << orders.quote << ' ' << setw( spacing )
            << orders.base << ' ' << setw( spacing ) << sum_stream.str()
            << "\n"
            << "|=====================================================================================\n";

         for (int i = 0; i < bids.size() || i < asks.size() ; i++)
         {
            if ( i < bids.size() )
            {
                bid_sum += bids[i].base;
                ss << ' ' << setw( spacing );
                prettify_num( bids[i].price );
                ss << ' ' << setw( spacing );
                prettify_num( bids[i].quote );
                ss << ' ' << setw( spacing );
                prettify_num( bids[i].base );
                ss << ' ' << setw( spacing );
                prettify_num( bid_sum );
                ss << ' ';
            }
            else
            {
                ss << setw( (spacing * 4) + 5 ) << ' ';
            }

            ss << '|';

            if ( i < asks.size() )
            {
               ask_sum += asks[i].base;
               ss << ' ' << setw( spacing );
               prettify_num( asks[i].price );
               ss << ' ' << setw( spacing );
               prettify_num( asks[i].quote );
               ss << ' ' << setw( spacing );
               prettify_num( asks[i].base );
               ss << ' ' << setw( spacing );
               prettify_num( ask_sum );
            }

            ss << '\n';
         }

         ss << endl
            << "Buy Total:  " << bid_sum << ' ' << orders.base << endl
            << "Sell Total: " << ask_sum << ' ' << orders.base << endl;

         return ss.str();
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

   signed_transaction set_publishing_manager(const string from,
                                            const vector<string> to,
                                            bool is_allowed,
                                            bool broadcast)
   {
      try
      {
         FC_ASSERT( !to.empty() );
         set_publishing_manager_operation spm_op;
         spm_op.from = get_account_id( from );
         spm_op.can_create_publishers = is_allowed;

         for( const auto& element : to )
         {
            spm_op.to.push_back( get_account_id( element ) );
         }

         signed_transaction tx;
         tx.operations.push_back( spm_op );
         set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
         tx.validate();

         return sign_transaction( tx, broadcast );
      } FC_CAPTURE_AND_RETHROW( (from)(to)(is_allowed)(broadcast) )
   }
   signed_transaction set_publishing_right(const string from,
                                            const vector<string> to,
                                            bool is_allowed,
                                            bool broadcast)
   {
      try
      {
         FC_ASSERT( !to.empty() );

         set_publishing_right_operation spr_op;
         spr_op.from = get_account_id( from );
         spr_op.is_publisher = is_allowed;

         for( const auto& element : to )
         {
            spr_op.to.push_back( get_account_id( element ) );
         }

         signed_transaction tx;
         tx.operations.push_back( spr_op );
         set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
         tx.validate();

         return sign_transaction( tx, broadcast );
      } FC_CAPTURE_AND_RETHROW( (from)(to)(is_allowed)(broadcast) )
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
            FC_ASSERT(false);

         fc::optional<asset_object> currency = find_asset(item.asset_symbol);
         FC_ASSERT(currency, "Unknown symbol");

         arr_prices.push_back({region_code_for, currency->amount_from_string(item.amount)});
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
         FC_ASSERT(!is_locked());
         account_id_type author_account = get_account_id( author );

         map<account_id_type, uint32_t> co_authors_id_to_split;
         if( !co_authors.empty() )
         {
            for( auto const& element : co_authors )
            {
               account_id_type co_author = get_account_id( element.first );
               co_authors_id_to_split[ co_author ] = element.second;
            }
         }

         // checking for duplicates
         FC_ASSERT( co_authors.size() == co_authors_id_to_split.size(), "Duplicity in the list of co-authors is not allowed." );

         fc::optional<asset_object> fee_asset_obj = get_asset(publishing_fee_symbol_name);
         FC_ASSERT(fee_asset_obj, "Could not find asset matching ${asset}", ("asset", publishing_fee_symbol_name));

         ShamirSecret ss(quorum, seeders.size(), secret);
         ss.calculate_split();
         content_submit_operation submit_op;
         for( int i =0; i<seeders.size(); i++ )
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
         submit_op.publishing_fee = fee_asset_obj->amount_from_string(publishing_fee_amount);
         submit_op.synopsis = synopsis;
         submit_op.cd = cd;

         signed_transaction tx;
         tx.operations.push_back( submit_op );
         set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
         tx.validate();

         return sign_transaction( tx, broadcast );
      } FC_CAPTURE_AND_RETHROW( (author)(URI)(price_amounts)(hash)(seeders)(quorum)(expiration)(publishing_fee_symbol_name)(publishing_fee_amount)(synopsis)(secret)(broadcast) )
   }

   fc::ripemd160 submit_content_async( string const& author,
                                       vector< pair< string, uint32_t>> co_authors,
                                       string const& content_dir,
                                       string const& samples_dir,
                                       string const& protocol,
                                       vector<regional_price_info> const& price_amounts,
                                       vector<account_id_type> const& seeders,
                                       fc::time_point_sec const& expiration,
                                       string const& synopsis,
                                       bool broadcast/* = false */)
   {
      auto& package_manager = decent::package::PackageManager::instance();

      try
      {
         FC_ASSERT(!is_locked());
         account_object author_account = get_account( author );

         map<account_id_type, uint32_t> co_authors_id_to_split;
         if( !co_authors.empty() )
         {
            for( auto const &element : co_authors )
            {
               account_id_type co_author = get_account_id( element.first );
               co_authors_id_to_split[ co_author ] = element.second;
            }
         }

         // checking for duplicates
         FC_ASSERT( co_authors.size() == co_authors_id_to_split.size(), "Duplicity in the list of co-authors is not allowed." );

         FC_ASSERT(false == price_amounts.empty());
         FC_ASSERT(time_point_sec(fc::time_point::now()) <= expiration);

         CryptoPP::Integer secret(randomGenerator, 256);
         while( secret >= DECENT_SHAMIR_ORDER ){
            CryptoPP::Integer tmp(randomGenerator, 256);
            secret = tmp;
         }

         fc::sha256 sha_key;
         secret.Encode((byte*)sha_key._hash, 32);

         elog("the encryption key is: ${k}", ("k", sha_key));

         uint32_t quorum = std::max((vector<account_id_type>::size_type)2, seeders.size()/3);
         ShamirSecret ss(quorum, seeders.size(), secret);
         ss.calculate_split();
         
         content_submit_operation submit_op;

         for( int i =0; i < seeders.size(); i++ )
         {
            const auto& s = _remote_db->get_seeder( seeders[i] );
            FC_ASSERT( s, "seeder not found" );
            Ciphertext cp;
            point p = ss.split[i];

            decent::encrypt::DIntegerString a(p.first);
            decent::encrypt::DIntegerString b(p.second);

            elog("Split ${i} = ${a} / ${b}",("i",i)("a",a)("b",b));

            decent::encrypt::el_gamal_encrypt( p, s->pubKey ,cp );
            submit_op.key_parts.push_back(cp);

         }

         submit_op.author = author_account.id;
         submit_op.co_authors = co_authors_id_to_split;
         submit_content_utility(submit_op, price_amounts);

         submit_op.seeders = seeders;
         submit_op.quorum = quorum;
         submit_op.expiration = expiration;
         submit_op.synopsis = synopsis;

         auto package_handle = package_manager.get_package(content_dir, samples_dir, sha_key);
         shared_ptr<submit_transfer_listener> listener_ptr = std::make_shared<submit_transfer_listener>(*this, package_handle, submit_op, protocol);
         _package_manager_listeners.push_back(listener_ptr);
         
         package_handle->add_event_listener(listener_ptr);
         package_handle->create(false);

         //We end up here and return the  to the upper layer. The create method will continue in the background, and once finished, it will call the respective callback of submit_transfer_listener class
         return fc::ripemd160();
      }
      FC_CAPTURE_AND_RETHROW( (author)(content_dir)(samples_dir)(protocol)(price_amounts)(seeders)(expiration)(synopsis)(broadcast) )
   }

signed_transaction content_cancellation(string author,
                                        string URI,
                                        bool broadcast)
{
   try
   {
      FC_ASSERT(!is_locked());

      content_cancellation_operation cc_op;
      cc_op.author = get_account_id( author );
      cc_op.URI = URI;

      signed_transaction tx;
      tx.operations.push_back( cc_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();
      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (author)(URI)(broadcast) )
}

   optional<content_download_status> get_download_status(string consumer, string URI) const {
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
         status.received_key_parts = bobj->key_particles.size();
         status.total_key_parts = content->key_parts.size();

         
         auto pack = PackageManager::instance().find_package(URI);

         if (!pack) {
             status.total_download_bytes = 0;
             status.received_download_bytes = 0;
             status.status_text = "Unknown";
         } else {
            if (pack->get_data_state() == PackageInfo::CHECKED) {

                status.total_download_bytes = pack->get_size();
                status.received_download_bytes = pack->get_size();
                status.status_text = "Downloaded";
                
            } else {
                status.total_download_bytes = pack->get_total_size();
                status.received_download_bytes = pack->get_downloaded_size();
                status.status_text = "Downloading...";                
            }
         }

         return status;
      } FC_CAPTURE_AND_RETHROW( (consumer)(URI) )
   }

   asset price_to_dct(asset price){
      if(price.asset_id == asset_id_type() )
         return price;
      auto asset = get_asset(price.asset_id);
      FC_ASSERT(asset.is_monitored_asset(), "unable to determine DCT price for UIA");
      auto current_price = asset.monitored_asset_opts->current_feed.core_exchange_rate;
      FC_ASSERT(!current_price.is_null(), "unable to determine DCT price without price feeds");
      return price * current_price;
   }

   void download_content(string const& consumer, string const& URI, string const& str_region_code_from, bool broadcast)
   {
      try
      {
         FC_ASSERT( !is_locked() );

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
         request_op.price = price_to_dct(*op_price);
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


   signed_transaction request_to_buy(string consumer,
                                     string URI,
                                     string price_asset_symbol,
                                     string price_amount,
                                     string str_region_code_from,
                                     bool broadcast/* = false */)
   { try {
      account_object consumer_account = get_account( consumer );
      fc::optional<asset_object> asset_obj = get_asset(price_asset_symbol);
      FC_ASSERT(asset_obj, "Could not find asset matching ${asset}", ("asset", price_asset_symbol));

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
      request_op.price = asset_obj->amount_from_string(price_amount);

      signed_transaction tx;
      tx.operations.push_back( request_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction( tx, broadcast );
      } FC_CAPTURE_AND_RETHROW( (consumer)(URI)(price_asset_symbol)(price_amount)(broadcast) )
   }

   void leave_rating_and_comment(string consumer,
                                 string URI,
                                 uint64_t rating,
                                 string comment,
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

         sign_transaction( tx, broadcast );

      } FC_CAPTURE_AND_RETHROW( (consumer)(URI)(rating)(comment)(broadcast) )
   }

   void seeding_startup( string account_id_type_or_name,
                         DInteger content_private_key,
                         string seeder_private_key,
                         uint64_t free_space,
                         uint32_t seeding_price,
                         string packages_path)
   {
      account_id_type seeder = get_account_id( account_id_type_or_name );
      use_network_node_api();
      fc::ecc::private_key seeder_priv_key = *(wif_to_key(seeder_private_key));
      (*_remote_net_node)->seeding_startup( seeder, content_private_key, seeder_priv_key, free_space, seeding_price, packages_path);
   }

   signed_transaction subscribe_to_author( string from,
                                           string to,
                                           string price_amount,
                                           string price_asset_symbol,
                                           bool broadcast/* = false */)
   { try {
         fc::optional<asset_object> asset_obj = get_asset(price_asset_symbol);
         FC_ASSERT( asset_obj->id == asset_id_type() );

         subscribe_operation subscribe_op;
         subscribe_op.from = get_account_id( from );
         subscribe_op.to = get_account_id( to );
         subscribe_op.price = asset_obj->amount_from_string(price_amount);

         signed_transaction tx;
         tx.operations.push_back( subscribe_op );
         set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
         tx.validate();

         return sign_transaction( tx, broadcast );
      } FC_CAPTURE_AND_RETHROW( (from)(to)(price_amount)(price_asset_symbol)(broadcast) ) }

   signed_transaction subscribe_by_author( string from,
                                           string to,
                                           bool broadcast/* = false */)
   { try {
         subscribe_by_author_operation subscribe_op;
         subscribe_op.from = get_account_id( from );
         subscribe_op.to = get_account_id( to );

         signed_transaction tx;
         tx.operations.push_back( subscribe_op );
         set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
         tx.validate();

         return sign_transaction( tx, broadcast );
      } FC_CAPTURE_AND_RETHROW( (from)(to)(broadcast)) }

   signed_transaction set_subscription( string account,
                                        bool allow_subscription,
                                        uint32_t subscription_period,
                                        string price_amount,
                                        string price_asset_symbol,
                                        bool broadcast/* = false */)
   { try {
         fc::optional<asset_object> asset_obj = get_asset(price_asset_symbol);
         FC_ASSERT(asset_obj, "Could not find asset matching ${asset}", ("asset", price_asset_symbol));

         account_object account_object_to_modify = get_account( account );
         account_object_to_modify.options.allow_subscription = allow_subscription;
         account_object_to_modify.options.subscription_period = subscription_period;
         account_object_to_modify.options.price_per_subscribe = asset_obj->amount_from_string(price_amount);

         account_update_operation account_update_op;
         account_update_op.account = account_object_to_modify.id;
         account_update_op.new_options = account_object_to_modify.options;

         signed_transaction tx;
         tx.operations.push_back( account_update_op );
         set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
         tx.validate();

         return sign_transaction( tx, broadcast );
      } FC_CAPTURE_AND_RETHROW( (account)(allow_subscription)(subscription_period)(price_amount)(price_asset_symbol)(broadcast) ) }

   signed_transaction set_automatic_renewal_of_subscription( string account_id_or_name,
                                                             subscription_id_type subscription_id,
                                                             bool automatic_renewal,
                                                             bool broadcast/* = false */)
   {
      try {

         account_id_type account = get_account_id( account_id_or_name );
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

   DInteger restore_encryption_key(std::string account, buying_id_type buying )
   {
      account_object buyer_account = get_account( account );
      const buying_object bo = get_object<buying_object>(buying);
      const content_object co = *(_remote_db->get_content(bo.URI));

      decent::encrypt::ShamirSecret ss( co.quorum, co.key_parts.size() );
      decent::encrypt::point message;

      DInteger el_gamal_priv_key = generate_private_el_gamal_key_from_secret ( get_private_key_for_account(buyer_account).get_secret() );

      int i=0;
      for( const auto key_particle : bo.key_particles )
      {
         auto result = decent::encrypt::el_gamal_decrypt( decent::encrypt::Ciphertext( key_particle ), el_gamal_priv_key, message );
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

   void dbg_make_mia(string creator, string symbol)
   {
      create_monitored_asset(get_account(creator).name, symbol, 2, "abcd", 3600, 1, true);
   }

   void dbg_push_blocks( const std::string& src_filename, uint32_t count )
   {
      use_debug_api();
      (*_remote_debug)->debug_push_blocks( src_filename, count );
      (*_remote_debug)->debug_stream_json_objects_flush();
   }

   void dbg_generate_blocks( const std::string& debug_wif_key, uint32_t count )
   {
      use_debug_api();
      (*_remote_debug)->debug_generate_blocks( debug_wif_key, count );
      (*_remote_debug)->debug_stream_json_objects_flush();
   }

   void dbg_stream_json_objects( const std::string& filename )
   {
      use_debug_api();
      (*_remote_debug)->debug_stream_json_objects( filename );
      (*_remote_debug)->debug_stream_json_objects_flush();
   }

   void dbg_update_object( const fc::variant_object& update )
   {
      use_debug_api();
      (*_remote_debug)->debug_update_object( update );
      (*_remote_debug)->debug_stream_json_objects_flush();
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

   void use_debug_api()
   {
      if( _remote_debug )
         return;
      try
      {
        _remote_debug = _remote_api->debug();
      }
      catch( const fc::exception& e )
      {
         std::cerr << "\nCouldn't get debug node API.  You probably are not configured\n"
         "to access the debug API on the node you are connecting to.\n"
         "\n"
         "To fix this problem:\n"
         "- Please ensure you are running debug_node, not decentd.\n"
         "- Please follow the instructions in README.md to set up an apiaccess file.\n"
         "\n";
      }
   }

   void network_add_nodes( const vector<string>& nodes )
   {
      use_network_node_api();
      for( const string& node_address : nodes )
      {
         (*_remote_net_node)->add_node( fc::ip::endpoint::from_string( node_address ) );
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

   void flood_network(string prefix, uint32_t number_of_transactions)
   {
      try
      {
         const account_object& master = *_wallet.my_accounts.get<by_name>().lower_bound("decent");
         int number_of_accounts = number_of_transactions / 2;
         number_of_transactions -= number_of_accounts;
         //auto key = derive_private_key("floodshill", 0);

         fc::time_point start = fc::time_point::now();
         for( int i = 0; i < number_of_accounts; ++i )
         {
            std::ostringstream brain_key;
            brain_key << "brain key for account " << prefix << i;
            signed_transaction trx = create_account_with_brain_key(brain_key.str(), prefix + fc::to_string(i), master.name,
                                                                   /* import = */ true,
                                                                   /* broadcast = */ true,
                                                                   /* save wallet = */ false);
         }
         fc::time_point end = fc::time_point::now();
         ilog("Created ${n} accounts in ${time} milliseconds",
              ("n", number_of_accounts)("time", (end - start).count() / 1000));

         start = fc::time_point::now();
         for( int i = 0; i < number_of_accounts; ++i )
         {
            signed_transaction trx = transfer(master.name, prefix + fc::to_string(i), "0.1", "DCT", "", true);
            trx = transfer(master.name, prefix + fc::to_string(i), "0.09", "DCT", "", true);
         }
         end = fc::time_point::now();
         ilog("Transferred to ${n} accounts in ${time} milliseconds",
              ("n", number_of_accounts*2)("time", (end - start).count() / 1000));


      }
      catch (...)
      {
         throw;
      }

   }

   operation get_prototype_operation( string operation_name )
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
   fc::sha512                  _checksum;

   chain_id_type           _chain_id;
   fc::api<login_api>      _remote_api;
   fc::api<database_api>   _remote_db;
   fc::api<network_broadcast_api>   _remote_net_broadcast;
   fc::api<history_api>    _remote_hist;
   optional< fc::api<network_node_api> > _remote_net_node;
   optional< fc::api<graphene::debug_miner::debug_api> > _remote_debug;

   flat_map<string, operation> _prototype_ops;

   static_variant_map _operation_which_map = create_static_variant_map< operation >();

#ifdef __unix__
   mode_t                  _old_umask;
#endif
   const string _wallet_filename_extension = ".wallet";

   mutable map<asset_id_type, asset_object> _asset_cache;
   vector<shared_ptr<graphene::wallet::detail::submit_transfer_listener>> _package_manager_listeners;
   seeders_tracker _seeders_tracker;
};

   std::string operation_printer::fee(const asset& a)const {
      out << "   (Fee: " << wallet.get_asset(a.asset_id).amount_to_pretty_string(a) << ")";
      return "";
   }

   template<typename T>
   std::string operation_printer::operator()(const T& op)const
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
      if( str_result != "" )
      {
         out << "   result: " << str_result;
      }
      return "";
   }

   string operation_printer::operator()(const transfer_operation& op) const
   {
      const auto& from_account = wallet.get_account(op.from);
      const auto& to_account = wallet.get_account(op.to);
      out << "Transfer " << wallet.get_asset(op.amount.asset_id).amount_to_pretty_string(op.amount)
          << " from " << from_account.name << " to " << to_account.name;
      std::string memo;
      if( op.memo )
      {
         if( wallet.is_locked() )
         {
            out << " -- Unlock wallet to see memo.";
         } else {
            try {

               FC_ASSERT(wallet._keys.count(op.memo->to) || wallet._keys.count(op.memo->from), "Memo is encrypted to a key ${to} or ${from} not in this wallet.", ("to", op.memo->to)("from",op.memo->from));
               if( wallet._keys.count(op.memo->to) ) {
                  vector<fc::ecc::private_key> keys_to_try_to;
                  auto my_memo_key = wif_to_key(wallet._keys.at(op.memo->to));

                  FC_ASSERT(my_memo_key, "Unable to recover private key to decrypt memo. Wallet may be corrupted.");
                  keys_to_try_to.push_back(*my_memo_key);
                  for( auto k: to_account.active.key_auths ) {
                     auto my_key = wif_to_key(wallet._keys.at(k.first));
                     if(my_key)
                        keys_to_try_to.push_back(*my_key);
                  }
                  for( auto k: to_account.owner.key_auths ) {
                     auto my_key = wif_to_key(wallet._keys.at(k.first));
                     if(my_key)
                        keys_to_try_to.push_back(*my_key);
                  }

                  bool found = false;
                  for( auto k : keys_to_try_to ){
                     try{
                           memo = op.memo->get_message(k, op.memo->from);
                           out << " -- Memo: " << memo;
                           found = true;
                           break;

                     }catch(...){}
                  }
                  if(!found){
                     vector<public_key_type> keys_to_try_from;
                     for( auto k : from_account.active.key_auths ){
                        keys_to_try_from.push_back(k.first);
                     }
                     for( auto k : from_account.owner.key_auths ){
                        keys_to_try_from.push_back(k.first);
                     }
                     for( auto k : keys_to_try_from ){
                        try{
                           memo = op.memo->get_message(*my_memo_key, k);
                           out << " -- Memo: " << memo;
                           found = true;
                           break;
                        }catch(...){}
                     }
                  }
                  FC_ASSERT(found);

               } else {
                  vector<fc::ecc::private_key> keys_to_try_from;
                  auto my_memo_key = wif_to_key(wallet._keys.at(op.memo->from));

                  FC_ASSERT(my_memo_key, "Unable to recover private key to decrypt memo. Wallet may be corrupted.");
                  keys_to_try_from.push_back(*my_memo_key);
                  for( auto k: from_account.active.key_auths ) {
                     auto my_key = wif_to_key(wallet._keys.at(k.first));
                     if(my_key)
                        keys_to_try_from.push_back(*my_key);
                  }
                  for( auto k: from_account.owner.key_auths ) {
                     auto my_key = wif_to_key(wallet._keys.at(k.first));
                     if(my_key)
                        keys_to_try_from.push_back(*my_key);
                  }

                  bool found = false;
                  for( auto k : keys_to_try_from ){
                     try{
                        memo = op.memo->get_message(k, op.memo->to);
                        out << " -- Memo: " << memo;
                        found = true;
                        break;
                     }catch(...){}
                  }
                  if(!found) {
                     vector<public_key_type> keys_to_try_to;
                     for( auto k : to_account.active.key_auths ) {
                        keys_to_try_to.push_back(k.first);
                     }
                     for( auto k : to_account.owner.key_auths ) {
                        keys_to_try_to.push_back(k.first);
                     }
                     for( auto k : keys_to_try_to ) {
                        try {
                           memo = op.memo->get_message(*my_memo_key, k);
                           out << " -- Memo: " << memo;
                           found = true;
                           break;

                        } catch( ... ) {}
                     }
                  }
               }
            } catch (const fc::exception& e) {
               out << " -- could not decrypt memo";
               elog("Error when decrypting memo: ${e}", ("e", e.to_detail_string()));
            }
         }
      }
      fee(op.fee);
      return memo;
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
   return fee(op.fee);
}


   std::string operation_printer::operator()(const account_create_operation& op) const
   {
      out << "Create Account '" << op.name << "'";
      return fee(op.fee);
   }

   std::string operation_printer::operator()(const account_update_operation& op) const
   {
      out << "Update Account '" << wallet.get_account(op.account).name << "'";
      return fee(op.fee);
   }

   std::string operation_printer::operator()(const asset_create_operation& op) const
   {
      out << "Create ";
      out << "Monitored Asset ";

      out << "'" << op.symbol << "' with issuer " << wallet.get_account(op.issuer).name;
      return fee(op.fee);
   }

   std::string operation_printer::operator()(const content_submit_operation& op) const
   {
      out << "Submit content by " << wallet.get_account(op.author).name << " -- URI: " << op.URI;

      return fee(op.fee);
   }

   std::string operation_printer::operator()(const request_to_buy_operation& op) const
   {
      out << "Request to buy by " << wallet.get_account(op.consumer).name << " -- URI: " << op.URI << " -- Price: " << op.price.amount.value;
      return fee(op.fee);
   }

   std::string operation_printer::operator()(const ready_to_publish_operation& op) const
   {
      out << "Ready to publish -- Seeder: " << wallet.get_account(op.seeder).name << " -- space: " << op.space << " -- Price per MB: " << op.price_per_MByte;
      return fee(op.fee);
   }

   std::string operation_result_printer::operator()(const void_result& x) const
   {
      return "";
   }

   std::string operation_result_printer::operator()(const object_id_type& oid)
   {
      return std::string(oid);
   }

   std::string operation_result_printer::operator()(const asset& a)
   {
      return _wallet.get_asset(a.asset_id).amount_to_pretty_string(a);
   }

   vector<account_id_type> seeders_tracker::track_content(const string URI)
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

   vector<account_id_type> seeders_tracker::untrack_content(const string URI)
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

   void ipfs_stats_listener:: package_download_error(const std::string&)
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

   wallet_api::wallet_api(const wallet_data& initial_data, fc::api<login_api> rapi)
      : my(new detail::wallet_api_impl(*this, initial_data, rapi))
   {
   }

   wallet_api::~wallet_api()
   {
   }

   bool wallet_api::copy_wallet_file(string destination_filename)
   {
      return my->copy_wallet_file(destination_filename);
   }

   optional<signed_block_with_info> wallet_api::get_block(uint32_t num)
   {
      optional<signed_block_with_info> result = my->_remote_db->get_block(num);

      const global_property_object& gpo = my->_remote_db->get_global_properties();
      const dynamic_global_property_object& dpo = my->_remote_db->get_dynamic_global_properties();
      share_type miner_pay_from_reward = 0;
      share_type miner_pay_from_fees = 0;

      //int64_t time_to_maint = my->_remote_db->get_time_to_maint_by_block_time(result->timestamp);//(dpo.next_maintenance_time - dpo.last_budget_time).to_seconds();
      miner_reward_input mri = my->_remote_db->get_time_to_maint_by_block_time(result->timestamp);
      int64_t time_to_maint = mri.time_to_maint;
      uint32_t blocks_in_interval = (uint64_t(time_to_maint) + mri.block_interval - 1) / mri.block_interval;

      if (blocks_in_interval > 0) {
         miner_pay_from_fees = mri.from_accumulated_fees / blocks_in_interval;
      }
      miner_pay_from_reward = my->_remote_db->get_asset_per_block_by_block_num(num);

      //this should never happen, but better check.
      if (miner_pay_from_fees < share_type(0))
         miner_pay_from_fees = share_type(0);

      result->miner_reward = miner_pay_from_fees + miner_pay_from_reward;

      return result;
   }

   uint64_t wallet_api::get_account_count() const
   {
      return my->_remote_db->get_account_count();
   }

   vector<account_object> wallet_api::list_my_accounts()
   {
      return vector<account_object>(my->_wallet.my_accounts.begin(), my->_wallet.my_accounts.end());
   }

   map<string,account_id_type> wallet_api::list_accounts(const string& lowerbound, uint32_t limit)
   {
       return my->_remote_db->lookup_accounts(lowerbound, limit);
   }

   vector<account_object> wallet_api::search_accounts(const string& term, const string& order, const string& id, uint32_t limit)
   {
      return my->_remote_db->search_accounts(term, order, object_id_type(id), limit);
   }

   vector<asset> wallet_api::list_account_balances(const string& id)
   {
      if( auto real_id = detail::maybe_id<account_id_type>(id) )
         return my->_remote_db->get_account_balances(*real_id, flat_set<asset_id_type>());
      return my->_remote_db->get_account_balances(get_account(id).id, flat_set<asset_id_type>());
   }

   vector<asset_object> wallet_api::list_assets(const string& lowerbound, uint32_t limit)const
   {
      return my->_remote_db->list_assets( lowerbound, limit );
   }

   vector<operation_detail> wallet_api::get_account_history(string name, int limit)const
   {
      vector<operation_detail> result;
      auto account_id = get_account(name).get_id();

      while( limit > 0 )
      {
         operation_history_id_type start;
         if( result.size() )
         {
            start = result.back().op.id;
            start = start + 1;
         }


         vector<operation_history_object> current = my->_remote_hist->get_account_history(account_id, "", operation_history_id_type(), std::min(100,limit), start);
         for( auto& o : current ) {
            std::stringstream ss;
            auto memo = o.op.visit(detail::operation_printer(ss, *my, o.result));
            result.push_back( operation_detail{ memo, ss.str(), o } );
         }
         if( current.size() < std::min(100,limit) )
            break;
         limit -= current.size();
      }

      return result;
   }

   vector<transaction_detail_object> wallet_api::search_account_history(string const& account_name,
                                                                        string const& order,
                                                                        string const& id,
                                                                        int limit) const
   {
      vector<transaction_detail_object> result;
      try
      {
         account_object account = get_account(account_name);
         result = my->_remote_db->search_account_history(account.id, order, object_id_type(id), limit);

         for (auto& item : result)
         {
            auto const& memo = item.m_transaction_encrypted_memo;
            if (memo)
            {
               item.m_str_description += " - ";
               auto it = my->_keys.find(memo->to);
               if (it == my->_keys.end())
                  // memo is encrypted for someone else
                  item.m_str_description += "{encrypted}";
               else
               {
                  // here the memo is encrypted for me
                  // so I can decrypt it
                  string mykey = it->second;
                  auto wtok = *wif_to_key(mykey);
                  string str_memo =
                     memo->get_message(wtok, memo->from);

                  item.m_str_description += str_memo;
               }
            }
         }
      }
      catch(...){}

      return result;
   }

   brain_key_info wallet_api::suggest_brain_key()const
   {
      brain_key_info result;
      // create a private key for secure entropy
      fc::sha256 sha_entropy1 = fc::ecc::private_key::generate().get_secret();
      fc::sha256 sha_entropy2 = fc::ecc::private_key::generate().get_secret();
      fc::bigint entropy1( sha_entropy1.data(), sha_entropy1.data_size() );
      fc::bigint entropy2( sha_entropy2.data(), sha_entropy2.data_size() );
      fc::bigint entropy(entropy1);
      entropy <<= 8*sha_entropy1.data_size();
      entropy += entropy2;
      string brain_key = "";

      for( int i=0; i<BRAIN_KEY_WORD_COUNT; i++ )
      {
         fc::bigint choice = entropy % graphene::words::word_list_size;
         entropy /= graphene::words::word_list_size;
         if( i > 0 )
            brain_key += " ";
         brain_key += graphene::words::word_list[ choice.to_int64() ];
      }

      brain_key = normalize_brain_key(brain_key);
      fc::ecc::private_key priv_key = derive_private_key( brain_key, 0 );
      result.brain_priv_key = brain_key;
      result.wif_priv_key = key_to_wif( priv_key );
      result.pub_key = priv_key.get_public_key();
      return result;
   }

   pair<brain_key_info, el_gamal_key_pair> wallet_api::generate_brain_key_el_gamal_key() const
   {
      pair<brain_key_info, el_gamal_key_pair> ret;
      ret.first = suggest_brain_key();

      fc::optional<fc::ecc::private_key> op_private_key = wif_to_key(ret.first.wif_priv_key);
      FC_ASSERT(op_private_key);
      ret.second.private_key = generate_private_el_gamal_key_from_secret ( op_private_key->get_secret() );
      ret.second.public_key = decent::encrypt::get_public_el_gamal_key( ret.second.private_key );

      return ret;
   }

   brain_key_info wallet_api::get_brain_key_info(string const& brain_key) const
   {
      brain_key_info result;

      string str_brain_key = normalize_brain_key(brain_key);
      fc::ecc::private_key priv_key = derive_private_key( str_brain_key, 0 );
      result.brain_priv_key = str_brain_key;
      result.wif_priv_key = key_to_wif( priv_key );
      result.pub_key = priv_key.get_public_key();
      return result;
   }

   el_gamal_key_pair wallet_api::generate_el_gamal_keys() const
   {
      el_gamal_key_pair ret;
      ret.private_key = decent::encrypt::generate_private_el_gamal_key();
      ret.public_key = decent::encrypt::get_public_el_gamal_key( ret.private_key );
      return ret;
   }
      
   

   DInteger wallet_api::generate_encryption_key() const
   {
      CryptoPP::Integer secret(randomGenerator, 256);
      return secret;
   }

   string wallet_api::serialize_transaction( signed_transaction tx )const
   {
      return fc::to_hex(fc::raw::pack(tx));
   }

   variant wallet_api::get_object( object_id_type id ) const
   {
      return my->_remote_db->get_objects({id});
   }

   string wallet_api::get_wallet_filename() const
   {
      return my->get_wallet_filename();
   }

   transaction_handle_type wallet_api::begin_builder_transaction()
   {
      return my->begin_builder_transaction();
   }

   void wallet_api::add_operation_to_builder_transaction(transaction_handle_type transaction_handle, const operation& op)
   {
      my->add_operation_to_builder_transaction(transaction_handle, op);
   }

   void wallet_api::replace_operation_in_builder_transaction(transaction_handle_type handle, unsigned operation_index, const operation& new_op)
   {
      my->replace_operation_in_builder_transaction(handle, operation_index, new_op);
   }

   asset wallet_api::set_fees_on_builder_transaction(transaction_handle_type handle, string fee_asset)
   {
      return my->set_fees_on_builder_transaction(handle, fee_asset);
   }

   vector<proposal_object> wallet_api::get_proposed_transactions( string account_or_id )const
   {
      account_id_type id = get_account_id(account_or_id);
      return my->_remote_db->get_proposed_transactions( id );
   }

   transaction wallet_api::preview_builder_transaction(transaction_handle_type handle)
   {
      return my->preview_builder_transaction(handle);
   }

   signed_transaction wallet_api::sign_builder_transaction(transaction_handle_type transaction_handle, bool broadcast)
   {
      return my->sign_builder_transaction(transaction_handle, broadcast);
   }

   signed_transaction wallet_api::propose_builder_transaction(
      transaction_handle_type handle,
      time_point_sec expiration,
      uint32_t review_period_seconds,
      bool broadcast)
   {
      return my->propose_builder_transaction(handle, expiration, review_period_seconds, broadcast);
   }

   signed_transaction wallet_api::propose_builder_transaction2(
      transaction_handle_type handle,
      string account_name_or_id,
      time_point_sec expiration,
      uint32_t review_period_seconds,
      bool broadcast)
   {
      return my->propose_builder_transaction2(handle, account_name_or_id, expiration, review_period_seconds, broadcast);
   }

   void wallet_api::remove_builder_transaction(transaction_handle_type handle)
   {
      return my->remove_builder_transaction(handle);
   }

   account_object wallet_api::get_account(string account_name_or_id) const
   {
      return my->get_account(account_name_or_id);
   }

   asset_object wallet_api::get_asset(string asset_name_or_id) const
   {
      auto a = my->find_asset(asset_name_or_id);
      FC_ASSERT(a);
      return *a;
   }

   monitored_asset_options wallet_api::get_monitored_asset_data(string asset_name_or_id) const
   {
      auto asset = get_asset(asset_name_or_id);
      FC_ASSERT(asset.is_monitored_asset() );
      return *asset.monitored_asset_opts;
   }

   account_id_type wallet_api::get_account_id(string account_name_or_id) const
   {
      return my->get_account_id(account_name_or_id);
   }

   asset_id_type wallet_api::get_asset_id(string asset_symbol_or_id) const
   {
      return my->get_asset_id(asset_symbol_or_id);
   }

   bool wallet_api::import_key(string account_name_or_id, string wif_key)
   {
      FC_ASSERT(!is_locked());
      // backup wallet
      fc::optional<fc::ecc::private_key> optional_private_key = wif_to_key(wif_key);
      if (!optional_private_key)
         FC_THROW("Invalid private key");
      string base58_public_key = optional_private_key->get_public_key().to_base58();
   //   copy_wallet_file( "before-import-key-" + base58_public_key );

      if( my->import_key(account_name_or_id, wif_key) )
      {
         save_wallet_file();
   //      copy_wallet_file( "after-import-key-" + base58_public_key );
         return true;
      }
      return false;
   }

   map<string, bool> wallet_api::import_accounts( string filename, string password )
   {
      FC_ASSERT( !is_locked() );
      FC_ASSERT( fc::exists( filename ) );

      const auto imported_keys = fc::json::from_file<exported_keys>( filename );

      const auto password_hash = fc::sha512::hash( password );
      FC_ASSERT( fc::sha512::hash( password_hash ) == imported_keys.password_checksum );

      map<string, bool> result;
      for( const auto& item : imported_keys.account_keys )
      {
          const auto import_this_account = [ & ]() -> bool
          {
              try
              {
                  const account_object account = get_account( item.account_name );
                  const auto& owner_keys = account.owner.get_keys();
                  const auto& active_keys = account.active.get_keys();

                  for( const auto& public_key : item.public_keys )
                  {
                      if( std::find( owner_keys.begin(), owner_keys.end(), public_key ) != owner_keys.end() )
                          return true;

                      if( std::find( active_keys.begin(), active_keys.end(), public_key ) != active_keys.end() )
                          return true;
                  }
              }
              catch( ... )
              {
              }

              return false;
          };

          const auto should_proceed = import_this_account();
          result[ item.account_name ] = should_proceed;

          if( should_proceed )
          {
              uint32_t import_successes = 0;
              uint32_t import_failures = 0;
              // TODO: First check that all private keys match public keys
              for( const auto& encrypted_key : item.encrypted_private_keys )
              {
                  try
                  {
                     const auto plain_text = fc::aes_decrypt( password_hash, encrypted_key );
                     const auto private_key = fc::raw::unpack<private_key_type>( plain_text );

                     import_key( item.account_name, string( graphene::utilities::key_to_wif( private_key ) ) );
                     ++import_successes;
                  }
                  catch( const fc::exception& e )
                  {
                     elog( "Couldn't import key due to exception ${e}", ("e", e.to_detail_string()) );
                     ++import_failures;
                  }
              }
              ilog( "successfully imported ${n} keys for account ${name}", ("n", import_successes)("name", item.account_name) );
              if( import_failures > 0 )
                 elog( "failed to import ${n} keys for account ${name}", ("n", import_failures)("name", item.account_name) );
          }
      }

      return result;
   }

   bool wallet_api::import_account_keys( string filename, string password, string src_account_name, string dest_account_name )
   {
      FC_ASSERT( !is_locked() );
      FC_ASSERT( fc::exists( filename ) );

      bool is_my_account = false;
      const auto accounts = list_my_accounts();
      for( const auto& account : accounts )
      {
          if( account.name == dest_account_name )
          {
              is_my_account = true;
              break;
          }
      }
      FC_ASSERT( is_my_account );

      const auto imported_keys = fc::json::from_file<exported_keys>( filename );

      const auto password_hash = fc::sha512::hash( password );
      FC_ASSERT( fc::sha512::hash( password_hash ) == imported_keys.password_checksum );

      bool found_account = false;
      for( const auto& item : imported_keys.account_keys )
      {
          if( item.account_name != src_account_name )
              continue;

          found_account = true;

          for( const auto& encrypted_key : item.encrypted_private_keys )
          {
              const auto plain_text = fc::aes_decrypt( password_hash, encrypted_key );
              const auto private_key = fc::raw::unpack<private_key_type>( plain_text );

              my->import_key( dest_account_name, string( graphene::utilities::key_to_wif( private_key ) ) );
          }

          return true;
      }
      save_wallet_file();

      FC_ASSERT( found_account );

      return false;
   }

   string wallet_api::normalize_brain_key(string s) const
   {
      return detail::normalize_brain_key( s );
   }

   variant wallet_api::info()
   {
      return my->info();
   }

   variant_object wallet_api::about() const
   {
       return my->about();
   }

   fc::ecc::private_key wallet_api::derive_private_key(const std::string& prefix_string, int sequence_number) const
   {
      return detail::derive_private_key( prefix_string, sequence_number );
   }

   signed_transaction wallet_api::register_account(string name,
                                                   public_key_type owner_pubkey,
                                                   public_key_type active_pubkey,
                                                   string  registrar_account,
                                                   bool broadcast)
   {
      return my->register_account( name, owner_pubkey, active_pubkey, registrar_account,  broadcast );
   }

   signed_transaction wallet_api::create_account_with_brain_key(string brain_key,
                                                                string account_name,
                                                                string registrar_account,
                                                                bool broadcast /* = false */)
   {
      return my->create_account_with_brain_key(
               brain_key, account_name, registrar_account, true,
               broadcast
               );
   }

   signed_transaction wallet_api::create_account_with_brain_key_noimport(string brain_key,
                                                                         string account_name,
                                                                         string registrar_account,
                                                                         bool broadcast /* = false */)
   {
      return my->create_account_with_brain_key(
                                               brain_key, account_name, registrar_account, false,
                                               broadcast
                                               );
   }


   signed_transaction wallet_api::transfer(string from, string to, string amount,
                                           string asset_symbol, string memo, bool broadcast /* = false */)
   {
      return my->transfer(from, to, amount, asset_symbol, memo, broadcast);
   }

   signed_transaction wallet_api::create_monitored_asset(string issuer,
                                                         string symbol,
                                                         uint8_t precision,
                                                         string description,
                                                         uint32_t feed_lifetime_sec,
                                                         uint8_t minimum_feeds,
                                                         bool broadcast)

   {
      return my->create_monitored_asset(issuer, symbol, precision, description, feed_lifetime_sec, minimum_feeds, broadcast);
   }

   void wallet_api::propose_transfer(string proposer,
                                     string from,
                                     string to,
                                     string amount,
                                     string asset_symbol,
                                     string memo,
                                     time_point_sec expiration)
   {
      return my->propose_transfer(proposer, from, to, amount, asset_symbol, memo, expiration);
   }

   signed_transaction wallet_api::update_monitored_asset(string symbol,
                                                         string description,
                                                         uint32_t feed_lifetime_sec,
                                                         uint8_t minimum_feeds,
                                                         bool broadcast /* = false */)
   {
      return my->update_monitored_asset(symbol, description, feed_lifetime_sec, minimum_feeds, broadcast);
   }

   signed_transaction wallet_api::publish_asset_feed(string publishing_account,
                                                     string symbol,
                                                     price_feed feed,
                                                     bool broadcast /* = false */)
   {
      return my->publish_asset_feed(publishing_account, symbol, feed, broadcast);
   }

   signed_transaction wallet_api::create_user_issued_asset(string issuer,
                                                           string symbol,
                                                           uint8_t precision,
                                                           string description,
                                                           uint64_t max_supply,
                                                           price core_exchange_rate,
                                                           bool broadcast /* = false */)
   {
      return my->create_user_issued_asset(issuer, symbol, precision, description, max_supply, core_exchange_rate, broadcast);
   }

   signed_transaction wallet_api::issue_asset(string to_account, string amount, string symbol,
                                              string memo, bool broadcast)
   {
      return my->issue_asset(to_account, amount, symbol, memo, broadcast);
   }

   signed_transaction wallet_api::update_user_issued_asset(string symbol,
                                                           string new_issuer,
                                                           string description,
                                                           uint64_t max_supply,
                                                           price core_exchange_rate,
                                                           bool broadcast /* = false */)
   {
      return my->update_user_issued_asset(symbol, new_issuer, description, max_supply, core_exchange_rate, broadcast);
   }

   signed_transaction wallet_api::fund_asset_fee_pool(string from,
                                                      string amount,
                                                      string symbol,
                                                      bool broadcast /* = false */)
   {
      return my->fund_asset_fee_pool(from, amount, symbol, broadcast);
   }

   signed_transaction wallet_api::reserve_asset(string from,
                                                string amount,
                                                string symbol,
                                                bool broadcast /* = false */)
   {
      return my->reserve_asset(from, amount, symbol, broadcast);
   }

   signed_transaction wallet_api::claim_fees(string amount,
                                             string symbol,
                                             bool broadcast /* = false */)
   {
      return my->claim_fees( amount, symbol, broadcast);
   }

   map<string,miner_id_type> wallet_api::list_miners(const string& lowerbound, uint32_t limit)
   {
      return my->_remote_db->lookup_miner_accounts(lowerbound, limit);
   }

   miner_object wallet_api::get_miner(string owner_account)
   {
      return my->get_miner(owner_account);
   }

   signed_transaction wallet_api::create_miner(string owner_account,
                                                 string url,
                                                 bool broadcast /* = false */)
   {
      return my->create_miner(owner_account, url, broadcast);
   }

   signed_transaction wallet_api::update_miner(
      string miner_name,
      string url,
      string block_signing_key,
      bool broadcast /* = false */)
   {
      return my->update_miner(miner_name, url, block_signing_key, broadcast);
   }

   vector< vesting_balance_object_with_info > wallet_api::get_vesting_balances( string account_name )
   {
      return my->get_vesting_balances( account_name );
   }

   signed_transaction wallet_api::withdraw_vesting(
      string miner_name,
      string amount,
      string asset_symbol,
      bool broadcast /* = false */)
   {
      return my->withdraw_vesting( miner_name, amount, asset_symbol, broadcast );
   }

   signed_transaction wallet_api::vote_for_miner(string voting_account,
                                                   string miner,
                                                   bool approve,
                                                   bool broadcast /* = false */)
   {
      return my->vote_for_miner(voting_account, miner, approve, broadcast);
   }

   signed_transaction wallet_api::set_voting_proxy(string account_to_modify,
                                                   optional<string> voting_account,
                                                   bool broadcast /* = false */)
   {
      return my->set_voting_proxy(account_to_modify, voting_account, broadcast);
   }

   signed_transaction wallet_api::set_desired_miner_count(string account_to_modify,
                                                                         uint16_t desired_number_of_miners,
                                                                         bool broadcast /* = false */)
   {
      return my->set_desired_miner_count(account_to_modify, desired_number_of_miners, broadcast);
   }

   void wallet_api::set_wallet_filename(string wallet_filename)
   {
      my->_wallet_filename = wallet_filename;
   }

   signed_transaction wallet_api::sign_transaction(signed_transaction tx, bool broadcast /* = false */)
   { try {
      return my->sign_transaction( tx, broadcast);
   } FC_CAPTURE_AND_RETHROW( (tx) ) }

   operation wallet_api::get_prototype_operation(string operation_name)
   {
      return my->get_prototype_operation( operation_name );
   }

   void wallet_api::dbg_make_mia(string creator, string symbol)
   {
      FC_ASSERT(!is_locked());
      my->dbg_make_mia(creator, symbol);
   }

   void wallet_api::dbg_push_blocks( std::string src_filename, uint32_t count )
   {
      my->dbg_push_blocks( src_filename, count );
   }

   void wallet_api::dbg_generate_blocks( std::string debug_wif_key, uint32_t count )
   {
      my->dbg_generate_blocks( debug_wif_key, count );
   }

   void wallet_api::dbg_stream_json_objects( const std::string& filename )
   {
      my->dbg_stream_json_objects( filename );
   }

   void wallet_api::dbg_update_object( fc::variant_object update )
   {
      my->dbg_update_object( update );
   }

   void wallet_api::network_add_nodes( const vector<string>& nodes )
   {
      my->network_add_nodes( nodes );
   }

   vector< variant > wallet_api::network_get_connected_peers()
   {
      return my->network_get_connected_peers();
   }

   void wallet_api::flood_network(string prefix, uint32_t number_of_transactions)
   {
      FC_ASSERT(!is_locked());
      my->flood_network(prefix, number_of_transactions);
   }

   signed_transaction wallet_api::propose_parameter_change(
      const string& proposing_account,
      fc::time_point_sec expiration_time,
      const variant_object& changed_values,
      bool broadcast /* = false */
      )
   {
      return my->propose_parameter_change( proposing_account, expiration_time, changed_values, broadcast );
   }

   signed_transaction wallet_api::propose_fee_change(
      const string& proposing_account,
      fc::time_point_sec expiration_time,
      const variant_object& changed_fees,
      bool broadcast /* = false */
      )
   {
      return my->propose_fee_change( proposing_account, expiration_time, changed_fees, broadcast );
   }

   signed_transaction wallet_api::approve_proposal(
      const string& fee_paying_account,
      const string& proposal_id,
      const approval_delta& delta,
      bool broadcast /* = false */
      )
   {
      return my->approve_proposal( fee_paying_account, proposal_id, delta, broadcast );
   }

   global_property_object wallet_api::get_global_properties() const
   {
      return my->get_global_properties();
   }

   dynamic_global_property_object wallet_api::get_dynamic_global_properties() const
   {
      return my->get_dynamic_global_properties();
   }

   string wallet_api::help()const
   {
      std::vector<std::string> method_names = my->method_documentation.get_method_names();
      std::stringstream ss;
      for (const std::string method_name : method_names)
      {
         try
         {
            ss << my->method_documentation.get_brief_description(method_name);
         }
         catch (const fc::key_not_found_exception&)
         {
            ss << method_name << " (no help available)\n";
         }
      }
      return ss.str();
   }

   string wallet_api::gethelp(const string& method)const
   {
      fc::api<wallet_api> tmp;
      std::stringstream ss;
      ss << "\n";

      if( method == "import_key" )
      {
         ss << "usage: import_key ACCOUNT_NAME_OR_ID  WIF_PRIVATE_KEY\n\n";
         ss << "example: import_key \"1.2.11\" 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3\n";
         ss << "example: import_key \"usera\" 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3\n";
      }
      else if( method == "transfer" )
      {
         ss << "usage: transfer FROM TO AMOUNT SYMBOL \"memo\" BROADCAST\n\n";
         ss << "example: transfer \"1.2.11\" \"1.2.4\" 1000.03 DCT \"memo\" true\n";
         ss << "example: transfer \"usera\" \"userb\" 1000.123 DCT \"memo\" true\n";
      }
      else if( method == "create_account_with_brain_key" )
      {
         ss << "usage: create_account_with_brain_key BRAIN_KEY ACCOUNT_NAME REGISTRAR BROADCAST\n\n";
         ss << "example: create_account_with_brain_key \"my really long brain key\" \"newaccount\" \"1.2.11\" true\n";
         ss << "example: create_account_with_brain_key \"my really long brain key\" \"newaccount\" \"someaccount\" true\n";
         ss << "\n";
         ss << "This method should be used if you would like the wallet to generate new keys derived from the brain key.\n";
         ss << "The BRAIN_KEY will be used as the owner key, and the active key will be derived from the BRAIN_KEY.  Use\n";
         ss << "register_account if you already know the keys you know the public keys that you would like to register.\n";

      }
      else if( method == "register_account" )
      {
         ss << "usage: register_account ACCOUNT_NAME OWNER_PUBLIC_KEY ACTIVE_PUBLIC_KEY REGISTRAR BROADCAST\n\n";
         ss << "example: register_account \"newaccount\" \"CORE6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV\" \"CORE6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV\" \"1.3.11\" true\n";
         ss << "\n";
         ss << "Use this method to register an account for which you do not know the private keys.";
      }
      else if( method == "create_monitored_asset" )
      {
         ss << "usage: ISSUER SYMBOL PRECISION DESCRIPTION FEED_LIFETIME_SEC MINIMUM_FEEDS BROADCAST\n\n";
         ss << "PRECISION: the number of digits after the decimal point\n\n";

         ss << "\nExample value of MONITORED ASSET_OPTIONS: \n";
         ss << fc::json::to_pretty_string( graphene::chain::monitored_asset_options() );
      }
      else
      {
         std::string doxygenHelpString = my->method_documentation.get_detailed_description(method);
         if (!doxygenHelpString.empty())
            ss << doxygenHelpString;
         else
            ss << "No help defined for method " << method << "\n";
      }

      return ss.str();
   }

   bool wallet_api::load_wallet_file( string wallet_filename )
   {
      return my->load_wallet_file( wallet_filename );
   }

   void wallet_api::save_wallet_file( string wallet_filename )
   {
      my->save_wallet_file( wallet_filename );
   }

   std::map<string,std::function<string(fc::variant,const fc::variants&)> >
   wallet_api::get_result_formatters() const
   {
      return my->get_result_formatters();
   }

   bool wallet_api::is_locked()const
   {
      return my->is_locked();
   }
   bool wallet_api::is_new()const
   {
      return my->_wallet.cipher_keys.size() == 0;
   }

   void wallet_api::encrypt_keys()
   {
      my->encrypt_keys();
   }

   void wallet_api::lock()
   { try {
      FC_ASSERT( !is_locked() );
      encrypt_keys();
      for( auto & key : my->_keys )
         key.second = key_to_wif(fc::ecc::private_key());
      my->_keys.clear();
      my->_checksum = fc::sha512();
      my->self.lock_changed(true);
   } FC_CAPTURE_AND_RETHROW() }

   void wallet_api::unlock(string password)
   { try {
      FC_ASSERT(password.size() > 0);
      auto pw = fc::sha512::hash(password.c_str(), password.size());
      vector<char> decrypted = fc::aes_decrypt(pw, my->_wallet.cipher_keys);
      auto pk = fc::raw::unpack<plain_keys>(decrypted);
      FC_ASSERT(pk.checksum == pw);
      my->_keys = std::move(pk.keys);
      my->_checksum = pk.checksum;
      my->self.lock_changed(false);
   } FC_CAPTURE_AND_RETHROW() }

   void wallet_api::set_password( string password )
   {
      if( !is_new() )
         FC_ASSERT( !is_locked(), "The wallet must be unlocked before the password can be set" );
      my->_checksum = fc::sha512::hash( password.c_str(), password.size() );
      lock();
   }

   map<public_key_type, string> wallet_api::dump_private_keys()
   {
      FC_ASSERT(!is_locked());
      return my->_keys;
   }


   string wallet_api::get_private_key( public_key_type pubkey )const
   {
      return key_to_wif( my->get_private_key( pubkey ) );
   }

   signed_block_with_info::signed_block_with_info( const signed_block& block )
      : signed_block( block )
   {
      block_id = id();
      signing_key = signee();
      transaction_ids.reserve( transactions.size() );
      for( const processed_transaction& tx : transactions )
         transaction_ids.push_back( tx.id() );
   }

   vesting_balance_object_with_info::vesting_balance_object_with_info( const vesting_balance_object& vbo, fc::time_point_sec now )
      : vesting_balance_object( vbo )
   {
      allowed_withdraw = get_allowed_withdraw( now );
      allowed_withdraw_time = now;
   }

   real_supply wallet_api::get_real_supply()const
   {
      return my->_remote_db->get_real_supply();
   }

   signed_transaction wallet_api::set_publishing_manager(const string from,
                                                         const vector<string> to,
                                                         bool is_allowed,
                                                         bool broadcast )
   {
      return my->set_publishing_manager( from, to, is_allowed, broadcast );
   }

   signed_transaction wallet_api::set_publishing_right(const string from,
                                                       const vector<string> to,
                                                       bool is_allowed,
                                                       bool broadcast )
   {
      return my->set_publishing_right( from, to, is_allowed, broadcast );
   }

   vector<account_id_type> wallet_api::list_publishing_managers( const string& lower_bound_name, uint32_t limit )
   {
      return my->_remote_db->list_publishing_managers( lower_bound_name, limit );
   }

   signed_transaction
   wallet_api::submit_content(string const& author,
                              vector< pair< string, uint32_t>> co_authors,
                              string const& URI,
                              vector <regional_price_info> const& price_amounts,
                              uint64_t size,
                              fc::ripemd160 const& hash,
                              vector<account_id_type> const& seeders,
                              uint32_t quorum,
                              fc::time_point_sec const& expiration,
                              string const& publishing_fee_asset,
                              string const& publishing_fee_amount,
                              string const& synopsis,
                              DInteger const& secret,
                              decent::encrypt::CustodyData const& cd,
                              bool broadcast)
   {
      return my->submit_content(author, co_authors, URI, price_amounts, hash, size, seeders, quorum, expiration, publishing_fee_asset, publishing_fee_amount, synopsis, secret, cd, broadcast);
   }

   fc::ripemd160
   wallet_api::submit_content_async(string const &author, vector< pair< string, uint32_t>> co_authors,
                                     string const &content_dir, string const &samples_dir,
                                     string const &protocol,
                                     vector<regional_price_info> const &price_amounts,
                                     vector<account_id_type> const &seeders,
                                     fc::time_point_sec const &expiration, string const &synopsis,
                                     bool broadcast)
   {
      return my->submit_content_async(author, co_authors, content_dir, samples_dir, protocol, price_amounts, seeders, expiration, synopsis, broadcast);

   }

   signed_transaction wallet_api::content_cancellation(string author,
                                                       string URI,
                                                       bool broadcast)
   {
      return my->content_cancellation(author, URI, broadcast);
   }

   void
   wallet_api::download_content(string const& consumer, string const& URI, string const& region_code_from, bool broadcast)
   {
      return my->download_content(consumer, URI, region_code_from, broadcast);
   }

   asset wallet_api::price_to_dct(asset price)
   {
      return my->price_to_dct(price);
   }

// HEAD
optional<content_download_status> wallet_api::get_download_status(string consumer,
                                                                  string URI) const
{
   return my->get_download_status(consumer, URI);
}

signed_transaction wallet_api::request_to_buy(string consumer,
                                              string URI,
                                              string price_asset_name,
                                              string price_amount,
                                              string str_region_code_from,
                                              bool broadcast)
{
   return my->request_to_buy(consumer, URI, price_asset_name, price_amount, str_region_code_from, broadcast);
}

void wallet_api::leave_rating_and_comment(string consumer,
                                                        string URI,
                                                        uint64_t rating,
                                                        string comment,
                                                        bool broadcast)
{
   return my->leave_rating_and_comment(consumer, URI, rating, comment, broadcast);
}

   void wallet_api::seeding_startup(string account_id_type_or_name,
                                    DInteger content_private_key,
                                    string seeder_private_key,
                                    uint64_t free_space,
                                    uint32_t seeding_price,
                                    string packages_path)
   {
      return my->seeding_startup(account_id_type_or_name, content_private_key, seeder_private_key, free_space, seeding_price, packages_path);
   }

   DInteger wallet_api::restore_encryption_key(string consumer, buying_id_type buying)
   {
      return my->restore_encryption_key(consumer, buying);
   }

   vector<buying_object> wallet_api::get_open_buyings()const
   {
      return my->_remote_db->get_open_buyings();
   }

   vector<buying_object> wallet_api::get_open_buyings_by_URI( const string& URI )const
   {
      return my->_remote_db->get_open_buyings_by_URI( URI );
   }

   vector<buying_object> wallet_api::get_open_buyings_by_consumer( const string& account_id_or_name )const
   {
      account_id_type consumer = get_account( account_id_or_name ).id;
      return my->_remote_db->get_open_buyings_by_consumer( consumer );
   }

   vector<buying_object> wallet_api::get_buying_history_objects_by_consumer( const string& account_id_or_name )const
   {
      account_id_type consumer = get_account( account_id_or_name ).id;
      vector<buying_object> result = my->_remote_db->get_buying_history_objects_by_consumer( consumer );

      for (int i = 0; i < result.size(); ++i)
      {
         buying_object& bobj = result[i];

         optional<content_object> content = my->_remote_db->get_content( bobj.URI );
         if (!content)
            continue;
         optional<asset> op_price = content->price.GetPrice(bobj.region_code_from);
         if (!op_price)
            continue;

         bobj.price = *op_price;

         bobj.size = content->size;
         bobj.rating = content->AVG_rating;
         bobj.synopsis = content->synopsis;

      }
      return result;
   }

   
   vector<buying_object_ex> wallet_api::search_my_purchases(const string& account_id_or_name,
                                                            const string& term,
                                                            const string& order,
                                                            const string& id,
                                                            uint32_t count)const
   {
      account_id_type consumer = get_account( account_id_or_name ).id;
      vector<buying_object> bobjects = my->_remote_db->get_buying_objects_by_consumer(consumer, order, object_id_type(id), term, count );
      vector<buying_object_ex> result;

      for (size_t i = 0; i < bobjects.size(); ++i)
      {
         buying_object const& buyobj = bobjects[i];

         optional<content_download_status> status = get_download_status(account_id_or_name, buyobj.URI);
         if (!status)
            continue;

         optional<content_object> content = my->_remote_db->get_content( buyobj.URI );
         if (!content)
            continue;

         result.emplace_back(buying_object_ex(bobjects[i], *status));
         buying_object_ex& bobj = result.back();

         bobj.author_account = my->get_account(content->author).name;
         bobj.times_bought = content->times_bought;
         bobj.hash = content->_hash;
         bobj.AVG_rating = content->AVG_rating;
         bobj.rating = content->AVG_rating;
      }

      return result;
   }

vector<rating_object_ex> wallet_api::search_feedback(const string& user,
                                                     const string& URI,
                                                     const string& id,
                                                     uint32_t count) const
{
   vector<rating_object_ex> result;
   vector<buying_object> temp = my->_remote_db->search_feedback(user, URI, object_id_type(id), count);

   for (auto const& item : temp)
      result.push_back(rating_object_ex( item, get_account(string(object_id_type(item.consumer))).name ));

   return result;
}

optional<content_object> wallet_api::get_content( const string& URI )const
{
   return my->_remote_db->get_content( URI );
}

optional<buying_object> wallet_api::get_buying_by_consumer_URI( const string& account_id_or_name, const string & URI )const
{
   account_id_type account = get_account( account_id_or_name ).id;
   return my->_remote_db->get_buying_by_consumer_URI( account, URI );
}

vector<content_summary> wallet_api::search_content(const string& term,
                                                   const string& order,
                                                   const string& user,
                                                   const string& region_code,
                                                   const string& id,
                                                   const string& type,
                                                   uint32_t count)const
{
   return my->_remote_db->search_content(term, order, user, region_code, object_id_type(id), type, count);
}

   vector<content_summary> wallet_api::search_user_content(const string& user,
                                                           const string& term,
                                                           const string& order,
                                                           const string& region_code,
                                                           const string& id,
                                                           const string& type,
                                                           uint32_t count)const
   {
      vector<content_summary> result = my->_remote_db->search_content(term, order, user, region_code, object_id_type(id), type, count);

      auto packages = PackageManager::instance().get_all_known_packages();
      for (auto package: packages)
      {
         auto state = package->get_manipulation_state();

         if (package->get_data_state() == PackageInfo::CHECKED){
            bool cont = false;
            auto hash = package->get_hash();
            for( const auto& item : result )
               if( item._hash == hash )
                  cont = true;
            if(cont)
               continue;
         }

         bool cont = false;
         for (auto listener: my->_package_manager_listeners)
         {

            if (listener->get_hash() == package->get_hash())
            {
               cont = true;
               content_summary newObject;
               newObject.synopsis = listener->op().synopsis;
               newObject.expiration = listener->op().expiration;
               newObject.author = my->get_account( listener->op().author ).name;
               
               if (state == PackageInfo::PACKING) {
                  newObject.status = "Packing";
               } else if (state == PackageInfo::ENCRYPTING) {
                  newObject.status = "Encrypting";
               } else if (state == PackageInfo::STAGING) {
                  newObject.status = "Staging";
               } else if (state == PackageInfo:: MS_IDLE )
                  newObject.status = "Submission failed";
               
               result.insert(result.begin(), newObject);
            }
         }
         if(cont)
            continue;
      }

      return result;
   }

pair<account_id_type, vector<account_id_type>> wallet_api::get_author_and_co_authors_by_URI( const string& URI )const
{
   return my->get_author_and_co_authors_by_URI( URI );
}

   vector<seeder_object> wallet_api::list_publishers_by_price( uint32_t count )const
   {
      return my->_remote_db->list_publishers_by_price( count );
   }

   optional<vector<seeder_object>> wallet_api::list_seeders_by_upload( const uint32_t count )const
   {
      return my->_remote_db->list_seeders_by_upload( count );
   }
   
   signed_transaction wallet_api::subscribe_to_author( string from,
                                                       string to,
                                                       string price_amount,
                                                       string price_asset_symbol,
                                                       bool broadcast/* = false */)
   {
      return my->subscribe_to_author(from, to, price_amount, price_asset_symbol, broadcast);
   }

   signed_transaction wallet_api::subscribe_by_author( string from,
                                                       string to,
                                                       bool broadcast/* = false */)
   {
      return my->subscribe_by_author(from, to, broadcast);
   }

   signed_transaction wallet_api::set_subscription( string account,
                                                    bool allow_subscription,
                                                    uint32_t subscription_period,
                                                    string price_amount,
                                                    string price_asset_symbol,
                                                    bool broadcast/* = false */)
   {
      return my->set_subscription(account, allow_subscription, subscription_period, price_amount, price_asset_symbol, broadcast);
   }

   signed_transaction wallet_api::set_automatic_renewal_of_subscription( string account_id_or_name,
                                                             subscription_id_type subscription_id,
                                                             bool automatic_renewal,
                                                             bool broadcast/* = false */)
   {
      return my->set_automatic_renewal_of_subscription(account_id_or_name, subscription_id, automatic_renewal, broadcast);
   }

   vector< subscription_object > wallet_api::list_active_subscriptions_by_consumer( const string& account_id_or_name, const uint32_t count) const
   {
      account_id_type account = get_account( account_id_or_name ).id;
      return my->_remote_db->list_active_subscriptions_by_consumer( account, count);
   }

   vector< subscription_object > wallet_api::list_subscriptions_by_consumer( const string& account_id_or_name, const uint32_t count) const
   {
      account_id_type account = get_account( account_id_or_name ).id;
      return my->_remote_db->list_subscriptions_by_consumer( account, count);
   }

   vector< subscription_object > wallet_api::list_active_subscriptions_by_author( const string& account_id_or_name, const uint32_t count) const
   {
      account_id_type account = get_account( account_id_or_name ).id;
      return my->_remote_db->list_active_subscriptions_by_author( account, count);
   }

   std::string wallet_api::sign_buffer(std::string const& str_buffer,
                                       std::string const& str_brainkey) const
   {
      if (str_buffer.empty() ||
          str_brainkey.empty())
         throw std::runtime_error("You need buffer and brainkey to sign");

      string normalized_brain_key = normalize_brain_key( str_brainkey );
      fc::ecc::private_key privkey = derive_private_key( normalized_brain_key, 0 );

      fc::sha256 digest(str_buffer);

      auto sign = privkey.sign_compact(digest);

      return fc::to_hex((const char*)sign.begin(), sign.size());
   }

   el_gamal_key_pair_str wallet_api::get_el_gammal_key(string const& consumer) const {
      try
      {
         FC_ASSERT( !is_locked() );
         
         account_object consumer_account = get_account( consumer );
         el_gamal_key_pair_str res;
         
         res.private_key = generate_private_el_gamal_key_from_secret ( my->get_private_key_for_account(consumer_account).get_secret() );
         res.public_key = decent::encrypt::get_public_el_gamal_key( res.private_key );
         return res;
      } FC_CAPTURE_AND_RETHROW( (consumer) )
   }
      
   bool wallet_api::verify_signature(std::string const& str_buffer,
                                     std::string const& str_publickey,
                                     std::string const& str_signature) const
   {
      if (str_buffer.empty() ||
          str_publickey.empty() ||
          str_signature.empty())
         throw std::runtime_error("You need buffer, public key and signature to verify");

      fc::ecc::compact_signature signature;
      fc::from_hex(str_signature, (char*)signature.begin(), signature.size());
      fc::sha256 digest(str_buffer);

      fc::ecc::public_key pub_key(signature, digest);
      public_key_type provided_key(str_publickey);

      if (provided_key == pub_key)
         return true;
      else
         return false;
   }

   fc::time_point_sec wallet_api::head_block_time() const
   {
      return my->head_block_time();
   }

   vector< subscription_object > wallet_api::list_subscriptions_by_author( const string& account_id_or_name, const uint32_t count) const
   {
      account_id_type account = get_account( account_id_or_name ).id;
      return my->_remote_db->list_subscriptions_by_author( account, count);
   }

   std::pair<string, decent::encrypt::CustodyData>  wallet_api::create_package(const std::string& content_dir, const std::string& samples_dir, const DInteger& aes_key) const {
      FC_ASSERT(!is_locked());
      fc::sha256 key1;
      aes_key.Encode((byte*)key1._hash, 32);

      auto pack = PackageManager::instance().get_package(content_dir, samples_dir, key1);
      decent::encrypt::CustodyData cd = pack->get_custody_data();
      return std::pair<string, decent::encrypt::CustodyData>(pack->get_hash().str(), cd);
   }


   void wallet_api::extract_package(const std::string& package_hash, const std::string& output_dir, const DInteger& aes_key) const {
      FC_ASSERT(!is_locked());
      fc::sha256 key1;
      aes_key.Encode((byte*)key1._hash, 32);

      auto pack = PackageManager::instance().find_package(fc::ripemd160(package_hash));
      if (pack == nullptr) {
          FC_THROW("Can not find package");
      }

      if (pack->get_manipulation_state() != PackageInfo::ManipulationState::MS_IDLE) {
          FC_THROW("Package is not in valid state");
      }
      pack->unpack(output_dir, key1);
   }

void graphene::wallet::detail::submit_transfer_listener::package_creation_complete() {
   uint64_t size = std::max((uint64_t)1, ( _info->get_size() + (1024 * 1024) -1 ) / (1024 * 1024));
   
   asset total_price_per_day;
   for( int i =0; i < _op.seeders.size(); i++ )
   {
      const auto& s = _wallet._remote_db->get_seeder( _op.seeders[i] );
      total_price_per_day += s->price.amount * size;
   }

   fc::microseconds duration = (_op.expiration - fc::time_point::now());
   uint64_t days = (duration.to_seconds()+3600*24-1) / 3600 / 24;

   _op.hash = _info->get_hash();
   _op.size = size;
   _op.publishing_fee = days * total_price_per_day;
   _op.cd = _info->get_custody_data();

   _info->start_seeding(_protocol, false);
}

void graphene::wallet::detail::submit_transfer_listener::package_seed_complete() {
   
   _op.URI = _info->get_url();
   
   signed_transaction tx;
   tx.operations.push_back( _op );
   _wallet.set_operation_fees( tx, _wallet._remote_db->get_global_properties().parameters.current_fees);
   tx.validate();
    _wallet.sign_transaction(tx, true);
}



   void wallet_api::remove_package(const std::string& package_hash) const {
      FC_ASSERT(!is_locked());
      PackageManager::instance().release_package(fc::ripemd160(package_hash));
   }

   void wallet_api::download_package(const std::string& url) const {
      FC_ASSERT(!is_locked());
      auto content = get_content(url);
      FC_ASSERT(content, "no such package in the system");
      auto pack = PackageManager::instance().get_package(url, content->_hash );
      pack->download(false);
   }

   std::string wallet_api::upload_package(const std::string& package_hash, const std::string& protocol) const {
      FC_ASSERT(!is_locked());
      auto package = PackageManager::instance().get_package(fc::ripemd160(package_hash));
      package->start_seeding(protocol, true);
      return package->get_url();
   }


   void wallet_api::set_transfer_logs(bool enable) const {
   // FC_ASSERT(!is_locked());
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


