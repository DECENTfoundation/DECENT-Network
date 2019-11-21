std::vector<chain::seeder_object> wallet_api::list_seeders_by_price( uint32_t count )const
{
   return my->_remote_db->list_seeders_by_price( count );
}

fc::optional<std::vector<chain::seeder_object>> wallet_api::list_seeders_by_upload( const uint32_t count )const
{
   return my->_remote_db->list_seeders_by_upload( count );
}

std::vector<chain::seeder_object> wallet_api::list_seeders_by_region( const std::string& region_code )const
{
   return my->_remote_db->list_seeders_by_region( region_code );
}

std::vector<chain::seeder_object> wallet_api::list_seeders_by_rating( const uint32_t count )const
{
   return my->_remote_db->list_seeders_by_rating( count );
}
