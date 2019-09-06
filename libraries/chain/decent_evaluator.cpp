/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
#include <graphene/chain/decent_evaluator.hpp>
#include <graphene/chain/protocol/decent.hpp>
#include <graphene/chain/hardfork.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/seeder_object.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/hardfork.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/buying_object.hpp>
#include <graphene/chain/seeder_object.hpp>
#include <graphene/chain/content_object.hpp>
#include <graphene/chain/subscription_object.hpp>
#include <graphene/chain/seeding_statistics_object.hpp>
#include <graphene/chain/transaction_detail_object.hpp>

#include <decent/encrypt/custodyutils.hpp>
#include <decent/encrypt/encryptionutils.hpp>

#include <boost/multiprecision/cpp_int.hpp>

namespace graphene { namespace chain {

namespace {

static decent::encrypt::CustodyUtils _custody_utils;

void content_payout(database& db, asset paid_price_after_exchange, const content_object& content){
   if( content.co_authors.empty() )
      db.adjust_balance( content.author, paid_price_after_exchange );
   else
   {
      asset price = paid_price_after_exchange;
      boost::multiprecision::int128_t price_for_co_author;
      for( auto const &element : content.co_authors )
      {
         price_for_co_author = ( paid_price_after_exchange.amount.value * element.second ) / 10000ll ;
         db.adjust_balance( element.first, asset( static_cast<share_type>(price_for_co_author), price.asset_id) );
         price.amount -= price_for_co_author;
      }

      if( price.amount != 0 ) {
         FC_ASSERT( price.amount > 0 );
         db.adjust_balance(content.author, price);
      }
   }
};

}

operation_result set_publishing_manager_evaluator::do_evaluate( const operation_type& o )
{try{
   for( const auto id : o.to )
      FC_ASSERT (db().find_object(id), "Account does not exist");
   FC_ASSERT( o.from == account_id_type(15) , "This operation is permitted only to DECENT account");

   return void_result();
}FC_CAPTURE_AND_RETHROW( (o) ) }

operation_result set_publishing_manager_evaluator::do_apply( const operation_type& o )
{try{
   for( auto to_id : o.to )
   {
      const account_object& to_acc = to_id(db());

      if( o.can_create_publishers == true ) {
         db().modify<account_object>(to_acc, [](account_object &ao) {
              ao.rights_to_publish.is_publishing_manager = true;

         });
         return void_result();
      }
      else
      {
         for( const account_id_type& publisher : to_acc.rights_to_publish.publishing_rights_forwarded )
         {
            auto& publisher_acc = db().get<account_object>(publisher);
            db().modify<account_object>( publisher_acc, [&](account_object& ao){
                 ao.rights_to_publish.publishing_rights_received.erase( publisher );
            });
         }
         db().modify<account_object>(to_acc, [](account_object& ao){
              ao.rights_to_publish.is_publishing_manager = false;
              ao.rights_to_publish.publishing_rights_forwarded.clear();
         });
      }
   }

   return void_result();
}FC_CAPTURE_AND_RETHROW( (o) ) }

operation_result set_publishing_right_evaluator::do_evaluate( const operation_type& o )
{try{
    const auto& from_acc = db().get<account_object>(o.from);
    for( const auto id : o.to )
       FC_ASSERT (db().find_object(id), "Account does not exist");
    FC_ASSERT( from_acc.rights_to_publish.is_publishing_manager, "Account does not have permission to give publishing rights" );

    return void_result();
}FC_CAPTURE_AND_RETHROW( (o) ) }

   operation_result set_publishing_right_evaluator::do_apply( const operation_type& o )
   {try{
      const auto& from_acc = db().get<account_object>(o.from);

      for( const account_id_type& element : o.to )
      {
         const auto& to_acc = db().get<account_object>( element );

         if(o.is_publisher)
         {
            db().modify<account_object>(from_acc, [&](account_object& ao){
                 ao.rights_to_publish.publishing_rights_forwarded.insert( element );
            });
            db().modify<account_object>(to_acc,[&](account_object& ao){
                 ao.rights_to_publish.publishing_rights_received.insert( o.from );
            });
         } else {
            db().modify<account_object>(from_acc, [&](account_object& ao){
                 ao.rights_to_publish.publishing_rights_forwarded.erase( element );
            });
            db().modify<account_object>(to_acc,[&](account_object& ao){
                 ao.rights_to_publish.publishing_rights_received.erase( o.from );
            });
         }
      }

      return void_result();
      }FC_CAPTURE_AND_RETHROW( (o) )
   }

