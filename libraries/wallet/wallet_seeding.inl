
vector<seeder_object> wallet_api::list_seeders_by_price( uint32_t count )const
{
   return my->_remote_db->list_seeders_by_price( count );
}

optional<vector<seeder_object>> wallet_api::list_seeders_by_upload( const uint32_t count )const
{
   return my->_remote_db->list_seeders_by_upload( count );
}

vector<seeder_object> wallet_api::list_seeders_by_region( const string& region_code )const
{
   return my->_remote_db->list_seeders_by_region( region_code );
}

vector<seeder_object> wallet_api::list_seeders_by_rating( const uint32_t count )const
{
   return my->_remote_db->list_seeders_by_rating( count );
}

