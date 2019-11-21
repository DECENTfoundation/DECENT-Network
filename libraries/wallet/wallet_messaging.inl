signed_transaction_info wallet_api::send_message(const std::string& from,
                                                 const std::vector<std::string>& to,
                                                 const std::string& text,
                                                 bool broadcast)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->send_message(from, to, text, broadcast);
}

signed_transaction_info wallet_api::send_unencrypted_message(const std::string& from,
                                                             const std::vector<std::string>& to,
                                                             const std::string& text,
                                                             bool broadcast)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->send_unencrypted_message(from, to, text, broadcast);
}

std::vector<message_data> wallet_api::get_message_objects(const std::string& sender, const std::string& receiver, uint32_t max_count) const
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   fc::optional<chain::account_id_type> receiver_id;
   if(receiver.size())
      receiver_id = get_account(receiver).get_id();
   fc::optional<chain::account_id_type> sender_id;
   if(sender.size())
      sender_id = get_account(sender).get_id();
   return my->get_message_objects(sender_id, receiver_id, max_count);
}

std::vector<text_message> wallet_api::get_messages(const std::string& receiver, uint32_t max_count) const
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->get_messages(receiver, max_count);
}

std::vector<text_message> wallet_api::get_sent_messages(const std::string& sender, uint32_t max_count) const
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->get_sent_messages(sender, max_count);
}
