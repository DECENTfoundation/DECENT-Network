std::vector<proposal_object> wallet_api::get_proposed_transactions(const std::string& account_or_id ) const
{
   account_id_type id = get_account(account_or_id).get_id();
   return my->_remote_db->get_proposed_transactions( id );
}

signed_transaction_info wallet_api::propose_transfer(const string& proposer,
                                                     const string& from,
                                                     const string& to,
                                                     const string& amount,
                                                     const string& asset_symbol,
                                                     const string& memo,
                                                     fc::time_point_sec expiration)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->propose_transfer(proposer, from, to, amount, asset_symbol, memo, expiration);
}

signed_transaction_info wallet_api::propose_parameter_change(const string& proposing_account,
                                                             fc::time_point_sec expiration_time,
                                                             const fc::variant_object& changed_values,
                                                             bool broadcast /* = false */)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->propose_parameter_change( proposing_account, expiration_time, changed_values, broadcast );
}

signed_transaction_info wallet_api::propose_fee_change(const string& proposing_account,
                                                       fc::time_point_sec expiration_time,
                                                       const fc::variant_object& changed_fees,
                                                       bool broadcast /* = false */)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->propose_fee_change( proposing_account, expiration_time, changed_fees, broadcast );
}

signed_transaction_info wallet_api::approve_proposal(const string& fee_paying_account,
                                                     const string& proposal_id,
                                                     const approval_delta& delta,
                                                     bool broadcast /* = false */)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->approve_proposal( fee_paying_account, proposal_id, delta, broadcast );
}
