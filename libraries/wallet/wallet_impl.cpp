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

#include "wallet_impl.hpp"
#include "operation_printer.hpp"
#include <graphene/utilities/key_conversion.hpp>
#include <graphene/utilities/keys_generator.hpp>
#include <decent/ipfs_check.hpp>

#include <boost/range/adaptor/map.hpp>
#include <fc/crypto/aes.hpp>
#include <fc/thread/thread.hpp>
#include <fc/thread/scoped_lock.hpp>

#ifndef WIN32
# include <sys/types.h>
# include <sys/stat.h>
#endif

namespace graphene { namespace wallet { namespace detail {

CryptoPP::AutoSeededRandomPool wallet_api_impl::randomGenerator;

template<typename StaticVariant>
struct from_which_visitor
{
   typedef StaticVariant result_type;

   template< typename Member >   // Member is member of static_variant
   result_type operator()( const Member& dummy )
   {
      Member result;
      from_variant( v, result );
      return result;    // converted from StaticVariant to Result automatically due to return type
   }

   const fc::variant& v;

   from_which_visitor( const fc::variant& _v ) : v(_v) {}
};

template< typename T >
T from_which_variant( int which, const fc::variant& v )
{
   // Parse a variant for a known which()
   T dummy;
   dummy.set_which(which);
   from_which_visitor<T> vtor(v);
   return dummy.visit(vtor);
}

struct static_variant_map_visitor : fc::visitor<void>
{
   static_variant_map_visitor(static_operation_map& self) : _self(self) {}

   template<typename T>
   result_type operator()( const T& dummy )
   {
      assert( which == static_cast<int>(_self.which_to_name.size()) );
      std::string name = clean_name( fc::get_typename<T>::name() );
      _self.name_to_which[ name ] = which;
      _self.which_to_name.push_back( name );
   }

   static std::string clean_name(const std::string& name)
   {
      std::string result;
      const static std::string prefix = "graphene::chain::";
      const static std::string suffix = "_operation";
      // graphene::chain::.*_operation
      if(    (name.size() >= prefix.size() + suffix.size())
         && (name.substr( 0, prefix.size() ) == prefix)
         && (name.substr( name.size()-suffix.size(), suffix.size() ) == suffix )
      )
         return name.substr( prefix.size(), name.size() - prefix.size() - suffix.size() );

      wlog( "don't know how to clean name: ${name}", ("name", name) );
      return name;
   }

