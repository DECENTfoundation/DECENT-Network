/* (c) 2019 DECENT Services. For details refers to LICENSE.txt */
#include <graphene/chain/non_fungible_token_evaluator.hpp>
#include <graphene/chain/non_fungible_token_object.hpp>
#include <graphene/chain/transaction_detail_object.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/hardfork.hpp>

namespace graphene { namespace chain {

void_result non_fungible_token_create_definition_evaluator::do_evaluate( const operation_type& op )
{ try {
   const database& d = db();
   FC_ASSERT( d.head_block_time() > HARDFORK_4_TIME );

   auto& nft_indx = d.get_index_type<non_fungible_token_index>().indices().get<by_symbol>();
   auto nft_symbol_itr = nft_indx.find( op.symbol );
   FC_ASSERT( nft_symbol_itr == nft_indx.end(), "Symbol name must be unique" );

   auto dotpos = op.symbol.rfind( '.' );
   if( dotpos != std::string::npos )
   {
      auto prefix = op.symbol.substr( 0, dotpos );
      auto nft_symbol_itr = nft_indx.find( prefix );
      FC_ASSERT( nft_symbol_itr != nft_indx.end(), "Non fungible token ${s} may only be created by issuer of ${p}, but ${p} has not been registered",
            ("s",op.symbol)("p",prefix) );
      FC_ASSERT( nft_symbol_itr->options.issuer == op.options.issuer, "Non fungible token ${s} may only be created by issuer of ${p}, ${i}",
            ("s",op.symbol)("p",prefix)("i", op.options.issuer(d).name) );
   }

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

graphene::db::object_id_type non_fungible_token_create_definition_evaluator::do_apply( const operation_type& op )
{ try {
   const non_fungible_token_object& new_nft =
      db().create<non_fungible_token_object>( [&]( non_fungible_token_object& nft ) {
         nft.symbol = op.symbol;
         nft.options = op.options;
         nft.definitions = op.definitions;
         nft.transferable = op.transferable;
         nft.current_supply = 0;
      });

   return new_nft.id;
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result non_fungible_token_update_definition_evaluator::do_evaluate( const operation_type& op )
{ try {
   const database& d = db();
   FC_ASSERT( d.head_block_time() > HARDFORK_4_TIME );

   const non_fungible_token_object& nft_obj = op.nft_id(d);
   FC_ASSERT( op.current_issuer == nft_obj.options.issuer );
   FC_ASSERT( op.options.max_supply >= nft_obj.current_supply, "Max supply must be at least ${c}", ("c", nft_obj.current_supply) );
   FC_ASSERT( (op.options.fixed_max_supply == nft_obj.options.fixed_max_supply) || !nft_obj.options.fixed_max_supply, "Max supply must remain fixed" );
   FC_ASSERT( (op.options.max_supply == nft_obj.options.max_supply) || !nft_obj.options.fixed_max_supply, "Can not change max supply (it's fixed)" );

   FC_ASSERT( (op.options.issuer != nft_obj.options.issuer) ||
              (op.options.max_supply != nft_obj.options.max_supply) ||
              (op.options.fixed_max_supply != nft_obj.options.fixed_max_supply) ||
              (op.options.description != nft_obj.options.description),
              "Operation has to change something" );

   nft_to_update = &nft_obj;
   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

graphene::db::object_id_type non_fungible_token_update_definition_evaluator::do_apply( const operation_type& op )
{ try {
   db().modify( *nft_to_update, [&]( non_fungible_token_object& nft_obj ){
      nft_obj.options = op.options;
   });

   return nft_to_update->id;
} FC_CAPTURE_AND_RETHROW( (op) ) }

template<typename T, typename U>
bool nft_find_data(const database& d, non_fungible_token_id_type nft_id, const T& v, size_t idx, const U& comparator)
{
   const auto &objs = d.get_index_type<non_fungible_token_data_index>().indices().get<by_nft>().equal_range(nft_id);
   return std::any_of(objs.first, objs.second, [&](const non_fungible_token_data_object &nft_data) { return comparator(v, nft_data.data[idx].as<T>()); } );
}

static void nft_check_data(const database& d, const non_fungible_token_data_type &data_type, non_fungible_token_id_type nft_id, const fc::variant &data, size_t idx)
{
   switch( data_type.type )
   {
      case non_fungible_token_data_type::string:
         FC_ASSERT( data.is_string(), "${d} must be string", ("d", data) );
         if( data_type.unique )
         {
            FC_ASSERT( !nft_find_data( d, nft_id, data.as_string(), idx, std::equal_to<std::string>()), "${d} must be unique", ("d", data));
         }
         break;

      case non_fungible_token_data_type::integer:
         FC_ASSERT( data.is_int64() || data.is_uint64(), "${d} must be integer value", ("d", data) );
         if( data_type.unique )
         {
            if( data.is_int64() )
            {
               FC_ASSERT( !nft_find_data( d, nft_id, data.as_int64(), idx, std::equal_to<int64_t>()), "${d} must be unique", ("d", data));
            }
            else
            {
               FC_ASSERT( !nft_find_data( d, nft_id, data.as_uint64(), idx, std::equal_to<uint64_t>()), "${d} must be unique", ("d", data));
            }
         }
         break;

      case non_fungible_token_data_type::boolean:
         FC_ASSERT( data.is_bool(), "${d} must be boolean value", ("d", data) );
         if( data_type.unique )
         {
            FC_ASSERT( !nft_find_data( d, nft_id, data.as_bool(), idx, std::equal_to<bool>()), "${d} must be unique", ("d", data));
         }
         break;
   }
}

void_result non_fungible_token_issue_evaluator::do_evaluate( const operation_type& op )
{ try {
   const database& d = db();
   FC_ASSERT( d.head_block_time() > HARDFORK_4_TIME );

   const non_fungible_token_object& nft_obj = op.nft_id(d);
   FC_ASSERT( op.issuer == nft_obj.options.issuer );
   FC_ASSERT( d.find_object(op.to), "Attempt to issue a non fungible token to a non-existing account" );

   FC_ASSERT( (nft_obj.current_supply + 1) <= nft_obj.options.max_supply, "Max supply reached" );

   FC_ASSERT( nft_obj.definitions.size() == op.data.size() );
   for( size_t i = 0; i < nft_obj.definitions.size(); i++ )
   {
      const non_fungible_token_data_type &data_type = nft_obj.definitions[i];
      nft_check_data(d, data_type, nft_obj.get_id(), op.data[i], i);
   }

   nft_to_update = &nft_obj;
   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

graphene::db::object_id_type non_fungible_token_issue_evaluator::do_apply( const operation_type& op )
{ try {
   database& d = db();
   const non_fungible_token_data_object& new_nft_data =
      d.create<non_fungible_token_data_object>( [&]( non_fungible_token_data_object& nft_data ) {
         nft_data.nft_id = op.nft_id;
         nft_data.owner = op.to;
         nft_data.data = op.data;
      });

   d.modify( *nft_to_update, [&]( non_fungible_token_object& nft ){
      ++nft.current_supply;
   });

   d.create<transaction_detail_object>( [&](transaction_detail_object& obj) {
      obj.m_from_account = nft_to_update->options.issuer;
      obj.m_to_account = op.to;
      obj.m_operation_type = (uint8_t)transaction_detail_object::non_fungible_token;
      obj.m_transaction_fee = op.fee;
      obj.m_nft_data_id = new_nft_data.get_id();
      obj.m_transaction_encrypted_memo = op.memo;
      obj.m_str_description = "issue token";
      obj.m_timestamp = d.head_block_time();
   });

   return new_nft_data.id;
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result non_fungible_token_transfer_evaluator::do_evaluate( const operation_type& op )
{ try {
   const database& d = db();
   FC_ASSERT( d.head_block_time() > HARDFORK_4_TIME );

   const non_fungible_token_data_object& nft_data_obj = op.nft_data_id(d);
   FC_ASSERT( op.from == nft_data_obj.owner );
   FC_ASSERT( nft_data_obj.nft_id(d).transferable || op.to == GRAPHENE_NULL_ACCOUNT, "Attempt to transfer a non transferable token" );
   FC_ASSERT( d.find_object(op.to), "Attempt to transfer a non fungible token data to a non-existing account" );

   nft_data_to_update = &nft_data_obj;
   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result non_fungible_token_transfer_evaluator::do_apply( const operation_type& op )
{ try {
   database& d = db();
   account_id_type owner = nft_data_to_update->owner;
   d.modify( *nft_data_to_update, [&]( non_fungible_token_data_object& nft_data ) {
      nft_data.owner = op.to;
   });

   if( op.to == GRAPHENE_NULL_ACCOUNT )
      d.modify( nft_data_to_update->nft_id(d), [&]( non_fungible_token_object& nft ){
         --nft.current_supply;
      });

   d.create<transaction_detail_object>( [&](transaction_detail_object& obj) {
      obj.m_from_account = owner;
      obj.m_to_account = op.to;
      obj.m_operation_type = (uint8_t)transaction_detail_object::non_fungible_token;
      obj.m_transaction_fee = op.fee;
      obj.m_nft_data_id = nft_data_to_update->get_id();
      obj.m_transaction_encrypted_memo = op.memo;
      obj.m_str_description = "transfer token";
      obj.m_timestamp = d.head_block_time();
   });

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result non_fungible_token_update_data_evaluator::do_evaluate( const operation_type& op )
{ try {
   const database& d = db();
   FC_ASSERT( d.head_block_time() > HARDFORK_4_TIME );

   const non_fungible_token_data_object& nft_data_obj = op.nft_data_id(d);
   const non_fungible_token_object &nft_obj = nft_data_obj.nft_id(d);

   for (const auto &v : op.data)
   {
      auto it = std::find_if(nft_obj.definitions.begin(), nft_obj.definitions.end(),
         [&](const non_fungible_token_data_type &dt) { return dt.name == v.first; });

      FC_ASSERT( it != nft_obj.definitions.end(), "Attempt to modify non existing data: ${n}", ("n", v.first) );
      switch( it->modifiable )
      {
         case non_fungible_token_data_type::issuer:
            FC_ASSERT( op.modifier == nft_obj.options.issuer, "Only issuer can modify data: ${n}", ("n", v.first) );
            break;
         case non_fungible_token_data_type::owner:
            FC_ASSERT( op.modifier == nft_data_obj.owner, "Only owner can modify data: ${n}", ("n", v.first) );
            break;
         case non_fungible_token_data_type::both:
            FC_ASSERT( op.modifier == nft_data_obj.owner || op.modifier == nft_obj.options.issuer, "Only issuer or owner can modify data: ${n}", ("n", v.first) );
            break;
         default:
            FC_ASSERT( false, "Attempt to modify a non modifiable data: ${n}", ("n", v.first) );
      }

      auto idx = std::distance(nft_obj.definitions.begin(), it);
      nft_check_data(d, *it, nft_obj.get_id(), v.second, idx);
   }

   nft_data_to_update = &nft_data_obj;
   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result non_fungible_token_update_data_evaluator::do_apply( const operation_type& op )
{ try {
   database& d = db();
   d.modify( *nft_data_to_update, [&]( non_fungible_token_data_object& nft_data ) {
      const non_fungible_token_object &nft = nft_data.nft_id(d);

      for (const auto &v : op.data)
      {
         auto it = std::find_if(nft.definitions.begin(), nft.definitions.end(),
            [&](const non_fungible_token_data_type &dt) { return dt.name == v.first; });

         auto idx = std::distance(nft.definitions.begin(), it);
         nft_data.data[idx] = v.second;
      }
   });

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

} } // graphene::chain
