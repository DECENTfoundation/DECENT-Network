
signed_transaction wallet_api::submit_content(const string& author,
                                              const vector< pair< string, uint32_t>>& co_authors,
                                              const string& URI,
                                              const vector <regional_price_info>& price_amounts,
                                              uint64_t size,
                                              const fc::ripemd160& hash,
                                              const vector<account_id_type>& seeders,
                                              uint32_t quorum,
                                              const fc::time_point_sec& expiration,
                                              const string& publishing_fee_asset,
                                              const string& publishing_fee_amount,
                                              const string& synopsis,
                                              const DInteger& secret,
                                              const decent::encrypt::CustodyData& cd,
                                              bool broadcast)
{
   return my->submit_content(author, co_authors, URI, price_amounts, hash, size,
                             seeders, quorum, expiration, publishing_fee_asset, publishing_fee_amount,
                             synopsis, secret, cd, broadcast);
}

content_keys wallet_api::submit_content_async(const string& author,
                                      const vector< pair< string, uint32_t>>& co_authors,
                                      const string& content_dir,
                                      const string& samples_dir,
                                      const string& protocol,
                                      const vector<regional_price_info>& price_amounts,
                                      const vector<account_id_type>& seeders,
                                      const fc::time_point_sec& expiration,
                                      const string& synopsis)
{
   return my->submit_content_async(author, co_authors, content_dir, samples_dir, protocol, price_amounts, seeders, expiration, synopsis);
}

signed_transaction wallet_api::content_cancellation(const string& author,
                                                    const string& URI,
                                                    bool broadcast)
{
   return my->content_cancellation(author, URI, broadcast);
}

void wallet_api::download_content(const string& consumer, const string& URI, const string& region_code_from, bool broadcast)
{
   return my->download_content(consumer, URI, region_code_from, broadcast);
}

optional<content_download_status> wallet_api::get_download_status(const string& consumer,
                                                                  const string& URI) const
{
   return my->get_download_status(consumer, URI);
}

signed_transaction wallet_api::request_to_buy(const string& consumer,
                                              const string& URI,
                                              const string& price_asset_name,
                                              const string& price_amount,
                                              const string& str_region_code_from,
                                              bool broadcast)
{
   return my->request_to_buy(consumer, URI, price_asset_name, price_amount, str_region_code_from, broadcast);
}

void wallet_api::seeding_startup(const string& account_id_type_or_name,
                                 DInteger content_private_key,
                                 const string& seeder_private_key,
                                 uint64_t free_space,
                                 uint32_t seeding_price,
                                 const string& seeding_symbol,
                                 const string& packages_path,
                                 const string& region_code)
{
  return my->seeding_startup(account_id_type_or_name, content_private_key, seeder_private_key,
                             free_space, seeding_price, seeding_symbol, packages_path, region_code);
}

void wallet_api::leave_rating_and_comment(const string& consumer,
                                          const string& URI,
                                          uint64_t rating,
                                          const string& comment,
                                          bool broadcast)
{
   return my->leave_rating_and_comment(consumer, URI, rating, comment, broadcast);
}

DInteger wallet_api::restore_encryption_key(const string& consumer, buying_id_type buying)
{
   return my->restore_encryption_key(consumer, buying);
}

DInteger wallet_api::generate_encryption_key() const
{
   CryptoPP::Integer secret(randomGenerator, 256);
   return secret;
}


vector<buying_object> wallet_api::get_open_buyings() const
{
   return my->_remote_db->get_open_buyings();
}

vector<buying_object> wallet_api::get_open_buyings_by_URI( const string& URI ) const
{
   return my->_remote_db->get_open_buyings_by_URI( URI );
}

vector<buying_object> wallet_api::get_open_buyings_by_consumer( const string& account_id_or_name ) const
{
   account_id_type consumer = get_account( account_id_or_name ).id;
   return my->_remote_db->get_open_buyings_by_consumer( consumer );
}

vector<buying_object> wallet_api::get_buying_history_objects_by_consumer( const string& account_id_or_name ) const
{
   account_id_type consumer = get_account( account_id_or_name ).id;
   vector<buying_object> result = my->_remote_db->get_buying_history_objects_by_consumer( consumer );

   for (int i = 0; i < (int)result.size(); ++i)
   {
      buying_object& bobj = result[i];

      optional<content_object> content = my->_remote_db->get_content( bobj.URI );
      if (!content)
         continue;
      optional<asset> op_price = content->price.GetPrice(bobj.region_code_from);
      if (!op_price)
         continue;

      bobj.price = *op_price;

      bobj.size = content->size;
      bobj.rating = content->AVG_rating;
      bobj.synopsis = content->synopsis;

   }
   return result;
}


vector<buying_object_ex> wallet_api::search_my_purchases(const string& account_id_or_name,
                                                         const string& term,
                                                         const string& order,
                                                         const string& id,
                                                         uint32_t count) const
{
   account_id_type consumer = get_account( account_id_or_name ).id;
   vector<buying_object> bobjects = my->_remote_db->get_buying_objects_by_consumer(consumer, order, object_id_type(id), term, count );
   vector<buying_object_ex> result;

   for (size_t i = 0; i < bobjects.size(); ++i)
   {
      buying_object const& buyobj = bobjects[i];

      optional<content_download_status> status = get_download_status(account_id_or_name, buyobj.URI);
      if (!status)
         continue;

      optional<content_object> content = my->_remote_db->get_content( buyobj.URI );
      if (!content)
         continue;

      result.emplace_back(buying_object_ex(bobjects[i], *status));
      buying_object_ex& bobj = result.back();

      bobj.author_account = my->get_account(content->author).name;
      bobj.times_bought = content->times_bought;
      bobj.hash = content->_hash;
      bobj.AVG_rating = content->AVG_rating;
      bobj.rating = content->AVG_rating;
   }

   return result;
}