   static_operation_map& _self;
   int which;
};

static_operation_map::static_operation_map()
{
   chain::operation dummy;
   static_variant_map_visitor vtor(*this);
   for(size_t i = 0; i < chain::operation::type_info::count; i++)
   {
      dummy.set_which(i);
      vtor.which = static_cast<int>(i);
      dummy.visit( vtor );
   }
}

// This function generates derived keys starting with index 0 and keeps incrementing
// the index until it finds a key that isn't registered in the block chain.  To be
// safer, it continues checking for a few more keys to make sure there wasn't a short gap
// caused by a failed registration or the like.
int wallet_api_impl::find_first_unused_derived_key_index(const chain::private_key_type& parent_key) const
{
   int first_unused_index = 0;
   int number_of_consecutive_unused_keys = 0;
   for (int key_index = 0; ; ++key_index)
   {
      chain::private_key_type derived_private_key = utilities::derive_private_key(utilities::key_to_wif(parent_key), key_index);
      chain::public_key_type derived_public_key = derived_private_key.get_public_key();
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

void wallet_api_impl::claim_registered_account(const chain::account_object& account)
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
void wallet_api_impl::claim_registered_miner(const std::string& miner_name)
{
   auto iter = _wallet.pending_miner_registrations.find(miner_name);
   FC_ASSERT(iter != _wallet.pending_miner_registrations.end());
   std::string wif_key = iter->second;

   // get the list key id this key is registered with in the chain
   fc::optional<chain::private_key_type> miner_private_key = utilities::wif_to_key(wif_key);
   FC_ASSERT(miner_private_key);

   auto pub_key = miner_private_key->get_public_key();
   _keys[pub_key] = wif_key;
   _wallet.pending_miner_registrations.erase(iter);
}

void wallet_api_impl::resync()
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
      std::vector<std::string> pending_account_names = boost::copy_range<std::vector<std::string>>(boost::adaptors::keys(_wallet.pending_account_registrations));

      // look those up on the blockchain
      std::vector<fc::optional<chain::account_object>> pending_account_objects = _remote_db->lookup_account_names( pending_account_names );

      // if any of them exist, claim them
      for( const fc::optional<chain::account_object>& optional_account : pending_account_objects )
         if( optional_account )
            claim_registered_account(*optional_account);
   }

   if (!_wallet.pending_miner_registrations.empty())
   {
      // make a vector of the owner accounts for miners pending registration
      std::vector<std::string> pending_miner_names = boost::copy_range<std::vector<std::string>>(boost::adaptors::keys(_wallet.pending_miner_registrations));

      // look up the owners on the blockchain
      std::vector<fc::optional<chain::account_object>> owner_account_objects = _remote_db->lookup_account_names(pending_miner_names);

      // if any of them have registered miners, claim them
      for( const fc::optional<chain::account_object>& optional_account : owner_account_objects )
         if (optional_account)
         {
            fc::optional<chain::miner_object> miner_obj = _remote_db->get_miner_by_account(optional_account->id);
            if (miner_obj)
               claim_registered_miner(optional_account->name);
         }
   }
}

wallet_api_impl::wallet_api_impl(wallet_api& s, const fc::api<app::login_api> &rapi, const chain::chain_id_type &chain_id, const server_data &ws)
   : self(s),
     _chain_id(chain_id),
     _remote_api(rapi),
     _remote_db(rapi->database()),
     _remote_net_broadcast(rapi->network_broadcast()),
     _remote_hist(rapi->history())
{
   chain::chain_id_type remote_chain_id = _remote_db->get_chain_id();
   if( remote_chain_id != _chain_id )
   {
      FC_THROW( "Remote server gave us an unexpected chain_id",
         ("remote_chain_id", remote_chain_id)
         ("chain_id", _chain_id) );
   }

   chain::operation op;
   for( std::size_t t = 0; t < chain::operation::type_info::count; t++ )
   {
      op.set_which( t );
      op.visit( op_prototype_visitor(_prototype_ops) );
   }

   _remote_db->set_block_applied_callback( [this](const fc::variant& block_id) { on_block_applied( block_id ); } );

   _wallet.chain_id = _chain_id;
   _wallet.ws_server = ws.server;
   _wallet.ws_user = ws.user;
   _wallet.ws_password = ws.password;
   decent::package::PackageManager::instance().recover_all_packages();
}

void wallet_api_impl::encrypt_keys()
{
   plain_ec_and_el_gamal_keys data;
   data.ec_keys = _keys;
   std::transform( _el_gamal_keys.begin(), _el_gamal_keys.end(), std::back_inserter( data.el_gamal_keys ),
      [](const std::pair<decent::encrypt::DInteger, decent::encrypt::DInteger>& el_gamal_pair) {
         return el_gamal_key_pair_str {el_gamal_pair.second, el_gamal_pair.first}; });
   data.checksum = _checksum;
   auto plain_txt = fc::raw::pack(data);
   _wallet.cipher_keys = fc::aes_encrypt( data.checksum, plain_txt );
}

void wallet_api_impl::encrypt_keys2()
{
   plain_ec_and_el_gamal_keys data;
   data.ec_keys = _keys;
   std::transform( _el_gamal_keys.begin(), _el_gamal_keys.end(), std::back_inserter( data.el_gamal_keys ),
                     [](const std::pair<decent::encrypt::DInteger, decent::encrypt::DInteger>& el_gamal_pair) {
                        return el_gamal_key_pair_str {el_gamal_pair.second, el_gamal_pair.first}; });
   data.checksum = _checksum;
   auto data_string = fc::json::to_string(data);
   std::vector<char> plain_txt;
   plain_txt.resize(data_string.length());
   memcpy(plain_txt.data(), data_string.data(), data_string.length());
   _wallet.cipher_keys = fc::aes_encrypt( data.checksum, plain_txt );
}

void wallet_api_impl::on_block_applied(const fc::variant& block_id)
{
   fc::async([this]{ resync(); }, "Resync after block");
}

void wallet_api_impl::set_operation_fees(chain::signed_transaction& tx, const chain::fee_schedule& s) const
{
   for( auto& op : tx.operations )
      s.set_fee(op, _remote_db->head_block_time());
}

fc::optional<chain::account_object> wallet_api_impl::find_account(chain::account_id_type account_id) const
{
   return _remote_db->get_accounts({account_id}).front();
}

fc::optional<chain::account_object> wallet_api_impl::find_account(const std::string& account_name_or_id) const
{
   FC_VERIFY_AND_THROW(!account_name_or_id.empty(), account_name_or_id_cannot_be_empty_exception);

   if( auto id = maybe_id<chain::account_id_type>(account_name_or_id) )
   {
      // It's an ID
      return find_account(*id);
   } else {
      auto rec = _remote_db->lookup_account_names({ account_name_or_id }).front();
      if(_wallet.my_accounts.get<chain::by_name>().count(account_name_or_id))
      {
         auto local_account = *_wallet.my_accounts.get<chain::by_name>().find(account_name_or_id);
         FC_VERIFY_AND_THROW(rec.valid(), account_in_wallet_not_on_blockchain_exception, "Account: ${acc}", ("acc", account_name_or_id));
         if(local_account.id != rec->id)
            elog("my account id ${id} different from blockchain id ${id2}", ("id", local_account.id)("id2", rec->id));
         if(local_account.name != rec->name)
            elog("my account name ${id} different from blockchain name ${id2}", ("id", local_account.name)("id2", rec->name));

         //return *_wallet.my_accounts.get<by_name>().find(account_name_or_id);
         return rec;
      }
      if(rec && rec->name != account_name_or_id)
         return {};
      return rec;
   }
}

chain::account_object wallet_api_impl::get_account(chain::account_id_type account_id) const
{
   auto rec = find_account(account_id);
   FC_VERIFY_AND_THROW(rec.valid(), account_does_not_exist_exception, "Account: ${acc}", ("acc", account_id));
   return *rec;
}

chain::account_object wallet_api_impl::get_account(const std::string& account_name_or_id) const
{
   auto rec = find_account(account_name_or_id);
   FC_VERIFY_AND_THROW(rec.valid(), account_does_not_exist_exception, "Account: ${acc}", ("acc", account_name_or_id));
   return *rec;
}

fc::optional<chain::asset_object> wallet_api_impl::find_asset(chain::asset_id_type asset_id) const
{
   return _remote_db->get_assets({asset_id}).front();
}

fc::optional<chain::asset_object> wallet_api_impl::find_asset(const std::string& asset_symbol_or_id) const
{
   FC_ASSERT( asset_symbol_or_id.size() > 0 );

   if( auto id = maybe_id<chain::asset_id_type>(asset_symbol_or_id) )
   {
      // It's an ID
      return find_asset(*id);
   } else {
      // It's a symbol
      auto rec = _remote_db->lookup_asset_symbols({asset_symbol_or_id}).front();
      if( rec )
      {
         if( rec->symbol != asset_symbol_or_id )
            return {};
      }
      return rec;
   }
}

chain::asset_object wallet_api_impl::get_asset(chain::asset_id_type asset_id) const
{
   auto opt = find_asset(asset_id);
   FC_VERIFY_AND_THROW(opt.valid(), asset_does_not_exist_exception, "Asset ${asset} does not exist", ("asset", asset_id));
   return *opt;
}

chain::asset_object wallet_api_impl::get_asset(const std::string& asset_symbol_or_id) const
{
   auto opt = find_asset(asset_symbol_or_id);
   FC_VERIFY_AND_THROW(opt.valid(), asset_does_not_exist_exception, "Asset ${asset} does not exist", ("asset", asset_symbol_or_id));
   return *opt;
}

fc::optional<chain::non_fungible_token_object> wallet_api_impl::find_non_fungible_token(chain::non_fungible_token_id_type nft_id) const
{
   return _remote_db->get_non_fungible_tokens({nft_id}).front();
}

fc::optional<chain::non_fungible_token_object> wallet_api_impl::find_non_fungible_token(const std::string& nft_symbol_or_id) const
{
   FC_ASSERT( nft_symbol_or_id.size() > 0 );
   if( auto id = maybe_id<chain::non_fungible_token_id_type>(nft_symbol_or_id) )
   {
      // It's an ID
      return find_non_fungible_token(*id);
   } else {
      // It's a symbol
      auto rec = _remote_db->get_non_fungible_tokens_by_symbols({nft_symbol_or_id}).front();
      if( rec )
      {
         if( rec->symbol != nft_symbol_or_id )
            return {};
      }
      return rec;
   }
}

chain::non_fungible_token_object wallet_api_impl::get_non_fungible_token(chain::non_fungible_token_id_type nft_id) const
{
   auto opt = find_non_fungible_token(nft_id);
   FC_ASSERT(opt, "Non fungible token ${nft} does not exist", ("nft", nft_id));
   return *opt;
}

chain::non_fungible_token_object wallet_api_impl::get_non_fungible_token(const std::string& nft_symbol_or_id) const
{
   auto opt = find_non_fungible_token(nft_symbol_or_id);
   FC_ASSERT(opt, "Non fungible token ${nft} does not exist", ("nft", nft_symbol_or_id));
   return *opt;
}

fc::optional<chain::non_fungible_token_data_object> wallet_api_impl::find_non_fungible_token_data(chain::non_fungible_token_data_id_type nft_data_id) const
{
   return _remote_db->get_non_fungible_token_data({nft_data_id}).front();
}

chain::non_fungible_token_data_object wallet_api_impl::get_non_fungible_token_data(chain::non_fungible_token_data_id_type nft_data_id) const
{
   auto opt = find_non_fungible_token_data(nft_data_id);
   FC_ASSERT(opt, "Non fungible token data ${nft} does not exist", ("nft", nft_data_id));
   return *opt;
}

void wallet_api_impl::set_wallet_filename(const boost::filesystem::path &wallet_filename)
{
   FC_VERIFY_AND_THROW(!wallet_filename.empty(), wallet_filename_cannot_be_empty_exception);
   _wallet_filename = wallet_filename;
}

chain::private_key_type wallet_api_impl::get_private_key(const chain::public_key_type& id) const
{
   auto it = _keys.find(id);
   FC_ASSERT( it != _keys.end() );

   fc::optional<chain::private_key_type> privkey = utilities::wif_to_key( it->second );
   FC_ASSERT( privkey );
   return *privkey;
}

chain::private_key_type wallet_api_impl::get_private_key_for_account(const chain::account_object& account) const
{
   std::vector<chain::public_key_type> active_keys = account.active.get_keys();
   if (active_keys.size() != 1)
      FC_THROW("Expecting a simple authority with one active key");
   return get_private_key(active_keys.front());
}

// imports the private key into the wallet, and associate it in some way (?) with the
// given account name.
// @returns true if the key matches a current active/owner/memo key for the named
//          account, false otherwise (but it is stored either way)
bool wallet_api_impl::import_key(const std::string& account_name_or_id, const std::string& wif_key)
{
   FC_ASSERT( !is_locked() );
   fc::optional<chain::private_key_type> optional_private_key = utilities::wif_to_key(wif_key);
   if (!optional_private_key)
      FC_THROW("Invalid private key");

   chain::public_key_type wif_pub_key = optional_private_key->get_public_key();
   chain::account_object account = get_account( account_name_or_id );

   // make a list of all current public keys for the named account
   boost::container::flat_set<chain::public_key_type> all_keys_for_account;
   std::vector<chain::public_key_type> active_keys = account.active.get_keys();
   std::vector<chain::public_key_type> owner_keys = account.owner.get_keys();

   if( std::find( owner_keys.begin(), owner_keys.end(), wif_pub_key ) != owner_keys.end() )
   {
      //we have the owner keys
      int active_key_index = find_first_unused_derived_key_index( *optional_private_key );
      chain::private_key_type active_privkey = utilities::derive_private_key( wif_key, active_key_index);

      int memo_key_index = find_first_unused_derived_key_index(active_privkey);
      chain::private_key_type memo_privkey = utilities::derive_private_key( utilities::key_to_wif(active_privkey), memo_key_index);

      chain::public_key_type active_pubkey = active_privkey.get_public_key();
      chain::public_key_type memo_pubkey = memo_privkey.get_public_key();
      _keys[active_pubkey] = utilities::key_to_wif( active_privkey );
      _keys[memo_pubkey] = utilities::key_to_wif( memo_privkey );

      decent::encrypt::DInteger active_el_gamal_priv_key = decent::encrypt::generate_private_el_gamal_key_from_secret( active_privkey.get_secret() );
      _el_gamal_keys[get_public_el_gamal_key( active_el_gamal_priv_key )] = active_el_gamal_priv_key;
      decent::encrypt::DInteger memo_el_gamal_priv_key = decent::encrypt::generate_private_el_gamal_key_from_secret( memo_privkey.get_secret() );
      _el_gamal_keys[get_public_el_gamal_key( memo_el_gamal_priv_key )] = memo_el_gamal_priv_key;

      _wallet.extra_keys[account.id].insert( active_pubkey );
      _wallet.extra_keys[account.id].insert( memo_pubkey );
   }

   std::copy(active_keys.begin(), active_keys.end(), std::inserter(all_keys_for_account, all_keys_for_account.end()));
   std::copy(owner_keys.begin(), owner_keys.end(), std::inserter(all_keys_for_account, all_keys_for_account.end()));
   all_keys_for_account.insert(account.options.memo_key);

   _keys[wif_pub_key] = wif_key;
   decent::encrypt::DInteger el_gamal_priv_key = decent::encrypt::generate_private_el_gamal_key_from_secret( optional_private_key->get_secret() );
   _el_gamal_keys[get_public_el_gamal_key( el_gamal_priv_key )] = el_gamal_priv_key;
   _wallet.update_account(account);
   _wallet.extra_keys[account.id].insert(wif_pub_key);

   return all_keys_for_account.find(wif_pub_key) != all_keys_for_account.end();
}

// @returns true if the key matches a current active/owner/memo key for the named
//          account, false otherwise (but it is stored either way)
bool wallet_api_impl::import_single_key(const std::string& account_name_or_id, const std::string& wif_key)
{
   FC_ASSERT( !is_locked() );
   fc::optional<chain::private_key_type> optional_private_key = utilities::wif_to_key(wif_key);
   if (!optional_private_key)
      FC_THROW("Invalid private key");

   chain::public_key_type wif_pub_key = optional_private_key->get_public_key();
   chain::account_object account = get_account( account_name_or_id );

   // make a list of all current public keys for the named account
   boost::container::flat_set<chain::public_key_type> all_keys_for_account;
   std::vector<chain::public_key_type> active_keys = account.active.get_keys();
   std::vector<chain::public_key_type> owner_keys = account.owner.get_keys();
   std::copy(active_keys.begin(), active_keys.end(), std::inserter(all_keys_for_account, all_keys_for_account.end()));
   std::copy(owner_keys.begin(), owner_keys.end(), std::inserter(all_keys_for_account, all_keys_for_account.end()));
   all_keys_for_account.insert(account.options.memo_key);

   _keys[wif_pub_key] = wif_key;
   decent::encrypt::DInteger el_gamal_priv_key = decent::encrypt::generate_private_el_gamal_key_from_secret( optional_private_key->get_secret() );
   _el_gamal_keys[get_public_el_gamal_key( el_gamal_priv_key )] = el_gamal_priv_key;
   _wallet.update_account(account);
   _wallet.extra_keys[account.id].insert(wif_pub_key);

   return all_keys_for_account.find(wif_pub_key) != all_keys_for_account.end();
}

int wallet_api_impl::get_wallet_file_version(const fc::variant& data)
{
   fc::variant_object vo;
   fc::from_variant( data, vo );
   if (vo.find("version") == vo.end()) {
      return 0;
   }

   return vo["version"].as<int>();
}

bool wallet_api_impl::load_old_wallet_file(const fc::variant& data, wallet_data& result)
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

bool wallet_api_impl::load_new_wallet_file(const fc::variant& data, wallet_data& result)
{
   bool ret;
   try {
      result = data.as<wallet_data>();

      const fc::variant_object& vo = data.get_object();
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

bool wallet_api_impl::load_wallet_file(boost::filesystem::path wallet_filename)
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
   std::vector<chain::account_id_type> account_ids_to_send;
   size_t n = _wallet.my_accounts.size();
   account_ids_to_send.reserve( std::min( account_pagination, n ) );
   auto it = _wallet.my_accounts.begin();

   for( size_t start=0; start<n; start+=account_pagination )
   {
      size_t end = std::min( start+account_pagination, n );
      assert( end > start );
      account_ids_to_send.clear();
      std::vector<chain::account_object> old_accounts;
      for( size_t i=start; i<end; i++ )
      {
         assert( it != _wallet.my_accounts.end() );
         old_accounts.push_back( *it );
         account_ids_to_send.push_back( old_accounts.back().id );
         ++it;
      }
      std::vector<fc::optional<chain::account_object>> accounts = _remote_db->get_accounts(account_ids_to_send);
      // server response should be same length as request
      FC_ASSERT( accounts.size() == account_ids_to_send.size() );
      size_t i = 0;
      for( const fc::optional<chain::account_object>& acct : accounts )
      {
         chain::account_object& old_acct = old_accounts[i];
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

void wallet_api_impl::save_wallet_file(boost::filesystem::path wallet_filename)
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

   ilog( "saving wallet to file ${fn}", ("fn", wallet_filename) );

   auto save_wallet_data = [](const wallet_data& data)
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
      return fc::json::to_pretty_string( mvo );
   };

   try
   {
      std::string data = save_wallet_data( _wallet );
#ifndef WIN32
      struct umask_guard {
         mode_t _old_umask;

         umask_guard() : _old_umask( umask( S_IRWXG | S_IRWXO ) ) {}
         ~umask_guard() { umask( _old_umask ); }
      } _umask_guard;
#endif

      boost::filesystem::ofstream outfile;
      outfile.exceptions( std::ios_base::failbit );
      outfile.open( wallet_filename );
      outfile.write( data.c_str(), data.length() );
      outfile.flush();
      outfile.close();
   }
   FC_CAPTURE_LOG_AND_RETHROW( ("Error save wallet file")(wallet_filename) )

   dlog("save_wallet_file() end");
}

transaction_handle_type wallet_api_impl::begin_builder_transaction()
{
   transaction_handle_type trx_handle = _builder_transactions.empty() ? 0 : (--_builder_transactions.end())->first + 1;
   _builder_transactions[trx_handle];
   return trx_handle;
}

void wallet_api_impl::add_operation_to_builder_transaction(transaction_handle_type transaction_handle, const chain::operation& op)
{
   FC_VERIFY_AND_THROW(_builder_transactions.count(transaction_handle), invalid_transaction_handle_exception);
   _builder_transactions[transaction_handle].operations.emplace_back(op);
}

void wallet_api_impl::replace_operation_in_builder_transaction(transaction_handle_type handle, uint32_t operation_index, const chain::operation& new_op)
{
   FC_VERIFY_AND_THROW(_builder_transactions.count(handle), invalid_transaction_handle_exception);

   chain::signed_transaction& trx = _builder_transactions[handle];
   FC_ASSERT( operation_index < trx.operations.size());
   trx.operations[operation_index] = new_op;
}

chain::asset wallet_api_impl::set_fees_on_builder_transaction(transaction_handle_type handle, const std::string& fee_asset)
{
   FC_VERIFY_AND_THROW(_builder_transactions.count(handle), invalid_transaction_handle_exception);
   FC_VERIFY_AND_THROW(fee_asset == GRAPHENE_SYMBOL, fees_can_be_paid_in_core_asset_exception);

   auto fee_asset_obj = get_asset(fee_asset);
   chain::asset total_fee = fee_asset_obj.amount(0);

   auto gprops = _remote_db->get_global_properties().parameters;
   for( auto& op : _builder_transactions[handle].operations )
      total_fee += gprops.current_fees->set_fee( op, _remote_db->head_block_time() );

   return total_fee;
}

chain::transaction wallet_api_impl::preview_builder_transaction(transaction_handle_type handle)
{
   FC_VERIFY_AND_THROW(_builder_transactions.count(handle), invalid_transaction_handle_exception);
   return _builder_transactions[handle];
}

chain::signed_transaction wallet_api_impl::sign_builder_transaction(transaction_handle_type transaction_handle, bool broadcast)
{
   FC_VERIFY_AND_THROW(_builder_transactions.count(transaction_handle), invalid_transaction_handle_exception);
   return _builder_transactions[transaction_handle] = sign_transaction(_builder_transactions[transaction_handle], broadcast);
}

chain::signed_transaction wallet_api_impl::propose_builder_transaction(transaction_handle_type handle, fc::time_point_sec expiration, uint32_t review_period_seconds, bool broadcast)
{
   FC_VERIFY_AND_THROW(_builder_transactions.count(handle), invalid_transaction_handle_exception);

   chain::proposal_create_operation op;
   op.expiration_time = expiration;
   chain::signed_transaction& trx = _builder_transactions[handle];
   std::transform(trx.operations.begin(), trx.operations.end(), std::back_inserter(op.proposed_ops),
                  [](const chain::operation& op) -> chain::op_wrapper { return op; });
   if( review_period_seconds )
      op.review_period_seconds = review_period_seconds;
   trx.operations = {op};
   _remote_db->get_global_properties().parameters.current_fees->set_fee( trx.operations.front(), _remote_db->head_block_time() );

   return trx = sign_transaction(trx, broadcast);
}

chain::signed_transaction wallet_api_impl::propose_builder_transaction2(transaction_handle_type handle, const std::string& account_name_or_id,
                                                                        fc::time_point_sec expiration, uint32_t review_period_seconds, bool broadcast)
{
   FC_VERIFY_AND_THROW(_builder_transactions.count(handle), invalid_transaction_handle_exception);

   chain::proposal_create_operation op;
   op.fee_paying_account = get_account(account_name_or_id).get_id();
   op.expiration_time = expiration;
   chain::signed_transaction& trx = _builder_transactions[handle];
   std::transform(trx.operations.begin(), trx.operations.end(), std::back_inserter(op.proposed_ops),
                  [](const chain::operation& op) -> chain::op_wrapper { return op; });
   if( review_period_seconds )
      op.review_period_seconds = review_period_seconds;
   trx.operations = {op};
   _remote_db->get_global_properties().parameters.current_fees->set_fee( trx.operations.front(), _remote_db->head_block_time() );

   return trx = sign_transaction(trx, broadcast);
}

void wallet_api_impl::remove_builder_transaction(transaction_handle_type handle)
{
   _builder_transactions.erase(handle);
}

chain::signed_transaction wallet_api_impl::register_account(const std::string& name, const chain::public_key_type& owner, const chain::public_key_type& active,
                                                            const chain::public_key_type& memo, const std::string& registrar_account, bool broadcast)
{ try {
   FC_VERIFY_AND_THROW(!find_account(name).valid(), account_already_exist_exception);

   chain::account_create_operation account_create_op;

   // TODO:  process when pay_from_account is ID

   chain::account_object registrar_account_object = get_account( registrar_account );
   chain::account_id_type registrar_account_id = registrar_account_object.id;

   account_create_op.registrar = registrar_account_id;
   account_create_op.name = name;
   account_create_op.owner = chain::authority(1, owner, chain::weight_type(1));
   account_create_op.active = chain::authority(1, active, chain::weight_type(1));
   account_create_op.options.memo_key = memo;

   chain::signed_transaction tx;

   tx.operations.push_back( account_create_op );

   auto current_fees = _remote_db->get_global_properties().parameters.current_fees;
   set_operation_fees( tx, current_fees );

   std::vector<chain::public_key_type> paying_keys = registrar_account_object.active.get_keys();

   auto dyn_props = _remote_db->get_dynamic_global_properties();
   tx.set_reference_block( dyn_props.head_block_id );
   tx.set_expiration( dyn_props.time + fc::seconds(30) );
   tx.validate();

   for( chain::public_key_type& key : paying_keys )
   {
      auto it = _keys.find(key);
      if( it != _keys.end() )
      {
         fc::optional<chain::private_key_type> privkey = utilities::wif_to_key( it->second );
         FC_VERIFY_AND_THROW(privkey.valid(), malformed_private_key_exception);
         tx.sign( *privkey, _chain_id );
      }
   }

   if( broadcast )
      _remote_net_broadcast->broadcast_transaction( tx );
   return tx;
} FC_CAPTURE_AND_RETHROW( (name)(owner)(active)(memo)(registrar_account)(broadcast) ) }

chain::signed_transaction wallet_api_impl::register_multisig_account(const std::string& name, const chain::authority& owner_authority, const chain::authority& active_authority,
                                                                     const chain::public_key_type& memo_pubkey, const std::string& registrar_account, bool broadcast)
{ try {
   FC_VERIFY_AND_THROW(!find_account(name).valid(), account_already_exist_exception);

   chain::account_create_operation account_create_op;
   chain::account_object acc = get_account( registrar_account );

   account_create_op.registrar = acc.id;
   account_create_op.name = name;
   account_create_op.owner = owner_authority;
   account_create_op.active = active_authority;
   account_create_op.options.memo_key = memo_pubkey;

   chain::signed_transaction tx;
   tx.operations.push_back( account_create_op );
   set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees );
   tx.validate();

   return sign_transaction( tx, broadcast );
} FC_CAPTURE_AND_RETHROW( (name)(owner_authority)(active_authority)(memo_pubkey)(registrar_account)(broadcast) ) }

chain::signed_transaction wallet_api_impl::create_account_with_private_key(const chain::private_key_type& owner_privkey, const std::string& account_name, const std::string& registrar_account,
                                                                           bool import, bool broadcast, bool save_wallet)
{ try {
   FC_VERIFY_AND_THROW(!find_account(account_name).valid(), account_already_exist_exception);

   int active_key_index = find_first_unused_derived_key_index(owner_privkey);
   chain::private_key_type active_privkey = utilities::derive_private_key( utilities::key_to_wif(owner_privkey), active_key_index);

   int memo_key_index = find_first_unused_derived_key_index(active_privkey);
   chain::private_key_type memo_privkey = utilities::derive_private_key( utilities::key_to_wif(active_privkey), memo_key_index);

   chain::public_key_type owner_pubkey = owner_privkey.get_public_key();
   chain::public_key_type active_pubkey = active_privkey.get_public_key();
   chain::public_key_type memo_pubkey = memo_privkey.get_public_key();

   chain::account_create_operation account_create_op;

   // TODO:  process when pay_from_account is ID

   chain::account_object registrar_account_object = get_account( registrar_account );
   chain::account_id_type registrar_account_id = registrar_account_object.id;

   account_create_op.registrar = registrar_account_id;
   account_create_op.name = account_name;
   account_create_op.owner = chain::authority(1, owner_pubkey, chain::weight_type(1));
   account_create_op.active = chain::authority(1, active_pubkey, chain::weight_type(1));
   account_create_op.options.memo_key = memo_pubkey;

   // current_fee_schedule()
   // find_account(pay_from_account)

   // account_create_op.fee = account_create_op.calculate_fee(db.current_fee_schedule());

   chain::signed_transaction tx;
   tx.operations.push_back( account_create_op );
   set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);

   std::vector<chain::public_key_type> paying_keys = registrar_account_object.active.get_keys();

   auto dyn_props = _remote_db->get_dynamic_global_properties();
   tx.set_reference_block( dyn_props.head_block_id );
   tx.set_expiration( dyn_props.time + fc::seconds(30) );
   tx.validate();

   for( chain::public_key_type& key : paying_keys )
   {
      auto it = _keys.find(key);
      if( it != _keys.end() )
      {
         fc::optional<chain::private_key_type> privkey = utilities::wif_to_key( it->second );
         FC_ASSERT( privkey.valid(), "Malformed private key in _keys" );
         tx.sign( *privkey, _chain_id );
      }
   }

   // we do not insert owner_privkey here because
   //    it is intended to only be used for key recovery
   if (import)
   {
      _wallet.pending_account_registrations[account_name].push_back(utilities::key_to_wif( active_privkey ));
      _wallet.pending_account_registrations[account_name].push_back(utilities::key_to_wif( memo_privkey ));
   }
   if( save_wallet )
      save_wallet_file();
   if( broadcast )
      _remote_net_broadcast->broadcast_transaction( tx );
   return tx;
} FC_CAPTURE_AND_RETHROW( (account_name)(registrar_account)(broadcast) ) }

chain::signed_transaction wallet_api_impl::create_account_with_brain_key(const std::string& brain_key, const std::string& account_name, const std::string& registrar_account,
                                                                         bool import, bool broadcast, bool save_wallet)
{ try {
   std::string normalized_brain_key = utilities::normalize_brain_key( brain_key );
   // TODO:  scan blockchain for accounts that exist with same brain key
   chain::private_key_type owner_privkey = utilities::derive_private_key( normalized_brain_key );
   return create_account_with_private_key(owner_privkey, account_name, registrar_account, import, broadcast, save_wallet);
} FC_CAPTURE_AND_RETHROW( (account_name)(registrar_account) ) }

chain::signed_transaction wallet_api_impl::update_account_keys(const std::string& name, fc::optional<chain::authority> owner_auth, fc::optional<chain::authority> active_auth,
                                                               fc::optional<chain::public_key_type> memo_pubkey, bool broadcast)
{ try {
   FC_ASSERT( owner_auth || active_auth || memo_pubkey, "at least one authority/public key needs to be specified");

   chain::account_object acc = get_account( name );
   chain::account_update_operation account_update_op;

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

   chain::signed_transaction tx;
   tx.operations.push_back( account_update_op );
   set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
   tx.validate();

   return sign_transaction( tx, broadcast );
} FC_CAPTURE_AND_RETHROW( (name)(owner_auth)(active_auth)(memo_pubkey)(broadcast) ) }

chain::signed_transaction wallet_api_impl::create_user_issued_asset(const std::string& issuer, const std::string& symbol, uint8_t precision, const std::string& description,
                                                                    uint64_t max_supply, chain::price core_exchange_rate, bool is_exchangeable, bool is_fixed_max_supply, bool broadcast)
{ try {
   chain::account_object issuer_account = get_account( issuer );
   FC_VERIFY_AND_THROW(!find_asset(symbol).valid(), asset_already_exists_exception);

   chain::asset_create_operation create_op;
   create_op.issuer = issuer_account.id;
   create_op.symbol = symbol;
   create_op.precision = precision;
   create_op.description = description;
   chain::asset_options opts;
   opts.max_supply = max_supply;
   opts.core_exchange_rate = core_exchange_rate;
   opts.is_exchangeable = is_exchangeable;
   opts.extensions.insert(chain::asset_options::fixed_max_supply_struct(is_fixed_max_supply));

   create_op.options = opts;
   create_op.monitored_asset_opts = fc::optional<chain::monitored_asset_options>();

   chain::signed_transaction tx;
   tx.operations.push_back( create_op );
   set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
   tx.validate();

   return sign_transaction( tx, broadcast );
} FC_CAPTURE_AND_RETHROW( (issuer)(symbol)(precision)(description)(max_supply)(is_exchangeable)(broadcast) ) }

chain::signed_transaction wallet_api_impl::issue_asset(const std::string& to_account, const std::string& amount, const std::string& symbol, const std::string& memo, bool broadcast)
{ try {
   auto asset_obj = get_asset(symbol);

   chain::account_object to = get_account(to_account);
   chain::account_object issuer = get_account(asset_obj.issuer);

   chain::asset_issue_operation issue_op;
   issue_op.issuer           = asset_obj.issuer;
   issue_op.asset_to_issue   = asset_obj.amount_from_string(amount);
   issue_op.issue_to_account = to.id;

   if( !memo.empty() )
   {
      issue_op.memo = chain::memo_data(memo, get_private_key(issuer.options.memo_key), to.options.memo_key);
   }

   chain::signed_transaction tx;
   tx.operations.push_back(issue_op);
   set_operation_fees(tx,_remote_db->get_global_properties().parameters.current_fees);
   tx.validate();

   return sign_transaction(tx, broadcast);
} FC_CAPTURE_AND_RETHROW( (to_account)(amount)(symbol)(memo)(broadcast) ) }

chain::signed_transaction wallet_api_impl::update_user_issued_asset(const std::string& symbol, const std::string& new_issuer, const std::string& description,
                                                                    uint64_t max_supply, chain::price core_exchange_rate, bool is_exchangeable, bool broadcast)
{ try {
   chain::asset_object asset_to_update = get_asset(symbol);

   chain::update_user_issued_asset_operation update_op;
   update_op.issuer = asset_to_update.issuer;
   update_op.asset_to_update = asset_to_update.id;
   update_op.new_description = description;
   update_op.max_supply = max_supply;
   update_op.core_exchange_rate = core_exchange_rate;
   update_op.is_exchangeable = is_exchangeable;

   fc::optional<chain::account_id_type> new_issuer_account_id;
   if( !new_issuer.empty() )
   {
      chain::account_object new_issuer_account = get_account(new_issuer);
      new_issuer_account_id = new_issuer_account.id;
   }
   update_op.new_issuer = new_issuer_account_id;

   chain::signed_transaction tx;
   tx.operations.push_back( update_op );
   set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
   tx.validate();

   return sign_transaction( tx, broadcast );
} FC_CAPTURE_AND_RETHROW( (symbol)(new_issuer)(description)(max_supply)(core_exchange_rate)(is_exchangeable)(broadcast) ) }

chain::signed_transaction wallet_api_impl::fund_asset_pools(const std::string& from, const std::string& uia_amount, const std::string& uia_symbol,
                                                            const std::string& DCT_amount, const std::string& DCT_symbol, bool broadcast)
{ try {
   chain::account_object from_account = get_account(from);
   chain::asset_object uia_asset_to_fund = get_asset(uia_symbol);
   chain::asset_object dct_asset_to_fund = get_asset(DCT_symbol);

   chain::asset_fund_pools_operation fund_op;
   fund_op.from_account = from_account.id;
   fund_op.uia_asset = uia_asset_to_fund.amount_from_string(uia_amount);
   fund_op.dct_asset = dct_asset_to_fund.amount_from_string(DCT_amount);

   chain::signed_transaction tx;
   tx.operations.push_back( fund_op );
   set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
   tx.validate();

   return sign_transaction( tx, broadcast );
} FC_CAPTURE_AND_RETHROW( (from)(uia_amount)(uia_symbol)(DCT_amount)(DCT_symbol)(broadcast) ) }

chain::signed_transaction wallet_api_impl::reserve_asset(const std::string& from, const std::string& amount, const std::string& symbol, bool broadcast)
{ try {
   chain::account_object from_account = get_account(from);
   chain::asset_object asset_to_reserve = get_asset(symbol);

   chain::asset_reserve_operation reserve_op;
   reserve_op.payer = from_account.id;
   reserve_op.amount_to_reserve = asset_to_reserve.amount_from_string(amount);

   chain::signed_transaction tx;
   tx.operations.push_back( reserve_op );
   set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
   tx.validate();

   return sign_transaction( tx, broadcast );
} FC_CAPTURE_AND_RETHROW( (from)(amount)(symbol)(broadcast) ) }

chain::signed_transaction wallet_api_impl::claim_fees(const std::string& uia_amount, const std::string& uia_symbol, const std::string& dct_amount, const std::string& dct_symbol, bool broadcast)
{ try {
   chain::asset_object uia_asset_to_claim = get_asset(uia_symbol);
   chain::asset_object dct_asset_to_claim = get_asset(dct_symbol);

   FC_ASSERT( dct_asset_to_claim.id == chain::asset_id_type() );

   chain::asset_claim_fees_operation claim_fees_op;
   claim_fees_op.issuer = uia_asset_to_claim.issuer;
   claim_fees_op.uia_asset = uia_asset_to_claim.amount_from_string(uia_amount);
   claim_fees_op.dct_asset = dct_asset_to_claim.amount_from_string(dct_amount);

   chain::signed_transaction tx;
   tx.operations.push_back( claim_fees_op );
   set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
   tx.validate();

   return sign_transaction( tx, broadcast );
} FC_CAPTURE_AND_RETHROW( (uia_amount)(uia_symbol)(dct_amount)(dct_symbol)(broadcast) ) }

chain::signed_transaction wallet_api_impl::create_monitored_asset(const std::string& issuer, const std::string& symbol, uint8_t precision, const std::string& description,
                                                                  uint32_t feed_lifetime_sec, uint8_t minimum_feeds, bool broadcast)
{ try {
   chain::account_object issuer_account = get_account( issuer );
   FC_VERIFY_AND_THROW(!find_asset(symbol).valid(), asset_already_exists_exception);

   chain::asset_create_operation create_op;
   create_op.issuer = issuer_account.id;
   create_op.symbol = symbol;
   create_op.precision = precision;
   create_op.description = description;

   chain::asset_options opts;
   opts.max_supply = 0;
   create_op.options = opts;

   chain::monitored_asset_options m_opts;
   m_opts.feed_lifetime_sec = feed_lifetime_sec;
   m_opts.minimum_feeds = minimum_feeds;
   create_op.monitored_asset_opts = m_opts;

   chain::signed_transaction tx;
   tx.operations.push_back( create_op );
   set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
   tx.validate();

   return sign_transaction( tx, broadcast );
} FC_CAPTURE_AND_RETHROW( (issuer)(symbol)(precision)(description)(feed_lifetime_sec)(minimum_feeds)(broadcast) ) }

chain::signed_transaction wallet_api_impl::update_monitored_asset(const std::string& symbol, const std::string& description, uint32_t feed_lifetime_sec, uint8_t minimum_feeds, bool broadcast)
{ try {
   chain::asset_object asset_to_update = get_asset(symbol);

   chain::update_monitored_asset_operation update_op;
   update_op.issuer = asset_to_update.issuer;
   update_op.asset_to_update = asset_to_update.id;
   update_op.new_description = description;
   update_op.new_feed_lifetime_sec = feed_lifetime_sec;
   update_op.new_minimum_feeds = minimum_feeds;

   chain::signed_transaction tx;
   tx.operations.push_back( update_op );
   set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
   tx.validate();

   return sign_transaction( tx, broadcast );
} FC_CAPTURE_AND_RETHROW( (symbol)(description)(feed_lifetime_sec)(minimum_feeds)(broadcast) ) }

chain::signed_transaction wallet_api_impl::publish_asset_feed(const std::string& publishing_account, const std::string& symbol, const chain::price_feed& feed, bool broadcast)
{ try {
   chain::asset_object asset_to_update = get_asset(symbol);

   chain::asset_publish_feed_operation publish_op;
   publish_op.publisher = get_account(publishing_account).get_id();
   publish_op.asset_id = asset_to_update.id;
   publish_op.feed = feed;

   chain::signed_transaction tx;
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

chain::signed_transaction wallet_api_impl::create_non_fungible_token(const std::string& issuer, const std::string& symbol, const std::string& description,
                                                                     const chain::non_fungible_token_data_definitions& definitions, uint32_t max_supply,
                                                                     bool fixed_max_supply, bool transferable, bool broadcast)
{ try {
   FC_VERIFY_AND_THROW(!find_non_fungible_token(symbol).valid(), nft_already_exist_exception);

   chain::non_fungible_token_create_definition_operation create_op;
   create_op.symbol = symbol;
   create_op.transferable = transferable;
   create_op.options.issuer = get_account(issuer).get_id();
   create_op.options.max_supply = max_supply;
   create_op.options.fixed_max_supply = fixed_max_supply;
   create_op.options.description = description;
   create_op.definitions = definitions;
   create_op.validate();

   chain::signed_transaction tx;
   tx.operations.push_back( create_op );
   set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees);
   tx.validate();

   return sign_transaction( tx, broadcast );
} FC_CAPTURE_AND_RETHROW( (issuer)(symbol)(description)(definitions)(max_supply)(fixed_max_supply)(transferable)(broadcast) ) }

chain::signed_transaction wallet_api_impl::update_non_fungible_token(const std::string& issuer, const std::string& symbol, const std::string& description,
                                                                     uint32_t max_supply, bool fixed_max_supply, bool broadcast)
{ try {
   chain::non_fungible_token_object nft_obj = get_non_fungible_token(symbol);
   chain::non_fungible_token_update_definition_operation update_op;
   update_op.current_issuer = nft_obj.options.issuer;
   update_op.nft_id = nft_obj.get_id();
   update_op.options.issuer = issuer.empty() ? nft_obj.options.issuer : get_account(issuer).get_id();
   update_op.options.max_supply = max_supply;
   update_op.options.fixed_max_supply = fixed_max_supply;
   update_op.options.description = description;
   update_op.validate();

   chain::signed_transaction tx;
   tx.operations.push_back( update_op );
   set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees);
   tx.validate();

   return sign_transaction( tx, broadcast );
} FC_CAPTURE_AND_RETHROW( (issuer)(symbol)(description)(max_supply)(fixed_max_supply)(broadcast) ) }

chain::signed_transaction wallet_api_impl::issue_non_fungible_token(const std::string& to_account, const std::string& symbol, const fc::variants& data,
                                                                    const std::string& memo, bool broadcast)
{ try {
   chain::non_fungible_token_object nft_obj = get_non_fungible_token(symbol);
   chain::account_object to = get_account(to_account);
   chain::account_object issuer = get_account(nft_obj.options.issuer);

   chain::non_fungible_token_issue_operation issue_op;
   issue_op.issuer = nft_obj.options.issuer;
   issue_op.to = to.id;
   issue_op.nft_id = nft_obj.get_id();
   issue_op.data = data;

   if(!memo.empty())
      issue_op.memo = chain::memo_data(memo, get_private_key(issuer.options.memo_key), to.options.memo_key);

   issue_op.validate();

   chain::signed_transaction tx;
   tx.operations.push_back( issue_op );
   set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees);
   tx.validate();

   return sign_transaction( tx, broadcast );
} FC_CAPTURE_AND_RETHROW( (to_account)(symbol)(data)(memo)(broadcast) ) }

chain::signed_transaction wallet_api_impl::transfer_non_fungible_token_data(const std::string& to_account, chain::non_fungible_token_data_id_type nft_data_id,
                                                                            const std::string& memo, bool broadcast)
{ try {
   chain::non_fungible_token_data_object nft_data = get_non_fungible_token_data(nft_data_id);
   chain::account_object from = get_account(nft_data.owner);
   chain::account_object to = get_account(to_account);

   chain::non_fungible_token_transfer_operation transfer_op;
   transfer_op.from = nft_data.owner;
   transfer_op.to = to.id;
   transfer_op.nft_data_id = nft_data.get_id();

   if(!memo.empty())
      transfer_op.memo = chain::memo_data(memo, get_private_key(from.options.memo_key), to.options.memo_key);

   transfer_op.validate();

   chain::signed_transaction tx;
   tx.operations.push_back( transfer_op );
   set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees);
   tx.validate();

   return sign_transaction( tx, broadcast );
} FC_CAPTURE_AND_RETHROW( (to_account)(nft_data_id)(memo)(broadcast) ) }

chain::signed_transaction wallet_api_impl::burn_non_fungible_token_data(chain::non_fungible_token_data_id_type nft_data_id, bool broadcast)
{
   return transfer_non_fungible_token_data(static_cast<std::string>(static_cast<db::object_id_type>(GRAPHENE_NULL_ACCOUNT)), nft_data_id, "", broadcast);
}

chain::signed_transaction wallet_api_impl::update_non_fungible_token_data(const std::string& modifier, chain::non_fungible_token_data_id_type nft_data_id,
                                                                          const std::vector<std::pair<std::string, fc::variant>>& data, bool broadcast)
{ try {
   chain::non_fungible_token_data_object nft_data = get_non_fungible_token_data(nft_data_id);
   chain::non_fungible_token_update_data_operation data_op;
   data_op.modifier = get_account(modifier).get_id();
   data_op.nft_data_id = nft_data.get_id();
   data_op.data = data;

   data_op.validate();

   chain::signed_transaction tx;
   tx.operations.push_back( data_op );
   set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees);
   tx.validate();

   return sign_transaction( tx, broadcast );
} FC_CAPTURE_AND_RETHROW( (modifier)(nft_data_id)(data)(broadcast) ) }

