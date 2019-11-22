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
#include <graphene/wallet/wallet_utility.hpp>
#include <graphene/utilities/keys_generator.hpp>
#include <fc/crypto/aes.hpp>
#include <fc/crypto/hex.hpp>
#include <fc/io/console.hpp>

namespace graphene { namespace wallet {

wallet_api::wallet_api( const fc::api<app::login_api> &rapi, const chain::chain_id_type &chain_id, const server_data &ws )
   : my(new detail::wallet_api_impl(*this, rapi, chain_id, ws))
{
}

wallet_api::~wallet_api()
{
}

wallet_about wallet_api::about() const
{
   return { my->_remote_db->about(), decent::get_about_info() };
}

fc::optional<chain::signed_block_with_info> wallet_api::get_block(uint32_t num) const
{
   return my->_remote_db->get_block(num);
}

chain::global_property_object wallet_api::get_global_properties() const
{
   return my->_remote_db->get_global_properties();
}

chain::dynamic_global_property_object wallet_api::get_dynamic_global_properties() const
{
   return my->_remote_db->get_dynamic_global_properties();
}

fc::variant wallet_api::get_object(db::object_id_type id) const
{
   return my->_remote_db->get_objects({id});
}

wallet_info wallet_api::info() const
{
   auto global_props = get_global_properties();
   auto dynamic_props = get_dynamic_global_properties();

   wallet_info result;
   result.head_block_num = dynamic_props.head_block_number;
   result.head_block_id = dynamic_props.head_block_id;
   result.head_block_age = fc::get_approximate_relative_time_string(dynamic_props.time, fc::time_point_sec(fc::time_point::now()), " old");
   result.next_maintenance_time = fc::get_approximate_relative_time_string(dynamic_props.next_maintenance_time);
   result.chain_id = my->_chain_id;
   result.participation = (100*dynamic_props.recent_slots_filled.popcount()) / 128.0;
   result.active_miners = global_props.active_miners;
   return result;
}

std::string wallet_api::help() const
{
   std::stringstream ss;
   for (const std::string& method_name : my->method_documentation.get_method_names())
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

std::string wallet_api::get_help(const std::string& method) const
{
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
      ss << fc::json::to_pretty_string(chain::monitored_asset_options());
   }
   else
   {
      std::string doxygenHelpString = my->method_documentation.get_brief_description(method).append(my->method_documentation.get_detailed_description(method));
      if (!doxygenHelpString.empty())
      {
         auto i = doxygenHelpString.find_first_not_of(" \t\n");
         ss << doxygenHelpString.substr(i == std::string::npos ? 0 : i);
      }
      else
         ss << "No help defined for method " << method << "\n";
   }

   return ss.str();
}

fc::time_point_sec wallet_api::head_block_time() const
{
   return my->_remote_db->head_block_time();
}

void wallet_api::network_add_nodes(const std::vector<std::string>& nodes) const
{
   my->network_add_nodes( nodes );
}

fc::variants wallet_api::network_get_connected_peers() const
{
   return my->network_get_connected_peers();
}

std::string wallet_api::sign_buffer(const std::string& str_buffer, const std::string& str_brainkey) const
{
   if(str_buffer.empty() || str_brainkey.empty())
      FC_THROW_EXCEPTION(need_buffer_and_brainkey_exception, "");

   std::string normalized_brain_key = utilities::normalize_brain_key( str_brainkey );
   chain::private_key_type privkey = utilities::derive_private_key( normalized_brain_key );

   fc::sha256 digest(str_buffer);

   auto sign = privkey.sign_compact(digest);

   return fc::to_hex((const char*)sign.begin(), sign.size());
}

bool wallet_api::verify_signature(const std::string& str_buffer, const std::string& str_publickey, const std::string& str_signature) const
{
   if (str_buffer.empty() ||
       str_publickey.empty() ||
       str_signature.empty())
      throw std::runtime_error("You need buffer, public key and signature to verify");

   fc::ecc::compact_signature signature;
   fc::from_hex(str_signature, (char*)signature.begin(), signature.size());
   fc::sha256 digest(str_buffer);

   chain::public_key_type pub_key(signature, digest);
   chain::public_key_type provided_key(str_publickey);

   return (provided_key == pub_key);
}

chain::transaction_id_type wallet_api::get_transaction_id(const chain::signed_transaction& trx) const
{
   return trx.id();
}

fc::optional<chain::processed_transaction> wallet_api::get_transaction_by_id(chain::transaction_id_type id) const
{
   return my->_remote_db->get_transaction_by_id( id );
}

std::vector<app::operation_info> wallet_api::list_operations() const
{
   return my->_remote_db->list_operations();
}

std::string wallet_api::from_command_file(const std::string& command_file_name) const
{
   std::string result;
   WalletAPI wapi;
   bool contains_submit_content_async = false;

   try
   {
      std::ifstream cf_in(command_file_name);
      std::string current_line;

      if (!cf_in.good())
      {
         FC_THROW_EXCEPTION(fc::file_not_found_exception, "File: ${f}", ("f", command_file_name));
      }

      wapi.Connect(my->get_wallet_filename(), { my->_wallet.ws_server, my->_wallet.ws_user, my->_wallet.ws_password });

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

               const std::string& method = args.front().get_string();

               if (method == "from_command_file")
               {
                  FC_THROW("Method from_command_file cannot be called from within a command file");
               }

               result += wapi.RunTask(current_line) + "\n";

               if (method == "submit_content_async")
               {
                  contains_submit_content_async = true;
               }
         }
      }

      if (contains_submit_content_async)
      {
         // hold on and periodically check if all package manager listeners are in a final state
         while (wapi.exec(&wallet_api::is_package_manager_task_waiting))
         {
               std::this_thread::sleep_for(std::chrono::milliseconds(100));
         }
      }

   } FC_CAPTURE_AND_RETHROW( (command_file_name) )

   return result;
}

std::vector<chain::account_object> wallet_api::list_my_accounts() const
{
   return std::vector<chain::account_object>(my->_wallet.my_accounts.begin(), my->_wallet.my_accounts.end());
}

boost::filesystem::path wallet_api::get_wallet_filename() const
{
   return my->get_wallet_filename();
}

void wallet_api::set_wallet_filename(const boost::filesystem::path& wallet_filename)
{
   return my->set_wallet_filename(wallet_filename);
}

std::string wallet_api::get_private_key(const chain::public_key_type& pubkey) const
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");

   return utilities::key_to_wif( my->get_private_key( pubkey ) );
}

bool wallet_api::is_new()const
{
   return my->_wallet.cipher_keys.size() == 0;
}

bool wallet_api::is_locked()const
{
   return my->is_locked();
}

bool wallet_api::lock()
{ try {
   if(is_locked()) {
      std::cout << "Wallet is already locked" << std::endl;
      return false;
   }

   my->encrypt_keys2();
   for( auto & key : my->_keys )
      key.second = utilities::key_to_wif(chain::private_key_type());
   my->_keys.clear();
   my->_el_gamal_keys.clear();
   my->_checksum = fc::sha512();
   my->self.lock_changed(true);
   return true;
} FC_RETHROW() }

