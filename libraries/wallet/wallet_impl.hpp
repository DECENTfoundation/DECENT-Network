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

#include <graphene/wallet/wallet.hpp>
#include <graphene/wallet/exceptions.hpp>
#include <graphene/wallet/api_documentation.hpp>
#include <decent/encrypt/encryptionutils.hpp>
#include <decent/package/package.hpp>
#include <decent/package/package_config.hpp>
#include <fc/thread/mutex.hpp>
#include <ipfs/client.h>

namespace graphene { namespace wallet { namespace detail {

// this class helps to gather seeding statistics. Tracks seeders currently in use.
class seeders_tracker
{
public:
   seeders_tracker(wallet_api_impl& wallet) : _wallet(wallet) {}

   bool is_empty() const { return seeder_to_content.empty(); }

   std::vector<chain::account_id_type> track_content(const std::string& URI);
   std::vector<chain::account_id_type> untrack_content(const std::string& URI);
   std::vector<chain::account_id_type> get_unfinished_seeders();

   void set_initial_stats(chain::account_id_type seeder, const uint64_t amount);
   uint64_t get_final_stats(chain::account_id_type seeder, const uint64_t amount);
   void remove_stats(chain::account_id_type seeder);

private:
   wallet_api_impl& _wallet;
   std::multimap<chain::account_id_type, chain::content_id_type> seeder_to_content;
   std::map<chain::account_id_type, uint64_t> initial_stats;
};

struct ipfs_stats_listener : public decent::package::EventListenerInterface
{
   ipfs_stats_listener(const std::string& URI, wallet_api_impl& api, chain::account_id_type consumer) : _URI(URI), _wallet(api), _consumer(consumer),
      _ipfs_client(decent::package::PackageManagerConfigurator::instance().get_ipfs_host(), decent::package::PackageManagerConfigurator::instance().get_ipfs_port()) {}

   virtual void package_download_start();
   virtual void package_download_complete();
   virtual void package_download_error(const std::string&);

private:
   std::string       _URI;
   wallet_api_impl&  _wallet;
   chain::account_id_type _consumer;
   ipfs::Client      _ipfs_client;
};

struct submit_transfer_listener : public decent::package::EventListenerInterface
{
   submit_transfer_listener(wallet_api_impl& wallet, std::shared_ptr<decent::package::PackageInfo> info, const chain::content_submit_operation& op, const std::string& protocol)
      : _wallet(wallet), _info(info), _op(op), _protocol(protocol), _is_finished(false) {}

   virtual void package_seed_complete();
   virtual void package_creation_complete();

   fc::ripemd160 get_hash() const { return _info->get_hash(); }
   const chain::content_submit_operation& op() const { return _op; }
   bool is_finished() { return _is_finished; }

private:
   wallet_api_impl&          _wallet;
   std::shared_ptr<decent::package::PackageInfo> _info;
   chain::content_submit_operation _op;
   std::string               _protocol;
   bool                      _is_finished;
};

struct static_operation_map
{
   static_operation_map();
   boost::container::flat_map<std::string, int> name_to_which;
   std::vector<std::string> which_to_name;
};

template<class T>
fc::optional<T> maybe_id( const std::string& name_or_id )
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
   return {};
}

class wallet_api_impl
{
   void claim_registered_account(const chain::account_object& account);

   // after a miner registration succeeds, this saves the private key in the wallet permanently
   void claim_registered_miner(const std::string& miner_name);

   void resync();
   int find_first_unused_derived_key_index(const chain::private_key_type& parent_key) const;

   // if the user executes the same command twice in quick succession,
   // we might generate the same transaction id, and cause the second
   // transaction to be rejected.  This can be avoided by altering the
   // second transaction slightly (bumping up the expiration time by
   // a second).  Keep track of recent transaction ids we've generated
   // so we can know if we need to do this
   struct recently_generated_transaction_record
   {
      fc::time_point_sec generation_time;
      chain::transaction_id_type transaction_id;
   };

