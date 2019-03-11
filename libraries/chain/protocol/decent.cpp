/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
#include <graphene/chain/protocol/decent.hpp>
#include <fc/network/url.hpp>

namespace graphene { namespace chain {

void set_publishing_manager_operation::validate()const
{
   FC_ASSERT( fee.amount >= 0 );
   FC_ASSERT( to.size() > 0 );
   FC_ASSERT( from == account_id_type(15), "Account does not have permission to this operation" );
}

void set_publishing_right_operation::validate()const
{
   FC_ASSERT( fee.amount >= 0 );
   FC_ASSERT( to.size() > 0 );
}

void content_submit_operation::validate()const
{
   FC_ASSERT( fee.amount >= 0 );

   uint32_t sum_of_splits = 0;
   for( auto const &element : co_authors )
   {
      FC_ASSERT( element.second >= 0 );
      sum_of_splits += element.second;
   }

   if( co_authors.find( author ) != co_authors.end() )
      FC_ASSERT( sum_of_splits == 10000 );
   else
      FC_ASSERT( sum_of_splits < 10000 );

   FC_ASSERT(false == price.empty());
   for (auto const& item : price)
   {
      FC_ASSERT(item.price.amount >= 0);
   }

   FC_ASSERT( size > 0 && size <= DECENT_MAX_FILE_SIZE, "max file size limit exceeded" );
   FC_ASSERT( seeders.size() == key_parts.size() );
   if(seeders.size() == 0){ //simplified content
      FC_ASSERT(quorum == 0);
   }else {
      FC_ASSERT(quorum >= 2 , "At least two seeders are needed to reconstruct the key");
      FC_ASSERT(seeders.size() >= quorum);
   }
   FC_ASSERT( expiration <= fc::time_point_sec::maximum() );
   FC_ASSERT( publishing_fee.amount >= 0);
   fc::url _url( URI );
   FC_ASSERT( _url.proto() == "ipfs" || _url.proto() == "magnet" || _url.proto() == "http" || _url.proto() == "https" );
}

void request_to_buy_operation::validate()const
{
   FC_ASSERT( fee.amount >= 0 );
   FC_ASSERT( price.amount >= 0 );
}

void leave_rating_and_comment_operation::validate()const
{
   FC_ASSERT( fee.amount >= 0 );
   FC_ASSERT( ( rating > 0 && rating <= 5 ) );
   if( !comment.empty() )
      FC_ASSERT( comment.length() <= DECENT_MAX_COMMENT_SIZE  );
}

void ready_to_publish_operation::validate()const
{
   FC_ASSERT( fee.amount >= 0 );
   FC_ASSERT( space > 0 );
   FC_ASSERT( !ipfs_ID.empty() );
}

void ready_to_publish2_operation::validate()const
{
   FC_ASSERT( fee.amount >= 0 );
   FC_ASSERT( space > 0 );
   FC_ASSERT( !ipfs_ID.empty() );
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
   for( const auto& element : stats )
      FC_ASSERT( element.second >= 0 );
}

}}