bool wallet_api::unlock(const std::string& password)
{ try {
   if(!is_locked()) {
      std::cout << "Wallet is already unlocked" << std::endl;
      return false;
   }

   if(password.size() == 0)
      FC_THROW_EXCEPTION(password_cannot_be_empty_exception, "");

   auto pw = fc::sha512::hash(password.c_str(), password.size());
   std::vector<char> decrypted = fc::aes_decrypt(pw, my->_wallet.cipher_keys);
   plain_ec_and_el_gamal_keys pk;
   bool update_wallet_file = false;

   if (my->_wallet.version == 0) {

      // supporting backward compatibility of wallet json file
      try {
         std::string data;
         data.reserve(decrypted.size());
         std::copy(decrypted.begin(), decrypted.end(), back_inserter(data));
         pk = fc::json::from_string(data).as<plain_ec_and_el_gamal_keys>();
         update_wallet_file = true;
      }
      catch(const fc::exception&)
      {
         pk = fc::raw::unpack<plain_keys>(decrypted);
         // wallet file is in old format, derive corresponding el gamal keys from private keys
         for( const auto& element : pk.ec_keys )
         {
            el_gamal_key_pair_str el_gamal_keys_str;
            el_gamal_keys_str.private_key = decent::encrypt::generate_private_el_gamal_key_from_secret( utilities::wif_to_key( element.second )->get_secret() );
            el_gamal_keys_str.public_key = get_public_el_gamal_key( el_gamal_keys_str.private_key );
            pk.el_gamal_keys.push_back( el_gamal_keys_str );
         }
         update_wallet_file = true;
      }
   }
   else {

      try {
         std::string data;
         data.reserve(decrypted.size());
         std::copy(decrypted.begin(), decrypted.end(), back_inserter(data));
         pk = fc::json::from_string(data).as<plain_ec_and_el_gamal_keys>();
      }
      catch(const fc::exception&) {
         throw;
      }
   }

   FC_ASSERT(pk.checksum == pw);
   my->_keys = std::move(pk.ec_keys);
   if( !pk.el_gamal_keys.empty() ) {
      std::transform(pk.el_gamal_keys.begin(), pk.el_gamal_keys.end(),
                     std::inserter(my->_el_gamal_keys, my->_el_gamal_keys.end()),
                     [](const el_gamal_key_pair_str el_gamal_pair) {
                         return std::make_pair(decent::encrypt::DInteger(el_gamal_pair.public_key), decent::encrypt::DInteger(el_gamal_pair.private_key));
                     });
   }
   my->_checksum = pk.checksum;

   if( update_wallet_file ) // upgrade structure for storing keys to new format
      save_wallet_file();

   my->self.lock_changed(false);
   return true;
} FC_RETHROW() }

void wallet_api::set_password(const std::string& password)
{
   if(!is_new()) {
      if(my->is_locked())
         FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   }
   my->_checksum = fc::sha512::hash( password.c_str(), password.size() );
   lock();
}

bool wallet_api::load_wallet_file(const boost::filesystem::path& wallet_filename)
{
   if( !wallet_filename.empty() )
      my->set_wallet_filename(wallet_filename);
   return my->load_wallet_file( wallet_filename );
}

void wallet_api::save_wallet_file(const boost::filesystem::path& wallet_filename)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   my->save_wallet_file( wallet_filename );
}

bool wallet_api::import_key(const std::string& account_name_or_id, const std::string& wif_key)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   bool result = my->import_key(account_name_or_id, wif_key);
   save_wallet_file();

   return result;
}

bool wallet_api::import_single_key(const std::string& account_name_or_id, const std::string& wif_key)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   bool result = my->import_single_key(account_name_or_id, wif_key);
   save_wallet_file();

   return result;
}

fc::variant wallet_api::dump_private_keys() const
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   fc::mutable_variant_object result;
   result["ec_keys"] = my->_keys;
   result["el_gamal_keys"] = my->_el_gamal_keys;   // map of public keys to private keys

   return result;
}

std::string wallet_api::derive_private_key(const std::string& prefix_string, int sequence_number) const
{
   chain::private_key_type private_key = graphene::utilities::derive_private_key( prefix_string, sequence_number );
   return graphene::utilities::key_to_wif( private_key );
}

graphene::chain::public_key_type wallet_api::get_public_key(const std::string& wif_private_key) const
{
   fc::optional<chain::private_key_type> private_key = graphene::utilities::wif_to_key( wif_private_key );
   if(!private_key)
      FC_THROW_EXCEPTION(invalid_wif_private_key_exception, "");

   return private_key->get_public_key();
}

uint64_t wallet_api::get_account_count() const
{
   return my->_remote_db->get_account_count();
}

std::map<std::string, chain::account_id_type> wallet_api::list_accounts(const std::string& lowerbound, uint32_t limit) const
{
    return my->_remote_db->lookup_accounts(lowerbound, limit);
}

std::vector<chain::account_object> wallet_api::search_accounts(const std::string& term, const std::string& order, const std::string& id, uint32_t limit) const
{
   return my->_remote_db->search_accounts(term, order, db::object_id_type(id), limit);
}

std::vector<extended_asset> wallet_api::list_account_balances(const std::string& id) const
{
   std::vector<chain::asset> assets;
   if( auto real_id = detail::maybe_id<chain::account_id_type>(id) )
      assets = my->_remote_db->get_account_balances(*real_id, boost::container::flat_set<chain::asset_id_type>());
   assets = my->_remote_db->get_account_balances(get_account(id).id, boost::container::flat_set<chain::asset_id_type>());

   std::vector<extended_asset> result;
   std::vector<chain::asset_id_type> asset_ids;
   result.reserve( assets.size() );
   asset_ids.reserve( assets.size() );

   for( const chain::asset& element : assets )
      asset_ids.push_back( element.asset_id );

   std::vector<fc::optional<chain::asset_object>> asset_objs = my->_remote_db->get_assets( asset_ids ) ;

   for( size_t i = 0; i < assets.size(); i++ )
      result.emplace_back( assets[i], asset_objs[i]->amount_to_pretty_string( assets[i].amount ) );
   FC_ASSERT( assets.size() == result.size() );

   return result;
}

std::vector<operation_detail> wallet_api::get_account_history(const std::string& name, int limit) const
{
   std::vector<operation_detail> result;
   auto account_id = get_account(name).get_id();

   while( limit > 0 )
      {
      chain::operation_history_id_type start;
      if( result.size() )
      {
         start = result.back().op.id;
         start = start + 1;
      }

      std::vector<chain::operation_history_object> current = my->_remote_hist->get_account_history(account_id, chain::operation_history_id_type(), std::min(100,limit), start);
      for( auto& o : current ) {
         std::stringstream ss;
         auto memo = o.op.visit(detail::operation_printer(ss, *my, o.result));
         result.push_back( operation_detail{ memo, ss.str(), o } );
      }
      if( (int)current.size() < std::min(100,limit) )
         break;
      limit -= static_cast<int>(current.size());
   }

   return result;
}