optional<buying_object> wallet_api::get_buying_by_consumer_URI( const string& account_id_or_name, const string& URI ) const
{
   account_id_type account = get_account( account_id_or_name ).id;
   return my->_remote_db->get_buying_by_consumer_URI( account, URI );
}

vector<rating_object_ex> wallet_api::search_feedback(const string& user,
                                                  const string& URI,
                                                  const string& id,
                                                  uint32_t count) const
{
    vector<rating_object_ex> result;
    vector<buying_object> temp = my->_remote_db->search_feedback(user, URI, object_id_type(id), count);

    for (auto const& item : temp)
       result.push_back(rating_object_ex( item, get_account(string(object_id_type(item.consumer))).name ));

    return result;
}

optional<content_object> wallet_api::get_content( const string& URI ) const
{
    return my->_remote_db->get_content( URI );
}


vector<content_summary> wallet_api::search_content(const string& term,
                                                   const string& order,
                                                   const string& user,
                                                   const string& region_code,
                                                   const string& id,
                                                   const string& type,
                                                   uint32_t count)const
{
   return my->_remote_db->search_content(term, order, user, region_code, object_id_type(id), type, count);
}

vector<content_summary> wallet_api::search_user_content(const string& user,
                                                       const string& term,
                                                       const string& order,
                                                       const string& region_code,
                                                       const string& id,
                                                       const string& type,
                                                       uint32_t count)const
{
  vector<content_summary> result = my->_remote_db->search_content(term, order, user, region_code, object_id_type(id), type, count);

  auto packages = PackageManager::instance().get_all_known_packages();
  for (auto package: packages)
  {
     auto state = package->get_manipulation_state();

     if (package->get_data_state() == PackageInfo::CHECKED){
        bool cont = false;
        auto hash = package->get_hash();
        for( const auto& item : result )
           if( item._hash == hash )
              cont = true;
        if(cont)
           continue;
     }

     bool cont = false;
     for (auto listener: my->_package_manager_listeners)
     {

        if (listener->get_hash() == package->get_hash())
        {
           cont = true;
           content_summary newObject;
           newObject.synopsis = listener->op().synopsis;
           newObject.expiration = listener->op().expiration;
           newObject.author = my->get_account( listener->op().author ).name;

           if (state == PackageInfo::PACKING) {
              newObject.status = "Packing";
           } else if (state == PackageInfo::ENCRYPTING) {
              newObject.status = "Encrypting";
           } else if (state == PackageInfo::STAGING) {
              newObject.status = "Staging";
           } else if (state == PackageInfo:: MS_IDLE )
              newObject.status = "Submission failed";

           result.insert(result.begin(), newObject);
        }
     }
     if (cont)
        continue;
  }

  return result;
}

pair<account_id_type, vector<account_id_type>> wallet_api::get_author_and_co_authors_by_URI( const string& URI )const
{
   return my->get_author_and_co_authors_by_URI( URI );
}

std::pair<string, decent::encrypt::CustodyData>  wallet_api::create_package(const std::string& content_dir,
                                                                            const std::string& samples_dir,
                                                                            const DInteger& aes_key) const
{
   FC_ASSERT(!is_locked());
   fc::sha256 key1;
#if CRYPTOPP_VERSION >= 600
   aes_key.Encode((CryptoPP::byte*)key1._hash, 32);
#else
   aes_key.Encode((byte*)key1._hash, 32);
#endif

   uint32_t sectors;
   if(my->head_block_time()>HARDFORK_1_TIME)
      sectors = DECENT_SECTORS;
   else
      sectors = DECENT_SECTORS_BIG;
   auto pack = PackageManager::instance().get_package(content_dir, samples_dir, key1, sectors);
   pack->create( true );
   decent::encrypt::CustodyData cd = pack->get_custody_data();
   return std::pair<string, decent::encrypt::CustodyData>(pack->get_hash().str(), cd);
}

void wallet_api::extract_package(const std::string& package_hash, const std::string& output_dir, const DInteger& aes_key) const
{
   FC_ASSERT(!is_locked());
   fc::sha256 key1;
#if CRYPTOPP_VERSION >= 600
   aes_key.Encode((CryptoPP::byte*)key1._hash, 32);
#else
   aes_key.Encode((byte*)key1._hash, 32);
#endif

   auto pack = PackageManager::instance().find_package(fc::ripemd160(package_hash));
   if (pack == nullptr) {
       FC_THROW("Can not find package");
   }

   if (pack->get_manipulation_state() != PackageInfo::ManipulationState::MS_IDLE) {
       FC_THROW("Package is not in valid state");
   }
   pack->unpack(output_dir, key1);
}

void wallet_api::download_package(const std::string& url) const
{
   FC_ASSERT(!is_locked());
   auto content = get_content(url);
   FC_ASSERT(content, "no such package in the system");
   auto pack = PackageManager::instance().get_package(url, content->_hash );
   pack->download(false);
}

std::string wallet_api::upload_package(const std::string& package_hash, const std::string& protocol) const
{
   FC_ASSERT(!is_locked());
   auto package = PackageManager::instance().get_package(fc::ripemd160(package_hash));
   package->start_seeding(protocol, true);
   return package->get_url();
}

void wallet_api::remove_package(const std::string& package_hash) const
{
   FC_ASSERT(!is_locked());
   PackageManager::instance().release_package(fc::ripemd160(package_hash));
}

content_keys wallet_api::generate_content_keys(vector<account_id_type> const& seeders) const
{
   return my->_remote_db->generate_content_keys(seeders);
}
