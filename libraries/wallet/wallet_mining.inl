
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