   operation_result content_submit_evaluator::do_evaluate(const operation_type& o )
   {try{
      FC_ASSERT( (db().head_block_time() > HARDFORK_4_TIME) || (o.co_authors.size() <= 10) );

      if( o.key_parts.size() == 0 ) //simplified content
         FC_ASSERT(db().head_block_time() > HARDFORK_2_TIME);

      db().get<account_object>(o.author);
      //Submission rights feature is disabled
      //FC_ASSERT( !author_account.rights_to_publish.publishing_rights_received.empty(), "Author does not have permission to publish a content" );

      if( !o.co_authors.empty() )
      {
         // sum of basis points
         uint32_t sum_of_splits = 0;

         for( auto const &element : o.co_authors )
         {
            // test whether co-autors exist
            const auto& idx = db().get_index_type<account_index>().indices().get<graphene::db::by_id>();
            auto itr = idx.find(element.first);
            FC_ASSERT (itr != idx.end() , "Account ${account} doesn't exist.", ( "account", element.first ) );
            sum_of_splits += element.second;
         }

         // author can have unassigned payout split value.
         // In such a case, missing value is automatically calculated, and equals to remaining available basis points
         // test whether the author is included as co-author
         auto it = o.co_authors.find( o.author );

         // if author is not included in co_authors map
         if( it == o.co_authors.end() )
         {
            FC_ASSERT( sum_of_splits < 10000, "Sum of splits exceeds allowed limit ( no remaining basis points for author's payout split )." );
         }
            // if author is included in co_authors map
         else
         {
            FC_ASSERT( sum_of_splits == 10000, "Sum of splits doesn't have required value ( 10000 basis points)." );
         }
      }

      const auto& idx_asset = db().get_index_type<asset_index>().indices().get<graphene::db::by_id>();
      for( const auto& element : o.price )
      {
         const auto& itr_asset = idx_asset.find( element.price.asset_id );
         FC_ASSERT( itr_asset != idx_asset.end(), "Asset ${a} does not exist",("a",element.price) );
         FC_ASSERT( RegionCodes::s_mapCodeToName.count( element.region ) , "Invalid region code" );
      }

      FC_ASSERT( db().head_block_time() <= o.expiration);
      fc::microseconds duration = (o.expiration - db().head_block_time() );
      uint32_t days = static_cast<uint32_t>(duration.to_seconds() / 3600 / 24);
      FC_ASSERT( days != 0, "time to expiration has to be at least one day" );

      auto& idx = db().get_index_type<seeder_index>().indices().get<by_seeder>();

      const auto& content_idx = db().get_index_type<content_index>().indices().get<by_URI>();
      auto content_itr = content_idx.find( o.URI );
      if( content_itr != content_idx.end() ) // is resubmit?
      {
         is_resubmit = true;
         FC_ASSERT( content_itr->author == o.author );
         FC_ASSERT( content_itr->size == o.size );
         FC_ASSERT( content_itr->_hash == o.hash );
         FC_ASSERT( content_itr->quorum == o.quorum );
         FC_ASSERT( content_itr->key_parts.size() == o.seeders.size() );
         for( const auto& element : content_itr->key_parts )
         {
            FC_ASSERT( std::find(o.seeders.begin(), o.seeders.end(), element.first ) != o.seeders.end() );
         }
         if( content_itr->cd.valid() )
         {
            FC_ASSERT( *(content_itr->cd) == *(o.cd) );
            FC_ASSERT( content_itr->expiration == o.expiration ); // expiration can not be changed for non CDN submit
         }
         else
         {
            FC_ASSERT( (db().head_block_time() > HARDFORK_4_TIME) || (content_itr->expiration == o.expiration) );
         }

         /* Resubmit that changes other stuff is not supported.
         for ( auto &p : o.seeders ) //check if seeders exist and accumulate their prices
         {
            auto itr = idx.find( p );
            FC_ASSERT( itr != idx.end(), "seeder does not exist" );

            auto itr2 = content_itr->key_parts.begin();
            while( itr2 != content_itr->key_parts.end() )
            {
               if( itr2->first == p )
               {
                  break;
               }
               itr2++;
            }
            if( itr2 == content_itr->key_parts.end() )
               FC_ASSERT( itr->free_space > o.size ); // only newly added seeders are tested against free space

            total_price_per_day += itr-> price.amount * o.size;
         }
         FC_ASSERT( days * total_price_per
         for ( auto &p : o.seeders ) //check if seeders exist and accumulate their prices
         {
            auto itr = idx.find( p );
            FC_ASSERT( itr != idx.end(), "seeder does not exist" );

            auto itr2 = content_itr->key_parts.begin();
            while( itr2 != content_itr->key_parts.end() )
            {
               if( itr2->first == p )
               {
                  break;
               }
               itr2++;
            }
            if( itr2 == content_itr->key_parts.end() )
               FC_ASSERT( itr->free_space > o.size ); // only newly added seeders are tested against free space

            total_price_per_day += itr-> price.amount * o.size;
         }
         FC_ASSERT( days * total_price_per_day <= o.publishing_fee + content_itr->publishing_fee_escrow );*/
      } else
      {
         asset total_price_per_day;
         for ( const auto &p : o.seeders ) //check if seeders exist and accumulate their prices
         {
            const auto& itr = idx.find( p );
            FC_ASSERT( itr != idx.end(), "seeder does not exist" );
            FC_ASSERT( itr->free_space > o.size );
            total_price_per_day += itr-> price.amount * o.size;
         }
         FC_ASSERT( days * total_price_per_day <= o.publishing_fee );
      }

      return void_result();
   }FC_CAPTURE_AND_RETHROW( (o) ) }

