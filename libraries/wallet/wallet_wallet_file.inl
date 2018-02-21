
vector<account_object> wallet_api::list_my_accounts()
{
   return vector<account_object>(my->_wallet.my_accounts.begin(), my->_wallet.my_accounts.end());
}

string wallet_api::get_wallet_filename() const
{
   return my->get_wallet_filename();
}

string wallet_api::get_private_key( public_key_type pubkey )const
{
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

void wallet_api::lock()
{ try {
   FC_ASSERT( !is_locked() );
   encrypt_keys();
   for( auto & key : my->_keys )
      key.second = key_to_wif(fc::ecc::private_key());
   my->_keys.clear();
   my->_checksum = fc::sha512();
   my->self.lock_changed(true);
} FC_CAPTURE_AND_RETHROW() }

void wallet_api::unlock(const string& password)
{ try {
   FC_ASSERT(password.size() > 0);
   auto pw = fc::sha512::hash(password.c_str(), password.size());
   vector<char> decrypted = fc::aes_decrypt(pw, my->_wallet.cipher_keys);
   auto pk = fc::raw::unpack<plain_keys>(decrypted);
   FC_ASSERT(pk.checksum == pw);
   my->_keys = std::move(pk.keys);
   my->_checksum = pk.checksum;
   my->self.lock_changed(false);
} FC_CAPTURE_AND_RETHROW() }

void wallet_api::set_password(const string& password )
{
   if( !is_new() )
      FC_ASSERT( !is_locked(), "The wallet must be unlocked before the password can be set" );
   my->_checksum = fc::sha512::hash( password.c_str(), password.size() );
   lock();
}

bool wallet_api::load_wallet_file(const string& wallet_filename )
{
   return my->load_wallet_file( wallet_filename );
}

void wallet_api::save_wallet_file(const string& wallet_filename )
{
   my->save_wallet_file( wallet_filename );
}

void wallet_api::set_wallet_filename(const string& wallet_filename)
{
   my->_wallet_filename = wallet_filename;
}

bool wallet_api::import_key(const string& account_name_or_id, const string& wif_key)
{
   FC_ASSERT(!is_locked());
   // backup wallet
   fc::optional<fc::ecc::private_key> optional_private_key = wif_to_key(wif_key);
   if (!optional_private_key)
      FC_THROW("Invalid private key");
   string base58_public_key = optional_private_key->get_public_key().to_base58();
//   copy_wallet_file( "before-import-key-" + base58_public_key );

   if( my->import_key(account_name_or_id, wif_key) )
   {
      save_wallet_file();
//      copy_wallet_file( "after-import-key-" + base58_public_key );
      return true;
   }
   return false;
}

map<public_key_type, string> wallet_api::dump_private_keys()
{
   FC_ASSERT(!is_locked());
   return my->_keys;
}








