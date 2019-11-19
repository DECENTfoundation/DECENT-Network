std::vector<account_object> wallet_api::list_my_accounts() const
{
   return std::vector<account_object>(my->_wallet.my_accounts.begin(), my->_wallet.my_accounts.end());
}

path wallet_api::get_wallet_filename() const
{
   return my->get_wallet_filename();
}

void wallet_api::set_wallet_filename( const path& wallet_filename )
{
   return my->set_wallet_filename( wallet_filename );
}

std::string wallet_api::get_private_key(const public_key_type& pubkey) const
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");

   return key_to_wif( my->get_private_key( pubkey ) );
}

bool wallet_api::is_new()const
{
   return my->_wallet.cipher_keys.size() == 0;
}

bool wallet_api::is_locked()const
{
   return my->is_locked();
}

bool wallet_api::lock()
{ try {
   if(is_locked()) {
      std::cout << "Wallet is already locked" << std::endl;
      return false;
   }

   my->encrypt_keys2();
   for( auto & key : my->_keys )
      key.second = key_to_wif(fc::ecc::private_key());
   my->_keys.clear();
   my->_el_gamal_keys.clear();
   my->_checksum = fc::sha512();
   my->self.lock_changed(true);
   return true;
} FC_RETHROW() }

bool wallet_api::unlock(const string& password)
{ try {
   if(!is_locked()) {
      std::cout << "Wallet is already unlocked" << std::endl;
      return false;
   }

   if(password.size() == 0)
      FC_THROW_EXCEPTION(password_cannot_be_empty_exception, "");

   auto pw = fc::sha512::hash(password.c_str(), password.size());
   std::vector<char> decrypted = fc::aes_decrypt(pw, my->_wallet.cipher_keys);
   plain_ec_and_el_gamal_keys pk;
   bool update_wallet_file = false;

   if (my->_wallet.version == 0) {

      // supporting backward compatibility of wallet json file
      try {
         string data;
         data.reserve(decrypted.size());
         std::copy(decrypted.begin(), decrypted.end(), back_inserter(data));
         pk = fc::json::from_string(data).as<plain_ec_and_el_gamal_keys>();
         update_wallet_file = true;
      }
      catch(const fc::exception&)
      {
         pk = fc::raw::unpack<plain_keys>(decrypted);
         // wallet file is in old format, derive corresponding el gamal keys from private keys
         for( const auto& element : pk.ec_keys )
         {
            el_gamal_key_pair_str el_gamal_keys_str;
            el_gamal_keys_str.private_key = decent::encrypt::generate_private_el_gamal_key_from_secret( wif_to_key( element.second )->get_secret() );
            el_gamal_keys_str.public_key = get_public_el_gamal_key( el_gamal_keys_str.private_key );
            pk.el_gamal_keys.push_back( el_gamal_keys_str );
         }
         update_wallet_file = true;
      }
   }
   else {

      try {
         string data;
         data.reserve(decrypted.size());
         std::copy(decrypted.begin(), decrypted.end(), back_inserter(data));
         pk = fc::json::from_string(data).as<plain_ec_and_el_gamal_keys>();
      }
      catch(const fc::exception&) {
         throw;
      }
   }

   FC_ASSERT(pk.checksum == pw);
   my->_keys = std::move(pk.ec_keys);
   if( !pk.el_gamal_keys.empty() ) {
      std::transform(pk.el_gamal_keys.begin(), pk.el_gamal_keys.end(),
                     std::inserter(my->_el_gamal_keys, my->_el_gamal_keys.end()),
                     [](const el_gamal_key_pair_str el_gamal_pair) {
                         return std::make_pair(decent::encrypt::DInteger(el_gamal_pair.public_key), decent::encrypt::DInteger(el_gamal_pair.private_key));
                     });
   }
   my->_checksum = pk.checksum;

   if( update_wallet_file ) // upgrade structure for storing keys to new format
      save_wallet_file();

   my->self.lock_changed(false);
   return true;
} FC_RETHROW() }

void wallet_api::set_password(const string& password )
{
   if(!is_new()) {
      if(my->is_locked())
         FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   }
   my->_checksum = fc::sha512::hash( password.c_str(), password.size() );
   lock();
}

bool wallet_api::load_wallet_file(const path& wallet_filename )
{
   if( !wallet_filename.empty() )
      my->set_wallet_filename(wallet_filename);
   return my->load_wallet_file( wallet_filename );
}

void wallet_api::save_wallet_file(const path& wallet_filename )
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   my->save_wallet_file( wallet_filename );
}

bool wallet_api::import_key(const string& account_name_or_id, const string& wif_key)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   bool result = my->import_key(account_name_or_id, wif_key);
   save_wallet_file();

   return result;
}

bool wallet_api::import_single_key(const string& account_name_or_id, const string& wif_key)
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   bool result = my->import_single_key(account_name_or_id, wif_key);
   save_wallet_file();

   return result;
}

fc::variant wallet_api::dump_private_keys() const
{
   if(my->is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   fc::mutable_variant_object result;
   result["ec_keys"] = my->_keys;
   result["el_gamal_keys"] = my->_el_gamal_keys;   // map of public keys to private keys

   return result;
}