std::vector<balance_change_result_detail> wallet_api::search_account_balance_history(const std::string& account_name,
                                                                                     const boost::container::flat_set<std::string>& assets_list,
                                                                                     const std::string& partner_account,
                                                                                     uint32_t from_block, uint32_t to_block,
                                                                                     uint32_t start_offset,
                                                                                     int limit) const
{
    std::vector<balance_change_result_detail> result;
    auto account_id = get_account(account_name).get_id();

    if( limit > 0 )
    {
       boost::container::flat_set<chain::asset_id_type> asset_id_list;
       if (!assets_list.empty()) {
           for( const auto& item : assets_list) {
              asset_id_list.insert( get_asset(item).get_id() );
           }
       }

       fc::optional<chain::account_id_type> partner_id;
       if (!partner_account.empty()) {
          partner_id = get_account(partner_account).get_id();
       }

       std::vector<app::balance_change_result> current = my->_remote_hist->search_account_balance_history(account_id, asset_id_list, partner_id, from_block, to_block, start_offset, limit);
       result.reserve( current.size() );
       for(const auto& item : current) {
          balance_change_result_detail info;
          info.hist_object = item.hist_object;
          info.balance     = item.balance;
          info.fee         = item.fee;
          info.timestamp   = item.timestamp;
          info.transaction_id = item.transaction_id;

          std::stringstream ss;
          info.memo = item.hist_object.op.visit(detail::operation_printer(ss, *my, item.hist_object.result));

          result.push_back(info);
       }
    }

    return result;
}

fc::optional<balance_change_result_detail> wallet_api::get_account_balance_for_transaction(const std::string& account_name, chain::operation_history_id_type operation_history_id) const
{
   auto account_id = get_account(account_name).get_id();

   fc::optional<app::balance_change_result> result = my->_remote_hist->get_account_balance_for_transaction(account_id, operation_history_id);
   if (!result) {
      return fc::optional<balance_change_result_detail>();
   }

   balance_change_result_detail info;
   info.hist_object = result->hist_object;
   info.balance     = result->balance;
   info.fee         = result->fee;

   std::stringstream ss;
   info.memo = result->hist_object.op.visit(detail::operation_printer(ss, *my, result->hist_object.result));

   return info;
}

std::vector<operation_detail> wallet_api::get_relative_account_history(const std::string& name, uint32_t stop, int limit, uint32_t start) const
{
   std::vector<operation_detail> result;
   auto account_id = get_account(name).get_id();

   std::vector<chain::operation_history_object> current = my->_remote_hist->get_relative_account_history(account_id, stop, limit, start);
   for( auto& o : current ) {
      std::stringstream ss;
      auto memo = o.op.visit(detail::operation_printer(ss, *my, o.result));
      result.push_back( operation_detail{ memo, ss.str(), o } );
   }

   return result;
}

std::vector<chain::transaction_detail_object> wallet_api::search_account_history(const std::string& account_name, const std::string& order, const std::string& id, int limit) const
{
   std::vector<chain::transaction_detail_object> result;
   try
   {
      chain::account_object account = get_account(account_name);
      result = my->_remote_db->search_account_history(account.id, order, db::object_id_type(id), limit);

      for (auto& item : result)
      {
         auto const& memo = item.m_transaction_encrypted_memo;
         if (memo)
         {
            item.m_str_description += " - ";
            auto it = my->_keys.find(memo->to);
            auto it2 = it == my->_keys.end() ? my->_keys.find(memo->from) : it;

            if (it2 == my->_keys.end())
               // memo is encrypted for someone else
               item.m_str_description += "{encrypted}";
            else
               // here the memo is encrypted for/by me so I can decrypt it
               item.m_str_description += memo->get_message(*utilities::wif_to_key(it2->second), it == it2 ? memo->from : memo->to);
         }
      }
   }
   catch(...){}

   return result;
}

chain::account_object wallet_api::get_account(const std::string& account_name_or_id) const
{
   return my->get_account(account_name_or_id);
}

brain_key_info wallet_api::suggest_brain_key() const
{
   brain_key_info result;
   result.brain_priv_key = utilities::generate_brain_key();

   chain::private_key_type priv_key = utilities::derive_private_key(result.brain_priv_key);
   result.wif_priv_key = utilities::key_to_wif( priv_key );
   result.pub_key = priv_key.get_public_key();
   return result;
}

