signed_transaction_info wallet_api::subscribe_to_author(const string& from,
                                                        const string& to,
                                                        const string& price_amount,
                                                        const string& price_asset_symbol,
                                                        bool broadcast/* = false */)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->subscribe_to_author(from, to, price_amount, price_asset_symbol, broadcast);
}

signed_transaction_info wallet_api::subscribe_by_author(const string& from,
                                                        const string& to,
                                                        bool broadcast/* = false */)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->subscribe_by_author(from, to, broadcast);
}

signed_transaction_info wallet_api::set_subscription(const string& account,
                                                     bool allow_subscription,
                                                     uint32_t subscription_period,
                                                     const string& price_amount,
                                                     const string& price_asset_symbol,
                                                     bool broadcast/* = false */)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->set_subscription(account, allow_subscription, subscription_period, price_amount, price_asset_symbol, broadcast);
}

signed_transaction_info wallet_api::set_automatic_renewal_of_subscription(const string& account_id_or_name,
                                                                          subscription_id_type subscription_id,
                                                                          bool automatic_renewal,
                                                                          bool broadcast/* = false */)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->set_automatic_renewal_of_subscription(account_id_or_name, subscription_id, automatic_renewal, broadcast);
}

std::vector<subscription_object> wallet_api::list_active_subscriptions_by_consumer(const std::string& account_id_or_name, uint32_t count) const
{
   return my->_remote_db->list_active_subscriptions_by_consumer(get_account(account_id_or_name).id, count);
}

std::vector<subscription_object> wallet_api::list_subscriptions_by_consumer(const std::string& account_id_or_name, uint32_t count) const
{
   return my->_remote_db->list_subscriptions_by_consumer(get_account(account_id_or_name).id, count);
}

std::vector<subscription_object> wallet_api::list_active_subscriptions_by_author(const std::string& account_id_or_name, uint32_t count) const
{
   return my->_remote_db->list_active_subscriptions_by_author(get_account(account_id_or_name).id, count);
}

std::vector<subscription_object> wallet_api::list_subscriptions_by_author(const std::string& account_id_or_name, uint32_t count) const
{
   return my->_remote_db->list_subscriptions_by_author(get_account(account_id_or_name).id, count);
}
