std::string wallet_api::derive_private_key(const std::string& prefix_string, int sequence_number) const
{
   fc::ecc::private_key private_key = graphene::utilities::derive_private_key( prefix_string, sequence_number );
   return graphene::utilities::key_to_wif( private_key );
}

graphene::chain::public_key_type wallet_api::get_public_key( const std::string& wif_private_key ) const
{
   fc::optional<fc::ecc::private_key> private_key = graphene::utilities::wif_to_key( wif_private_key );
   if(!private_key)
      FC_THROW_EXCEPTION(invalid_wif_private_key_exception, "");

   return private_key->get_public_key();
}

uint64_t wallet_api::get_account_count() const
{
   return my->_remote_db->get_account_count();
}

std::map<std::string, account_id_type> wallet_api::list_accounts(const std::string& lowerbound, uint32_t limit) const
{
    return my->_remote_db->lookup_accounts(lowerbound, limit);
}

std::vector<account_object> wallet_api::search_accounts(const std::string& term, const std::string& order, const std::string& id, uint32_t limit) const
{
   return my->_remote_db->search_accounts(term, order, db::object_id_type(id), limit);
}

std::vector<extended_asset> wallet_api::list_account_balances(const std::string& id) const
{
   std::vector<asset> assets;
   if( auto real_id = detail::maybe_id<account_id_type>(id) )
      assets = my->_remote_db->get_account_balances(*real_id, boost::container::flat_set<asset_id_type>());
   assets = my->_remote_db->get_account_balances(get_account(id).id, boost::container::flat_set<asset_id_type>());

   std::vector<extended_asset> result;
   std::vector<asset_id_type> asset_ids;
   result.reserve( assets.size() );
   asset_ids.reserve( assets.size() );

   for( const asset& element : assets )
      asset_ids.push_back( element.asset_id );

   std::vector<fc::optional<asset_object>> asset_objs = my->_remote_db->get_assets( asset_ids ) ;

   for( size_t i = 0; i < assets.size(); i++ )
      result.emplace_back( assets[i], asset_objs[i]->amount_to_pretty_string( assets[i].amount ) );
   FC_ASSERT( assets.size() == result.size() );

   return result;
}

