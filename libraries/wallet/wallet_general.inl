

variant_object wallet_api::about() const
{
    return my->about();
}

optional<signed_block_with_info> wallet_api::get_block(uint32_t num)
{
   optional<signed_block_with_info> result = my->_remote_db->get_block(num);
   if( !result )
      return optional<signed_block_with_info>();

   share_type miner_pay_from_fees = my->_remote_db->get_miner_pay_from_fees_by_block_time(result->timestamp);
   share_type miner_pay_from_reward = my->_remote_db->get_asset_per_block_by_block_num(num);

   //this should never happen, but better check.
   if (miner_pay_from_fees < share_type(0))
      miner_pay_from_fees = share_type(0);

   result->miner_reward = miner_pay_from_fees + miner_pay_from_reward;

   return result;
}

global_property_object wallet_api::get_global_properties() const
{
   return my->get_global_properties();
}

dynamic_global_property_object wallet_api::get_dynamic_global_properties() const
{
   return my->get_dynamic_global_properties();
}

variant wallet_api::get_object( object_id_type id ) const
{
   return my->_remote_db->get_objects({id});
}

variant wallet_api::info()
{
   return my->info();
}


string wallet_api::help() const
{
   std::vector<std::string> method_names = my->method_documentation.get_method_names();
   std::stringstream ss;
   for (const std::string method_name : method_names)
   {
      try
      {
         ss << my->method_documentation.get_brief_description(method_name);
      }
      catch (const fc::key_not_found_exception&)
      {
         ss << method_name << " (no help available)\n";
      }
   }
   return ss.str();
}

string wallet_api::get_help(const string& method)const
{
   fc::api<wallet_api> tmp;
   std::stringstream ss;
   ss << "\n";

   if( method == "import_key" )
   {
      ss << "usage: import_key ACCOUNT_NAME_OR_ID  WIF_PRIVATE_KEY\n\n";
      ss << "example: import_key \"1.2.11\" 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3\n";
      ss << "example: import_key \"usera\" 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3\n";
   }
   else if( method == "transfer" )
   {
      ss << "usage: transfer FROM TO AMOUNT SYMBOL \"memo\" BROADCAST\n\n";
      ss << "example: transfer \"1.2.11\" \"1.2.4\" 1000.03 DCT \"memo\" true\n";
      ss << "example: transfer \"usera\" \"userb\" 1000.123 DCT \"memo\" true\n";
   }
   else if( method == "create_account_with_brain_key" )
   {
      ss << "usage: create_account_with_brain_key BRAIN_KEY ACCOUNT_NAME REGISTRAR BROADCAST\n\n";
      ss << "example: create_account_with_brain_key \"my really long brain key\" \"newaccount\" \"1.2.11\" true\n";
      ss << "example: create_account_with_brain_key \"my really long brain key\" \"newaccount\" \"someaccount\" true\n";
      ss << "\n";
      ss << "This method should be used if you would like the wallet to generate new keys derived from the brain key.\n";
      ss << "The BRAIN_KEY will be used as the owner key, and the active key will be derived from the BRAIN_KEY.  Use\n";
      ss << "register_account if you already know the keys you know the public keys that you would like to register.\n";

   }
   else if( method == "register_account" )
   {
      ss << "usage: register_account ACCOUNT_NAME OWNER_PUBLIC_KEY ACTIVE_PUBLIC_KEY REGISTRAR BROADCAST\n\n";
      ss << "example: register_account \"newaccount\" \"CORE6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV\" \"CORE6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV\" \"1.3.11\" true\n";
      ss << "\n";
      ss << "Use this method to register an account for which you do not know the private keys.";
   }
   else if( method == "create_monitored_asset" )
   {
      ss << "usage: ISSUER SYMBOL PRECISION DESCRIPTION FEED_LIFETIME_SEC MINIMUM_FEEDS BROADCAST\n\n";
      ss << "PRECISION: the number of digits after the decimal point\n\n";

      ss << "\nExample value of MONITORED ASSET_OPTIONS: \n";
      ss << fc::json::to_pretty_string( graphene::chain::monitored_asset_options() );
   }
   else
   {
      std::string doxygenHelpString = my->method_documentation.get_brief_description(method).append(my->method_documentation.get_detailed_description(method));
      if (!doxygenHelpString.empty())
      {
         auto i = doxygenHelpString.find_first_not_of(" \t\n");
         ss << doxygenHelpString.substr(i == std::string::npos ? 0 : i);
      }
      else
         ss << "No help defined for method " << method << "\n";
   }

   return ss.str();
}

fc::time_point_sec wallet_api::head_block_time() const
{
   return my->head_block_time();
}


void wallet_api::network_add_nodes( const vector<string>& nodes )
{
   my->network_add_nodes( nodes );
}

vector< variant > wallet_api::network_get_connected_peers()
{
   return my->network_get_connected_peers();
}


std::string wallet_api::sign_buffer(const std::string& str_buffer,
                                    const std::string& str_brainkey) const
{
   if (str_buffer.empty() ||
       str_brainkey.empty())
      throw std::runtime_error("You need buffer and brainkey to sign");

   string normalized_brain_key = graphene::utilities::normalize_brain_key( str_brainkey );
   fc::ecc::private_key privkey = graphene::utilities::derive_private_key( normalized_brain_key );

   fc::sha256 digest(str_buffer);

   auto sign = privkey.sign_compact(digest);

   return fc::to_hex((const char*)sign.begin(), sign.size());
}


bool wallet_api::verify_signature(const std::string& str_buffer,
                                  const std::string& str_publickey,
                                  const std::string& str_signature) const
{
   if (str_buffer.empty() ||
       str_publickey.empty() ||
       str_signature.empty())
      throw std::runtime_error("You need buffer, public key and signature to verify");

   fc::ecc::compact_signature signature;
   fc::from_hex(str_signature, (char*)signature.begin(), signature.size());
   fc::sha256 digest(str_buffer);

   fc::ecc::public_key pub_key(signature, digest);
   public_key_type provided_key(str_publickey);

   return (provided_key == pub_key);
}

transaction_id_type wallet_api::get_transaction_id( const signed_transaction& trx ) const
{
   return trx.id();
}

optional<signed_transaction> wallet_api::get_transaction_by_id( const transaction_id_type& id ) const
{
   return my->_remote_db->get_transaction_by_id( id );
}

vector<operation_info> wallet_api::list_operations()
{
   return my->list_operations();
}

void wallet_api::from_command_file( const std::string& command_file_name ) const
{
   return my->from_command_file( command_file_name );
}