signed_transaction_info wallet_api::register_account_with_keys(const std::string& name,
                                                               const chain::public_key_type& owner,
                                                               const chain::public_key_type& active,
                                                               const chain::public_key_type& memo,
                                                               const std::string& registrar_account,
                                                               bool broadcast /* = false */)
{
   if(is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->register_account( name, owner, active, memo, registrar_account,  broadcast );
}

signed_transaction_info wallet_api::register_account(const std::string& name,
                                                     const chain::public_key_type& owner,
                                                     const chain::public_key_type& active,
                                                     const std::string& registrar_account,
                                                     bool broadcast /* = false */)
{
   if(is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->register_account( name, owner, active, active, registrar_account, broadcast );
}

signed_transaction_info wallet_api::register_multisig_account(const std::string& name,
                                                              const chain::authority& owner,
                                                              const chain::authority& active,
                                                              const chain::public_key_type& memo,
                                                              const std::string& registrar_account,
                                                              bool broadcast /* = false */)
{
   if(is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->register_multisig_account( name, owner, active, memo, registrar_account,  broadcast );
}

signed_transaction_info wallet_api::create_account_with_brain_key(const std::string& brain_key,
                                                                  const std::string& account_name,
                                                                  const std::string& registrar_account,
                                                                  bool broadcast /* = false */)
{
   if(is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->create_account_with_brain_key( brain_key, account_name, registrar_account, true, broadcast);
}

signed_transaction_info wallet_api::update_account_keys(const std::string& name,
                                                        const std::string& owner,
                                                        const std::string& active,
                                                        const std::string& memo,
                                                        bool broadcast /* = false */)
{
   if(is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   fc::optional<chain::authority> new_owner, new_active;
   fc::optional<chain::public_key_type> new_memo;
   chain::account_object acc = my->get_account( name );

   if( !owner.empty() )
      new_owner = chain::authority( 1, chain::public_key_type( owner ), chain::weight_type( 1 ) );

   if( !active.empty() )
      new_active = chain::authority( 1, chain::public_key_type( active ), chain::weight_type( 1 ) );

   if( !memo.empty() )
      new_memo = chain::public_key_type( memo );

   return my->update_account_keys( name, new_owner, new_active, new_memo, broadcast );
}

signed_transaction_info wallet_api::update_account_keys_to_multisig(const std::string& name,
                                                                    const chain::authority& owner,
                                                                    const chain::authority& active,
                                                                    const chain::public_key_type& memo,
                                                                    bool broadcast /* = false */)
{
   fc::optional<chain::authority> new_owner, new_active;
   fc::optional<chain::public_key_type> new_memo;
   chain::account_object acc = my->get_account( name );

   if( acc.owner != owner )
      new_owner = owner;

   if( acc.active != active )
      new_active = active;

   if( acc.options.memo_key != memo )
      new_memo = memo;

   if(!new_owner && !new_active && !new_memo)
      FC_THROW_EXCEPTION(new_auth_needs_to_be_different_from_existing_exception, "");

   return my->update_account_keys( name, new_owner, new_active, new_memo, broadcast );
}

el_gamal_key_pair wallet_api::generate_el_gamal_keys() const
{
   el_gamal_key_pair ret;
   ret.private_key = decent::encrypt::generate_private_el_gamal_key();
   ret.public_key = decent::encrypt::get_public_el_gamal_key( ret.private_key );
   return ret;
}

el_gamal_key_pair_str wallet_api::get_el_gammal_key(const std::string& consumer) const
{
   try
   {
      if(is_locked())
         FC_THROW_EXCEPTION(wallet_is_locked_exception, "");

      chain::account_object consumer_account = get_account( consumer );
      el_gamal_key_pair_str res;

      res.private_key = decent::encrypt::generate_private_el_gamal_key_from_secret ( my->get_private_key_for_account(consumer_account).get_secret() );
      res.public_key = decent::encrypt::get_public_el_gamal_key( res.private_key );
      return res;
   } FC_CAPTURE_AND_RETHROW( (consumer) )
}

std::pair<brain_key_info, el_gamal_key_pair> wallet_api::generate_brain_key_el_gamal_key() const
{
   std::pair<brain_key_info, el_gamal_key_pair> ret;
   ret.first = suggest_brain_key();

   fc::optional<chain::private_key_type> op_private_key = utilities::wif_to_key(ret.first.wif_priv_key);
   FC_ASSERT(op_private_key);
   ret.second.private_key = decent::encrypt::generate_private_el_gamal_key_from_secret ( op_private_key->get_secret() );
   ret.second.public_key = decent::encrypt::get_public_el_gamal_key( ret.second.private_key );

   return ret;
}

brain_key_info wallet_api::get_brain_key_info(const std::string& brain_key) const
{
   brain_key_info result;
   result.brain_priv_key = graphene::utilities::normalize_brain_key( brain_key );

   chain::private_key_type priv_key = graphene::utilities::derive_private_key( result.brain_priv_key );
   result.wif_priv_key = utilities::key_to_wif( priv_key );
   result.pub_key = priv_key.get_public_key();
   return result;
}

signed_transaction_info wallet_api::transfer(const std::string& from,
                                             const std::string& to,
                                             const std::string& amount,
                                             const std::string& asset_symbol,
                                             const std::string& memo,
                                             bool broadcast /* = false */)
{
   if(is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->transfer(from, to, amount, asset_symbol, memo, broadcast);
}

std::vector<chain::asset_object> wallet_api::list_assets(const std::string& lowerbound, uint32_t limit) const
{
   return my->_remote_db->list_assets( lowerbound, limit );
}

chain::asset_object wallet_api::get_asset(const std::string& asset_name_or_id) const
{
   auto a = my->find_asset(asset_name_or_id);
   if(!a)
      FC_THROW_EXCEPTION(asset_not_found_exception, "");

   return *a;
}

chain::monitored_asset_options wallet_api::get_monitored_asset_data(const std::string& asset_name_or_id) const
{
   auto asset = get_asset(asset_name_or_id);
   if(!asset.is_monitored_asset())
      FC_THROW_EXCEPTION(asset_not_monitored_exception, "");

   return *asset.monitored_asset_opts;
}

signed_transaction_info wallet_api::create_monitored_asset(const std::string& issuer,
                                                           const std::string& symbol,
                                                           uint8_t precision,
                                                           const std::string& description,
                                                           uint32_t feed_lifetime_sec,
                                                           uint8_t minimum_feeds,
                                                           bool broadcast)

{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");

   return my->create_monitored_asset(issuer, symbol, precision, description, feed_lifetime_sec, minimum_feeds, broadcast);
}

signed_transaction_info wallet_api::update_monitored_asset(const std::string& symbol,
                                                           const std::string& description,
                                                           uint32_t feed_lifetime_sec,
                                                           uint8_t minimum_feeds,
                                                           bool broadcast /* = false */)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->update_monitored_asset(symbol, description, feed_lifetime_sec, minimum_feeds, broadcast);
}

signed_transaction_info wallet_api::create_user_issued_asset(const std::string& issuer,
                                                             const std::string& symbol,
                                                             uint8_t precision,
                                                             const std::string& description,
                                                             uint64_t max_supply,
                                                             chain::price core_exchange_rate,
                                                             bool is_exchangeable,
                                                             bool is_fixed_max_supply,
                                                             bool broadcast /* = false */)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->create_user_issued_asset(issuer, symbol, precision, description, max_supply, core_exchange_rate, is_exchangeable, is_fixed_max_supply, broadcast);
}

signed_transaction_info wallet_api::issue_asset(const std::string& to_account,
                                                const std::string& amount,
                                                const std::string& symbol,
                                                const std::string& memo,
                                                bool broadcast)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->issue_asset(to_account, amount, symbol, memo, broadcast);
}

signed_transaction_info wallet_api::update_user_issued_asset(const std::string& symbol,
                                                             const std::string& new_issuer,
                                                             const std::string& description,
                                                             uint64_t max_supply,
                                                             chain::price core_exchange_rate,
                                                             bool is_exchangeable,
                                                             bool broadcast /* = false */)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->update_user_issued_asset(symbol, new_issuer, description, max_supply, core_exchange_rate, is_exchangeable, broadcast);
}

signed_transaction_info wallet_api::fund_asset_pools(const std::string& from,
                                                     const std::string& uia_amount,
                                                     const std::string& uia_symbol,
                                                     const std::string& DCT_amount,
                                                     const std::string& DCT_symbol,
                                                     bool broadcast /* = false */)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->fund_asset_pools(from, uia_amount, uia_symbol, DCT_amount, DCT_symbol, broadcast);
}

signed_transaction_info wallet_api::reserve_asset(const std::string& from,
                                                  const std::string& amount,
                                                  const std::string& symbol,
                                                  bool broadcast /* = false */)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->reserve_asset(from, amount, symbol, broadcast);
}

std::string wallet_api::price_to_dct(const std::string& amount, const std::string& asset_symbol_or_id) const
{
   chain::asset_object price_o = my->get_asset(asset_symbol_or_id);
   chain::asset price = price_o.amount_from_string(amount);
   chain::asset result = my->_remote_db->price_to_dct(price);
   return std::to_string(result.amount.value);
}

signed_transaction_info wallet_api::claim_fees(const std::string& uia_amount,
                                               const std::string& uia_symbol,
                                               const std::string& dct_amount,
                                               const std::string& dct_symbol,
                                               bool broadcast /* = false */)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->claim_fees( uia_amount, uia_symbol, dct_amount, dct_symbol, broadcast);
}

