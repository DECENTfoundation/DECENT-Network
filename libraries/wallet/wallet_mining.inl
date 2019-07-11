
map<string,miner_id_type> wallet_api::list_miners(const string& lowerbound, uint32_t limit)
{
   return my->_remote_db->lookup_miner_accounts(lowerbound, limit);
}

miner_object wallet_api::get_miner(const string& owner_account)
{
   return my->get_miner(owner_account);
}

signed_transaction_info wallet_api::create_miner(const string& owner_account,
                                                 const string& url,
                                                 bool broadcast /* = false */)
{
   if(is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->create_miner(owner_account, url, broadcast);
}

signed_transaction_info wallet_api::update_miner(const string& miner_name,
                                                 const string& url,
                                                 const string& block_signing_key,
                                                 bool broadcast /* = false */)
{
   if(is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->update_miner(miner_name, url, block_signing_key, broadcast);
}

vector< vesting_balance_object_with_info > wallet_api::get_vesting_balances( const string& account_name )
{
   return my->get_vesting_balances( account_name );
}

signed_transaction_info wallet_api::withdraw_vesting(const string& miner_name,
                                                     const string& amount,
                                                     const string& asset_symbol,
                                                     bool broadcast /* = false */)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->withdraw_vesting( miner_name, amount, asset_symbol, broadcast );
}

signed_transaction_info wallet_api::vote_for_miner(const string& voting_account,
                                                   const string& miner,
                                                   bool approve,
                                                   bool broadcast /* = false */)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->vote_for_miner(voting_account, miner, approve, broadcast);
}

signed_transaction_info wallet_api::set_voting_proxy(const string& account_to_modify,
                                                     optional<string> voting_account,
                                                     bool broadcast /* = false */)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->set_voting_proxy(account_to_modify, voting_account, broadcast);
}

signed_transaction_info wallet_api::set_desired_miner_count(const string& account_to_modify,
                                                            uint16_t desired_number_of_miners,
                                                            bool broadcast /* = false */)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->set_desired_miner_count(account_to_modify, desired_number_of_miners, broadcast);
}

vector<miner_voting_info> wallet_api::search_miner_voting(const string& account_id,
                                                          const string& term,
                                                          bool only_my_votes,
                                                          const string& order,
                                                          const string& id,
                                                          uint32_t count ) const
{
   return my->search_miner_voting(account_id, term, only_my_votes, order, id, count);
}