chain::miner_object wallet_api_impl::get_miner(const std::string& owner_account) const
{ try {
   fc::optional<chain::miner_id_type> miner_id = maybe_id<chain::miner_id_type>(owner_account);
   if (miner_id)
   {
      std::vector<chain::miner_id_type> ids_to_get;
      ids_to_get.push_back(*miner_id);
      std::vector<fc::optional<chain::miner_object>> miner_objects = _remote_db->get_miners(ids_to_get);
      FC_VERIFY_AND_THROW(miner_objects.front().valid(), no_miner_is_registered_for_this_owner_id_exception, "Owner id: ${id}", ("id", owner_account));
      return *miner_objects.front();
   }
   else
   {
      // then maybe it's the owner account
      chain:: account_id_type owner_account_id = get_account(owner_account).get_id();
      fc::optional<chain::miner_object> miner = _remote_db->get_miner_by_account(owner_account_id);
      FC_VERIFY_AND_THROW(miner.valid(), no_miner_is_registered_for_this_owner_id_exception, "Owner id: ${id}", ("id", owner_account));
      return *miner;
   }
} FC_CAPTURE_AND_RETHROW( (owner_account) ) }

chain::signed_transaction wallet_api_impl::create_miner(const std::string& owner_account, const std::string& url, bool broadcast)
{ try {
   chain::account_object miner_account = get_account(owner_account);
   chain::private_key_type active_private_key = get_private_key_for_account(miner_account);
   int miner_key_index = find_first_unused_derived_key_index(active_private_key);
   chain::private_key_type miner_private_key = utilities::derive_private_key(utilities::key_to_wif(active_private_key), miner_key_index);
   chain::public_key_type miner_public_key = miner_private_key.get_public_key();

   chain::miner_create_operation miner_create_op;
   miner_create_op.miner_account = miner_account.id;
   miner_create_op.block_signing_key = miner_public_key;
   miner_create_op.url = url;

   if (_remote_db->get_miner_by_account(miner_create_op.miner_account))
      FC_THROW("Account ${owner_account} is already a miner", ("owner_account", owner_account));

   chain::signed_transaction tx;
   tx.operations.push_back( miner_create_op );
   set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
   tx.validate();

   _wallet.pending_miner_registrations[owner_account] = utilities::key_to_wif(miner_private_key);

   return sign_transaction( tx, broadcast );
} FC_CAPTURE_AND_RETHROW( (owner_account)(broadcast) ) }