signed_transaction_info wallet_api::publish_asset_feed(const std::string& publishing_account,
                                                       const std::string& symbol,
                                                       const chain::price_feed& feed,
                                                       bool broadcast /* = false */)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->publish_asset_feed(publishing_account, symbol, feed, broadcast);
}

std::multimap<fc::time_point_sec, chain::price_feed> wallet_api::get_feeds_by_miner(const std::string& account_name_or_id, uint32_t count) const
{
   chain::account_id_type account_id = get_account( account_name_or_id ).id;
   return my->_remote_db->get_feeds_by_miner( account_id, count );
}

chain::real_supply wallet_api::get_real_supply() const
{
   return my->_remote_db->get_real_supply();
}

transaction_handle_type wallet_api::begin_builder_transaction()
{
   return my->begin_builder_transaction();
}

void wallet_api::add_operation_to_builder_transaction(transaction_handle_type transaction_handle, const chain::operation& op)
{
   my->add_operation_to_builder_transaction(transaction_handle, op);
}

void wallet_api::replace_operation_in_builder_transaction(transaction_handle_type handle, unsigned operation_index, const chain::operation& new_op)
{
   my->replace_operation_in_builder_transaction(handle, operation_index, new_op);
}

chain::asset wallet_api::set_fees_on_builder_transaction(transaction_handle_type handle, const std::string& fee_asset)
{
   return my->set_fees_on_builder_transaction(handle, fee_asset);
}

chain::transaction wallet_api::preview_builder_transaction(transaction_handle_type handle)
{
   return my->preview_builder_transaction(handle);
}

signed_transaction_info wallet_api::sign_builder_transaction(transaction_handle_type transaction_handle, bool broadcast)
{
   if(is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->sign_builder_transaction(transaction_handle, broadcast);
}

signed_transaction_info wallet_api::propose_builder_transaction(transaction_handle_type handle,
                                                                fc::time_point_sec expiration,
                                                                uint32_t review_period_seconds,
                                                                bool broadcast)
{
   if(is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->propose_builder_transaction(handle, expiration, review_period_seconds, broadcast);
}

signed_transaction_info wallet_api::propose_builder_transaction2(transaction_handle_type handle,
                                                                 const std::string& account_name_or_id,
                                                                 fc::time_point_sec expiration,
                                                                 uint32_t review_period_seconds,
                                                                 bool broadcast)
{
   if(is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->propose_builder_transaction2(handle, account_name_or_id, expiration, review_period_seconds, broadcast);
}

void wallet_api::remove_builder_transaction(transaction_handle_type handle)
{
   return my->remove_builder_transaction(handle);
}

std::string wallet_api::serialize_transaction(const chain::signed_transaction& tx) const
{
   return fc::to_hex(fc::raw::pack(tx));
}

signed_transaction_info wallet_api::sign_transaction(const chain::transaction& tx, bool broadcast /* = false */)
{
    try {
       if(is_locked())
          FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
       return my->sign_transaction( tx, broadcast );
    } FC_CAPTURE_AND_RETHROW( (tx) )
}

chain::operation wallet_api::get_prototype_operation(const std::string& operation_name) const
{
   auto it = my->_prototype_ops.find( operation_name );
   if(it == my->_prototype_ops.end())
      FC_THROW_EXCEPTION(unsupported_operation_exception, "Operation name: ${operation_name}", ("operation_name", operation_name));

   return it->second;
}

std::map<std::string, chain::miner_id_type> wallet_api::list_miners(const std::string& lowerbound, uint32_t limit) const
{
   return my->_remote_db->lookup_miner_accounts(lowerbound, limit);
}

chain::miner_object wallet_api::get_miner(const std::string& owner_account) const
{
   return my->get_miner(owner_account);
}

signed_transaction_info wallet_api::create_miner(const std::string& owner_account,
                                                 const std::string& url,
                                                 bool broadcast /* = false */)
{
   if(is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->create_miner(owner_account, url, broadcast);
}

signed_transaction_info wallet_api::update_miner(const std::string& miner_name,
                                                 const std::string& url,
                                                 const std::string& block_signing_key,
                                                 bool broadcast /* = false */)
{
   if(is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->update_miner(miner_name, url, block_signing_key, broadcast);
}

std::vector<vesting_balance_object_with_info> wallet_api::get_vesting_balances(const std::string& account_name) const
{
   return my->get_vesting_balances( account_name );
}

signed_transaction_info wallet_api::withdraw_vesting(const std::string& miner_name,
                                                     const std::string& amount,
                                                     const std::string& asset_symbol,
                                                     bool broadcast /* = false */)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->withdraw_vesting( miner_name, amount, asset_symbol, broadcast );
}

signed_transaction_info wallet_api::vote_for_miner(const std::string& voting_account,
                                                   const std::string& miner,
                                                   bool approve,
                                                   bool broadcast /* = false */)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->vote_for_miner(voting_account, miner, approve, broadcast);
}

signed_transaction_info wallet_api::set_voting_proxy(const std::string& account_to_modify,
                                                     fc::optional<std::string> voting_account,
                                                     bool broadcast /* = false */)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->set_voting_proxy(account_to_modify, voting_account, broadcast);
}

signed_transaction_info wallet_api::set_desired_miner_count(const std::string& account_to_modify,
                                                            uint16_t desired_number_of_miners,
                                                            bool broadcast /* = false */)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->set_desired_miner_count(account_to_modify, desired_number_of_miners, broadcast);
}

std::vector<app::miner_voting_info> wallet_api::search_miner_voting(const std::string& account_id,
                                                                    const std::string& term,
                                                                    bool only_my_votes,
                                                                    const std::string& order,
                                                                    const std::string& id,
                                                                    uint32_t count) const
      {
   return my->_remote_db->search_miner_voting(account_id, term, only_my_votes, order, id, count);
}

std::vector<chain::seeder_object> wallet_api::list_seeders_by_price(uint32_t count) const
{
   return my->_remote_db->list_seeders_by_price( count );
}

fc::optional<std::vector<chain::seeder_object>> wallet_api::list_seeders_by_upload(uint32_t count) const
{
   return my->_remote_db->list_seeders_by_upload( count );
}

std::vector<chain::seeder_object> wallet_api::list_seeders_by_region(const std::string& region_code) const
{
   return my->_remote_db->list_seeders_by_region( region_code );
}

std::vector<chain::seeder_object> wallet_api::list_seeders_by_rating(uint32_t count) const
{
   return my->_remote_db->list_seeders_by_rating( count );
}

std::vector<chain::proposal_object> wallet_api::get_proposed_transactions(const std::string& account_or_id) const
{
   chain::account_id_type id = get_account(account_or_id).get_id();
   return my->_remote_db->get_proposed_transactions( id );
}

signed_transaction_info wallet_api::propose_transfer(const std::string& proposer,
                                                     const std::string& from,
                                                     const std::string& to,
                                                     const std::string& amount,
                                                     const std::string& asset_symbol,
                                                     const std::string& memo,
                                                     fc::time_point_sec expiration)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->propose_transfer(proposer, from, to, amount, asset_symbol, memo, expiration);
}