   struct timestamp_index{};
   typedef boost::multi_index_container<recently_generated_transaction_record,
      boost::multi_index::indexed_by<
         boost::multi_index::hashed_unique<
            boost::multi_index::member<recently_generated_transaction_record, chain::transaction_id_type, &recently_generated_transaction_record::transaction_id>, std::hash<chain::transaction_id_type>
         >,
         boost::multi_index::ordered_non_unique<boost::multi_index::tag<timestamp_index>,
            boost::multi_index::member<recently_generated_transaction_record, fc::time_point_sec, &recently_generated_transaction_record::generation_time>
         >
      >
   > recently_generated_transaction_set_type;
   recently_generated_transaction_set_type _recently_generated_transactions;
   std::map<transaction_handle_type, chain::signed_transaction> _builder_transactions;
   fc::mutex _resync_mutex;
   boost::filesystem::path _wallet_filename;

public:
   wallet_api& self;
   api_documentation method_documentation;
   wallet_api_impl( wallet_api& s, const fc::api<app::login_api> &rapi, const chain::chain_id_type &chain_id, const server_data &ws );

   void encrypt_keys();
   void encrypt_keys2();

   void on_block_applied( const fc::variant& block_id );

   bool is_locked() const
   {
      return _checksum == fc::sha512();
   }

   template<typename T>
   T get_object(db::object_id<T::space_id, T::type_id, T> id)const
   {
      auto ob = _remote_db->get_objects({id}).front();
      return ob.template as<T>();
   }

   void set_operation_fees(chain::signed_transaction& tx, const chain::fee_schedule& s) const;

   fc::optional<chain::account_object> find_account(chain::account_id_type account_id) const;
   fc::optional<chain::account_object> find_account(const std::string& account_name_or_id) const;

   chain::account_object get_account(chain::account_id_type account_id) const;
   chain::account_object get_account(const std::string& account_name_or_id) const;

   fc::optional<chain::asset_object> find_asset(chain::asset_id_type asset_id) const;
   fc::optional<chain::asset_object> find_asset(const std::string& asset_symbol_or_id) const;

   chain::asset_object get_asset(chain::asset_id_type asset_id) const;
   chain::asset_object get_asset(const std::string& asset_symbol_or_id) const;

   fc::optional<chain::non_fungible_token_object> find_non_fungible_token(chain::non_fungible_token_id_type nft_id) const;
   fc::optional<chain::non_fungible_token_object> find_non_fungible_token(const std::string& nft_symbol_or_id) const;

   chain::non_fungible_token_object get_non_fungible_token(chain::non_fungible_token_id_type nft_id) const;
   chain::non_fungible_token_object get_non_fungible_token(const std::string& nft_symbol_or_id) const;

   fc::optional<chain::non_fungible_token_data_object> find_non_fungible_token_data(chain::non_fungible_token_data_id_type nft_data_id) const;
   chain::non_fungible_token_data_object get_non_fungible_token_data(chain::non_fungible_token_data_id_type nft_data_id) const;

   const boost::filesystem::path& get_wallet_filename() const { return _wallet_filename; }
   void set_wallet_filename(const boost::filesystem::path &wallet_filename);

   chain::private_key_type get_private_key(const chain::public_key_type& id) const;
   chain::private_key_type get_private_key_for_account(const chain::account_object& account) const;

   // imports the private key into the wallet, and associate it in some way (?) with the
   // given account name.
   // @returns true if the key matches a current active/owner/memo key for the named
   //          account, false otherwise (but it is stored either way)
   bool import_key(const std::string& account_name_or_id, const std::string& wif_key);

   // @returns true if the key matches a current active/owner/memo key for the named
   //          account, false otherwise (but it is stored either way)
   bool import_single_key(const std::string& account_name_or_id, const std::string& wif_key);

