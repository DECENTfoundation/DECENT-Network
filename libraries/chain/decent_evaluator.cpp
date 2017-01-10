#include <graphene/chain/decent_evaluator.hpp>
#include <graphene/chain/protocol/decent.hpp>

#include <graphene/chain/database.hpp>
#include <graphene/chain/seeder_object.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/hardfork.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/buying_object.hpp>
#include <graphene/chain/seeder_object.hpp>
#include <graphene/chain/content_object.hpp>
#include <graphene/chain/rating_object.hpp>


#include <decent/encrypt/encryptionutils.hpp>

namespace graphene { namespace chain {
  
   void_result content_submit_evaluator::do_evaluate(const content_submit_operation& o )
   {
      auto& idx = db().get_index_type<seeder_index>().indices().get<by_seeder>();
      asset total_price_per_day;
      for ( const auto &p : o.seeders ){
         const auto& itr = idx.find( p );
         FC_ASSERT( itr != idx.end(), "seeder does not exist" );
         FC_ASSERT( itr->free_space > o.size );
         total_price_per_day += itr-> price;
      }
      FC_ASSERT( o.seeders.size() == o.key_parts.size() );
      FC_ASSERT( db().head_block_time() <= o.expiration);
      fc::microseconds duration = (db().head_block_time() - o.expiration);
      uint64_t days = duration.to_seconds() / 3600 / 24;
      FC_ASSERT( days*total_price_per_day <= o.publishing_fee );
      //TODO_DECENT - URI check, synopsis check
      //TODO_DECENT - what if it is resubmit? Drop 2
      //TODO_DECENT - return escrow after expiration
   }
   
   void_result content_submit_evaluator::do_apply(const content_submit_operation& o )
   {
      db().create<content_object>( [&](content_object& co){
         co.author = o.author;
         co.price = o.price;
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
         co.quorum = o.quorum;
         co.expiration = o.expiration;
         co.created = db().head_block_time();
         co.times_bought = 0;
         co.AVG_rating = 0;
         co.total_rating = 0;
      });
      db().adjust_balance(o.author,-o.publishing_fee);
      auto& idx = db().get_index_type<seeder_index>().indices().get<by_seeder>();
      //TODO_DECENT we should better remove the space after the first PoC
      for ( const auto &p : o.seeders ){
         const auto& itr = idx.find( p );
         FC_ASSERT( itr != idx.end(), "seeder does not exist" );
         db().modify<seeder_object>( *itr, [&](seeder_object& so){
              so.free_space -= o.size;
         });
      }

   }
   
   void_result request_to_buy_evaluator::do_evaluate(const request_to_buy_operation& o )
   {
      auto& idx = db().get_index_type<content_index>().indices().get<by_URI>();
      const auto& content = idx.find( o.URI );
      FC_ASSERT( content!= idx.end() );
      FC_ASSERT( o.price >= content->price );
      FC_ASSERT( content->expiration > db().head_block_time() );
      //check if the content exist, has not expired and the price is higher as expected
   }
   
   void_result request_to_buy_evaluator::do_apply(const request_to_buy_operation& o )
   {
      db().create<buying_object>([&](buying_object& bo){
           bo.consumer = o.consumer;
           bo.URI = o.URI;
           bo.expiration_time = db().head_block_time() + 24*3600;
           bo.pubKey = o.pubKey;
      });
   }

   void_result deliver_keys_evaluator::do_evaluate(const deliver_keys_operation& o )
   {try{
      //TODO_DECENT rewrite the next statement!!!
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
      FC_ASSERT( decent::crypto::verify_delivery_proof( proof, firstK, secondK, seeder_pubKey, buyer_pubKey) );
   }catch( ... )
   {
      fc::unhandled_exception e( FC_LOG_MESSAGE( warn, "throw"), std::current_exception() );
      wlog( "${details}", ("details",e.to_detail_string()) ); 
   }}

   void_result deliver_keys_evaluator::do_apply(const deliver_keys_operation& o )
   {try{
      const auto& buying = db().get<buying_object>(o.buying);
      bool expired = ( buying.expiration_time > db().head_block_time() );
      auto& idx = db().get_index_type<content_index>().indices().get<by_URI>();
      const auto& content = idx.find( buying.URI );
      bool delivered;
      db().modify<buying_object>(buying, [&](buying_object& bo){
           bo.seeders_answered.push_back( o.seeder );
           delivered = ( bo.seeders_answered.size() >= content->quorum );
           bo.key_particles.push_back( o.key );
      });
      if( delivered || expired )
      {
         db().create<buying_history_object>([&](buying_history_object& bho){
              bho.consumer = buying.consumer;
              bho.delivered = delivered;
              bho.seeders_answered = buying.seeders_answered;
              bho.key_particles = buying.key_particles;
              bho.URI = buying.URI;
              bho.time = db().head_block_time();
         });
         db().remove(buying);
      }
      if( delivered )
      {
         db().modify<content_object>( *content, []( content_object& co ){ co.times_bought++; });
      }
   }catch( ... )
   {
      fc::unhandled_exception e( FC_LOG_MESSAGE( warn, "throw"), std::current_exception() ); 
      wlog( "${details}", ("details",e.to_detail_string()) ); 
   }
   }