   operation_result content_submit_evaluator::do_apply(const operation_type& o)
   {try{
      graphene::chain::ContentObjectPropertyManager synopsis_parser(o.synopsis);
      std::string title = synopsis_parser.get<graphene::chain::ContentObjectTitle>();

      operation_result result;
      if( is_resubmit )
      {
         auto& content_idx = db().get_index_type<content_index>().indices().get<by_URI>();
         const auto& content_itr = content_idx.find( o.URI );

         graphene::chain::ContentObjectPropertyManager old_synopsis_parser(content_itr->synopsis);
         std::string old_title = old_synopsis_parser.get<graphene::chain::ContentObjectTitle>();
         auto& transaction_detail_idx = db().get_index_type<transaction_detail_index>().indices().get<by_description>();
         const auto& transaction_detail_itr = transaction_detail_idx.find( old_title );

         db().modify<transaction_detail_object>(*transaction_detail_itr,[&](transaction_detail_object& tdo) {
            tdo.m_str_description = title;
            tdo.m_timestamp = db().head_block_time();
         });

         db().modify<content_object>(*content_itr,[&](content_object& co) {
                                        map<uint32_t, asset> prices;
                                        for (auto const& item : o.price)
                                        {
                                           prices[item.region] = item.price;
                                        }

                                        auto it_no_regions = prices.find(RegionCodes::OO_none);
                                        if (it_no_regions != prices.end())
                                           co.price.SetSimplePrice(it_no_regions->second);
                                        else
                                        {
                                           // clearing the region code list first
                                           co.price.ClearPrices();
                                           for (auto const& price_item : prices)
                                           {
                                              co.price.SetRegionPrice(price_item.first, price_item.second);
                                           }
                                        }

                                        co.synopsis = o.synopsis;
                                        co.co_authors = o.co_authors;
                                        /*
                                        co.publishing_fee_escrow += o.publishing_fee;
                                        auto itr1 = o.seeders.begin();
                                        auto itr2 = o.key_parts.begin();
                                        co.key_parts.clear();
                                        co.last_proof.clear();
                                        while ( itr1 != o.seeders.end() && itr2 != o.key_parts.end() )
                                        {
                                           co.key_parts.emplace(std::make_pair( *itr1, *itr2 ));
                                           itr1++;
                                           itr2++;
                                        }
                                        co.quorum = o.quorum;*/
                                        if( !co.cd.valid() )
                                        {
                                          co.expiration = o.expiration;
                                        }
                                     });
      }
      else
      {
         const auto& content = db().create<content_object>([&](content_object& co)
                                     {  //create new content object and store all values from the operation
                                        co.author = o.author;
                                        co.co_authors = o.co_authors;
                                        map<uint32_t, asset> prices;
                                        for (auto const& item : o.price)
                                        {
                                           prices[item.region] = item.price;
                                        }

                                        auto it_no_regions = prices.find(RegionCodes::OO_none);
                                        if (it_no_regions != prices.end())
                                           co.price.SetSimplePrice(it_no_regions->second);
                                        else
                                        {
                                           for (auto const& price_item : prices)
                                           {
                                              co.price.SetRegionPrice(price_item.first, price_item.second);
                                           }
                                        }

                                        co.size = o.size;
                                        co.synopsis = o.synopsis;
                                        co.URI = o.URI;
                                        co.publishing_fee_escrow = o.publishing_fee;
                                        auto itr1 = o.seeders.begin();
                                        auto itr2 = o.key_parts.begin();
                                        while ( itr1 != o.seeders.end() && itr2 != o.key_parts.end() )
                                        {
                                           co.key_parts.emplace(std::make_pair( *itr1, *itr2 ));
                                           itr1++;
                                           itr2++;
                                        }
                                        co._hash = o.hash;
                                        co.cd = o.cd;
                                        co.quorum = o.quorum;
                                        co.expiration = o.expiration;
                                        co.created = db().head_block_time();
                                        co.times_bought = 0;
                                        co.AVG_rating = 0;
                                        co.num_of_ratings = 0;
                                     });

         result = content.id;
         db().adjust_balance(o.author,-o.publishing_fee);  //pay the escrow from author's account
         auto& idx = db().get_index_type<seeder_index>().indices().get<by_seeder>();
         // Reserve the space on seeder's boxes
         // TODO_DECENT - we should better reserve the disk space after the first PoC
         for ( const auto &p : o.seeders )
         {
            const auto& itr = idx.find( p );
            db().modify<seeder_object>( *itr, [&](seeder_object& so)
            {
               so.free_space -= o.size;
            });
            db().modify<content_object>(content, [&](content_object& co){
               co.seeder_price.emplace(std::make_pair(p, itr->price.amount));
            });
         }

         const auto& idx2 = db().get_index_type<seeding_statistics_index>().indices().get<by_seeder>();
         for( const auto& element : o.seeders )
         {
            const auto& stats = idx2.find( element );
            db().modify<seeding_statistics_object>( *stats, [](seeding_statistics_object& so){
               so.total_content_requested_to_seed += 1;
            });
         }

         auto& d = db();
         db().create<transaction_detail_object>([&o, &title, &d](transaction_detail_object& obj)
                                                {
                                                   obj.m_operation_type = (uint8_t)transaction_detail_object::content_submit;

                                                   obj.m_from_account = o.author;
                                                   obj.m_to_account = account_id_type();
                                                   obj.m_transaction_amount = o.publishing_fee;
                                                   obj.m_transaction_fee = o.fee;
                                                   obj.m_str_description = title;
                                                   obj.m_timestamp = d.head_block_time();
                                                });
      }

      return result;
   }FC_CAPTURE_AND_RETHROW( (o) ) }

