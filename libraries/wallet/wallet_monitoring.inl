
void wallet_api::reset_counters(const std::vector<std::string>& names)
{
   return my->reset_counters(names);
}

std::vector<monitoring::counter_item_cli> wallet_api::get_counters(const std::vector<std::string>& names)
{
   return my->get_counters(names);
}