chain::signed_transaction wallet_api_impl::update_miner(const std::string& miner_name, const std::string& url, const std::string& block_signing_key, bool broadcast)
{ try {
   chain::miner_object miner = get_miner(miner_name);
   chain::account_object miner_account = get_account( miner.miner_account );
   chain::private_key_type active_private_key = get_private_key_for_account(miner_account);

   chain::miner_update_operation miner_update_op;
   miner_update_op.miner = miner.id;
   miner_update_op.miner_account = miner_account.id;
   if( !url.empty() )
      miner_update_op.new_url = url;
   if( !block_signing_key.empty() )
      miner_update_op.new_signing_key = chain::public_key_type( block_signing_key );

   chain::signed_transaction tx;
   tx.operations.push_back( miner_update_op );
   set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees );
   tx.validate();

   return sign_transaction( tx, broadcast );
} FC_CAPTURE_AND_RETHROW( (miner_name)(url)(block_signing_key)(broadcast) ) }

std::vector<vesting_balance_object_with_info> wallet_api_impl::get_vesting_balances(const std::string& account_name) const
{ try {
   fc::optional<chain::vesting_balance_id_type> vbid = maybe_id<chain::vesting_balance_id_type>( account_name );
   std::vector<vesting_balance_object_with_info> result;
   fc::time_point_sec now = _remote_db->get_dynamic_global_properties().time;

   if( vbid )
   {
      result.emplace_back( get_object<chain::vesting_balance_object>(*vbid), now );
      return result;
   }

   // try casting to avoid a round-trip if we were given an account ID
   fc::optional<chain::account_id_type> acct_id = maybe_id<chain::account_id_type>( account_name );
   if( !acct_id )
      acct_id = get_account( account_name ).id;

   std::vector<chain::vesting_balance_object> vbos = _remote_db->get_vesting_balances( *acct_id );
   if( vbos.size() == 0 )
      return result;

   for( const chain::vesting_balance_object& vbo : vbos )
      result.emplace_back( vbo, now );

   return result;
} FC_CAPTURE_AND_RETHROW( (account_name) ) }

