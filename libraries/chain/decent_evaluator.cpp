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
#include <graphene/chain/seeding_statistics_object.hpp>

#include <decent/encrypt/encryptionutils.hpp>

namespace graphene { namespace chain {
  
   void_result content_submit_evaluator::do_evaluate(const content_submit_operation& o )
   {try{
      auto& idx = db().get_index_type<seeder_index>().indices().get<by_seeder>();
      asset total_price_per_day;
      for ( const auto &p : o.seeders ){
         const auto& itr = idx.find( p );
         FC_ASSERT( itr != idx.end(), "seeder does not exist" );
         FC_ASSERT( itr->free_space > o.size );
         total_price_per_day += itr-> price.amount * o.size;
      }
      FC_ASSERT( o.seeders.size() == o.key_parts.size() );
      FC_ASSERT( db().head_block_time() <= o.expiration);
      fc::microseconds duration = (o.expiration - db().head_block_time() );
      uint64_t days = duration.to_seconds() / 3600 / 24;
      ilog("yo: ${n}", ("n", days));
      idump((total_price_per_day));
      FC_ASSERT( days*total_price_per_day <= o.publishing_fee );
      //TODO_DECENT - URI check, synopsis check
      //TODO_DECENT - what if it is resubmit? Drop 2
      //TODO_DECENT - return escrow after expiration
   }FC_CAPTURE_AND_RETHROW( (o) ) }
   
   void_result content_submit_evaluator::do_apply(const content_submit_operation& o )
   {try{
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
         co.cd = o.cd;
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

   }FC_CAPTURE_AND_RETHROW( (o) ) }
   
   void_result request_to_buy_evaluator::do_evaluate(const request_to_buy_operation& o )
   {try{
      auto& idx = db().get_index_type<content_index>().indices().get<by_URI>();
      const auto& content = idx.find( o.URI );
      FC_ASSERT( content!= idx.end() );
      FC_ASSERT( o.price <= db().get_balance( o.consumer, o.price.asset_id ) );
      FC_ASSERT( o.price >= content->price );
      FC_ASSERT( content->expiration > db().head_block_time() );
      FC_ASSERT( o.price.asset_id == content->price.asset_id );

   }FC_CAPTURE_AND_RETHROW( (o) ) }

