
void wallet_api::send_message(const std::string& from, const std::vector<string>& to, const string& text)
{
   return my->send_message(from, to, text);
}

vector<message_object> wallet_api::get_message_objects(const std::string& sender, const std::string& receiver, uint32_t max_count) const
{
   optional<account_id_type> receiver_id;
   if(receiver.size())
      receiver_id = get_account(receiver).get_id();
   optional<account_id_type> sender_id;
   if(sender.size())
      sender_id = get_account(sender).get_id();
   return my->get_message_objects(sender_id, receiver_id, max_count);
}

vector<text_message> wallet_api::get_messages(const std::string& receiver, uint32_t max_count) const
{
   return my->get_messages(receiver, max_count);
}

vector<text_message> wallet_api::get_sent_messages(const std::string& sender, uint32_t max_count) const
{
   return my->get_sent_messages(sender, max_count);
}

