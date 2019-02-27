/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
#include <graphene/chain/transaction_detail_object.hpp>

namespace graphene { namespace chain {

share_type transaction_detail_object::get_transaction_amount() const
{
   return m_transaction_amount.amount;
}

share_type transaction_detail_object::get_transaction_fee() const
{
   return m_transaction_fee.amount;
}

non_fungible_token_data_id_type transaction_detail_object::get_non_fungible_token_id() const
{
   return m_nft_data_id.valid() ? *m_nft_data_id : non_fungible_token_data_id_type(GRAPHENE_DB_MAX_INSTANCE_ID);
}

} } // graphene::chain