   void_result request_to_buy_evaluator::do_apply(const request_to_buy_operation& o )
   {try{
      const auto& object = db().create<buying_object>([&](buying_object& bo){
           bo.consumer = o.consumer;
           bo.URI = o.URI;
           bo.expiration_time = db().head_block_time() + 24*3600;
           bo.pubKey = o.pubKey;
           bo.price = o.price;
      });
      db().adjust_balance( o.consumer, -o.price );
      elog("Created bying_object with id ${i}", ("i", object.id));
      edump((object));
   }FC_CAPTURE_AND_RETHROW( (o) ) }

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
   }FC_CAPTURE_AND_RETHROW( (o) ) }

   void_result deliver_keys_evaluator::do_apply(const deliver_keys_operation& o )
   {try{
      const auto& buying = db().get<buying_object>(o.buying);
      bool expired = ( buying.expiration_time < db().head_block_time() );
      auto& idx = db().get_index_type<content_index>().indices().get<by_URI>();
      const auto& content = idx.find( buying.URI );
      bool delivered;

      if( std::find(buying.seeders_answered.begin(), buying.seeders_answered.end(), o.seeder) == buying.seeders_answered.end() )
         db().modify<buying_object>(buying, [&](buying_object& bo){
              bo.seeders_answered.push_back( o.seeder );
              bo.key_particles.push_back( decent::crypto::ciphertext(o.key) );
         });
      delivered = buying.seeders_answered.size() >= content->quorum;
      //if the content has already been delivered or expired, just note the key particles and go on
      if( buying.delivered || buying.expired )
         return void_result();

      if( delivered )
      {
         db().modify<content_object>( *content, []( content_object& co ){ co.times_bought++; });
         db().adjust_balance( content->author, buying.price );
         db().modify<buying_object>(buying, [&](buying_object& bo){
              bo.price.amount = 0;
              bo.delivered = true;
              bo.expiration_or_delivery_time = db().head_block_time();
         });
      } else if (expired)
      {
         db().buying_expire(buying);
      }
   }FC_CAPTURE_AND_RETHROW( (o) ) }

   void_result leave_rating_evaluator::do_evaluate(const leave_rating_operation& o )
   {try{
      //check in buying history if the object exists
      auto& idx = db().get_index_type<content_index>().indices().get<by_URI>();
      const auto& content = idx.find( o.URI );
      auto& bidx = db().get_index_type<buying_index>().indices().get<by_consumer_URI>();
      const auto& bo = bidx.find( std::make_tuple(o.consumer, o.URI) );
      FC_ASSERT( content != idx.end() && bo != bidx.end() );
      FC_ASSERT( bo->delivered, "not delivered" );
      FC_ASSERT( !bo->rated, "already rated" );
      FC_ASSERT( o.rating >= 0 && o.rating <=10 );
   }FC_CAPTURE_AND_RETHROW( (o) ) }
   
   void_result leave_rating_evaluator::do_apply(const leave_rating_operation& o )
   {try{
      //create rating object and adjust content statistics
      auto& bidx = db().get_index_type<buying_index>().indices().get<by_consumer_URI>();
      const auto& bo = bidx.find( std::make_tuple(o.consumer, o.URI) );
      auto& idx = db().get_index_type<content_index>().indices().get<by_URI>();
      const auto& content = idx.find( o.URI );

      db().create<rating_object>([&]( rating_object& ro ){
           ro.buying = bo->id;
           ro.consumer = o.consumer;
           ro.URI = o.URI;
           ro.rating = o.rating;
      });

      db().modify<buying_object>( *bo, [&]( buying_object& b ){ b.rated = true; });
      db().modify<content_object> ( *content, [&](content_object& co){
           if(co.total_rating == 1)
              co.AVG_rating = o.rating * 1000;
           else
              co.AVG_rating = (co.AVG_rating * co.total_rating + o.rating * 1000) / (co.total_rating++);

           co.total_rating++;
      });
   }FC_CAPTURE_AND_RETHROW( (o) ) }
   
   void_result ready_to_publish_evaluator::do_evaluate(const ready_to_publish_operation& o )
   {try{
   }FC_CAPTURE_AND_RETHROW( (o) ) }
   
   void_result ready_to_publish_evaluator::do_apply(const ready_to_publish_operation& o )
   {try{
      auto& idx = db().get_index_type<seeder_index>().indices().get<by_seeder>();
      const auto& sor = idx.find( o.seeder );
      if( sor == idx.end() ) {
         db().create<seeder_object>([&](seeder_object &so) {
              so.seeder = o.seeder;
              so.free_space = o.space;
              so.pubKey = o.pubKey;
              so.price = asset(o.price_per_MByte);
              so.expiration = db().head_block_time() + 24 * 3600;
              so.ipfs_IDs = o.ipfs_IDs;
              so.stats = db().create<seeding_statistics_object>([&](seeding_statistics_object &sso) {
                 sso.seeder = o.seeder;
                 sso.total_upload = 0;
              }).id;
         });
      } else{
         db().modify<seeder_object>(*sor,[&](seeder_object &so) {
            so.free_space = o.space;
            so.price = asset(o.price_per_MByte);
            so.pubKey = o.pubKey;
            so.expiration = db().head_block_time() + 24 * 3600;
            so.ipfs_IDs.clear();
            so.ipfs_IDs = o.ipfs_IDs;
         });
      }
   }FC_CAPTURE_AND_RETHROW( (o) ) }
   
   void_result proof_of_custody_evaluator::do_evaluate(const proof_of_custody_operation& o )
   {try{
      auto& idx = db().get_index_type<content_index>().indices().get<by_URI>();
      const auto& content = idx.find( o.URI );
      FC_ASSERT( content != idx.end() );
      FC_ASSERT( content->expiration > db().head_block_time() );
      //verify that the seed is not to old...
      fc::ripemd160 bid = db().get_block_id_for_num(o.proof.reference_block);
      for(int i = 0; i < 5; i++)
         FC_ASSERT(bid._hash[i] == o.proof.seed.data[i],"Block ID does not match; wrong chain?");
      FC_ASSERT(db().head_block_num() <= o.proof.reference_block - 6,"Block reference is too old");
      FC_ASSERT( _custody_utils.verify_by_miner( content->cd, o.proof ) == 0, "Invalid proof of delivery" );
      elog("proof_of_custody OK");
   }FC_CAPTURE_AND_RETHROW( (o) ) }
   
   void_result proof_of_custody_evaluator::do_apply(const proof_of_custody_operation& o )
   {try{
      //pay the seeder
      auto& idx = db().get_index_type<content_index>().indices().get<by_URI>();
      const auto& content = idx.find( o.URI );
      //TODO_DECENT rewrite the next statement
      const seeder_object& seeder = db().get<seeder_object>(o.seeder);
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
   }FC_CAPTURE_AND_RETHROW( (o) ) }

   void_result return_escrow_submission_evaluator::do_evaluate(const return_escrow_submission_operation& o ) {}
   void_result return_escrow_submission_evaluator::do_apply(const return_escrow_submission_operation& o ) {}

   void_result return_escrow_buying_evaluator::do_evaluate(const return_escrow_buying_operation& o ) {}
   void_result return_escrow_buying_evaluator::do_apply(const return_escrow_buying_operation& o ) {}

   void_result report_stats_evaluator::do_evaluate(const report_stats_operation& o )
   {try{
      auto& idx = db().get_index_type<seeder_index>().indices().get<by_seeder>();
      const auto& itr = idx.find( o.seeder );
      FC_ASSERT( itr != idx.end(), "seeder does not exist" );

      }FC_CAPTURE_AND_RETHROW( (o) ) }

   void_result report_stats_evaluator::do_apply(const report_stats_operation& o )
   {try{

      }FC_CAPTURE_AND_RETHROW( (o) ) }

}} // graphene::chain
