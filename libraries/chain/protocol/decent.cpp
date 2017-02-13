#include <graphene/chain/protocol/decent.hpp>

namespace graphene { namespace chain {

void content_submit_operation::validate()const
{
   FC_ASSERT( fee.amount >= 0 );
   FC_ASSERT( price.amount >= 0 );
   FC_ASSERT( size > 0 && size <= UINT64_MAX);
   FC_ASSERT( seeders.size() > 0 );
   FC_ASSERT( seeders.size() == key_parts.size() );
   FC_ASSERT( quorum > 2 && quorum < UINT32_MAX);
   FC_ASSERT( expiration > fc::time_point_sec( fc::time_point::now() ) && expiration <= fc::time_point_sec::maximum() );
   FC_ASSERT( publishing_fee.amount >= 0);
}

void request_to_buy_operation::validate()const
{
   FC_ASSERT( fee.amount >= 0 );
   FC_ASSERT( price.amount >= 0 );
}

void leave_rating_operation::validate()const
{
   FC_ASSERT( fee.amount >= 0 );
   FC_ASSERT( rating >= 0 && rating <= UINT64_MAX );
}

void ready_to_publish_operation::validate()const
{
   FC_ASSERT( fee.amount >= 0 );
   FC_ASSERT( space > 0 && space <= UINT64_MAX );
   FC_ASSERT( price_per_MByte >= 0 && price_per_MByte <= UINT32_MAX );
}

void proof_of_custody_operation::validate()const
{
   FC_ASSERT( fee.amount >= 0 );
}

void deliver_keys_operation::validate()const
{
   FC_ASSERT( fee.amount >= 0 );
}

}}