   operation_result content_cancellation_evaluator::do_evaluate(const operation_type& o)
   {
      try {
         auto& idx = db().get_index_type<content_index>().indices().get<by_URI>();
         const auto& content_itr = idx.find( o.URI );
         FC_ASSERT( content_itr != idx.end() );
         FC_ASSERT( o.author == content_itr->author );
         FC_ASSERT( content_itr->expiration > db().head_block_time() );
         FC_ASSERT( !content_itr->is_blocked );

         return void_result();
      }FC_CAPTURE_AND_RETHROW((o))
   }

   operation_result content_cancellation_evaluator::do_apply(const operation_type& o)
   {
      try {
         auto& idx = db().get_index_type<content_index>().indices().get<by_URI>();
         const auto& content_itr = idx.find( o.URI );
         db().modify<content_object>(*content_itr, [&](content_object &content_obj) {
            content_obj.is_blocked = true;
            if( content_obj.expiration > db().head_block_time() + (24 * 60 * 60) )
               content_obj.expiration = db().head_block_time() + (24 * 60 * 60);
         });

         return void_result();
      }FC_CAPTURE_AND_RETHROW((o))
   }

   operation_result request_to_buy_evaluator::do_evaluate(const operation_type& o )
   {try{
      database& d = db();

      auto& idx = d.get_index_type<content_index>().indices().get<by_URI>();
      const auto& content = idx.find( o.URI );
      FC_ASSERT( content != idx.end() );
      FC_ASSERT( o.price <= d.get_balance( o.consumer, o.price.asset_id ) );
      FC_ASSERT( content->expiration > db().head_block_time() );

      optional<asset> content_price_got = content->price.GetPrice(o.region_code_from);

      FC_ASSERT( content_price_got.valid(), "content is not available for this region" );
      content_price = *content_price_got;

      FC_ASSERT( !content->is_blocked , "content has been canceled" );

      auto &range = d.get_index_type<subscription_index>().indices().get<by_from_to>();
      const auto &subscription = range.find(boost::make_tuple(o.consumer, content->author));

      /// Check whether subscription exists. If so, consumer doesn't need pay for content
      if (subscription != range.end() && subscription->expiration > d.head_block_time() ) {
         is_subscriber = true;
         return void_result();
      }
      FC_ASSERT( d.are_assets_exchangeable( o.price.asset_id(d), content_price.asset_id(d) ), "price for the content and price of the content are not exchangeable");

      /*
       * Case 1: paid in some asset, price in the same asset
       * Case 2: paid in DCT, price in MIA
       * Case 3: paid in DCT, price in UIA
       * Case 4: paid in UIA, price in DCT
       * Case 5: paid in UIA, price in MIA
       */

      paid_price = o.price;

      if( content_price.asset_id == o.price.asset_id ){ // no need to convert
         paid_price_after_conversion = paid_price;
         FC_ASSERT(paid_price_after_conversion >= content_price );
         return void_result();
      }

      asset paid_price_in_dct;
      FC_ASSERT( paid_price.asset_id(d).can_convert(paid_price, paid_price_in_dct, d) && paid_price_in_dct.asset_id == asset_id_type() );
      FC_ASSERT( content_price.asset_id(d).can_convert(paid_price_in_dct, paid_price_after_conversion, d), "cannot convert ${f} to ${t}");
      FC_ASSERT( paid_price_after_conversion.asset_id == content_price.asset_id );
      FC_ASSERT( paid_price_after_conversion >= content_price );

      return void_result();
   }FC_CAPTURE_AND_RETHROW( (o) ) }

