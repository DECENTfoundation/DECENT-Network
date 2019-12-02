/* (c) 2019 DECENT Services. For details refers to LICENSE.txt */
#include <graphene/chain/protocol/non_fungible_token.hpp>
#include <graphene/chain/protocol/asset_ops.hpp>

namespace graphene { namespace chain {

void non_fungible_token_data_type::validate() const
{
   FC_ASSERT( modifiable >= nobody && modifiable <= both );
   FC_ASSERT( type >= string && type <= boolean );

   bool name_valid = name.valid();
   if( name_valid )
   {
      FC_ASSERT( name->size() <= 32 );
   }

   if( modifiable != nobody )
   {
      FC_ASSERT( name_valid && !name->empty(), "Modifiable data type must have name" );
   }
}

void non_fungible_token_create_definition_operation::validate() const
{
   FC_ASSERT( fee.amount >= 0 );
   FC_ASSERT( is_valid_symbol(symbol) );

   std::for_each(definitions.begin(), definitions.end(), [this](const non_fungible_token_data_type &dt) {
      dt.validate();

      if( dt.name.valid() && !dt.name->empty() )
      {
         FC_ASSERT( std::count_if(definitions.begin(), definitions.end(),
               [&](const non_fungible_token_data_type &other) { return dt.name == other.name; }) == 1,
            "Duplicated data name: ${n}", ("n", dt.name) );
      }
   });
}

share_type non_fungible_token_create_definition_operation::calculate_fee(const fee_parameters_type& param,
                                                                         const fc::time_point_sec now) const
{
   share_type fee = 5 * param.basic_fee;

   switch( symbol.size() )
   {
      case 3: fee = param.basic_fee * 5000;
         break;
      case 4: fee = param.basic_fee * 200;
         break;
      default:
         break;
   }

   std::for_each(definitions.begin(), definitions.end(), [&](const non_fungible_token_data_type &dt) {
      fee += calculate_data_fee(fc::raw::pack_size(dt), param.price_per_kbyte);
   });

   return fee;
}

void non_fungible_token_update_definition_operation::validate() const
{
   FC_ASSERT( fee.amount >= 0 );
}

void non_fungible_token_issue_operation::validate() const
{
   FC_ASSERT( fee.amount >= 0 );
}

share_type non_fungible_token_issue_operation::calculate_fee(const fee_parameters_type& param,
                                                             const fc::time_point_sec now) const
{
   share_type fee = param.fee;
   std::for_each(data.begin(), data.end(), [&](const fc::variant &v) { fee += calculate_data_fee(fc::raw::pack_size(v), param.price_per_kbyte); } );
   fee += calculate_data_fee(fc::raw::pack_size(memo), param.price_per_kbyte);
   return fee;
}

void non_fungible_token_transfer_operation::validate()const
{
   FC_ASSERT( fee.amount >= 0 );
   FC_ASSERT( from != to );
}

share_type non_fungible_token_transfer_operation::calculate_fee(const fee_parameters_type& k, const fc::time_point_sec now)const
{
   return k.fee + calculate_data_fee( fc::raw::pack_size(memo), k.price_per_kbyte );
}

bool non_fungible_token_transfer_operation::is_partner_account_id(account_id_type acc_id) const
{
    return from == acc_id || to == acc_id;
}

void non_fungible_token_update_data_operation::validate()const
{
   FC_ASSERT( fee.amount >= 0 );
   FC_ASSERT( !data.empty() );

   std::set<std::string> names;
   std::for_each(data.begin(), data.end(), [&](const std::pair<std::string, fc::variant> &v) {
      FC_ASSERT( !v.first.empty(), "Data name can not be empty" );
      FC_ASSERT( names.insert(v.first).second, "Duplicate data name not allowed" );
   });
}

share_type non_fungible_token_update_data_operation::calculate_fee(const fee_parameters_type& param,
                                                                   const fc::time_point_sec now) const
{
   share_type fee = param.fee;
   std::for_each(data.begin(), data.end(), [&](const std::pair<std::string, fc::variant> &v) {
      fee += calculate_data_fee(fc::raw::pack_size(v), param.price_per_kbyte);
   });

   return fee;
}

} } // namespace graphene::chain