signed_transaction_info wallet_api::propose_parameter_change(const std::string& proposing_account,
                                                             fc::time_point_sec expiration_time,
                                                             const fc::variant_object& changed_values,
                                                             bool broadcast /* = false */)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->propose_parameter_change( proposing_account, expiration_time, changed_values, broadcast );
}

signed_transaction_info wallet_api::propose_fee_change(const std::string& proposing_account,
                                                       fc::time_point_sec expiration_time,
                                                       const fc::variant_object& changed_fees,
                                                       bool broadcast /* = false */)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->propose_fee_change( proposing_account, expiration_time, changed_fees, broadcast );
}

signed_transaction_info wallet_api::approve_proposal(const std::string& fee_paying_account,
                                                     const std::string& proposal_id,
                                                     const approval_delta& delta,
                                                     bool broadcast /* = false */)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->approve_proposal( fee_paying_account, proposal_id, delta, broadcast );
}

signed_transaction_info wallet_api::submit_content(const std::string& author,
                                                   const std::vector<std::pair<std::string, uint32_t>>& co_authors,
                                                   const std::string& URI,
                                                   const std::vector<regional_price_info>& price_amounts,
                                                   uint64_t size,
                                                   const fc::ripemd160& hash,
                                                   const std::vector<chain::account_id_type>& seeders,
                                                   uint32_t quorum,
                                                   const fc::time_point_sec& expiration,
                                                   const std::string& publishing_fee_asset,
                                                   const std::string& publishing_fee_amount,
                                                   const std::string& synopsis,
                                                   const decent::encrypt::DInteger& secret,
                                                   const decent::encrypt::CustodyData& cd,
                                                   bool broadcast)
{
   FC_ASSERT( !my->is_locked(), "the wallet is locked and needs to be unlocked to have access to private keys" );
   return my->submit_content(author, co_authors, URI, price_amounts, hash, size,
                             seeders, quorum, expiration, publishing_fee_asset, publishing_fee_amount,
                             synopsis, secret, cd, broadcast);
}

chain::content_keys wallet_api::submit_content_async(const std::string& author,
                                              const std::vector<std::pair<std::string, uint32_t>>& co_authors,
                                              const std::string& content_dir,
                                              const std::string& samples_dir,
                                              const std::string& protocol,
                                              const std::vector<regional_price_info>& price_amounts,
                                              const std::vector<chain::account_id_type>& seeders,
                                              const fc::time_point_sec& expiration,
                                              const std::string& synopsis)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->submit_content_async(author, co_authors, content_dir, samples_dir, protocol, price_amounts, seeders, expiration, synopsis);
}

signed_transaction_info wallet_api::content_cancellation(const std::string& author,
                                                         const std::string& URI,
                                                         bool broadcast)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->content_cancellation(author, URI, broadcast);
}

void wallet_api::download_content(const std::string& consumer, const std::string& URI, const std::string& region_code_from, bool broadcast)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->download_content(consumer, URI, region_code_from, broadcast);
}

content_download_status wallet_api::get_download_status(const std::string& consumer, const std::string& URI) const
{
   return my->get_download_status(consumer, URI);
}

signed_transaction_info wallet_api::request_to_buy(const std::string& consumer,
                                                   const std::string& URI,
                                                   const std::string& price_asset_name,
                                                   const std::string& price_amount,
                                                   const std::string& str_region_code_from,
                                                   bool broadcast)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->request_to_buy(consumer, URI, price_asset_name, price_amount, str_region_code_from, broadcast);
}

signed_transaction_info wallet_api::leave_rating_and_comment(const std::string& consumer,
                                                             const std::string& URI,
                                                             uint64_t rating,
                                                             const std::string& comment,
                                                             bool broadcast)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->leave_rating_and_comment(consumer, URI, rating, comment, broadcast);
}

decent::encrypt::DInteger wallet_api::restore_encryption_key(const std::string& consumer, chain::buying_id_type buying)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->restore_encryption_key(consumer, buying);
}

decent::encrypt::DInteger wallet_api::generate_encryption_key() const
{
   CryptoPP::Integer secret(detail::wallet_api_impl::randomGenerator, 256);
   return secret;
}

std::vector<chain::buying_object> wallet_api::get_open_buyings() const
{
   return my->_remote_db->get_open_buyings();
}

std::vector<chain::buying_object> wallet_api::get_open_buyings_by_URI(const std::string& URI) const
{
   return my->_remote_db->get_open_buyings_by_URI( URI );
}

std::vector<chain::buying_object> wallet_api::get_open_buyings_by_consumer(const std::string& account_id_or_name) const
{
   chain::account_id_type consumer = get_account( account_id_or_name ).id;
   return my->_remote_db->get_open_buyings_by_consumer( consumer );
}

std::vector<chain::buying_object> wallet_api::get_buying_history_objects_by_consumer(const std::string& account_id_or_name) const
{
   chain::account_id_type consumer = get_account( account_id_or_name ).id;
   std::vector<chain::buying_object> result = my->_remote_db->get_buying_history_objects_by_consumer( consumer );

   for (int i = 0; i < (int)result.size(); ++i)
   {
      chain::buying_object& bobj = result[i];

      fc::optional<chain::content_object> content = my->_remote_db->get_content( bobj.URI );
      if (!content)
         continue;
      fc::optional<chain::asset> op_price = content->price.GetPrice(bobj.region_code_from);
      if (!op_price)
         continue;

      bobj.price = *op_price;

      bobj.size = content->size;
      bobj.rating = content->AVG_rating;
      bobj.synopsis = content->synopsis;

   }
   return result;
}

std::vector<buying_object_ex> wallet_api::search_my_purchases(const std::string& account_id_or_name,
                                                              const std::string& term,
                                                              const std::string& order,
                                                              const std::string& id,
                                                              uint32_t count) const
{
   chain::account_id_type consumer = get_account( account_id_or_name ).id;
   std::vector<chain::buying_object> bobjects = my->_remote_db->get_buying_objects_by_consumer(consumer, order, db::object_id_type(id), term, count );
   std::vector<buying_object_ex> result;

   for (size_t i = 0; i < bobjects.size(); ++i)
   {
      chain::buying_object const& buyobj = bobjects[i];
      content_download_status status = get_download_status(account_id_or_name, buyobj.URI);

      fc::optional<chain::content_object> content = my->_remote_db->get_content( buyobj.URI );
      if (!content)
         continue;

      result.emplace_back(buying_object_ex(bobjects[i], status));
      buying_object_ex& bobj = result.back();

      bobj.author_account = my->get_account(content->author).name;
      bobj.times_bought = content->times_bought;
      bobj.hash = content->_hash;
      bobj.AVG_rating = content->AVG_rating;
      bobj.rating = content->AVG_rating;
   }

   return result;
}

fc::optional<chain::buying_object> wallet_api::get_buying_by_consumer_URI(const std::string& account_id_or_name, const std::string& URI) const
{
   chain::account_id_type account = get_account( account_id_or_name ).id;
   return my->_remote_db->get_buying_by_consumer_URI( account, URI );
}

