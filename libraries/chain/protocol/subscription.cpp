/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
#include <graphene/chain/protocol/subscription.hpp>

namespace graphene { namespace chain {

void subscribe_operation::validate() const
{
   FC_ASSERT( fee.amount >= 0 );
   FC_ASSERT( price.amount >= 0 );
}

void subscribe_by_author_operation::validate() const
{
   FC_ASSERT( fee.amount >= 0 );
}

} } // graphene::chain