std::vector<operation_detail> wallet_api::get_account_history(const std::string& name, int limit) const
{
   std::vector<operation_detail> result;
   auto account_id = get_account(name).get_id();

   while( limit > 0 )
      {
      operation_history_id_type start;
      if( result.size() )
      {
         start = result.back().op.id;
         start = start + 1;
      }

      std::vector<operation_history_object> current = my->_remote_hist->get_account_history(account_id, operation_history_id_type(), std::min(100,limit), start);
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

std::vector<balance_change_result_detail> wallet_api::search_account_balance_history(const std::string& account_name,
                                                                                     const boost::container::flat_set<string>& assets_list,
                                                                                     const std::string& partner_account,
                                                                                     uint32_t from_block, uint32_t to_block,
                                                                                     uint32_t start_offset,
                                                                                     int limit) const
{
    std::vector<balance_change_result_detail> result;
    auto account_id = get_account(account_name).get_id();

    if( limit > 0 )
    {
       boost::container::flat_set<asset_id_type> asset_id_list;
       if (!assets_list.empty()) {
           for( const auto& item : assets_list) {
              asset_id_list.insert( get_asset(item).get_id() );
           }
       }

       fc::optional<account_id_type> partner_id;
       if (!partner_account.empty()) {
          partner_id = get_account(partner_account).get_id();
       }

       std::vector<balance_change_result> current = my->_remote_hist->search_account_balance_history(account_id, asset_id_list, partner_id, from_block, to_block, start_offset, limit);
       result.reserve( current.size() );
       for(const auto& item : current) {
          balance_change_result_detail info;
          info.hist_object = item.hist_object;
          info.balance     = item.balance;
          info.fee         = item.fee;
          info.timestamp   = item.timestamp;
          info.transaction_id = item.transaction_id;

          std::stringstream ss;
          info.memo = item.hist_object.op.visit(detail::operation_printer(ss, *my, item.hist_object.result));

          result.push_back(info);
       }
    }

    return result;
}

fc::optional<balance_change_result_detail> wallet_api::get_account_balance_for_transaction(const string& account_name, operation_history_id_type operation_history_id) const
{
   auto account_id = get_account(account_name).get_id();

   fc::optional<balance_change_result> result;
   result = my->_remote_hist->get_account_balance_for_transaction(account_id, operation_history_id);
   if (!result) {
      return fc::optional<balance_change_result_detail>();
   }

   balance_change_result_detail info;
   info.hist_object = result->hist_object;
   info.balance     = result->balance;
   info.fee         = result->fee;

   std::stringstream ss;
   info.memo = result->hist_object.op.visit(detail::operation_printer(ss, *my, result->hist_object.result));

   return info;
}

std::vector<operation_detail> wallet_api::get_relative_account_history(const std::string& name,
                                                                       uint32_t stop,
                                                                       int limit,
                                                                       uint32_t start) const
{
   std::vector<operation_detail> result;
   auto account_id = get_account(name).get_id();

   std::vector<operation_history_object> current = my->_remote_hist->get_relative_account_history(account_id, stop, limit, start);
   for( auto& o : current ) {
      std::stringstream ss;
      auto memo = o.op.visit(detail::operation_printer(ss, *my, o.result));
      result.push_back( operation_detail{ memo, ss.str(), o } );
   }

   return result;
}

std::vector<transaction_detail_object> wallet_api::search_account_history(std::string const& account_name,
                                                                          std::string const& order,
                                                                          std::string const& id,
                                                                          int limit) const
{
   std::vector<transaction_detail_object> result;
   try
   {
      account_object account = get_account(account_name);
      result = my->_remote_db->search_account_history(account.id, order, db::object_id_type(id), limit);

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

signed_transaction_info wallet_api::register_account_with_keys(const std::string& name,
                                                               const public_key_type& owner,
                                                               const public_key_type& active,
                                                               const public_key_type& memo,
                                                               const std::string& registrar_account,
                                                               bool broadcast /* = false */)
{
   if(is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->register_account( name, owner, active, memo, registrar_account,  broadcast );
}

signed_transaction_info wallet_api::register_account(const std::string& name,
                                                     const public_key_type& owner,
                                                     const public_key_type& active,
                                                     const std::string& registrar_account,
                                                     bool broadcast /* = false */)
{
   if(is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->register_account( name, owner, active, active, registrar_account, broadcast );
}

signed_transaction_info wallet_api::register_multisig_account(const std::string& name,
                                                              const authority& owner,
                                                              const authority& active,
                                                              const public_key_type& memo,
                                                              const std::string& registrar_account,
                                                              bool broadcast /* = false */)
{
   if(is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->register_multisig_account( name, owner, active, memo, registrar_account,  broadcast );
}

signed_transaction_info wallet_api::create_account_with_brain_key(const string& brain_key,
                                                                  const string& account_name,
                                                                  const string& registrar_account,
                                                                  bool broadcast /* = false */)
{
   if(is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->create_account_with_brain_key( brain_key, account_name, registrar_account, true, broadcast);
}

signed_transaction_info wallet_api::update_account_keys(const string& name,
                                                        const string& owner,
                                                        const string& active,
                                                        const string& memo,
                                                        bool broadcast /* = false */)
{
   if(is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
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

signed_transaction_info wallet_api::update_account_keys_to_multisig(const std::string& name,
                                                                    const authority& owner,
                                                                    const authority& active,
                                                                    const public_key_type& memo,
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

   if(!new_owner && !new_active && !new_memo)
      FC_THROW_EXCEPTION(new_auth_needs_to_be_different_from_existing_exception, "");

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
      if(is_locked())
         FC_THROW_EXCEPTION(wallet_is_locked_exception, "");

      account_object consumer_account = get_account( consumer );
      el_gamal_key_pair_str res;

      res.private_key = decent::encrypt::generate_private_el_gamal_key_from_secret ( my->get_private_key_for_account(consumer_account).get_secret() );
      res.public_key = decent::encrypt::get_public_el_gamal_key( res.private_key );
      return res;
   } FC_CAPTURE_AND_RETHROW( (consumer) )
}

std::pair<brain_key_info, el_gamal_key_pair> wallet_api::generate_brain_key_el_gamal_key() const
{
   std::pair<brain_key_info, el_gamal_key_pair> ret;
   ret.first = suggest_brain_key();

   fc::optional<fc::ecc::private_key> op_private_key = wif_to_key(ret.first.wif_priv_key);
   FC_ASSERT(op_private_key);
   ret.second.private_key = decent::encrypt::generate_private_el_gamal_key_from_secret ( op_private_key->get_secret() );
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

signed_transaction_info wallet_api::transfer(const string& from,
                                             const string& to,
                                             const string& amount,
                                             const string& asset_symbol,
                                             const string& memo,
                                             bool broadcast /* = false */)
{
   if(is_locked())
      FC_THROW_EXCEPTION(wallet_is_locked_exception, "");
   return my->transfer(from, to, amount, asset_symbol, memo, broadcast);
}
