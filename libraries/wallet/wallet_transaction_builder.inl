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
   return my->get_prototype_operation( operation_name );
}
