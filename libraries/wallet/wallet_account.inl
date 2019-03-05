
std::string wallet_api::derive_private_key(const std::string& prefix_string, int sequence_number) const
{
   fc::ecc::private_key private_key = graphene::utilities::derive_private_key( prefix_string, sequence_number );
   return graphene::utilities::key_to_wif( private_key );
}

graphene::chain::public_key_type wallet_api::get_public_key( const std::string& wif_private_key ) const
{
   fc::optional<fc::ecc::private_key> private_key = graphene::utilities::wif_to_key( wif_private_key );
   FC_ASSERT( private_key , "invalid wif private key");
   return private_key->get_public_key();
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

vector<extended_asset> wallet_api::list_account_balances(const string& id)
{
   vector<asset> assets;
   if( auto real_id = detail::maybe_id<account_id_type>(id) )
      assets = my->_remote_db->get_account_balances(*real_id, flat_set<asset_id_type>());
   assets = my->_remote_db->get_account_balances(get_account(id).id, flat_set<asset_id_type>());

   vector<extended_asset> result;
   vector<asset_id_type> asset_ids;
   result.reserve( assets.size() );
   asset_ids.reserve( assets.size() );

   for( const asset& element : assets )
      asset_ids.push_back( element.asset_id );

   vector<optional<asset_object>> asset_objs = my->_remote_db->get_assets( asset_ids ) ;

   for( size_t i = 0; i < assets.size(); i++ )
      result.emplace_back( assets[i], asset_objs[i]->amount_to_pretty_string( assets[i].amount ) );
   FC_ASSERT( assets.size() == result.size() );

   return result;
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
      if( (int)current.size() < std::min(100,limit) )
         break;
      limit -= static_cast<int>(current.size());
   }

   return result;
}

vector<balance_operation_detail> wallet_api::search_account_balance_history(const string& account_name,
                                                                            const flat_set<string>& assets_list,
                                                                            const string& partner_account,
                                                                            uint32_t from_block, uint32_t to_block,
                                                                            uint32_t start_offset,
                                                                            int limit) const
{
    vector<balance_operation_detail> result;
    auto account_id = get_account(account_name).get_id();

    flat_set<asset_id_type> asset_id_list;
    if (!assets_list.empty()) {
        for( const auto& item : assets_list) {
           asset_id_list.insert( get_asset(item).get_id() );
        }
    }

   fc::optional<account_id_type> partner_id;
    if (!partner_account.empty()) {
       partner_id = get_account(partner_account).get_id();
    }

    vector<balance_change_result> current = my->_remote_hist->search_account_balance_history(account_id, asset_id_list, partner_id, from_block, to_block, start_offset, limit);
    result.reserve( current.size() );
    for(const auto& item : current) {
       balance_operation_detail info;
       info.hist_object = item.hist_object;
       info.balance     = item.balance;
       info.fee         = item.fee;

       std::stringstream ss;
       info.memo = item.hist_object.op.visit(detail::operation_printer(ss, *my, item.hist_object.result));

       result.push_back(info);
    }

    return result;
}

fc::optional<balance_operation_detail> wallet_api::get_account_balance_for_transaction(const string& account_name,
                                                                                       operation_history_id_type operation_history_id)
{
   auto account_id = get_account(account_name).get_id();

   fc::optional<balance_change_result> result;
   result = my->_remote_hist->get_account_balance_for_transaction(account_id, operation_history_id);
   if (!result) {
      return fc::optional<balance_operation_detail>();
   }

   balance_operation_detail info;
   info.hist_object = result->hist_object;
   info.balance     = result->balance;
   info.fee         = result->fee;

   std::stringstream ss;
   info.memo = result->hist_object.op.visit(detail::operation_printer(ss, *my, result->hist_object.result));

   return info;
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
            auto it2 = it == my->_keys.end() ? my->_keys.find(memo->from) : it;

            if (it2 == my->_keys.end())
               // memo is encrypted for someone else
               item.m_str_description += "{encrypted}";
            else
               // here the memo is encrypted for/by me so I can decrypt it
               item.m_str_description += memo->get_message(*wif_to_key(it2->second), it == it2 ? memo->from : memo->to);
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
   result.brain_priv_key = graphene::utilities::generate_brain_key();

   fc::ecc::private_key priv_key = graphene::utilities::derive_private_key(result.brain_priv_key);
   result.wif_priv_key = key_to_wif( priv_key );
   result.pub_key = priv_key.get_public_key();
   return result;
}

signed_transaction wallet_api::register_account_with_keys(const string& name,
                                                          public_key_type owner,
                                                          public_key_type active,
                                                          public_key_type memo,
                                                          const string& registrar_account,
                                                          bool broadcast /* = false */)
{
   return my->register_account( name, owner, active, memo, registrar_account,  broadcast );
}

signed_transaction wallet_api::register_account(const string& name,
                                                public_key_type owner,
                                                public_key_type active,
                                                const string& registrar_account,
                                                bool broadcast /* = false */)
{
   return my->register_account( name, owner, active, active, registrar_account, broadcast );
}

signed_transaction wallet_api::register_multisig_account(const string& name,
                                                         authority owner,
                                                         authority active,
                                                         public_key_type memo,
                                                         const string& registrar_account,
                                                         bool broadcast /* = false */)
{
   return my->register_multisig_account( name, owner, active, memo, registrar_account,  broadcast );
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

signed_transaction wallet_api::update_account_keys(const string& name,
                                                   const string& owner,
                                                   const string& active,
                                                   const string& memo,
                                                   bool broadcast /* = false */)
{
   fc::optional<authority> new_owner, new_active;
   fc::optional<public_key_type> new_memo;
   account_object acc = my->get_account( name );

   if( !owner.empty() )
      new_owner = authority( 1, public_key_type( owner ), 1 );

   if( !active.empty() )
      new_active = authority( 1, public_key_type( active ), 1 );

   if( !memo.empty() )
      new_memo = public_key_type( memo );

   return my->update_account_keys( name, new_owner, new_active, new_memo, broadcast );
}

signed_transaction wallet_api::update_account_keys_to_multisig(const string& name,
                                                               authority owner,
                                                               authority active,
                                                               public_key_type memo,
                                                               bool broadcast /* = false */)
{
   fc::optional<authority> new_owner, new_active;
   fc::optional<public_key_type> new_memo;
   account_object acc = my->get_account( name );

   if( acc.owner != owner )
      new_owner = owner;

   if( acc.active != active )
      new_active = active;

   if( acc.options.memo_key != memo )
      new_memo = memo;

   FC_ASSERT( new_owner || new_active || new_memo, "new authority needs to be different from the existing one" );

   return my->update_account_keys( name, new_owner, new_active, new_memo, broadcast );
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
   result.brain_priv_key = graphene::utilities::normalize_brain_key( brain_key );

   fc::ecc::private_key priv_key = graphene::utilities::derive_private_key( result.brain_priv_key );
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
