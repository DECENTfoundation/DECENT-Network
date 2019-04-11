
vector<proposal_object> wallet_api::get_proposed_transactions(const string& account_or_id ) const
{
   account_id_type id = get_account(account_or_id).get_id();
   return my->_remote_db->get_proposed_transactions( id );
}

void wallet_api::propose_transfer(const string& proposer,
                                  const string& from,
                                  const string& to,
                                  const string& amount,
                                  const string& asset_symbol,
                                  const string& memo,
                                  time_point_sec expiration)
{
   FC_ASSERT( !my->is_locked(), "the wallet is locked and needs to be unlocked to have access to private keys" );
   return my->propose_transfer(proposer, from, to, amount, asset_symbol, memo, expiration);
}

signed_transaction wallet_api::propose_parameter_change( const string& proposing_account,
                                                         fc::time_point_sec expiration_time,
                                                         const variant_object& changed_values,
                                                         bool broadcast /* = false */)
{
   FC_ASSERT( !my->is_locked(), "the wallet is locked and needs to be unlocked to have access to private keys" );
   return my->propose_parameter_change( proposing_account, expiration_time, changed_values, broadcast );
}

signed_transaction wallet_api::propose_fee_change( const string& proposing_account,
                                                   fc::time_point_sec expiration_time,
                                                   const variant_object& changed_fees,
                                                   bool broadcast /* = false */)
{
   FC_ASSERT( !my->is_locked(), "the wallet is locked and needs to be unlocked to have access to private keys" );
   return my->propose_fee_change( proposing_account, expiration_time, changed_fees, broadcast );
}

signed_transaction wallet_api::approve_proposal(const string& fee_paying_account,
                                                const string& proposal_id,
                                                const approval_delta& delta,
                                                bool broadcast /* = false */)
{
   FC_ASSERT( !my->is_locked(), "the wallet is locked and needs to be unlocked to have access to private keys" );
   return my->approve_proposal( fee_paying_account, proposal_id, delta, broadcast );
}