   void_result leave_rating_evaluator::do_evaluate(const leave_rating_operation& o )
   {
      //check in buying history if the object exists
      auto& idx = db().get_index_type<content_index>().indices().get<by_URI>();
      const auto& content = idx.find( o.URI );
      auto& bidx = db().get_index_type<buying_history_index>().indices().get<by_consumer_URI>();
      const auto& bho = bidx.find( std::make_tuple(o.consumer, o.URI) );
      FC_ASSERT( content != idx.end() && bho != bidx.end() );
      FC_ASSERT( bho->delivered, "not delivered" );
      FC_ASSERT( !bho->rated, "already rated" );
      FC_ASSERT( o.rating >= 0 && o.rating <=10 );
   }
   
   void_result leave_rating_evaluator::do_apply(const leave_rating_operation& o )
   {
      //create rating object and adjust content statistics
      auto& bidx = db().get_index_type<buying_history_index>().indices().get<by_consumer_URI>();
      const auto& bho = bidx.find( std::make_tuple(o.consumer, o.URI) );
      auto& idx = db().get_index_type<content_index>().indices().get<by_URI>();
      const auto& content = idx.find( o.URI );

      db().create<rating_object>([&]( rating_object& ro ){
           ro.buying = bho->id;
           ro.consumer = o.consumer;
           ro.URI = o.URI;
           ro.rating = o.rating;
      });

      db().modify<buying_history_object>( *bho, [&]( buying_history_object& b ){ b.rated = true; });
      db().modify<content_object> ( *content, [&](content_object& co){
           if(co.total_rating == 1)
              co.AVG_rating = o.rating * 1000;
           else
              co.AVG_rating = (co.AVG_rating * co.total_rating + o.rating * 1000) / (co.total_rating++);

           co.total_rating++;
      });
   }
   
   void_result ready_to_publish_evaluator::do_evaluate(const ready_to_publish_operation& o )
   {
      //empty
      FC_ASSERT( o.space > 0 );
   }
   
   void_result ready_to_publish_evaluator::do_apply(const ready_to_publish_operation& o )
   {
      auto& idx = db().get_index_type<seeder_index>().indices().get<by_seeder>();
      const auto& sor = idx.find( o.seeder );
      if( sor == idx.end() ) {
         db().create<seeder_object>([&](seeder_object &so) {
              so.seeder = o.seeder;
              so.free_space = o.space;
              so.pubKey = o.pubKey;
              so.price = asset(o.price_per_MByte);
         });
      } else{
         db().modify<seeder_object>(*sor,[&](seeder_object &so) {
            so.free_space = o.space;
            so.price = asset(o.price_per_MByte);
         });
      }
   }
   
   void_result proof_of_custody_evaluator::do_evaluate(const proof_of_custody_operation& o )
   {
      //validate the proof - TODO_DECENT
      auto& idx = db().get_index_type<content_index>().indices().get<by_URI>();
      const auto& content = idx.find( o.URI );
      FC_ASSERT( content != idx.end() );
      FC_ASSERT( content->expiration > db().head_block_time() );
   }
   
   void_result proof_of_custody_evaluator::do_apply(const proof_of_custody_operation& o )
   {
      //pay the seeder
      auto& idx = db().get_index_type<content_index>().indices().get<by_URI>();
      const auto& content = idx.find( o.URI );
      //TODO_DECENT rewrite the next statement
      const auto& seeder = db().get<seeder_object>(o.seeder);
      auto last_proof = content->last_proof.find( o.seeder );
      if( last_proof == content->last_proof.end() )
      {
         //the inital proof, no payments yet
         db().modify<content_object>(*content, [&](content_object& co){
              co.last_proof.emplace(std::make_pair(o.seeder, db().head_block_time()));
         });
      }else{
         fc::microseconds diff = db().head_block_time() - last_proof->second;
         if( diff > fc::days( 1 ) )
            diff = fc::days( 1 ) ;
         uint64_t ratio = 100 * diff.count() / fc::days( 1 ).count();
         uint64_t loss = ( 100 - ratio ) / 4;
         uint64_t total_reward_ratio = ( ratio * ( 100 - loss ) ) / 100;
         asset reward ( seeder.price.amount * total_reward_ratio / 100 );
         db().modify<content_object>( *content, [&] (content_object& co ){
              co.last_proof[o.seeder] = db().head_block_time();
              co.publishing_fee_escrow -= reward;
         });
         db().adjust_balance(seeder.seeder, reward );
      }
   }

}} // graphene::chain
