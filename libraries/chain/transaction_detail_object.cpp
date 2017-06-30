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

} } // graphene::chain
