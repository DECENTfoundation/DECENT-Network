
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

void wallet_api::unlock(string password)
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

void wallet_api::set_password( string password )
{
   if( !is_new() )
      FC_ASSERT( !is_locked(), "The wallet must be unlocked before the password can be set" );
   my->_checksum = fc::sha512::hash( password.c_str(), password.size() );
   lock();
}

bool wallet_api::load_wallet_file( string wallet_filename )
{
   return my->load_wallet_file( wallet_filename );
}

void wallet_api::save_wallet_file( string wallet_filename )
{
   my->save_wallet_file( wallet_filename );
}

void wallet_api::set_wallet_filename(string wallet_filename)
{
   my->_wallet_filename = wallet_filename;
}

bool wallet_api::import_key(string account_name_or_id, string wif_key)
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

//obsolete
map<string, bool> wallet_api::import_accounts( string filename, string password )
{
   FC_ASSERT( !is_locked() );
   FC_ASSERT( fc::exists( filename ) );

   const auto imported_keys = fc::json::from_file<exported_keys>( filename );

   const auto password_hash = fc::sha512::hash( password );
   FC_ASSERT( fc::sha512::hash( password_hash ) == imported_keys.password_checksum );

   map<string, bool> result;
   for( const auto& item : imported_keys.account_keys )
   {
       const auto import_this_account = [ & ]() -> bool
       {
           try
           {
               const account_object account = get_account( item.account_name );
               const auto& owner_keys = account.owner.get_keys();
               const auto& active_keys = account.active.get_keys();

               for( const auto& public_key : item.public_keys )
               {
                   if( std::find( owner_keys.begin(), owner_keys.end(), public_key ) != owner_keys.end() )
                       return true;

                   if( std::find( active_keys.begin(), active_keys.end(), public_key ) != active_keys.end() )
                       return true;
               }
           }
           catch( ... )
           {
           }

           return false;
       };

       const auto should_proceed = import_this_account();
       result[ item.account_name ] = should_proceed;

       if( should_proceed )
       {
           uint32_t import_successes = 0;
           uint32_t import_failures = 0;
           // TODO: First check that all private keys match public keys
           for( const auto& encrypted_key : item.encrypted_private_keys )
           {
               try
               {
                  const auto plain_text = fc::aes_decrypt( password_hash, encrypted_key );
                  const auto private_key = fc::raw::unpack<private_key_type>( plain_text );

                  import_key( item.account_name, string( graphene::utilities::key_to_wif( private_key ) ) );
                  ++import_successes;
               }
               catch( const fc::exception& e )
               {
                  elog( "Couldn't import key due to exception ${e}", ("e", e.to_detail_string()) );
                  ++import_failures;
               }
           }
           ilog( "successfully imported ${n} keys for account ${name}", ("n", import_successes)("name", item.account_name) );
           if( import_failures > 0 )
              elog( "failed to import ${n} keys for account ${name}", ("n", import_failures)("name", item.account_name) );
       }
   }

   return result;
}

//obsolete
bool wallet_api::import_account_keys( string filename, string password, string src_account_name, string dest_account_name )
{
   FC_ASSERT( !is_locked() );
   FC_ASSERT( fc::exists( filename ) );

   bool is_my_account = false;
   const auto accounts = list_my_accounts();
   for( const auto& account : accounts )
   {
       if( account.name == dest_account_name )
       {
           is_my_account = true;
           break;
       }
   }
   FC_ASSERT( is_my_account );

   const auto imported_keys = fc::json::from_file<exported_keys>( filename );

   const auto password_hash = fc::sha512::hash( password );
   FC_ASSERT( fc::sha512::hash( password_hash ) == imported_keys.password_checksum );

   bool found_account = false;
   for( const auto& item : imported_keys.account_keys )
   {
       if( item.account_name != src_account_name )
           continue;

       found_account = true;

       for( const auto& encrypted_key : item.encrypted_private_keys )
       {
           const auto plain_text = fc::aes_decrypt( password_hash, encrypted_key );
           const auto private_key = fc::raw::unpack<private_key_type>( plain_text );

           my->import_key( dest_account_name, string( graphene::utilities::key_to_wif( private_key ) ) );
       }

       return true;
   }
   save_wallet_file();

   FC_ASSERT( found_account );

   return false;
}

map<public_key_type, string> wallet_api::dump_private_keys()
{
   FC_ASSERT(!is_locked());
   return my->_keys;
}








