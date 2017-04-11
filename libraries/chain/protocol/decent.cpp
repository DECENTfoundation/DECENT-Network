#include <graphene/chain/protocol/decent.hpp>
#include <fc/network/url.hpp>

namespace graphene { namespace chain {

void content_submit_operation::validate()const
{
   FC_ASSERT( fee.amount >= 0 );
#ifdef PRICE_REGIONS
   FC_ASSERT(false == price.empty());
   for (auto const& item : price)
   {
      FC_ASSERT(item.second.amount >= 0);
   }
#else
   FC_ASSERT( price.amount >= 0 );
#endif
   FC_ASSERT( size > 0 && size <= 100 ); //TODO_DECENT - increase in testnet
   FC_ASSERT( seeders.size() > 0 );
   FC_ASSERT( seeders.size() == key_parts.size() );
   FC_ASSERT( quorum >= 1 && quorum < UINT32_MAX);
   FC_ASSERT( seeders.size() >= quorum );
   FC_ASSERT( expiration <= fc::time_point_sec::maximum() );
   FC_ASSERT( publishing_fee.amount >= 0);
   fc::url _url( URI );
   FC_ASSERT( _url.proto() == "ipfs" || _url.proto() == "magnet" );
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
   //FC_ASSERT( ipfs_IDs.size() != 0 );
}

void proof_of_custody_operation::validate()const
{
   FC_ASSERT( fee.amount >= 0 );
}

void deliver_keys_operation::validate()const
{
   FC_ASSERT( fee.amount >= 0 );
}

void report_stats_operation::validate()const
{
   FC_ASSERT( fee.amount >= 0 );
   FC_ASSERT( stats.size() != 0);
}

}}
