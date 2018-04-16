
fc::ecc::private_key wallet_api::derive_private_key(const std::string& prefix_string, int sequence_number) const
{
   return detail::derive_private_key( prefix_string, sequence_number );
}

uint64_t wallet_api::get_account_count() const
{
   return my->_remote_db->get_account_count();
}

map<string,account_id_type> wallet_api::list_accounts(const string& lowerbound, uint32_t limit)
{
    return my->_remote_db->lookup_accounts(lowerbound, limit);
}

vector<account_object> wallet_api::search_accounts(const string& term, const string& order, const string& id, uint32_t limit)
{
   return my->_remote_db->search_accounts(term, order, object_id_type(id), limit);
}

vector<asset> wallet_api::list_account_balances(const string& id)
{
   if( auto real_id = detail::maybe_id<account_id_type>(id) )
      return my->_remote_db->get_account_balances(*real_id, flat_set<asset_id_type>());
   return my->_remote_db->get_account_balances(get_account(id).id, flat_set<asset_id_type>());
}

vector<operation_detail> wallet_api::get_account_history(const string& name, int limit)const
{
   vector<operation_detail> result;
   auto account_id = get_account(name).get_id();

   while( limit > 0 )
   {
      operation_history_id_type start;
      if( result.size() )
      {
         start = result.back().op.id;
         start = start + 1;
      }


      vector<operation_history_object> current = my->_remote_hist->get_account_history(account_id, operation_history_id_type(), std::min(100,limit), start);
      for( auto& o : current ) {
         std::stringstream ss;
         auto memo = o.op.visit(detail::operation_printer(ss, *my, o.result));
         result.push_back( operation_detail{ memo, ss.str(), o } );
      }
      if( current.size() < std::min(100,limit) )
         break;
      limit -= current.size();
   }

   return result;
}

vector<operation_detail> wallet_api::get_relative_account_history(const string& name,
                                                                  uint32_t stop,
                                                                  int limit,
                                                                  uint32_t start)const
{
   vector<operation_detail> result;
   auto account_id = get_account(name).get_id();

   vector<operation_history_object> current = my->_remote_hist->get_relative_account_history(account_id, stop, limit, start);
   for( auto& o : current ) {
      std::stringstream ss;
      auto memo = o.op.visit(detail::operation_printer(ss, *my, o.result));
      result.push_back( operation_detail{ memo, ss.str(), o } );
   }

   return result;
}

vector<transaction_detail_object> wallet_api::search_account_history(string const& account_name,
                                                                     string const& order,
                                                                     string const& id,
                                                                     int limit) const
{
   vector<transaction_detail_object> result;
   try
   {
      account_object account = get_account(account_name);
      result = my->_remote_db->search_account_history(account.id, order, object_id_type(id), limit);

      for (auto& item : result)
      {
         auto const& memo = item.m_transaction_encrypted_memo;
         if (memo)
         {
            item.m_str_description += " - ";
            auto it = my->_keys.find(memo->to);
            if (it == my->_keys.end())
               // memo is encrypted for someone else
               item.m_str_description += "{encrypted}";
            else
            {
               // here the memo is encrypted for me
               // so I can decrypt it
               string mykey = it->second;
               auto wtok = *wif_to_key(mykey);
               string str_memo =
                  memo->get_message(wtok, memo->from);

               item.m_str_description += str_memo;
            }
         }
      }
   }
   catch(...){}

   return result;
}

account_object wallet_api::get_account(const string& account_name_or_id) const
{
   return my->get_account(account_name_or_id);
}

