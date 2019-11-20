signed_transaction_info wallet_api::submit_content(const std::string& author,
                                                   const std::vector<std::pair<std::string, uint32_t>>& co_authors,
                                                   const std::string& URI,
                                                   const std::vector<regional_price_info>& price_amounts,
                                                   uint64_t size,
                                                   const fc::ripemd160& hash,
                                                   const std::vector<account_id_type>& seeders,
                                                   uint32_t quorum,
                                                   const fc::time_point_sec& expiration,
                                                   const std::string& publishing_fee_asset,
                                                   const std::string& publishing_fee_amount,
                                                   const std::string& synopsis,
                                                   const decent::encrypt::DInteger& secret,
                                                   const decent::encrypt::CustodyData& cd,
                                                   bool broadcast)
{
   FC_ASSERT( !my->is_locked(), "the wallet is locked and needs to be unlocked to have access to private keys" );
   return my->submit_content(author, co_authors, URI, price_amounts, hash, size,
                             seeders, quorum, expiration, publishing_fee_asset, publishing_fee_amount,
                             synopsis, secret, cd, broadcast);
}

content_keys wallet_api::submit_content_async(const std::string& author,
                                              const std::vector<std::pair<std::string, uint32_t>>& co_authors,
                                              const std::string& content_dir,
                                              const std::string& samples_dir,
                                              const std::string& protocol,
                                              const std::vector<regional_price_info>& price_amounts,
                                              const std::vector<account_id_type>& seeders,
                                              const fc::time_point_sec& expiration,
                                              const std::string& synopsis)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->submit_content_async(author, co_authors, content_dir, samples_dir, protocol, price_amounts, seeders, expiration, synopsis);
}

signed_transaction_info wallet_api::content_cancellation(const std::string& author,
                                                         const std::string& URI,
                                                         bool broadcast)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->content_cancellation(author, URI, broadcast);
}

void wallet_api::download_content(const std::string& consumer, const std::string& URI, const std::string& region_code_from, bool broadcast)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->download_content(consumer, URI, region_code_from, broadcast);
}

content_download_status wallet_api::get_download_status(const std::string& consumer, const std::string& URI) const
{
   return my->get_download_status(consumer, URI);
}

signed_transaction_info wallet_api::request_to_buy(const std::string& consumer,
                                                   const std::string& URI,
                                                   const std::string& price_asset_name,
                                                   const std::string& price_amount,
                                                   const std::string& str_region_code_from,
                                                   bool broadcast)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->request_to_buy(consumer, URI, price_asset_name, price_amount, str_region_code_from, broadcast);
}

signed_transaction_info wallet_api::leave_rating_and_comment(const std::string& consumer,
                                                             const std::string& URI,
                                                             uint64_t rating,
                                                             const std::string& comment,
                                                             bool broadcast)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->leave_rating_and_comment(consumer, URI, rating, comment, broadcast);
}

decent::encrypt::DInteger wallet_api::restore_encryption_key(const std::string& consumer, buying_id_type buying)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->restore_encryption_key(consumer, buying);
}

decent::encrypt::DInteger wallet_api::generate_encryption_key() const
{
   CryptoPP::Integer secret(randomGenerator, 256);
   return secret;
}

std::vector<buying_object> wallet_api::get_open_buyings() const
{
   return my->_remote_db->get_open_buyings();
}

std::vector<buying_object> wallet_api::get_open_buyings_by_URI( const std::string& URI ) const
{
   return my->_remote_db->get_open_buyings_by_URI( URI );
}

std::vector<buying_object> wallet_api::get_open_buyings_by_consumer( const std::string& account_id_or_name ) const
{
   account_id_type consumer = get_account( account_id_or_name ).id;
   return my->_remote_db->get_open_buyings_by_consumer( consumer );
}

std::vector<buying_object> wallet_api::get_buying_history_objects_by_consumer( const std::string& account_id_or_name ) const
{
   account_id_type consumer = get_account( account_id_or_name ).id;
   std::vector<buying_object> result = my->_remote_db->get_buying_history_objects_by_consumer( consumer );

   for (int i = 0; i < (int)result.size(); ++i)
   {
      buying_object& bobj = result[i];

      fc::optional<content_object> content = my->_remote_db->get_content( bobj.URI );
      if (!content)
         continue;
      fc::optional<asset> op_price = content->price.GetPrice(bobj.region_code_from);
      if (!op_price)
         continue;

      bobj.price = *op_price;

      bobj.size = content->size;
      bobj.rating = content->AVG_rating;
      bobj.synopsis = content->synopsis;

   }
   return result;
}

std::vector<buying_object_ex> wallet_api::search_my_purchases(const std::string& account_id_or_name,
                                                              const std::string& term,
                                                              const std::string& order,
                                                              const std::string& id,
                                                              uint32_t count) const
{
   account_id_type consumer = get_account( account_id_or_name ).id;
   std::vector<buying_object> bobjects = my->_remote_db->get_buying_objects_by_consumer(consumer, order, db::object_id_type(id), term, count );
   std::vector<buying_object_ex> result;

   for (size_t i = 0; i < bobjects.size(); ++i)
   {
      buying_object const& buyobj = bobjects[i];
      content_download_status status = get_download_status(account_id_or_name, buyobj.URI);

      fc::optional<content_object> content = my->_remote_db->get_content( buyobj.URI );
      if (!content)
         continue;

      result.emplace_back(buying_object_ex(bobjects[i], status));
      buying_object_ex& bobj = result.back();

      bobj.author_account = my->get_account(content->author).name;
      bobj.times_bought = content->times_bought;
      bobj.hash = content->_hash;
      bobj.AVG_rating = content->AVG_rating;
      bobj.rating = content->AVG_rating;
   }

   return result;
}

