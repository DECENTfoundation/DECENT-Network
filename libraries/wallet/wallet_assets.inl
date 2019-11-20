std::vector<asset_object> wallet_api::list_assets(const std::string& lowerbound, uint32_t limit) const
{
   return my->_remote_db->list_assets( lowerbound, limit );
}

asset_object wallet_api::get_asset(const std::string& asset_name_or_id) const
{
   auto a = my->find_asset(asset_name_or_id);
   if(!a)
      FC_THROW_EXCEPTION(asset_not_found_exception, "");

   return *a;
}

monitored_asset_options wallet_api::get_monitored_asset_data(const std::string& asset_name_or_id) const
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
                                                             price core_exchange_rate,
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
                                                             price core_exchange_rate,
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
   return my->price_to_dct(amount, asset_symbol_or_id);
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
                                                       const price_feed& feed,
                                                       bool broadcast /* = false */)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->publish_asset_feed(publishing_account, symbol, feed, broadcast);
}

std::multimap<fc::time_point_sec, price_feed> wallet_api::get_feeds_by_miner(const std::string& account_name_or_id, uint32_t count) const
{
   account_id_type account_id = get_account( account_name_or_id ).id;
   return my->_remote_db->get_feeds_by_miner( account_id, count );
}

real_supply wallet_api::get_real_supply() const
{
   return my->_remote_db->get_real_supply();
}