   operation_result request_to_buy_evaluator::do_apply(const operation_type& o )
   {try{
      database& d = db();

      const auto& idx = d.get_index_type<content_index>().indices().get<by_URI>();
      const auto& content = idx.find( o.URI );

      if( is_subscriber ) //if it is subscription, price is ignored
      {
         paid_price = asset( 0 );
         paid_price_after_conversion = asset( 0 );
      }

      if( content_price.asset_id == o.price.asset_id ){ // no need to convert
         paid_price_after_conversion = paid_price;
      }else {
         asset paid_price_in_dct = paid_price.asset_id(d).convert(paid_price, d);
         paid_price_after_conversion = content_price.asset_id(d).convert(paid_price_in_dct, d);
      }

      bool delivered = false;
      asset escrow = paid_price_after_conversion;
      if( content->key_parts.size() == 0 ){ //simplified content buying
         delivered = true;
         escrow = asset(0);
      }

      const auto& object = db().create<buying_object>([&](buying_object& bo)
                                                      { //create new buying object
                                                         bo.consumer = o.consumer;
                                                         bo.URI = o.URI;
                                                         bo.expiration_time = d.head_block_time() + 24*3600;
                                                         bo.pubKey = o.pubKey;

                                                         bo.price = escrow; // escrow, will be reset to zero
                                                         bo.paid_price_before_exchange = paid_price;
                                                         bo.paid_price_after_exchange = paid_price_after_conversion;

                                                         bo.synopsis = content->synopsis;
                                                         bo.size = content->size;
                                                         bo.created = content->created;
                                                         bo.delivered = delivered;
                                                         bo.region_code_from = o.region_code_from;
                                                         bo.expiration_or_delivery_time = db().head_block_time();
                                                      });

      d.adjust_balance( o.consumer, -paid_price );
      if(delivered) {
         content_payout(d, paid_price_after_conversion, *content);
         d.modify<content_object>( *content, []( content_object& co ){ co.times_bought++; });

         finish_buying_operation op;
         op.author = content->author;
         op.co_authors = content->co_authors;
         op.payout = paid_price_after_conversion;
         op.consumer = object.consumer;
         op.buying = object.id;

         db().push_applied_operation(op);
      }
      const auto& idx2 = d.get_index_type<seeding_statistics_index>().indices().get<by_seeder>();
      for( auto& element : content->key_parts )
      {
         const auto& stats = idx2.find( element.first );
         d.modify<seeding_statistics_object>( *stats, [](seeding_statistics_object& so){
            so.missed_delivered_keys++;
         });
      }

      db().create<transaction_detail_object>([&](transaction_detail_object& obj)
                                             {
                                                obj.m_operation_type = (uint8_t)transaction_detail_object::content_buy;

                                                const auto& idx = d.get_index_type<content_index>().indices().get<by_URI>();
                                                auto itr = idx.find(o.URI);
                                                if (itr != idx.end())
                                                {
                                                   obj.m_from_account = itr->author;
                                                   graphene::chain::ContentObjectPropertyManager synopsis_parser(itr->synopsis);
                                                   obj.m_str_description = synopsis_parser.get<graphene::chain::ContentObjectTitle>();
                                                }

                                                obj.m_to_account = o.consumer;
                                                obj.m_transaction_amount = paid_price;
                                                obj.m_transaction_fee = o.fee;
                                                obj.m_timestamp = d.head_block_time();
                                             });

      return object.id;
   }FC_CAPTURE_AND_RETHROW( (o) ) }

   operation_result deliver_keys_evaluator::do_evaluate(const operation_type& o )
   {try{
      const auto& buying = db().get<buying_object>(o.buying);
      auto& idx = db().get_index_type<content_index>().indices().get<by_URI>();

      const auto& content = idx.find( buying.URI );
      FC_ASSERT( content != idx.end() );

      auto& sidx = db().get_index_type<seeder_index>().indices().get<by_seeder>();
      const auto& seeder = sidx.find(o.seeder);

      const auto& seeder_pubKey = seeder->pubKey;
      const auto& buyer_pubKey = buying.pubKey;
      const auto& firstK = content->key_parts.at( o.seeder );
      const auto& secondK = o.key;
      const auto& proof = o.proof;
      if(!(db().get_node_properties().skip_flags&db().skip_undo_history_check)) {
         FC_ASSERT(decent::encrypt::verify_delivery_proof(proof, firstK, secondK, seeder_pubKey, buyer_pubKey));
      }

      return void_result();
   }FC_CAPTURE_AND_RETHROW( (o) ) }