chain::signed_transaction wallet_api_impl::create_linear_vesting(const std::string& creator, const std::string& owner, const std::string& amount, const std::string& asset_symbol,
                                                                 const fc::time_point_sec start, const uint32_t cliff_seconds, const uint32_t duration_seconds, bool broadcast)
   { try {
   chain::account_object creator_obj = get_account(creator);
   chain::account_object owner_obj = get_account(owner);
   chain::asset_object asset_obj = get_asset(asset_symbol);

   chain::vesting_balance_create_operation vesting_balance_create_op;

   vesting_balance_create_op.creator = creator_obj.id;
   vesting_balance_create_op.owner = owner_obj.id;
   vesting_balance_create_op.amount = asset_obj.amount_from_string(amount);
   vesting_balance_create_op.policy = chain::linear_vesting_policy_initializer{ start, cliff_seconds, duration_seconds };

   chain::signed_transaction tx;
   tx.operations.push_back( vesting_balance_create_op );
   set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees );
   tx.validate();

   return sign_transaction( tx, broadcast );
} FC_CAPTURE_AND_RETHROW( (creator)(owner)(amount)(asset_symbol)(start)(cliff_seconds)(duration_seconds)(broadcast) ) }

chain::signed_transaction wallet_api_impl::withdraw_vesting(const std::string& miner_name, const std::string& amount, const std::string& asset_symbol, bool broadcast)
{ try {
   chain::asset_object asset_obj = get_asset( asset_symbol );
   fc::optional<chain::vesting_balance_id_type> vbid = maybe_id<chain::vesting_balance_id_type>(miner_name);
   if( !vbid )
   {
      chain::miner_object wit = get_miner( miner_name );
      FC_ASSERT( wit.pay_vb );
      vbid = wit.pay_vb;
   }

   chain::vesting_balance_object vbo = get_object<chain::vesting_balance_object>( *vbid );
   chain::vesting_balance_withdraw_operation vesting_balance_withdraw_op;

   vesting_balance_withdraw_op.vesting_balance = *vbid;
   vesting_balance_withdraw_op.owner = vbo.owner;
   vesting_balance_withdraw_op.amount = asset_obj.amount_from_string(amount);

   chain::signed_transaction tx;
   tx.operations.push_back( vesting_balance_withdraw_op );
   set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees );
   tx.validate();

   return sign_transaction( tx, broadcast );
} FC_CAPTURE_AND_RETHROW( (miner_name)(amount) ) }

chain::signed_transaction wallet_api_impl::vote_for_miner(const std::string& voting_account, const std::string& miner, bool approve, bool broadcast)
{ try {
   chain::account_object voting_account_object = get_account(voting_account);
   chain::account_id_type miner_owner_account_id = get_account(miner).get_id();
   fc::optional<chain::miner_object> miner_obj = _remote_db->get_miner_by_account(miner_owner_account_id);
   FC_VERIFY_AND_THROW(miner_obj.valid(), account_is_not_registered_as_miner_exception, "Account: ${miner}", ("miner", miner));

   if (approve)
   {
      auto insert_result = voting_account_object.options.votes.insert(miner_obj->vote_id);
      FC_VERIFY_AND_THROW(insert_result.second, account_was_already_voting_for_miner_exception, "Account: ${account} miner: $miner}", ("account", voting_account)("miner", miner));
   }
   else
   {
      auto votes_removed = voting_account_object.options.votes.erase(miner_obj->vote_id);
      FC_VERIFY_AND_THROW(votes_removed, account_is_already_not_voting_miner_exception, "Account: ${account} miner: $miner}", ("account", voting_account)("miner", miner));
   }
   chain::account_update_operation account_update_op;
   account_update_op.account = voting_account_object.id;
   account_update_op.new_options = voting_account_object.options;

   chain::signed_transaction tx;
   tx.operations.push_back( account_update_op );
   set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
   tx.validate();

   return sign_transaction( tx, broadcast );
} FC_CAPTURE_AND_RETHROW( (voting_account)(miner)(approve)(broadcast) ) }

chain::signed_transaction wallet_api_impl::set_voting_proxy(const std::string& account_to_modify, fc::optional<std::string> voting_account, bool broadcast)
{ try {
   chain::account_object account_object_to_modify = get_account(account_to_modify);
   if (voting_account)
   {
      chain::account_id_type new_voting_account_id = get_account(*voting_account).get_id();
      FC_VERIFY_AND_THROW(account_object_to_modify.options.voting_account != new_voting_account_id, voting_proxy_is_already_set_to_voter_exception,
         "For: ${account} voter: ${voter}", ("account", account_to_modify)("voter", *voting_account));

      account_object_to_modify.options.voting_account = new_voting_account_id;
   }
   else
   {
      FC_VERIFY_AND_THROW(account_object_to_modify.options.voting_account != GRAPHENE_PROXY_TO_SELF_ACCOUNT, account_was_already_voting_for_itself_exception,
         "Account: ${account}", ("account", account_to_modify));

      account_object_to_modify.options.voting_account = GRAPHENE_PROXY_TO_SELF_ACCOUNT;
   }

   chain::account_update_operation account_update_op;
   account_update_op.account = account_object_to_modify.id;
   account_update_op.new_options = account_object_to_modify.options;

   chain::signed_transaction tx;
   tx.operations.push_back( account_update_op );
   set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
   tx.validate();

   return sign_transaction( tx, broadcast );
} FC_CAPTURE_AND_RETHROW( (account_to_modify)(voting_account)(broadcast) ) }

chain::signed_transaction wallet_api_impl::set_desired_miner_count(const std::string& account_to_modify, uint16_t desired_number_of_miners, bool broadcast)
{ try {
   chain::account_object account_object_to_modify = get_account(account_to_modify);

   FC_VERIFY_AND_THROW(account_object_to_modify.options.num_miner != desired_number_of_miners, account_was_already_voting_for_miners_exception,
      "Account: ${account} number of miners: ${miners} ", ("account", account_to_modify)("miners", desired_number_of_miners));

   account_object_to_modify.options.num_miner = desired_number_of_miners;

   chain::account_update_operation account_update_op;
   account_update_op.account = account_object_to_modify.id;
   account_update_op.new_options = account_object_to_modify.options;

   chain::signed_transaction tx;
   tx.operations.push_back( account_update_op );
   set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
   tx.validate();

   return sign_transaction( tx, broadcast );
} FC_CAPTURE_AND_RETHROW( (account_to_modify)(desired_number_of_miners)(broadcast) ) }

