
transaction_handle_type wallet_api::begin_builder_transaction()
{
   return my->begin_builder_transaction();
}

void wallet_api::add_operation_to_builder_transaction(transaction_handle_type transaction_handle, const operation& op)
{
   my->add_operation_to_builder_transaction(transaction_handle, op);
}

void wallet_api::replace_operation_in_builder_transaction(transaction_handle_type handle, unsigned operation_index, const operation& new_op)
{
   my->replace_operation_in_builder_transaction(handle, operation_index, new_op);
}

asset wallet_api::set_fees_on_builder_transaction(transaction_handle_type handle, const string& fee_asset)
{
   return my->set_fees_on_builder_transaction(handle, fee_asset);
}

transaction wallet_api::preview_builder_transaction(transaction_handle_type handle)
{
   return my->preview_builder_transaction(handle);
}

pair<transaction_id_type,signed_transaction> wallet_api::sign_builder_transaction(transaction_handle_type transaction_handle,
                                                                                  bool broadcast)
{
   FC_ASSERT( !my->is_locked(), "the wallet is locked and needs to be unlocked to have access to private keys" );
   signed_transaction tx = my->sign_builder_transaction(transaction_handle, broadcast);
   return std::make_pair(tx.id(),tx);
}

pair<transaction_id_type,signed_transaction> wallet_api::propose_builder_transaction(transaction_handle_type handle,
                                                           time_point_sec expiration,
                                                           uint32_t review_period_seconds,
                                                           bool broadcast)
{
   FC_ASSERT( !my->is_locked(), "the wallet is locked and needs to be unlocked to have access to private keys" );
   signed_transaction tx = my->propose_builder_transaction(handle, expiration, review_period_seconds, broadcast);
   return std::make_pair(tx.id(),tx);
}

pair<transaction_id_type,signed_transaction> wallet_api::propose_builder_transaction2(transaction_handle_type handle,
                                                                                      const string& account_name_or_id,
                                                                                      time_point_sec expiration,
                                                                                      uint32_t review_period_seconds,
                                                                                      bool broadcast)
{
   FC_ASSERT( !my->is_locked(), "the wallet is locked and needs to be unlocked to have access to private keys" );
   signed_transaction tx = my->propose_builder_transaction2(handle, account_name_or_id, expiration, review_period_seconds, broadcast);
   return std::make_pair(tx.id(),tx);
}

void wallet_api::remove_builder_transaction(transaction_handle_type handle)
{
   return my->remove_builder_transaction(handle);
}

string wallet_api::serialize_transaction( signed_transaction tx )const
{
   return fc::to_hex(fc::raw::pack(tx));
}

pair<transaction_id_type,signed_transaction> wallet_api::sign_transaction(signed_transaction tx, bool broadcast /* = false */)
{
    try {
       FC_ASSERT( !my->is_locked(), "the wallet is locked and needs to be unlocked to have access to private keys" );
       signed_transaction t = my->sign_transaction( tx, broadcast );
       return std::make_pair(t.id(),t);
    } FC_CAPTURE_AND_RETHROW( (tx) )
}

operation wallet_api::get_prototype_operation(const string& operation_name)
{
   return my->get_prototype_operation( operation_name );
}