   operation_result deliver_keys_evaluator::do_apply(const operation_type& o )
   {try{
      //start with getting the buying and content objects...
      const auto& buying = db().get<buying_object>(o.buying);
      bool expired = ( buying.expiration_time < db().head_block_time() );
      auto& idx = db().get_index_type<content_index>().indices().get<by_URI>();
      const auto& content = idx.find( buying.URI );
      bool delivered;
      // if the response (key particle) has not been seen before, note it
      if( std::find(buying.seeders_answered.begin(), buying.seeders_answered.end(), o.seeder) == buying.seeders_answered.end() )
      {
         db().modify<buying_object>(buying, [&](buying_object& bo){
            bo.seeders_answered.push_back( o.seeder );
            bo.key_particles.push_back( decent::encrypt::Ciphertext(o.key) );
         });

         const auto& idx2 = db().get_index_type<seeding_statistics_index>().indices().get<by_seeder>();
         const auto& stats = idx2.find( o.seeder );
         db().modify<seeding_statistics_object>( *stats, [](seeding_statistics_object& so){
            so.missed_delivered_keys--;
            so.total_delivered_keys++;
         } );
      }
      delivered = buying.seeders_answered.size() >= content->quorum;
      //if the content has already been delivered or expired, just note the key particles and go on
      if( buying.delivered || buying.expired )
         return void_result();
      //The content just has been successfuly delivered, take care of the payment
      if( delivered )
      {
         db().modify<content_object>( *content, []( content_object& co ){ co.times_bought++; });

         content_payout(db(), buying.paid_price_after_exchange, *content);

         db().modify<buying_object>(buying, [&](buying_object& bo){
              bo.price.amount = 0;
              bo.delivered = true;
              bo.expiration_or_delivery_time = db().head_block_time();
         });

         finish_buying_operation op;
         op.author = content->author;
         op.co_authors = content->co_authors;
         op.payout = buying.paid_price_after_exchange;
         op.consumer = buying.consumer;
         op.buying = buying.id;

         db().push_applied_operation(op);
      } else if (expired) //the content just expired, clean up
      {
         db().buying_expire(buying);
      }

      return void_result();
   }FC_CAPTURE_AND_RETHROW( (o) ) }

   operation_result leave_rating_evaluator::do_evaluate(const operation_type& o )
   {try{
      //check in buying history if the object exists
      auto& idx = db().get_index_type<content_index>().indices().get<by_URI>();
      const auto& content = idx.find( o.URI );
      auto& bidx = db().get_index_type<buying_index>().indices().get<by_consumer_URI>();
      const auto& bo = bidx.find( std::make_tuple(o.consumer, o.URI) );
      FC_ASSERT( content != idx.end() && bo != bidx.end() );
      FC_ASSERT( bo->delivered, "not delivered" );
      FC_ASSERT( !bo->rated_or_commented, "already rated or commented" );

      return void_result();
   }FC_CAPTURE_AND_RETHROW( (o) ) }

   operation_result leave_rating_evaluator::do_apply(const operation_type& o )
   {try{
      //adjust content statistics
      auto& bidx = db().get_index_type<buying_index>().indices().get<by_consumer_URI>();
      const auto& bo = bidx.find( std::make_tuple(o.consumer, o.URI) );
      auto& idx = db().get_index_type<content_index>().indices().get<by_URI>();
      const auto& content = idx.find( o.URI );

      db().modify<buying_object>( *bo, [&]( buying_object& b ){
           b.rated_or_commented = true;
           b.rating = static_cast<uint32_t>(o.rating);
           b.comment = o.comment;
      });

      db().modify<content_object> ( *content, [&](content_object& co){
           co.AVG_rating = (co.AVG_rating * co.num_of_ratings + static_cast<uint32_t>(o.rating) * 1000) / (co.num_of_ratings + 1);
           co.num_of_ratings++;
      });

      auto& d = db();
      db().create<transaction_detail_object>([&o, &d](transaction_detail_object& obj)
                                             {
                                                obj.m_operation_type = (uint8_t)transaction_detail_object::content_rate;

                                                const auto& idx = d.get_index_type<content_index>().indices().get<by_URI>();
                                                auto itr = idx.find(o.URI);
                                                if (itr != idx.end())
                                                {
                                                   obj.m_to_account = itr->author;

                                                   graphene::chain::ContentObjectPropertyManager synopsis_parser(itr->synopsis);
                                                   obj.m_str_description = synopsis_parser.get<graphene::chain::ContentObjectTitle>();
                                                }

                                                obj.m_from_account = o.consumer;

                                                obj.m_transaction_amount = asset();
                                                obj.m_transaction_fee = o.fee;
                                                obj.m_str_description = std::to_string(o.rating) + " (" + obj.m_str_description + ")";
                                                obj.m_timestamp = d.head_block_time();
                                             });

      return void_result();
   }FC_CAPTURE_AND_RETHROW( (o) ) }