fc::optional<buying_object> wallet_api::get_buying_by_consumer_URI( const std::string& account_id_or_name, const std::string& URI ) const
{
   account_id_type account = get_account( account_id_or_name ).id;
   return my->_remote_db->get_buying_by_consumer_URI( account, URI );
}

std::vector<rating_object_ex> wallet_api::search_feedback(const std::string& user, const std::string& URI, const std::string& id, uint32_t count) const
{
    std::vector<rating_object_ex> result;
    std::vector<buying_object> temp = my->_remote_db->search_feedback(user, URI, db::object_id_type(id), count);

    for (auto const& item : temp)
       result.push_back(rating_object_ex( item, get_account(std::string(db::object_id_type(item.consumer))).name));

    return result;
}

fc::optional<content_object> wallet_api::get_content( const std::string& URI ) const
{
    return my->_remote_db->get_content( URI );
}

std::vector<content_summary> wallet_api::search_content(const std::string& term,
                                                        const std::string& order,
                                                        const std::string& user,
                                                        const std::string& region_code,
                                                        const std::string& id,
                                                        const std::string& type,
                                                        uint32_t count)const
{
   return my->_remote_db->search_content(term, order, user, region_code, db::object_id_type(id), type, count);
}

std::vector<content_summary> wallet_api::search_user_content(const std::string& user,
                                                             const std::string& term,
                                                             const std::string& order,
                                                             const std::string& region_code,
                                                             const std::string& id,
                                                             const std::string& type,
                                                             uint32_t count)const
{
  std::vector<content_summary> result = my->_remote_db->search_content(term, order, user, region_code, db::object_id_type(id), type, count);

  auto packages = decent::package::PackageManager::instance().get_all_known_packages();
  for (auto package: packages)
  {
     auto state = package->get_manipulation_state();

     if (package->get_data_state() == decent::package::PackageInfo::CHECKED)
     {
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

           if (state == decent::package::PackageInfo::PACKING) {
              newObject.status = "Packing";
           } else if (state == decent::package::PackageInfo::ENCRYPTING) {
              newObject.status = "Encrypting";
           } else if (state == decent::package::PackageInfo::STAGING) {
              newObject.status = "Staging";
           } else if (state == decent::package::PackageInfo:: MS_IDLE )
              newObject.status = "Submission failed";

           result.insert(result.begin(), newObject);
        }
     }
     if (cont)
        continue;
  }

  return result;
}

std::pair<account_id_type, std::vector<account_id_type>> wallet_api::get_author_and_co_authors_by_URI( const std::string& URI )const
{
   return my->get_author_and_co_authors_by_URI( URI );
}

std::pair<std::string, decent::encrypt::CustodyData> wallet_api::create_package(const std::string& content_dir,
                                                                                const std::string& samples_dir,
                                                                                const decent::encrypt::DInteger& aes_key) const
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   fc::sha256 key1;
#if CRYPTOPP_VERSION >= 600
   aes_key.Encode((CryptoPP::byte*)key1._hash, 32);
#else
   aes_key.Encode((byte*)key1._hash, 32);
#endif

   auto pack = decent::package::PackageManager::instance().get_package(content_dir, samples_dir, key1);
   pack->create( true );
   decent::encrypt::CustodyData cd = pack->get_custody_data();
   return std::pair<std::string, decent::encrypt::CustodyData>(pack->get_hash().str(), cd);
}

void wallet_api::extract_package(const std::string& package_hash, const std::string& output_dir, const decent::encrypt::DInteger& aes_key) const
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   fc::sha256 key1;
#if CRYPTOPP_VERSION >= 600
   aes_key.Encode((CryptoPP::byte*)key1._hash, 32);
#else
   aes_key.Encode((byte*)key1._hash, 32);
#endif

   auto pack = decent::package::PackageManager::instance().find_package(fc::ripemd160(package_hash));
   if (pack == nullptr) {
      FC_THROW_EXCEPTION(cannot_find_package_exception, "");
   }

   if (pack->get_manipulation_state() != decent::package::PackageInfo::ManipulationState::MS_IDLE) {
      FC_THROW_EXCEPTION(package_is_not_in_valid_state_exception, "");
   }
   pack->unpack(output_dir, key1);
}

void wallet_api::download_package(const std::string& url) const
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   auto content = get_content(url);
   if(!content)
      FC_THROW_EXCEPTION(no_such_content_at_this_url_exception, "URL: ${url}", ("url", url));

   auto pack = decent::package::PackageManager::instance().get_package(url, content->_hash );
   pack->download(false);
}

std::string wallet_api::upload_package(const std::string& package_hash, const std::string& protocol) const
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   auto package = decent::package::PackageManager::instance().get_package(fc::ripemd160(package_hash));
   package->start_seeding(protocol, true);
   return package->get_url();
}

void wallet_api::remove_package(const std::string& package_hash) const
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   decent::package::PackageManager::instance().release_package(fc::ripemd160(package_hash));
}

content_keys wallet_api::generate_content_keys(std::vector<account_id_type> const& seeders) const
{
   return my->_remote_db->generate_content_keys(seeders);
}

bool wallet_api::is_package_manager_task_waiting() const
{
   return my->is_package_manager_task_waiting();
}