std::vector<rating_object_ex> wallet_api::search_feedback(const std::string& user, const std::string& URI, const std::string& id, uint32_t count) const
{
    std::vector<rating_object_ex> result;
    std::vector<chain::buying_object> temp = my->_remote_db->search_feedback(user, URI, db::object_id_type(id), count);

    for (auto const& item : temp)
       result.push_back(rating_object_ex( item, get_account(std::string(db::object_id_type(item.consumer))).name));

    return result;
}

fc::optional<chain::content_object> wallet_api::get_content(const std::string& URI) const
{
    return my->_remote_db->get_content( URI );
}

std::vector<chain::content_summary> wallet_api::search_content(const std::string& term,
                                                        const std::string& order,
                                                        const std::string& user,
                                                        const std::string& region_code,
                                                        const std::string& id,
                                                        const std::string& type,
                                                        uint32_t count) const
{
   return my->_remote_db->search_content(term, order, user, region_code, db::object_id_type(id), type, count);
}

std::vector<chain::content_summary> wallet_api::search_user_content(const std::string& user,
                                                             const std::string& term,
                                                             const std::string& order,
                                                             const std::string& region_code,
                                                             const std::string& id,
                                                             const std::string& type,
                                                             uint32_t count) const
{
  std::vector<chain::content_summary> result = my->_remote_db->search_content(term, order, user, region_code, db::object_id_type(id), type, count);

  auto packages = decent::package::PackageManager::instance().get_all_known_packages();
  for (auto package: packages)
  {
     auto state = package->get_manipulation_state();

     if (package->get_data_state() == decent::package::PackageInfo::CHECKED)
     {
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
           chain::content_summary newObject;
           newObject.synopsis = listener->op().synopsis;
           newObject.expiration = listener->op().expiration;
           newObject.author = my->get_account( listener->op().author ).name;

           if (state == decent::package::PackageInfo::PACKING) {
              newObject.status = "Packing";
           } else if (state == decent::package::PackageInfo::ENCRYPTING) {
              newObject.status = "Encrypting";
           } else if (state == decent::package::PackageInfo::STAGING) {
              newObject.status = "Staging";
           } else if (state == decent::package::PackageInfo:: MS_IDLE )
              newObject.status = "Submission failed";

           result.insert(result.begin(), newObject);
        }
     }
     if (cont)
        continue;
  }

  return result;
}

std::pair<chain::account_id_type, std::vector<chain::account_id_type>> wallet_api::get_author_and_co_authors_by_URI(const std::string& URI) const
{
   return my->get_author_and_co_authors_by_URI( URI );
}

std::pair<std::string, decent::encrypt::CustodyData> wallet_api::create_package(const std::string& content_dir,
                                                                                const std::string& samples_dir,
                                                                                const decent::encrypt::DInteger& aes_key) const
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   fc::sha256 key1;
#if CRYPTOPP_VERSION >= 600
   aes_key.Encode((CryptoPP::byte*)key1._hash, 32);
#else
   aes_key.Encode((byte*)key1._hash, 32);
#endif

   auto pack = decent::package::PackageManager::instance().get_package(content_dir, samples_dir, key1);
   pack->create( true );
   decent::encrypt::CustodyData cd = pack->get_custody_data();
   return std::pair<std::string, decent::encrypt::CustodyData>(pack->get_hash().str(), cd);
}

void wallet_api::extract_package(const std::string& package_hash, const std::string& output_dir, const decent::encrypt::DInteger& aes_key) const
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   fc::sha256 key1;
#if CRYPTOPP_VERSION >= 600
   aes_key.Encode((CryptoPP::byte*)key1._hash, 32);
#else
   aes_key.Encode((byte*)key1._hash, 32);
#endif

   auto pack = decent::package::PackageManager::instance().find_package(fc::ripemd160(package_hash));
   if (pack == nullptr) {
      FC_THROW_EXCEPTION(cannot_find_package_exception, "");
   }

   if (pack->get_manipulation_state() != decent::package::PackageInfo::ManipulationState::MS_IDLE) {
      FC_THROW_EXCEPTION(package_is_not_in_valid_state_exception, "");
   }
   pack->unpack(output_dir, key1);
}

void wallet_api::download_package(const std::string& url) const
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   auto content = get_content(url);
   if(!content)
      FC_THROW_EXCEPTION(no_such_content_at_this_url_exception, "URL: ${url}", ("url", url));

   auto pack = decent::package::PackageManager::instance().get_package(url, content->_hash );
   pack->download(false);
}

std::string wallet_api::upload_package(const std::string& package_hash, const std::string& protocol) const
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   auto package = decent::package::PackageManager::instance().get_package(fc::ripemd160(package_hash));
   package->start_seeding(protocol, true);
   return package->get_url();
}

void wallet_api::remove_package(const std::string& package_hash) const
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   decent::package::PackageManager::instance().release_package(fc::ripemd160(package_hash));
}

chain::content_keys wallet_api::generate_content_keys(const std::vector<chain::account_id_type>& seeders) const
{
   return my->_remote_db->generate_content_keys(seeders);
}

bool wallet_api::is_package_manager_task_waiting() const
{
   for(const auto& listener : my->_package_manager_listeners)
   {
      if(!listener->is_finished())
         return true;
   }

   return false;
}

signed_transaction_info wallet_api::subscribe_to_author(const std::string& from,
                                                        const std::string& to,
                                                        const std::string& price_amount,
                                                        const std::string& price_asset_symbol,
                                                        bool broadcast/* = false */)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->subscribe_to_author(from, to, price_amount, price_asset_symbol, broadcast);
}

signed_transaction_info wallet_api::subscribe_by_author(const std::string& from,
                                                        const std::string& to,
                                                        bool broadcast/* = false */)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->subscribe_by_author(from, to, broadcast);
}

signed_transaction_info wallet_api::set_subscription(const std::string& account,
                                                     bool allow_subscription,
                                                     uint32_t subscription_period,
                                                     const std::string& price_amount,
                                                     const std::string& price_asset_symbol,
                                                     bool broadcast/* = false */)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->set_subscription(account, allow_subscription, subscription_period, price_amount, price_asset_symbol, broadcast);
}

signed_transaction_info wallet_api::set_automatic_renewal_of_subscription(const std::string& account_id_or_name,
                                                                          chain::subscription_id_type subscription_id,
                                                                          bool automatic_renewal,
                                                                          bool broadcast/* = false */)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->set_automatic_renewal_of_subscription(account_id_or_name, subscription_id, automatic_renewal, broadcast);
}

std::vector<chain::subscription_object> wallet_api::list_active_subscriptions_by_consumer(const std::string& account_id_or_name, uint32_t count) const
{
   return my->_remote_db->list_active_subscriptions_by_consumer(get_account(account_id_or_name).id, count);
}

std::vector<chain::subscription_object> wallet_api::list_subscriptions_by_consumer(const std::string& account_id_or_name, uint32_t count) const
{
   return my->_remote_db->list_subscriptions_by_consumer(get_account(account_id_or_name).id, count);
}