   operation_result ready_to_publish_obsolete_evaluator::do_evaluate(const operation_type& o)
   {try{
      return void_result();
   }FC_CAPTURE_AND_RETHROW( (o) ) }

   operation_result ready_to_publish_obsolete_evaluator::do_apply(const operation_type& o)
   {try{
      auto& idx = db().get_index_type<seeder_index>().indices().get<by_seeder>();
      const auto& sor = idx.find( o.seeder );
      operation_result result;
      if( sor == idx.end() ) { //this is initial publish request
         const seeding_statistics_object &sso = db().create<seeding_statistics_object>([&o](seeding_statistics_object &sso) {
              sso.seeder = o.seeder;
              sso.total_upload = 0;
         });
         result = sso.id;
         db().create<seeder_object>([&](seeder_object& so) {
              so.seeder = o.seeder;
              so.free_space = o.space;
              so.pubKey = o.pubKey;
              so.price = asset(o.price_per_MByte);
              so.expiration = db().head_block_time() + DECENT_RTP_VALIDITY;
              so.ipfs_ID = o.ipfs_ID;
              so.stats = sso.id;
         });
      } else
      { //this is republish case
         db().modify<seeder_object>(*sor,[&](seeder_object &so) {
            so.free_space = o.space;
            so.price = asset(o.price_per_MByte);
            so.pubKey = o.pubKey;
            so.expiration = db().head_block_time() + 24 * 3600;
            so.ipfs_ID = o.ipfs_ID;

         });
      }

      return result;
   }FC_CAPTURE_AND_RETHROW( (o) ) }

   operation_result ready_to_publish_evaluator::do_evaluate(const operation_type& o )
   {try{
      FC_ASSERT(db().head_block_time() >= HARDFORK_1_TIME );
      return void_result();
   }FC_CAPTURE_AND_RETHROW( (o) ) }

   operation_result ready_to_publish_evaluator::do_apply(const operation_type& o )
   {try{
      auto& idx = db().get_index_type<seeder_index>().indices().get<by_seeder>();
      const auto& sor = idx.find( o.seeder );
      operation_result result;
      if( sor == idx.end() ) { //this is initial publish request
         const seeding_statistics_object &sso = db().create<seeding_statistics_object>([&o](seeding_statistics_object &sso) {
              sso.seeder = o.seeder;
              sso.total_upload = 0;
         });
         result = sso.id;
         db().create<seeder_object>([&](seeder_object& so) {
              so.seeder = o.seeder;
              so.free_space = o.space;
              so.pubKey = o.pubKey;
              so.price = asset(o.price_per_MByte);
              so.expiration = db().head_block_time() + DECENT_RTP_VALIDITY;
              so.ipfs_ID = o.ipfs_ID;
              so.stats = sso.id;
              if( o.region_code.valid() )
                 so.region_code = *o.region_code;
              else
                 so.region_code = "";
         });
      } else
      { //this is republish case
         db().modify<seeder_object>(*sor,[&](seeder_object &so) {
              so.free_space = o.space;
              so.price = asset(o.price_per_MByte);
              so.pubKey = o.pubKey;
              so.expiration = db().head_block_time() + 24 * 3600;
              so.ipfs_ID = o.ipfs_ID;

              if( o.region_code.valid() )
                 so.region_code = *o.region_code;
              else
                 so.region_code = "";
         });
      }

      return result;
   }FC_CAPTURE_AND_RETHROW( (o) ) }

   operation_result proof_of_custody_evaluator::do_evaluate(const operation_type& o )
   {try{
      auto& idx = db().get_index_type<content_index>().indices().get<by_URI>();
      const auto& content = idx.find( o.URI );
      FC_ASSERT( content != idx.end(), "content not found" );
      FC_ASSERT( content->expiration > db().head_block_time(), "content expired" );
      //verify that the seed is not too old...
      if (o.proof.valid() )
      {
         auto& proof = *o.proof;

         fc::ripemd160 bid = db().get_block_id_for_num(proof.reference_block);
         for(int i = 0; i < 5; i++)
            FC_ASSERT(bid._hash[i] == proof.seed.data[i],"Block ID does not match; wrong chain?");
         FC_ASSERT(db().head_block_num() <= proof.reference_block + 6,"Block reference is too old");
      }
      //
      FC_ASSERT( content->cd.valid() == o.proof.valid() );

      if(!(db().get_node_properties().skip_flags&db().skip_validate)) {
         FC_ASSERT( !(content->cd.valid() ) || _custody_utils.verify_by_miner( *(content->cd), *(o.proof) ) == 0, "Invalid proof of custody" );
      }
      //ilog("proof_of_custody OK");

      return void_result();
   }FC_CAPTURE_AND_RETHROW( (o) ) }