   int get_wallet_file_version(const fc::variant& data);
   bool load_old_wallet_file(const fc::variant& data, wallet_data& result);
   bool load_new_wallet_file(const fc::variant& data, wallet_data& result);
   bool load_wallet_file(boost::filesystem::path wallet_filename = boost::filesystem::path());
   void save_wallet_file(boost::filesystem::path wallet_filename = boost::filesystem::path());
   // transaction builder
   transaction_handle_type begin_builder_transaction();
   void add_operation_to_builder_transaction(transaction_handle_type transaction_handle, const chain::operation& op);
   void replace_operation_in_builder_transaction(transaction_handle_type handle, uint32_t operation_index, const chain::operation& new_op);
   void remove_builder_transaction(transaction_handle_type handle);
   chain::asset set_fees_on_builder_transaction(transaction_handle_type handle, const std::string& fee_asset);
   chain::transaction preview_builder_transaction(transaction_handle_type handle);
   chain::signed_transaction sign_builder_transaction(transaction_handle_type transaction_handle, bool broadcast = true);
   chain::signed_transaction propose_builder_transaction(transaction_handle_type handle, fc::time_point_sec expiration, uint32_t review_period_seconds, bool broadcast);
   chain::signed_transaction propose_builder_transaction2(transaction_handle_type handle, const std::string& account_name_or_id, fc::time_point_sec expiration,
                                                          uint32_t review_period_seconds, bool broadcast);
   // account
   chain::signed_transaction register_account(const std::string& name, const chain::public_key_type& owner, const chain::public_key_type& active, const chain::public_key_type& memo,
                                              const std::string& registrar_account, bool broadcast);
   chain::signed_transaction register_multisig_account(const std::string& name, const chain::authority& owner_authority, const chain::authority& active_authority,
                                                       const chain::public_key_type& memo_pubkey, const std::string& registrar_account, bool broadcast);
   chain::signed_transaction create_account_with_private_key(const chain::private_key_type& owner_privkey, const std::string& account_name, const std::string& registrar_account,
                                                             bool import, bool broadcast, bool save_wallet = true);
   chain::signed_transaction create_account_with_brain_key(const std::string& brain_key, const std::string& account_name, const std::string& registrar_account,
                                                           bool import, bool broadcast, bool save_wallet = true);
   chain::signed_transaction update_account_keys(const std::string& name, fc::optional<chain::authority> owner_auth, fc::optional<chain::authority> active_auth,
                                                 fc::optional<chain::public_key_type> memo_pubkey, bool broadcast);
   // asset
   chain::signed_transaction create_user_issued_asset(const std::string& issuer, const std::string& symbol, uint8_t precision, const std::string& description, uint64_t max_supply,
                                                      chain::price core_exchange_rate, bool is_exchangeable, bool is_fixed_max_supply, bool broadcast);
   chain::signed_transaction issue_asset(const std::string& to_account, const std::string& amount, const std::string& symbol, const std::string& memo, bool broadcast);
   chain::signed_transaction update_user_issued_asset(const std::string& symbol, const std::string& new_issuer, const std::string& description, uint64_t max_supply,
                                                      chain::price core_exchange_rate, bool is_exchangeable, bool broadcast);
   chain::signed_transaction fund_asset_pools(const std::string& from, const std::string& uia_amount, const std::string& uia_symbol,
                                              const std::string& DCT_amount, const std::string& DCT_symbol, bool broadcast);
   chain::signed_transaction reserve_asset(const std::string& from, const std::string& amount, const std::string& symbol, bool broadcast);
   chain::signed_transaction claim_fees(const std::string& uia_amount, const std::string& uia_symbol, const std::string& dct_amount, const std::string& dct_symbol, bool broadcast);
   chain::signed_transaction create_monitored_asset(const std::string& issuer, const std::string& symbol, uint8_t precision, const std::string& description,
                                                    uint32_t feed_lifetime_sec, uint8_t minimum_feeds, bool broadcast);
   chain::signed_transaction update_monitored_asset(const std::string& symbol, const std::string& description, uint32_t feed_lifetime_sec, uint8_t minimum_feeds, bool broadcast);
   chain::signed_transaction publish_asset_feed(const std::string& publishing_account, const std::string& symbol, const chain::price_feed& feed, bool broadcast);
   // non fungible token
   chain::signed_transaction create_non_fungible_token(const std::string& issuer, const std::string& symbol, const std::string& description,
                                                       const chain::non_fungible_token_data_definitions& definitions, uint32_t max_supply,
                                                       bool fixed_max_supply, bool transferable, bool broadcast);
   chain::signed_transaction update_non_fungible_token(const std::string& issuer, const std::string& symbol, const std::string& description,
                                                       uint32_t max_supply, bool fixed_max_supply, bool broadcast);
   chain::signed_transaction issue_non_fungible_token(const std::string& to_account, const std::string& symbol, const fc::variants& data, const std::string& memo, bool broadcast);
   chain::signed_transaction transfer_non_fungible_token_data(const std::string& to_account, chain::non_fungible_token_data_id_type nft_data_id,
                                                              const std::string& memo, bool broadcast);
   chain::signed_transaction burn_non_fungible_token_data(chain::non_fungible_token_data_id_type nft_data_id, bool broadcast);
   chain::signed_transaction update_non_fungible_token_data(const std::string& modifier, chain::non_fungible_token_data_id_type nft_data_id,
                                                            const std::vector<std::pair<std::string, fc::variant>>& data, bool broadcast);
   // miner
   chain::miner_object get_miner(const std::string& owner_account) const;
   chain::signed_transaction create_miner(const std::string& owner_account, const std::string& url, bool broadcast);
   chain::signed_transaction update_miner(const std::string& miner_name, const std::string& url, const std::string& block_signing_key, bool broadcast);
   std::vector<vesting_balance_object_with_info> get_vesting_balances(const std::string& account_name) const;
   chain::signed_transaction withdraw_vesting(const std::string& miner_name, const std::string& amount, const std::string& asset_symbol, bool broadcast);
   chain::signed_transaction vote_for_miner(const std::string& voting_account, const std::string& miner, bool approve, bool broadcast);
   chain::signed_transaction set_voting_proxy(const std::string& account_to_modify, fc::optional<std::string> voting_account, bool broadcast);
   chain::signed_transaction set_desired_miner_count(const std::string& account_to_modify, uint16_t desired_number_of_miners, bool broadcast);
   // transaction
   chain::signed_transaction sign_transaction(chain::signed_transaction tx, bool broadcast);
   chain::signed_transaction transfer(const std::string& from, const std::string& to, const std::string& amount, const std::string& asset_symbol,
                                      const std::string& memo, bool broadcast);
   chain::signed_transaction propose_transfer(const std::string& proposer, const std::string& from, const std::string& to, const std::string& amount, const std::string& asset_symbol,
                                              const std::string& memo, fc::time_point_sec expiration);
   std::string decrypt_memo(const chain::memo_data& memo, const chain::account_object& from_account, const chain::account_object& to_account) const;
   // proposal
   chain::signed_transaction propose_parameter_change(const std::string& proposing_account, fc::time_point_sec expiration_time, const fc::variant_object& changed_values, bool broadcast);
   chain::signed_transaction propose_fee_change(const std::string& proposing_account, fc::time_point_sec expiration_time, const fc::variant_object& changed_fees, bool broadcast);
   chain::signed_transaction approve_proposal(const std::string& fee_paying_account, const std::string& proposal_id, const approval_delta& delta, bool broadcast);
   // content
   void submit_content_utility(chain::content_submit_operation& submit_op, const std::vector<regional_price_info>& price_amounts);
   chain::signed_transaction submit_content(const std::string& author, const std::vector<std::pair<std::string, uint32_t>>& co_authors, const std::string& URI,
                                            const std::vector<regional_price_info>& price_amounts, const fc::ripemd160& hash, uint64_t size,
                                            const std::vector<chain::account_id_type>& seeders, uint32_t quorum, fc::time_point_sec expiration,
                                            const std::string& publishing_fee_symbol_name, const std::string& publishing_fee_amount, const std::string& synopsis,
                                            const decent::encrypt::DInteger& secret, const decent::encrypt::CustodyData& cd, bool broadcast);
   chain::content_keys submit_content_async(const std::string& author, const std::vector<std::pair<std::string, uint32_t>>& co_authors,
                                            const std::string& content_dir, const std::string& samples_dir, const std::string& protocol,
                                            const std::vector<regional_price_info>& price_amounts, const std::vector<chain::account_id_type>& seeders,
                                            fc::time_point_sec expiration, const std::string& synopsis);
   chain::signed_transaction content_cancellation(const std::string& author, const std::string& URI, bool broadcast);
   content_download_status get_download_status(const std::string& consumer, const std::string& URI) const;
   void download_content(const std::string& consumer, const std::string& URI, const std::string& str_region_code_from, bool broadcast);
   chain::signed_transaction request_to_buy(const std::string& consumer, const std::string& URI, const std::string& price_asset_symbol,
                                            const std::string& price_amount, const std::string& str_region_code_from, bool broadcast);
   chain::signed_transaction leave_rating_and_comment(const std::string& consumer, const std::string& URI, uint64_t rating, const std::string& comment, bool broadcast);
   chain::signed_transaction subscribe_to_author(const std::string& from, const std::string& to, const std::string& price_amount, const std::string& price_asset_symbol, bool broadcast);
   chain::signed_transaction subscribe_by_author(const std::string& from, const std::string& to, bool broadcast);
   chain::signed_transaction set_subscription(const std::string& account, bool allow_subscription, uint32_t subscription_period,
                                              const std::string& price_amount, const std::string& price_asset_symbol, bool broadcast);
   chain::signed_transaction set_automatic_renewal_of_subscription(const std::string& account_id_or_name, chain::subscription_id_type subscription_id,
                                                                   bool automatic_renewal, bool broadcast);
   decent::encrypt::DInteger restore_encryption_key(const std::string& account, chain::buying_id_type buying);
   std::pair<chain::account_id_type, std::vector<chain::account_id_type>> get_author_and_co_authors_by_URI(const std::string& URI) const;
   //message
   std::vector<message_data> get_message_objects(fc::optional<chain::account_id_type> sender, fc::optional<chain::account_id_type> receiver, uint32_t max_count) const;
   std::vector<text_message> get_messages(const std::string& receiver, uint32_t max_count) const;
   std::vector<text_message> get_sent_messages(const std::string& sender, uint32_t max_count) const;
   chain::signed_transaction send_message(const std::string& from, const std::vector<std::string>& to, const std::string& text, bool broadcast);
   chain::signed_transaction send_unencrypted_message(const std::string& from, const std::vector<std::string>& to, const std::string& text, bool broadcast);
   // monitoring
   void use_monitoring_api();
   void reset_counters(const std::vector<std::string>& names);
   std::vector<monitoring::counter_item_cli> get_counters(const std::vector<std::string>& names);
   // network node
   void use_network_node_api();
   void network_add_nodes(const std::vector<std::string>& nodes);
   fc::variants network_get_connected_peers();

   wallet_data             _wallet;

   std::map<chain::public_key_type, std::string> _keys;
   std::map<decent::encrypt::DInteger, decent::encrypt::DInteger> _el_gamal_keys;   // public_key/private_key
   fc::sha512                  _checksum;

   chain::chain_id_type _chain_id;
   fc::api<app::login_api> _remote_api;
   fc::api<app::database_api> _remote_db;
   fc::api<app::network_broadcast_api> _remote_net_broadcast;
   fc::api<app::history_api> _remote_hist;
   fc::optional<fc::api<app::network_node_api>> _remote_net_node;
   fc::optional<fc::api<app::monitoring_api>> _remote_monitoring;

   boost::container::flat_map<std::string, chain::operation> _prototype_ops;
   static_operation_map _operation_which_map;

   std::vector<std::shared_ptr<detail::submit_transfer_listener>> _package_manager_listeners;
   seeders_tracker _seeders_tracker;

   static CryptoPP::AutoSeededRandomPool randomGenerator;
};

} } } // namespace graphene::wallet::detail