std::vector<chain::subscription_object> wallet_api::list_active_subscriptions_by_author(const std::string& account_id_or_name, uint32_t count) const
{
   return my->_remote_db->list_active_subscriptions_by_author(get_account(account_id_or_name).id, count);
}

std::vector<chain::subscription_object> wallet_api::list_subscriptions_by_author(const std::string& account_id_or_name, uint32_t count) const
{
   return my->_remote_db->list_subscriptions_by_author(get_account(account_id_or_name).id, count);
}

signed_transaction_info wallet_api::send_message(const std::string& from,
                                                 const std::vector<std::string>& to,
                                                 const std::string& text,
                                                 bool broadcast)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->send_message(from, to, text, broadcast);
}

signed_transaction_info wallet_api::send_unencrypted_message(const std::string& from,
                                                             const std::vector<std::string>& to,
                                                             const std::string& text,
                                                             bool broadcast)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->send_unencrypted_message(from, to, text, broadcast);
}

std::vector<message_data> wallet_api::get_message_objects(const std::string& sender, const std::string& receiver, uint32_t max_count) const
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   fc::optional<chain::account_id_type> receiver_id;
   if(receiver.size())
      receiver_id = get_account(receiver).get_id();
   fc::optional<chain::account_id_type> sender_id;
   if(sender.size())
      sender_id = get_account(sender).get_id();
   return my->get_message_objects(sender_id, receiver_id, max_count);
}

std::vector<text_message> wallet_api::get_messages(const std::string& receiver, uint32_t max_count) const
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->get_messages(receiver, max_count);
}

std::vector<text_message> wallet_api::get_sent_messages(const std::string& sender, uint32_t max_count) const
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->get_sent_messages(sender, max_count);
}

void wallet_api::reset_counters(const std::vector<std::string>& names) const
{
   return my->reset_counters(names);
}

std::vector<monitoring::counter_item_cli> wallet_api::get_counters(const std::vector<std::string>& names) const
{
   return my->get_counters(names);
}

std::vector<chain::non_fungible_token_object> wallet_api::list_non_fungible_tokens(const std::string& lowerbound, uint32_t limit) const
{
   return my->_remote_db->list_non_fungible_tokens(lowerbound, limit);
}

chain::non_fungible_token_object wallet_api::get_non_fungible_token(const std::string& nft_symbol_or_id) const
{
   return my->get_non_fungible_token(nft_symbol_or_id);
}

signed_transaction_info wallet_api::create_non_fungible_token(const std::string& issuer,
                                                               const std::string& symbol,
                                                               const std::string& description,
                                                               const chain::non_fungible_token_data_definitions& definitions,
                                                               uint32_t max_supply,
                                                               bool fixed_max_supply,
                                                               bool transferable,
                                                               bool broadcast /* = false */)
{
   return my->create_non_fungible_token(issuer, symbol, description, definitions, max_supply, fixed_max_supply, transferable, broadcast);
}

signed_transaction_info wallet_api::update_non_fungible_token(const std::string& issuer,
                                                               const std::string& symbol,
                                                               const std::string& description,
                                                               uint32_t max_supply,
                                                               bool fixed_max_supply,
                                                               bool broadcast /* = false */)
{
   return my->update_non_fungible_token(issuer, symbol, description, max_supply, fixed_max_supply, broadcast);
}

signed_transaction_info wallet_api::issue_non_fungible_token(const std::string& to_account,
                                                               const std::string& symbol,
                                                               const fc::variants& data,
                                                               const std::string& memo,
                                                               bool broadcast /* = false */)
{
   return my->issue_non_fungible_token(to_account, symbol, data, memo, broadcast);
}

std::vector<chain::non_fungible_token_data_object> wallet_api::list_non_fungible_token_data(const std::string& nft_symbol_or_id) const
{
   return my->_remote_db->list_non_fungible_token_data(get_non_fungible_token(nft_symbol_or_id).get_id());
}

std::map<chain::non_fungible_token_id_type, uint32_t> wallet_api::get_non_fungible_token_summary(const std::string& account) const
{
   return my->_remote_db->get_non_fungible_token_summary(get_account(account).get_id());
}

std::vector<chain::non_fungible_token_data_object> wallet_api::get_non_fungible_token_balances(const std::string& account, const std::set<std::string>& symbols_or_ids) const
{
   std::set<chain::non_fungible_token_id_type> ids;
   std::for_each(symbols_or_ids.begin(), symbols_or_ids.end(), [&](const std::string& symbol) { ids.insert(get_non_fungible_token(symbol).get_id()); } );
   return my->_remote_db->get_non_fungible_token_balances(get_account(account).get_id(), ids);
}

std::vector<chain::transaction_detail_object> wallet_api::search_non_fungible_token_history(chain::non_fungible_token_data_id_type nft_data_id) const
{
   return my->_remote_db->search_non_fungible_token_history(nft_data_id);
}

signed_transaction_info wallet_api::transfer_non_fungible_token_data(const std::string& to_account,
                                                                     chain::non_fungible_token_data_id_type nft_data_id,
                                                                     const std::string& memo,
                                                                     bool broadcast /* = false */)
{
   return my->transfer_non_fungible_token_data(to_account, nft_data_id, memo, broadcast);
}

signed_transaction_info wallet_api::burn_non_fungible_token_data(chain::non_fungible_token_data_id_type nft_data_id, bool broadcast /* = false */)
{
   return my->burn_non_fungible_token_data(nft_data_id, broadcast);
}

signed_transaction_info wallet_api::update_non_fungible_token_data(const std::string& modifier,
                                                                   chain::non_fungible_token_data_id_type nft_data_id,
                                                                   const std::vector<std::pair<std::string, fc::variant>>& data,
                                                                   bool broadcast /* = false */)
{
   return my->update_non_fungible_token_data(modifier, nft_data_id, data, broadcast);
}

std::map<std::string, std::function<std::string(fc::variant,const fc::variants&)> > wallet_api::get_result_formatters() const
{
   std::map<std::string, std::function<std::string(fc::variant,const fc::variants&)> > m;
   m["help"] = m["get_help"] = m["from_command_file"] = [](fc::variant result, const fc::variants& a)
   {
      return result.get_string();
   };

   return m;
}

vesting_balance_object_with_info::vesting_balance_object_with_info(const chain::vesting_balance_object& vbo, fc::time_point_sec now)
   : chain::vesting_balance_object( vbo )
{
   allowed_withdraw = get_allowed_withdraw( now );
   allowed_withdraw_time = now;
}

} } // graphene::wallet

void fc::to_variant(const graphene::chain::account_multi_index_type& accts, fc::variant& vo)
{
   vo = std::vector<graphene::chain::account_object>(accts.begin(), accts.end());
}

void fc::from_variant(const fc::variant& var, graphene::chain::account_multi_index_type& vo)
{
   const std::vector<graphene::chain::account_object>& v = var.as<std::vector<graphene::chain::account_object>>();
   vo = graphene::chain::account_multi_index_type(v.begin(), v.end());
}