chain::signed_transaction wallet_api_impl::sign_transaction(chain::signed_transaction tx, bool broadcast)
{
   boost::container::flat_set<chain::account_id_type> req_active_approvals;
   boost::container::flat_set<chain::account_id_type> req_owner_approvals;
   std::vector<chain::authority> other_auths;

   tx.get_required_authorities( req_active_approvals, req_owner_approvals, other_auths );

   for( const auto& auth : other_auths )
      for( const auto& a : auth.account_auths )
         req_active_approvals.insert(a.first);

   // std::merge lets us de-duplicate account_id's that occur in both
   //   sets, and dump them into a vector (as required by remote_db api)
   //   at the same time
   std::vector<chain::account_id_type> v_approving_account_ids;
   std::merge(req_active_approvals.begin(), req_active_approvals.end(),
               req_owner_approvals.begin() , req_owner_approvals.end(),
               std::back_inserter(v_approving_account_ids));

   /// TODO: fetch the accounts specified via other_auths as well.

   std::vector<fc::optional<chain::account_object>> approving_account_objects = _remote_db->get_accounts( v_approving_account_ids );

   /// TODO: recursively check one layer deeper in the authority tree for keys

   FC_ASSERT( approving_account_objects.size() == v_approving_account_ids.size() );

   boost::container::flat_map<chain::account_id_type, chain::account_object*> approving_account_lut;
   size_t i = 0;
   for( fc::optional<chain::account_object>& approving_acct : approving_account_objects )
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

   boost::container::flat_set<chain::public_key_type> approving_key_set;
   for( chain::account_id_type acct_id : req_active_approvals )
   {
      const auto it = approving_account_lut.find( acct_id );
      if( it == approving_account_lut.end() )
         continue;
      const chain::account_object* acct = it->second;
      std::vector<chain::public_key_type> v_approving_keys = acct->active.get_keys();
      for( const chain::public_key_type& approving_key : v_approving_keys )
         approving_key_set.insert( approving_key );
   }
   for( chain::account_id_type acct_id : req_owner_approvals )
   {
      const auto it = approving_account_lut.find( acct_id );
      if( it == approving_account_lut.end() )
         continue;
      const chain::account_object* acct = it->second;
      std::vector<chain::public_key_type> v_approving_keys = acct->owner.get_keys();
      for( const chain::public_key_type& approving_key : v_approving_keys )
         approving_key_set.insert( approving_key );
   }
   for( const chain::authority& a : other_auths )
   {
      for( const auto& k : a.key_auths )
         approving_key_set.insert( k.first );
   }

   auto dyn_props = _remote_db->get_dynamic_global_properties();
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

      for( chain::public_key_type& key : approving_key_set )
      {
         auto it = _keys.find(key);
         if( it != _keys.end() )
         {
            fc::optional<chain::private_key_type> privkey = utilities::wif_to_key( it->second );
            FC_ASSERT( privkey.valid(), "Malformed private key in _keys" );
            tx.sign( *privkey, _chain_id );
         }
         /// TODO: if transaction has enough signatures to be "valid" don't add any more,
         /// there are cases where the wallet may have more keys than strictly necessary and
         /// the transaction will be rejected if the transaction validates without requiring
         /// all signatures provided
      }

      chain::transaction_id_type this_transaction_id = tx.id();
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

chain::signed_transaction wallet_api_impl::transfer(const std::string& from, const std::string& to, const std::string& amount, const std::string& asset_symbol,
                                                    const std::string& memo, bool broadcast)
{ try {
   chain::account_object from_account = get_account(from);
   chain::account_id_type from_id = from_account.id;

   chain::asset_object asset_obj = get_asset(asset_symbol);

   chain::account_object to_account;
   db::object_id_type to_obj_id;

   bool is_account = true;

   try {
      to_account = get_account(to);
      to_obj_id = db::object_id_type(to_account.id);
   }
   catch ( const fc::exception& )
   {
      is_account = false;
   }

   if( !is_account )
   {
      to_obj_id = db::object_id_type(to);
      chain::content_id_type content_id = to_obj_id.as<chain::content_id_type>();
      const chain::content_object content_obj = get_object<chain::content_object>(content_id);
      to_account = get_account( content_obj.author);
   }

   chain::signed_transaction tx;
   chain::transfer_operation xfer_op;

   xfer_op.from = from_id;
   xfer_op.to = to_obj_id;
   xfer_op.amount = asset_obj.amount_from_string(amount);

   if( !memo.empty() )
   {
      xfer_op.memo = chain::memo_data(memo, get_private_key(from_account.options.memo_key), to_account.options.memo_key);
   }

   tx.operations.push_back(xfer_op);

   set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
   tx.validate();

   return sign_transaction(tx, broadcast);
} FC_CAPTURE_AND_RETHROW( (from)(to)(amount)(asset_symbol)(memo)(broadcast) ) }

chain::signed_transaction wallet_api_impl::propose_transfer(const std::string& proposer, const std::string& from, const std::string& to, const std::string& amount,
                                                            const std::string& asset_symbol, const std::string& memo, fc::time_point_sec expiration)
{
   transaction_handle_type propose_num = begin_builder_transaction();
   chain::transfer_obsolete_operation op;
   chain::account_object from_account = get_account(from);
   chain::account_object to_account = get_account(to);
   op.from = from_account.id;
   op.to = to_account.id;
   chain::asset_object asset_obj = get_asset(asset_symbol);
   op.amount = asset_obj.amount_from_string(amount);
   if( !memo.empty() )
   {
      op.memo = chain::memo_data(memo, get_private_key(from_account.options.memo_key), to_account.options.memo_key);
   }

   add_operation_to_builder_transaction(propose_num, op);
   set_fees_on_builder_transaction(propose_num, std::string(GRAPHENE_SYMBOL));

   return propose_builder_transaction2(propose_num, proposer, expiration, 0, true);
}

std::string wallet_api_impl::decrypt_memo(const chain::memo_data& memo, const chain::account_object& from_account, const chain::account_object& to_account) const
{
   FC_ASSERT(_keys.count(memo.to) || _keys.count(memo.from), "Memo is encrypted to a key ${to} or ${from} not in this wallet.", ("to", memo.to)("from",memo.from));
   std::string memo_result;
   if( _keys.count(memo.to) ) {
      std::vector<chain::private_key_type> keys_to_try_to;
      auto my_memo_key = utilities::wif_to_key(_keys.at(memo.to));

      FC_ASSERT(my_memo_key, "Unable to recover private key to decrypt memo. Wallet may be corrupted.");
      keys_to_try_to.push_back(*my_memo_key);
      for( auto k: to_account.active.key_auths ) {
         auto key_itr = _keys.find(k.first);
         if( key_itr == _keys.end() )
            continue;
         auto my_key = utilities::wif_to_key(key_itr->second);
         if(my_key)
            keys_to_try_to.push_back(*my_key);
      }
      for( auto k: to_account.owner.key_auths ) {
         auto key_itr = _keys.find(k.first);
         if( key_itr == _keys.end() )
            continue;
         auto my_key = utilities::wif_to_key(key_itr->second);
         if(my_key)
            keys_to_try_to.push_back(*my_key);
      }

      for( auto k : keys_to_try_to ){
         try{
            memo_result = memo.get_message(k, memo.from);
            return memo_result;
         }catch(...){}
      }

      std::vector<chain::public_key_type> keys_to_try_from;
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
      std::vector<chain::private_key_type> keys_to_try_from;
      auto my_memo_key = utilities::wif_to_key(_keys.at(memo.from));

      FC_ASSERT(my_memo_key, "Unable to recover private key to decrypt memo. Wallet may be corrupted.");
      keys_to_try_from.push_back(*my_memo_key);
      for( auto k: from_account.active.key_auths ) {
         auto key_itr = _keys.find(k.first);
         if( key_itr == _keys.end() )
            continue;
         auto my_key = utilities::wif_to_key(key_itr->second);
         if(my_key)
            keys_to_try_from.push_back(*my_key);
      }
      for( auto k: from_account.owner.key_auths ) {
         auto key_itr = _keys.find(k.first);
         if( key_itr == _keys.end() )
            continue;
         auto my_key = utilities::wif_to_key(key_itr->second);
         if(my_key)
            keys_to_try_from.push_back(*my_key);
      }

      for( auto k : keys_to_try_from ){
         try{
            memo_result = memo.get_message(k, memo.to);
            return memo_result;
         }catch(...){}
      }

      std::vector<chain::public_key_type> keys_to_try_to;
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

chain::signed_transaction wallet_api_impl::propose_parameter_change(const std::string& proposing_account, fc::time_point_sec expiration_time, const fc::variant_object& changed_values, bool broadcast)
{
   FC_ASSERT( !changed_values.contains("current_fees") );

   const chain::chain_parameters& current_params = _remote_db->get_global_properties().parameters;
   chain::chain_parameters new_params = current_params;
   fc::reflector<chain::chain_parameters>::visit(
      fc::from_variant_visitor<chain::chain_parameters>( changed_values, new_params )
      );

   chain::miner_update_global_parameters_operation update_op;
   update_op.new_parameters = new_params;

   chain::proposal_create_operation prop_op;

   prop_op.expiration_time = expiration_time;
   prop_op.review_period_seconds = current_params.miner_proposal_review_period;
   prop_op.fee_paying_account = get_account(proposing_account).id;

   prop_op.proposed_ops.emplace_back( update_op );
   current_params.current_fees->set_fee( prop_op.proposed_ops.back().op, _remote_db->head_block_time() );

   chain::signed_transaction tx;
   tx.operations.push_back(prop_op);
   set_operation_fees(tx, current_params.current_fees);
   tx.validate();

   return sign_transaction(tx, broadcast);
}

chain::signed_transaction wallet_api_impl::propose_fee_change(const std::string& proposing_account, fc::time_point_sec expiration_time, const fc::variant_object& changed_fees, bool broadcast)
{
   const chain::chain_parameters& current_params = _remote_db->get_global_properties().parameters;
   const chain::fee_schedule_type& current_fees = *(current_params.current_fees);

   boost::container::flat_map< int, chain::fee_parameters > fee_map;
   fee_map.reserve( current_fees.parameters.size() );
   for( const chain::fee_parameters& op_fee : current_fees.parameters )
      fee_map[ op_fee.which() ] = op_fee;
   uint32_t scale = current_fees.scale;

   for( const auto& item : changed_fees )
   {
      const std::string& key = item.key();
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

      chain::fee_parameters fp = from_which_variant<chain::fee_parameters>( which, item.value() );
      fee_map[ which ] = fp;
   }

   chain::fee_schedule_type new_fees;

   for( const std::pair< int, chain::fee_parameters >& item : fee_map )
      new_fees.parameters.insert( item.second );
   new_fees.scale = scale;

   chain::chain_parameters new_params = current_params;
   new_params.current_fees = new_fees;

   chain::miner_update_global_parameters_operation update_op;
   update_op.new_parameters = new_params;

   chain::proposal_create_operation prop_op;

   prop_op.expiration_time = expiration_time;
   prop_op.review_period_seconds = current_params.miner_proposal_review_period;
   prop_op.fee_paying_account = get_account(proposing_account).id;

   prop_op.proposed_ops.emplace_back( update_op );
   current_params.current_fees->set_fee( prop_op.proposed_ops.back().op, _remote_db->head_block_time() );

   chain::signed_transaction tx;
   tx.operations.push_back(prop_op);
   set_operation_fees(tx, current_params.current_fees);
   tx.validate();

   return sign_transaction(tx, broadcast);
}

chain::signed_transaction wallet_api_impl::approve_proposal(const std::string& fee_paying_account, const std::string& proposal_id, const approval_delta& delta, bool broadcast)
{
   chain::proposal_update_operation update_op;

   update_op.fee_paying_account = get_account(fee_paying_account).id;
   update_op.proposal = fc::variant(proposal_id).as<chain::proposal_id_type>();
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
      update_op.key_approvals_to_add.insert( chain::public_key_type( k ) );
   for( const std::string& k : delta.key_approvals_to_remove )
      update_op.key_approvals_to_remove.insert( chain::public_key_type( k ) );

   chain::signed_transaction tx;
   tx.operations.push_back(update_op);
   set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees);
   tx.validate();
   return sign_transaction(tx, broadcast);
}

void wallet_api_impl::submit_content_utility(chain::content_submit_operation& submit_op, const std::vector<regional_price_info>& price_amounts)
{
   std::vector<chain::regional_price> arr_prices;

   for (auto const& item : price_amounts)
   {

      uint32_t region_code_for = chain::RegionCodes::OO_none;

      auto it = chain::RegionCodes::s_mapNameToCode.find(item.region);
      if (it != chain::RegionCodes::s_mapNameToCode.end())
         region_code_for = it->second;
      else
         FC_ASSERT(false, "Invalid region code");

      chain::asset_object currency = get_asset(item.asset_symbol);

      arr_prices.push_back({region_code_for, currency.amount_from_string(item.amount)});
   }

   submit_op.price = arr_prices;
}

chain::signed_transaction wallet_api_impl::submit_content(const std::string& author, const std::vector<std::pair<std::string, uint32_t>>& co_authors,
                                                          const std::string& URI, const std::vector<regional_price_info>& price_amounts, const fc::ripemd160& hash,
                                                          uint64_t size, const std::vector<chain::account_id_type>& seeders, uint32_t quorum, fc::time_point_sec expiration,
                                                          const std::string& publishing_fee_symbol_name, const std::string& publishing_fee_amount, const std::string& synopsis,
                                                          const decent::encrypt::DInteger& secret, const decent::encrypt::CustodyData& cd, bool broadcast)
{
   try
   {
      auto& pmc = decent::package::PackageManagerConfigurator::instance();
      decent::check_ipfs_minimal_version(pmc.get_ipfs_host(), pmc.get_ipfs_port());
      chain::account_id_type author_account = get_account( author ).get_id();

      std::map<chain::account_id_type, uint32_t> co_authors_id_to_split;
      if( !co_authors.empty() )
      {
         for( auto const& element : co_authors )
         {
            chain::account_id_type co_author = get_account( element.first ).get_id();
            co_authors_id_to_split[ co_author ] = element.second;
         }
      }

      // checking for duplicates
      FC_ASSERT( co_authors.size() == co_authors_id_to_split.size(), "Duplicity in the list of co-authors is not allowed." );

      chain::asset_object fee_asset_obj = get_asset(publishing_fee_symbol_name);

      decent::encrypt::ShamirSecret ss(static_cast<uint16_t>(quorum), static_cast<uint16_t>(seeders.size()), secret);
      ss.calculate_split();
      chain::content_submit_operation submit_op;
      for( int i =0; i<(int)seeders.size(); i++ )
      {
         const auto& s = _remote_db->get_seeder( seeders[i] );
         decent::encrypt::Ciphertext cp;
         decent::encrypt::point p = ss.split[i];
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

      chain::signed_transaction tx;
      tx.operations.push_back( submit_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (author)(URI)(price_amounts)(hash)(seeders)(quorum)(expiration)(publishing_fee_symbol_name)(publishing_fee_amount)(synopsis)(secret)(broadcast) )
}

app::content_keys wallet_api_impl::submit_content_async(const std::string& author, const std::vector<std::pair<std::string, uint32_t>>& co_authors,
                                                        const std::string& content_dir, const std::string& samples_dir, const std::string& protocol,
                                                        const std::vector<regional_price_info>& price_amounts, const std::vector<chain::account_id_type>& seeders,
                                                        fc::time_point_sec expiration, const std::string& synopsis)
{
   try
   {
      auto& pmc = decent::package::PackageManagerConfigurator::instance();
      decent::check_ipfs_minimal_version(pmc.get_ipfs_host(), pmc.get_ipfs_port());
      chain::account_object author_account = get_account( author );

      std::map<chain::account_id_type, uint32_t> co_authors_id_to_split;
      if( !co_authors.empty() )
      {
         for( auto const &element : co_authors )
         {
            chain::account_id_type co_author = get_account( element.first ).get_id();
            co_authors_id_to_split[ co_author ] = element.second;
         }
      }

      // checking for duplicates
      FC_VERIFY_AND_THROW(co_authors.size() == co_authors_id_to_split.size(), duplicity_at_the_list_of_coauthors_not_allowed_exception);
      FC_VERIFY_AND_THROW(!price_amounts.empty(), the_prices_of_the_content_per_region_cannot_be_empty_exception);
      FC_VERIFY_AND_THROW(expiration < fc::time_point_sec(fc::time_point::now()), invalid_content_expiration_exception);

      CryptoPP::Integer secret(randomGenerator, 256);
      while( secret >= Params::instance().DECENT_SHAMIR_ORDER ){
         CryptoPP::Integer tmp(randomGenerator, 256);
         secret = tmp;
      }

      app::content_keys keys;
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
         const auto& s = _remote_db->get_seeder( seeders[i] );
         FC_VERIFY_AND_THROW(s.valid(), seeder_not_found_exception, "Seeder: ${s}", ("s", seeders[i]));

         decent::encrypt::point p = ss.split[i];
         dlog("Split ${i} = ${a} / ${b}",("i",i)("a",decent::encrypt::DIntegerString(p.first))("b",decent::encrypt::DIntegerString(p.second)));

         decent::encrypt::Ciphertext cp;
         decent::encrypt::el_gamal_encrypt( p, s->pubKey, cp );
         keys.parts.push_back(cp);
      }

      // check synopsis format
      chain::ContentObjectPropertyManager synopsis_parser(synopsis);
      synopsis_parser.get<chain::ContentObjectType>();
      synopsis_parser.get<chain::ContentObjectTitle>();
      synopsis_parser.get<chain::ContentObjectDescription>();

      chain::content_submit_operation submit_op;
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
      std::shared_ptr<submit_transfer_listener> listener_ptr = std::make_shared<submit_transfer_listener>(*this, package_handle, submit_op, protocol);
      _package_manager_listeners.push_back(listener_ptr);

      package_handle->add_event_listener(listener_ptr);
      package_handle->create(false);

      //We end up here and return the  to the upper layer. The create method will continue in the background, and once finished, it will call the respective callback of submit_transfer_listener class
      return keys;
   }
   FC_CAPTURE_AND_RETHROW( (author)(content_dir)(samples_dir)(protocol)(price_amounts)(seeders)(expiration)(synopsis) )
}

chain::signed_transaction wallet_api_impl::content_cancellation(const std::string& author, const std::string& URI, bool broadcast)
{
   try
   {
      chain::content_cancellation_operation cc_op;
      cc_op.author = get_account( author ).get_id();
      cc_op.URI = URI;

      chain::signed_transaction tx;
      tx.operations.push_back( cc_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();
      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (author)(URI)(broadcast) )
}

content_download_status wallet_api_impl::get_download_status(const std::string& consumer, const std::string& URI) const
{
   try
   {
      chain::account_id_type acc = get_account(consumer).id;
      fc::optional<chain::buying_object> bobj = _remote_db->get_buying_by_consumer_URI( acc, URI );
      FC_VERIFY_AND_THROW(bobj.valid(), cannot_find_download_object_exception);

      fc::optional<chain::content_object> content = _remote_db->get_content( URI );

      if (!content) {
            FC_THROW("Invalid content URI");
      }

      content_download_status status;
      status.received_key_parts = static_cast<uint32_t>(bobj->key_particles.size());
      status.total_key_parts = static_cast<uint32_t>(content->key_parts.size());

      auto pack = decent::package::PackageManager::instance().find_package(URI);

      if (!pack) {
            status.total_download_bytes = 0;
            status.received_download_bytes = 0;
            status.status_text = "Unknown";
      } else {
         if (pack->get_data_state() == decent::package::PackageInfo::CHECKED) {

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

void wallet_api_impl::download_content(const std::string& consumer, const std::string& URI, const std::string& str_region_code_from, bool broadcast)
{
   try
   {
      fc::optional<chain::content_object> content = _remote_db->get_content( URI );
      chain::account_object consumer_account = get_account( consumer );

      FC_VERIFY_AND_THROW(content.valid(), invalid_content_uri_exception, "URI: ${uri}", ("uri", URI));

      uint32_t region_code_from = chain::RegionCodes::OO_none;

      auto it = chain::RegionCodes::s_mapNameToCode.find(str_region_code_from);
      if (it != chain::RegionCodes::s_mapNameToCode.end())
         region_code_from = it->second;
      //
      // may want to throw here to forbid purchase from unknown region
      // but seems can also try to allow purchase if the content has default price
      //

      fc::optional<chain::asset> op_price = content->price.GetPrice(region_code_from);
      FC_VERIFY_AND_THROW(op_price.valid(), content_not_available_for_this_region_exception);

      chain::request_to_buy_operation request_op;
      request_op.consumer = consumer_account.id;
      request_op.URI = URI;

      decent::encrypt::DInteger el_gamal_priv_key = decent::encrypt::generate_private_el_gamal_key_from_secret ( get_private_key_for_account(consumer_account).get_secret() );

      request_op.pubKey = decent::encrypt::get_public_el_gamal_key( el_gamal_priv_key );
      request_op.price = _remote_db->price_to_dct(*op_price);
      request_op.region_code_from = region_code_from;

      chain::signed_transaction tx;
      tx.operations.push_back( request_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();
      sign_transaction( tx, broadcast );
      auto& package_manager = decent::package::PackageManager::instance();
      auto package = package_manager.get_package( URI, content->_hash );
      package->download();

   } FC_CAPTURE_AND_RETHROW( (consumer)(URI)(broadcast) )
}

chain::signed_transaction wallet_api_impl::request_to_buy(const std::string& consumer, const std::string& URI, const std::string& price_asset_symbol,
                                                          const std::string& price_amount, const std::string& str_region_code_from, bool broadcast)
{ try {
   chain::account_object consumer_account = get_account( consumer );
   chain::asset_object asset_obj = get_asset(price_asset_symbol);

   chain::request_to_buy_operation request_op;
   request_op.consumer = consumer_account.id;
   request_op.URI = URI;

   decent::encrypt::DInteger el_gamal_priv_key = decent::encrypt::generate_private_el_gamal_key_from_secret ( get_private_key_for_account(consumer_account).get_secret() );
   request_op.pubKey = decent::encrypt::get_public_el_gamal_key( el_gamal_priv_key );

   fc::optional<chain::content_object> content = _remote_db->get_content( URI );
   uint32_t region_code_from = chain::RegionCodes::OO_none;

   auto it = chain::RegionCodes::s_mapNameToCode.find(str_region_code_from);
   if (it != chain::RegionCodes::s_mapNameToCode.end())
      region_code_from = it->second;
   //
   // may want to throw here to forbid purchase from unknown region
   // but seems can also try to allow purchase if the content has default price
   //

   request_op.region_code_from = region_code_from;
   request_op.price = asset_obj.amount_from_string(price_amount);

   chain::signed_transaction tx;
   tx.operations.push_back( request_op );
   set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
   tx.validate();

   return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (consumer)(URI)(price_asset_symbol)(price_amount)(broadcast) )
}

chain::signed_transaction wallet_api_impl::leave_rating_and_comment(const std::string& consumer, const std::string& URI, uint64_t rating,
                                                                    const std::string& comment, bool broadcast)
{
   try
   {
      FC_ASSERT( rating >0 && rating <=5, "Rating shall be 1-5 stars");

      chain::account_object consumer_account = get_account( consumer );

      chain::leave_rating_and_comment_operation leave_rating_op;
      leave_rating_op.consumer = consumer_account.id;
      leave_rating_op.URI = URI;
      leave_rating_op.rating = rating;
      leave_rating_op.comment = comment;

      chain::signed_transaction tx;
      tx.operations.push_back( leave_rating_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction( tx, broadcast );

   } FC_CAPTURE_AND_RETHROW( (consumer)(URI)(rating)(comment)(broadcast) )
}

chain::signed_transaction wallet_api_impl::subscribe_to_author(const std::string& from, const std::string& to, const std::string& price_amount,
                                                               const std::string& price_asset_symbol, bool broadcast)
{ try {
      chain::asset_object asset_obj = get_asset(price_asset_symbol);
      FC_ASSERT( asset_obj.id == chain::asset_id_type() );

      chain::subscribe_operation subscribe_op;
      subscribe_op.from = get_account( from ).get_id();
      subscribe_op.to = get_account( to ).get_id();
      subscribe_op.price = asset_obj.amount_from_string(price_amount);

      chain::signed_transaction tx;
      tx.operations.push_back( subscribe_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (from)(to)(price_amount)(price_asset_symbol)(broadcast) ) }

chain::signed_transaction wallet_api_impl::subscribe_by_author(const std::string& from, const std::string& to, bool broadcast)
{ try {
      chain::subscribe_by_author_operation subscribe_op;
      subscribe_op.from = get_account( from ).get_id();
      subscribe_op.to = get_account( to ).get_id();

      chain::signed_transaction tx;
      tx.operations.push_back( subscribe_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction( tx, broadcast );
} FC_CAPTURE_AND_RETHROW( (from)(to)(broadcast) ) }

chain::signed_transaction wallet_api_impl::set_subscription(const std::string& account, bool allow_subscription, uint32_t subscription_period,
                                                            const std::string& price_amount, const std::string& price_asset_symbol, bool broadcast)
{ try {
      chain::asset_object asset_obj = get_asset(price_asset_symbol);

      chain::account_object account_object_to_modify = get_account( account );
      account_object_to_modify.options.allow_subscription = allow_subscription;
      account_object_to_modify.options.subscription_period = subscription_period;
      account_object_to_modify.options.price_per_subscribe = asset_obj.amount_from_string(price_amount);

      chain::account_update_operation account_update_op;
      account_update_op.account = account_object_to_modify.id;
      account_update_op.new_options = account_object_to_modify.options;

      chain::signed_transaction tx;
      tx.operations.push_back( account_update_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction( tx, broadcast );
} FC_CAPTURE_AND_RETHROW( (account)(allow_subscription)(subscription_period)(price_amount)(price_asset_symbol)(broadcast) ) }

chain::signed_transaction wallet_api_impl::set_automatic_renewal_of_subscription(const std::string& account_id_or_name, chain::subscription_id_type subscription_id,
                                                                                 bool automatic_renewal, bool broadcast)
{
   try {
      chain::account_id_type account = get_account( account_id_or_name ).get_id();
      fc::optional<chain::subscription_object> subscription_obj = _remote_db->get_subscription(subscription_id);

      FC_VERIFY_AND_THROW(subscription_obj.valid(), could_not_find_matching_subcription_exception, "Subscription: ${subscription}", ("subscription", subscription_id));

      chain::automatic_renewal_of_subscription_operation aros_op;
      aros_op.consumer = account;
      aros_op.subscription = subscription_id;
      aros_op.automatic_renewal = automatic_renewal;

      chain::signed_transaction tx;
      tx.operations.push_back( aros_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction( tx, broadcast );
} FC_CAPTURE_AND_RETHROW( (account_id_or_name)(subscription_id)(automatic_renewal)(broadcast) ) }

decent::encrypt::DInteger wallet_api_impl::restore_encryption_key(const std::string& account, chain::buying_id_type buying)
{
   chain::account_object buyer_account = get_account( account );
   const chain::buying_object bo = get_object<chain::buying_object>(buying);
   const chain::content_object co = *(_remote_db->get_content(bo.URI));

   decent::encrypt::ShamirSecret ss(static_cast<uint16_t>(co.quorum), static_cast<uint16_t>(co.key_parts.size()) );
   decent::encrypt::point message;

   const auto& el_gamal_priv_key = _el_gamal_keys.find( bo.pubKey );
   FC_ASSERT( el_gamal_priv_key != _el_gamal_keys.end(), "Wallet does not contain required ElGamal key" );

   int i=0;
   for( const auto key_particle : bo.key_particles )
   {
      FC_ASSERT(decent::encrypt::el_gamal_decrypt(decent::encrypt::Ciphertext(key_particle), el_gamal_priv_key->second, message) == decent::encrypt::ok);
      dlog("Split ${i} = ${a} / ${b}",("i",i++)("a",decent::encrypt::DIntegerString(message.first))("b",decent::encrypt::DIntegerString(message.second)));
      ss.add_point( message );
   }

   FC_ASSERT( ss.resolvable() );
   ss.calculate_secret();
   return ss.secret;
}

std::pair<chain::account_id_type, std::vector<chain::account_id_type>> wallet_api_impl::get_author_and_co_authors_by_URI(const std::string& URI) const
{
   fc::optional<chain::content_object> co = _remote_db->get_content( URI );
   FC_VERIFY_AND_THROW(co.valid(), invalid_content_uri_exception, "URI: ${uri}", ("uri", URI));
   std::pair<chain::account_id_type, std::vector<chain::account_id_type>> result;
   result.first = co->author;
   for( auto const& co_author : co->co_authors )
      result.second.emplace_back( co_author.first );

   return result;
}

std::vector<message_data> wallet_api_impl::get_message_objects(fc::optional<chain::account_id_type> sender, fc::optional<chain::account_id_type> receiver, uint32_t max_count) const
{
   try {
      std::vector<message_data> result;
      for (const auto& obj : _remote_api->messaging()->get_message_objects(sender, receiver, max_count)) {
         result.emplace_back(obj);

         for (const auto& receivers_data_item : obj.receivers_data) {
            try {
               if( obj.sender_pubkey == chain::public_key_type() || receivers_data_item.receiver_pubkey == chain::public_key_type() )
               {
                  result.back().text = receivers_data_item.get_message(chain::private_key_type(), chain::public_key_type());
                  break;
               }

               auto it = _keys.find(receivers_data_item.receiver_pubkey);
               if (it != _keys.end()) {
                  fc::optional<chain::private_key_type> privkey = utilities::wif_to_key(it->second);
                  if (privkey)
                     result.back().text = receivers_data_item.get_message(*privkey, obj.sender_pubkey );
                  else
                     std::cout << "Cannot decrypt message." << std::endl;
               }
               else {
                  it = _keys.find(obj.sender_pubkey);
                  if (it != _keys.end()) {
                     fc::optional<chain::private_key_type> privkey = utilities::wif_to_key(it->second);
                     if (privkey)
                        result.back().text = receivers_data_item.get_message(*privkey, receivers_data_item.receiver_pubkey);
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
      return result;
   } FC_CAPTURE_AND_RETHROW((sender)(receiver))
}

std::vector<text_message> wallet_api_impl::get_messages(const std::string& receiver, uint32_t max_count) const
{
   const auto& receiver_id = get_account(receiver).get_id();
   auto itr = _wallet.my_accounts.get<db::by_id>().find(receiver_id);
   if (itr == _wallet.my_accounts.get<db::by_id>().end())
      return std::vector<text_message>();

   fc::optional<chain::account_id_type> sender_id;
   std::vector<text_message> messages;

   for (const auto& obj : get_message_objects(sender_id, fc::optional<chain::account_id_type>(receiver_id), max_count)) {
      text_message msg;
      msg.created = obj.created;
      chain::account_object account_sender = get_account(obj.sender);
      msg.from = account_sender.name;
      for (const auto& item : obj.receivers_data)
      {
         msg.to.push_back(get_account(item.receiver).name);
      }
      msg.text = obj.text;

      messages.push_back(msg);
   }
   return messages;
}

std::vector<text_message> wallet_api_impl::get_sent_messages(const std::string& sender, uint32_t max_count) const
{
   const auto& sender_id = get_account(sender).get_id();
   auto itr = _wallet.my_accounts.get<db::by_id>().find(sender_id);
   if (itr == _wallet.my_accounts.get<db::by_id>().end())
      return std::vector<text_message>();

   fc::optional<chain::account_id_type> receiver_id;
   std::vector<text_message> messages;

   for (const auto& obj : get_message_objects(fc::optional<chain::account_id_type>(sender_id), receiver_id, max_count)) {
      text_message msg;

      msg.created = obj.created;
      chain::account_object account_sender = get_account(obj.sender);
      msg.from = account_sender.name;
      for (const auto& item : obj.receivers_data)
      {
         msg.to.push_back(get_account(item.receiver).name);
      }
      msg.text = obj.text;

      messages.push_back(msg);
   }
   return messages;
}

chain::signed_transaction wallet_api_impl::send_message(const std::string& from, const std::vector<std::string>& to, const std::string& text, bool broadcast)
{
   try {
      std::set<std::string> unique_to( to.begin(), to.end() );
      if( to.size() != unique_to.size() )
      {
         ilog( "duplicate entries has been removed" );
         std::cout<<"duplicate entries has been removed"<<std::endl;
      }

      chain::account_object from_account = get_account(from);
      chain::account_id_type from_id = from_account.id;

      chain::message_payload pl;
      pl.from = from_id;
      pl.pub_from = from_account.options.memo_key;

      for (const auto& receiver : unique_to) {
         chain::account_object to_account = get_account(receiver);
         pl.receivers_data.emplace_back(text, get_private_key(from_account.options.memo_key), to_account.options.memo_key, to_account.get_id());
      }

      chain::custom_operation cust_op;
      cust_op.id = chain::custom_operation::custom_operation_subtype_messaging;
      cust_op.payer = from_id;
      cust_op.set_messaging_payload(pl);

      chain::signed_transaction tx;
      tx.operations.push_back(cust_op);

      set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction(tx, broadcast);

   } FC_CAPTURE_AND_RETHROW((from)(to)(text)(broadcast))
}

chain::signed_transaction wallet_api_impl::send_unencrypted_message(const std::string& from, const std::vector<std::string>& to, const std::string& text, bool broadcast)
{
   try {
      std::set<std::string> unique_to( to.begin(), to.end() );
      if( to.size() != unique_to.size() )
      {
         ilog( "duplicate entries has been removed" );
         std::cout<<"duplicate entries has been removed"<<std::endl;
      }

      chain::account_object from_account = get_account(from);
      chain::account_id_type from_id = from_account.id;
      chain::message_payload pl;
      pl.from = from_id;
      pl.pub_from = chain::public_key_type();

      for (const auto &receiver : unique_to) {
         chain::account_object to_account = get_account(receiver);
         pl.receivers_data.emplace_back(text, chain::private_key_type(), chain::public_key_type(), to_account.get_id());
      }

      chain::custom_operation cust_op;
      cust_op.id = chain::custom_operation::custom_operation_subtype_messaging;
      cust_op.payer = from_id;
      cust_op.set_messaging_payload(pl);

      chain::signed_transaction tx;
      tx.operations.push_back(cust_op);

      set_operation_fees(tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction(tx, broadcast);

   } FC_CAPTURE_AND_RETHROW((from)(to)(text)(broadcast))
}

void wallet_api_impl::use_monitoring_api()
{
   if( _remote_monitoring )
      return;
   try
   {
      _remote_monitoring = _remote_api->monitoring();
   }
   catch( const fc::exception& e )
   {
      std::cerr << "\nCouldn't get monitoring API. You probably are not configured\n"
      "to access the monitoring API on the decentd you are\n"
      "connecting to.  Please follow the instructions in README.md to set up an apiaccess file.\n"
      "\n";
      throw(e);
   }
}

void wallet_api_impl::reset_counters(const std::vector<std::string>& names)
{
   use_monitoring_api();
   (*_remote_monitoring)->reset_counters(names);
}

std::vector<monitoring::counter_item_cli> wallet_api_impl::get_counters(const std::vector<std::string>& names)
{
   use_monitoring_api();
   std::vector<monitoring::counter_item_cli> cli_result;
   std::vector<monitoring::counter_item> result;
   result = (*_remote_monitoring)->get_counters(names);
   std::for_each(result.begin(), result.end(), [&](monitoring::counter_item& item) {
      monitoring::counter_item_cli item_cli;
      item_cli.name = item.name;
      item_cli.value = item.value;
      item_cli.last_reset = fc::time_point_sec(item.last_reset);
      item_cli.persistent = item.persistent;
      cli_result.push_back(item_cli);
   });

   std::sort(cli_result.begin(), cli_result.end(), [&](monitoring::counter_item_cli& item1, monitoring::counter_item_cli& item2) { return item1.name < item2.name; });
   return cli_result;
}

void wallet_api_impl::use_network_node_api()
{
   if( _remote_net_node )
      return;
   try
   {
      _remote_net_node = _remote_api->network_node();
   }
   catch( const fc::exception& e )
   {
      std::cerr << "\nCouldn't get network node API. You probably are not configured\n"
      "to access the network API on the decentd you are\n"
      "connecting to.  Please follow the instructions in README.md to set up an apiaccess file.\n"
      "\n";
      throw(e);
   }
}

void wallet_api_impl::network_add_nodes(const std::vector<std::string>& nodes)
{
   use_network_node_api();
   for( const std::string& node_address : nodes )
   {
      (*_remote_net_node)->add_node( fc::ip::endpoint::resolve_string( node_address ).back() );
   }
}

fc::variants wallet_api_impl::network_get_connected_peers()
{
   use_network_node_api();
   const auto peers = (*_remote_net_node)->get_connected_peers();
   fc::variants result;
   result.reserve( peers.size() );
   for( const auto& peer : peers )
   {
      fc::variant v;
      fc::to_variant( peer, v );
      result.push_back( v );
   }
   return result;
}

void submit_transfer_listener::package_creation_complete()
{
   uint64_t size = std::max((uint64_t)1, ( _info->get_size() + (1024 * 1024) -1 ) / (1024 * 1024));

   chain::asset total_price_per_day;
   for( chain::account_id_type account : _op.seeders )
   {
      fc::optional<chain::seeder_object> s = _wallet._remote_db->get_seeder( account );
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

void submit_transfer_listener::package_seed_complete()
{
   _op.URI = _info->get_url();

   chain::signed_transaction tx;
   tx.operations.push_back( _op );
   _wallet.set_operation_fees( tx, _wallet._remote_db->get_global_properties().parameters.current_fees);
   tx.validate();
   _wallet.sign_transaction(tx, true);
   _is_finished = true;
}

void submit_transfer_listener::package_creation_error(const std::string& msg)
{
   elog("Package creation error: ${msg}", ("msg", msg));
}

} } } // namespace graphene::wallet::detail