brain_key_info wallet_api::suggest_brain_key() const
{
   brain_key_info result;
   // create a private key for secure entropy
   fc::sha256 sha_entropy1 = fc::ecc::private_key::generate().get_secret();
   fc::sha256 sha_entropy2 = fc::ecc::private_key::generate().get_secret();
   fc::bigint entropy1( sha_entropy1.data(), sha_entropy1.data_size() );
   fc::bigint entropy2( sha_entropy2.data(), sha_entropy2.data_size() );
   fc::bigint entropy(entropy1);
   entropy <<= 8*sha_entropy1.data_size();
   entropy += entropy2;
   string brain_key;

   for( int i=0; i<BRAIN_KEY_WORD_COUNT; i++ )
   {
      fc::bigint choice = entropy % graphene::words::word_list_size;
      entropy /= graphene::words::word_list_size;
      if( i > 0 )
         brain_key += " ";
      brain_key += graphene::words::word_list[ choice.to_int64() ];
   }

   brain_key = detail::normalize_brain_key(brain_key);
   fc::ecc::private_key priv_key = derive_private_key( brain_key, 0 );
   result.brain_priv_key = brain_key;
   result.wif_priv_key = key_to_wif( priv_key );
   result.pub_key = priv_key.get_public_key();
   return result;
}


signed_transaction wallet_api::register_account(const string& name,
                                                public_key_type owner_pubkey,
                                                public_key_type active_pubkey,
                                                const string& registrar_account,
                                                bool broadcast)
{
   return my->register_account( name, owner_pubkey, active_pubkey, registrar_account,  broadcast );
}

signed_transaction wallet_api::create_account_with_brain_key(const string& brain_key,
                                                             const string& account_name,
                                                             const string& registrar_account,
                                                             bool broadcast /* = false */)
{
   return my->create_account_with_brain_key(
            brain_key, account_name, registrar_account, true,
            broadcast);
}

el_gamal_key_pair wallet_api::generate_el_gamal_keys() const
{
   el_gamal_key_pair ret;
   ret.private_key = decent::encrypt::generate_private_el_gamal_key();
   ret.public_key = decent::encrypt::get_public_el_gamal_key( ret.private_key );
   return ret;
}

el_gamal_key_pair_str wallet_api::get_el_gammal_key(string const& consumer) const
{
   try
   {
      FC_ASSERT( !is_locked() );

      account_object consumer_account = get_account( consumer );
      el_gamal_key_pair_str res;

      res.private_key = generate_private_el_gamal_key_from_secret ( my->get_private_key_for_account(consumer_account).get_secret() );
      res.public_key = decent::encrypt::get_public_el_gamal_key( res.private_key );
      return res;
   } FC_CAPTURE_AND_RETHROW( (consumer) )
}

pair<brain_key_info, el_gamal_key_pair> wallet_api::generate_brain_key_el_gamal_key() const
{
   pair<brain_key_info, el_gamal_key_pair> ret;
   ret.first = suggest_brain_key();

   fc::optional<fc::ecc::private_key> op_private_key = wif_to_key(ret.first.wif_priv_key);
   FC_ASSERT(op_private_key);
   ret.second.private_key = generate_private_el_gamal_key_from_secret ( op_private_key->get_secret() );
   ret.second.public_key = decent::encrypt::get_public_el_gamal_key( ret.second.private_key );

   return ret;
}

brain_key_info wallet_api::get_brain_key_info(string const& brain_key) const
{
   brain_key_info result;

   string str_brain_key = detail::normalize_brain_key(brain_key);
   fc::ecc::private_key priv_key = derive_private_key( str_brain_key, 0 );
   result.brain_priv_key = str_brain_key;
   result.wif_priv_key = key_to_wif( priv_key );
   result.pub_key = priv_key.get_public_key();
   return result;
}

signed_transaction wallet_api::transfer(const string& from, const string& to,
                                        const string& amount,
                                        const string& asset_symbol,
                                        const string& memo,
                                        bool broadcast /* = false */)
{
   return my->transfer(from, to, amount, asset_symbol, memo, broadcast);
}

pair<transaction_id_type,signed_transaction> wallet_api::transfer2(const string& from,
                                                                   const string& to,
                                                                   const string& amount,
                                                                   const string& asset_symbol,
                                                                   const string& memo)
{
   auto trx = transfer( from, to, amount, asset_symbol, memo, true );
   return std::make_pair(trx.id(),trx);
}

