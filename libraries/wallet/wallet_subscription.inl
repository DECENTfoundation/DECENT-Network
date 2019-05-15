
signed_transaction_info wallet_api::subscribe_to_author(const string& from,
                                                        const string& to,
                                                        const string& price_amount,
                                                        const string& price_asset_symbol,
                                                        bool broadcast/* = false */)
{
   FC_ASSERT( !my->is_locked(), "the wallet is locked and needs to be unlocked to have access to private keys" );
   return my->subscribe_to_author(from, to, price_amount, price_asset_symbol, broadcast);
}

signed_transaction_info wallet_api::subscribe_by_author(const string& from,
                                                        const string& to,
                                                        bool broadcast/* = false */)
{
   FC_ASSERT( !my->is_locked(), "the wallet is locked and needs to be unlocked to have access to private keys" );
   return my->subscribe_by_author(from, to, broadcast);
}

signed_transaction_info wallet_api::set_subscription(const string& account,
                                                     bool allow_subscription,
                                                     uint32_t subscription_period,
                                                     const string& price_amount,
                                                     const string& price_asset_symbol,
                                                     bool broadcast/* = false */)
{
   FC_ASSERT( !my->is_locked(), "the wallet is locked and needs to be unlocked to have access to private keys" );
   return my->set_subscription(account, allow_subscription, subscription_period, price_amount, price_asset_symbol, broadcast);
}

signed_transaction_info wallet_api::set_automatic_renewal_of_subscription(const string& account_id_or_name,
                                                                          subscription_id_type subscription_id,
                                                                          bool automatic_renewal,
                                                                          bool broadcast/* = false */)
{
   FC_ASSERT( !my->is_locked(), "the wallet is locked and needs to be unlocked to have access to private keys" );
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

vector< subscription_object > wallet_api::list_subscriptions_by_author( const string& account_id_or_name, const uint32_t count) const
{
   account_id_type account = get_account( account_id_or_name ).id;
   return my->_remote_db->list_subscriptions_by_author( account, count);
}
