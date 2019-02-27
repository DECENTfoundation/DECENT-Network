/* (c) 2019 DECENT Services. For details refers to LICENSE.txt */
#include <graphene/chain/protocol/non_fungible_token.hpp>
#include <graphene/chain/protocol/asset_ops.hpp>

namespace graphene { namespace chain {

void non_fungible_token_data_type::validate() const
{
   FC_ASSERT( name.size() <= 32 );

   if( modifiable )
   {
      FC_ASSERT( !name.empty(), "Modifiable data type must have name" );
   }
}

share_type non_fungible_token_data_type::calculate_fee(uint64_t basic_fee) const
{
   return name.size() * basic_fee;
}  

void non_fungible_token_options::validate() const
{
   FC_ASSERT( description.size() <= 1000 );
}

share_type non_fungible_token_options::calculate_fee(uint64_t basic_fee) const
{
   return description.size() * basic_fee;
}

void non_fungible_token_create_operation::validate() const
{
   FC_ASSERT( fee.amount >= 0 );
   FC_ASSERT( is_valid_symbol(symbol) );

   options.validate();
   std::for_each(definitions.begin(), definitions.end(), [this](const non_fungible_token_data_type &dt) {
      dt.validate();

      if( !dt.name.empty() )
      {
         FC_ASSERT( std::count_if(definitions.begin(), definitions.end(),
               [&](const non_fungible_token_data_type &other) { return dt.name == other.name; }) == 1,
            "Duplicated data name: ${n}", ("n", dt.name) );
      }
   });
}

share_type non_fungible_token_create_operation::calculate_fee(const fee_parameters_type& param) const
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

   fee += options.calculate_fee(param.basic_fee);
   fee += definitions.size() * 5 * param.basic_fee;
   std::for_each(definitions.begin(), definitions.end(), [&](const non_fungible_token_data_type &dt) { fee += dt.calculate_fee(1000 * param.basic_fee); } );

   return fee;
}

void non_fungible_token_update_operation::validate() const
{
   FC_ASSERT( fee.amount >= 0 );

   options.validate();
}

share_type non_fungible_token_update_operation::calculate_fee(const fee_parameters_type& param) const
{
   return 5 * param.basic_fee + options.calculate_fee(param.basic_fee);
}

void non_fungible_token_issue_operation::validate() const
{
   FC_ASSERT( fee.amount >= 0 );
}

share_type non_fungible_token_issue_operation::calculate_fee(const fee_parameters_type& param) const
{
   return param.basic_fee + data.size() * 5 * param.basic_fee + calculate_data_fee( fc::raw::pack_size(memo), param.basic_fee );
}

void non_fungible_token_transfer_operation::validate()const
{
   FC_ASSERT( fee.amount >= 0 );
   FC_ASSERT( from != to );
}

bool non_fungible_token_transfer_operation::is_partner_account_id(account_id_type acc_id) const
{
    return from == acc_id || to == acc_id;
}

void non_fungible_token_data_operation::validate()const
{
   FC_ASSERT( fee.amount >= 0 );
   FC_ASSERT( !data.empty() );
}

share_type non_fungible_token_data_operation::calculate_fee(const fee_parameters_type& param) const
{
   return param.basic_fee + data.size() * 5 * param.basic_fee;
}

} } // namespace graphene::chain