   operation_result proof_of_custody_evaluator::do_apply(const operation_type& o )
   {try{
      //get the seeder and content
      auto& idx = db().get_index_type<content_index>().indices().get<by_URI>();
      const auto& content = idx.find( o.URI );
      const auto& sidx = db().get_index_type<seeder_index>().indices().get<by_seeder>();
      const auto& sitr = sidx.find(o.seeder);
      FC_ASSERT(sitr!=sidx.end(), "seeder not found");
      const seeder_object& seeder = *sitr;

      auto last_proof = content->last_proof.find( o.seeder );
      if( last_proof == content->last_proof.end() ) //initial PoR
      {
         //the initial proof, no payments yet
         db().modify<content_object>(*content, [&](content_object& co){
              co.last_proof.emplace(std::make_pair(o.seeder, db().head_block_time()));
         });

         const auto& idx2 = db().get_index_type<seeding_statistics_index>().indices().get<by_seeder>();
         const auto& stats = idx2.find( o.seeder );
         db().modify<seeding_statistics_object>( *stats, [&content](seeding_statistics_object& so){
            so.total_content_seeded += ( content->size * 1000 * 1000 ) / content->key_parts.size();
            so.num_of_content_seeded++;
            if( so.total_content_requested_to_seed > 0 )
               so.total_content_requested_to_seed -= 1;
         });
      }else{
         const auto& idx2 = db().get_index_type<seeding_statistics_index>().indices().get<by_seeder>();
         const auto& stats = idx2.find( o.seeder );
         db().modify<seeding_statistics_object>( *stats, [](seeding_statistics_object& so){
            so.num_of_pors++;
         });
         //recurrent PoR, calculate payment
         //the PoR shall be ideally broadcasted once per 24h. if the seeder pushes them too often, he is penalized by a
         // loss factor equal to one forth of the time remaining to 24h. E.g. by pushing it in 12h he is penalized by
         // loss = (12/24)/4 = 12,5%; if it is pushed in 18h (i.e. 6 hours prematurely) the loss = (6/24)/4=6,25%.
         fc::microseconds diff = db().head_block_time() - last_proof->second;
         if( diff > fc::days( 1 ) )
            diff = fc::days( 1 ) ;
         uint64_t ratio = 10000 * diff.count() / fc::days( 1 ).count();
         uint64_t loss = ( 10000 - ratio ) / 4;
         uint64_t total_reward_ratio = ( ratio * ( 10000 - loss ) ) / 10000;
         asset reward(0);
         if (db().head_block_num() > 404727) {
            const auto& itr = content->seeder_price.find(o.seeder);
            FC_ASSERT(itr != content->seeder_price.end());
            reward.amount = itr->second * total_reward_ratio * content->size / 10000;
         }
         else
            reward.amount = seeder.price.amount * total_reward_ratio * content->size / 10000 ;
         //Fix due to block halt at #404726
         if( db().head_block_num() > 404727 && reward.amount > content->publishing_fee_escrow.amount ) {
            reward.amount = std::max(int64_t(0), content->publishing_fee_escrow.amount.value );
         }
         //take care of the payment
         db().modify<content_object>( *content, [&] (content_object& co ){
            co.last_proof[o.seeder] = db().head_block_time();
            co.publishing_fee_escrow -= reward;
         });
         db().adjust_balance(seeder.seeder, reward );
         pay_seeder_operation op;
         op.author = content->author;
         op.seeder = seeder.seeder;
         op.payout = reward;
         db().push_applied_operation(op);
      }

      return void_result();
   }FC_CAPTURE_AND_RETHROW( (o) ) }

   operation_result report_stats_evaluator::do_evaluate(const operation_type& o)
   {
	   try {
         for (const auto& item : o.stats)
         {
            FC_ASSERT( db().find_object(item.first), "Invalid seeder account specified." );
         }
	   }FC_CAPTURE_AND_RETHROW((o))

      return void_result();
   }

   operation_result report_stats_evaluator::do_apply(const operation_type& o)
   {
      try {
         auto& idx = db().get_index_type<seeding_statistics_index>().indices().get<by_seeder>();
         for (const auto& item : o.stats)
         {
            const auto &so = idx.find(item.first);
            db().modify<seeding_statistics_object>(*so, [&item](seeding_statistics_object &sso) {
               sso.total_upload += item.second;
            });
         }
      }FC_CAPTURE_AND_RETHROW((o))

      return void_result();
   }

}} // graphene::chain
