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
#include <graphene/chain/subscription_object.hpp>


#include <decent/encrypt/encryptionutils.hpp>

namespace graphene { namespace chain {
  
   void_result content_submit_evaluator::do_evaluate(const content_submit_operation& op )
   {
      try {
         auto &idx = db().get_index_type<seeder_index>().indices().get<by_seeder>();
         asset total_price_per_day;
         for (const auto &p : op.seeders) {
            const auto &itr = idx.find(p);
            FC_ASSERT(itr != idx.end(), "seeder does not exist");
            FC_ASSERT(itr->free_space > op.size);
            total_price_per_day += itr->price;
         }
         FC_ASSERT(op.seeders.size() == op.key_parts.size());
         FC_ASSERT(db().head_block_time() <= op.expiration);
         fc::microseconds duration = (db().head_block_time() - op.expiration);
         uint64_t days = duration.to_seconds() / 3600 / 24;
         FC_ASSERT(days * total_price_per_day <= op.publishing_fee);
         //TODO_DECENT - URI check, synopsis check
         //TODO_DECENT - what if it is resubmit? Drop 2
         //TODO_DECENT - return escrow after expiration

         return void_result();
      } FC_CAPTURE_AND_RETHROW( (op) )
   }
   
   void_result content_submit_evaluator::do_apply(const content_submit_operation& op )
   {
      try {
         db().create<content_object>([&](content_object &co) {
            co.author = op.author;
            co.price = op.price;
            co.size = op.size;
            co.synopsis = op.synopsis;
            co.URI = op.URI;
            co.publishing_fee_escrow = op.publishing_fee;
            auto itr1 = op.seeders.begin();
            auto itr2 = op.key_parts.begin();
            while (itr1 != op.seeders.end() && itr2 != op.key_parts.end()) {
               co.key_parts.emplace(std::make_pair(*itr1, *itr2));
               itr1++;
               itr2++;
            }
            co._hash = op.hash;
            co.quorum = op.quorum;
            co.expiration = op.expiration;
            co.created = db().head_block_time();
            co.times_bought = 0;
            co.AVG_rating = 0;
            co.total_rating = 0;
         });
         db().adjust_balance(op.author, -op.publishing_fee);
         auto &idx = db().get_index_type<seeder_index>().indices().get<by_seeder>();
         //TODO_DECENT we should better remove the space after the first PoC
         for (const auto &p : op.seeders) {
            const auto &itr = idx.find(p);
            FC_ASSERT(itr != idx.end(), "seeder does not exist");
            db().modify<seeder_object>(*itr, [&](seeder_object &so) {
               so.free_space -= op.size;
            });
         }

         return void_result();
      } FC_CAPTURE_AND_RETHROW( (op) )
   }
   
   void_result request_to_buy_evaluator::do_evaluate(const request_to_buy_operation& op )
   {
      try {
         auto &idx = db().get_index_type<content_index>().indices().get<by_URI>();
         const auto &content = idx.find(op.URI);
         FC_ASSERT(content != idx.end());
         FC_ASSERT(content->expiration > db().head_block_time());

         auto &range = db().get_index_type<subscription_index>().indices().get<by_from_to>();
         const auto &subscription = range.find(boost::make_tuple(op.consumer, content->author));

         /// Check whether subscription exists. If so, consumer doesn't pay for content
         if (op.consumer == subscription->from && content->author == subscription->to &&
             subscription->expiration > db().head_block_time())
            FC_ASSERT(op.price.amount >= 0);
         else
            FC_ASSERT(op.price >= content->price);

         return void_result();
      } FC_CAPTURE_AND_RETHROW( (op) )
   }
   
   void_result request_to_buy_evaluator::do_apply(const request_to_buy_operation& op )
   {
      try {
         db().create<buying_object>([&](buying_object &bo) {
            bo.consumer = op.consumer;
            bo.URI = op.URI;
            bo.expiration_time = db().head_block_time() + 24 * 3600;
            bo.pubKey = op.pubKey;
         });

         return void_result();
      } FC_CAPTURE_AND_RETHROW( (op) )
   }

   void_result deliver_keys_evaluator::do_evaluate(const deliver_keys_operation& op )
   {try{
      const auto& buying = db().get<buying_object>(op.buying);
      auto& idx = db().get_index_type<content_index>().indices().get<by_URI>();

      const auto& content = idx.find( buying.URI );
      FC_ASSERT( content != idx.end() );

      auto& sidx = db().get_index_type<seeder_index>().indices().get<by_seeder>();
      const auto& seeder = sidx.find(op.seeder);

      const auto& seeder_pubKey = seeder->pubKey;
      const auto& buyer_pubKey = buying.pubKey;
      const auto& firstK = content->key_parts.at( op.seeder );
      const auto& secondK = op.key;
      const auto& proof = op.proof;
      FC_ASSERT( decent::crypto::verify_delivery_proof( proof, firstK, secondK, seeder_pubKey, buyer_pubKey) );

      return void_result();
   }catch( ... )
   {
      fc::unhandled_exception e( FC_LOG_MESSAGE( warn, "throw"), std::current_exception() );
      wlog( "${details}", ("details",e.to_detail_string()) );
      throw e;
   }}

   void_result deliver_keys_evaluator::do_apply(const deliver_keys_operation& op )
   {try{
      const auto& buying = db().get<buying_object>(op.buying);
      bool expired = ( buying.expiration_time > db().head_block_time() );
      auto& idx = db().get_index_type<content_index>().indices().get<by_URI>();
      const auto& content = idx.find( buying.URI );
      bool delivered;
      db().modify<buying_object>(buying, [&](buying_object& bo){
           bo.seeders_answered.push_back( op.seeder );
           delivered = ( bo.seeders_answered.size() >= content->quorum );
           bo.key_particles.push_back( op.key );
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

      return void_result();
   }catch( ... )
   {
      fc::unhandled_exception e( FC_LOG_MESSAGE( warn, "throw"), std::current_exception() ); 
      wlog( "${details}", ("details",e.to_detail_string()) );
      throw e;
   }
   }

   void_result leave_rating_evaluator::do_evaluate(const leave_rating_operation& op )
   {
      try {
         //check in buying history if the object exists
         auto &idx = db().get_index_type<content_index>().indices().get<by_URI>();
         const auto &content = idx.find(op.URI);
         auto &bidx = db().get_index_type<buying_history_index>().indices().get<by_consumer_URI>();
         const auto &bho = bidx.find(std::make_tuple(op.consumer, op.URI));
         FC_ASSERT(content != idx.end() && bho != bidx.end());
         FC_ASSERT(bho->delivered, "not delivered");
         FC_ASSERT(!bho->rated, "already rated");
         FC_ASSERT(op.rating >= 0 && op.rating <= 10);

         return void_result();
      } FC_CAPTURE_AND_RETHROW( (op) )
   }
   
   void_result leave_rating_evaluator::do_apply(const leave_rating_operation& op )
   {
      try {
         //create rating object and adjust content statistics
         auto &bidx = db().get_index_type<buying_history_index>().indices().get<by_consumer_URI>();
         const auto &bho = bidx.find(std::make_tuple(op.consumer, op.URI));
         auto &idx = db().get_index_type<content_index>().indices().get<by_URI>();
         const auto &content = idx.find(op.URI);

         db().create<rating_object>([&](rating_object &ro) {
            ro.buying = bho->id;
            ro.consumer = op.consumer;
            ro.URI = op.URI;
            ro.rating = op.rating;
         });

         db().modify<buying_history_object>(*bho, [&](buying_history_object &b) { b.rated = true; });
         db().modify<content_object>(*content, [&](content_object &co) {
            if (co.total_rating == 1)
               co.AVG_rating = op.rating * 1000;
            else
               co.AVG_rating = (co.AVG_rating * co.total_rating + op.rating * 1000) / (co.total_rating++);

            co.total_rating++;
         });

         return void_result();
      } FC_CAPTURE_AND_RETHROW( (op) )
   }
   
   void_result ready_to_publish_evaluator::do_evaluate(const ready_to_publish_operation& op )
   {
      try {
         //empty
         FC_ASSERT(op.space > 0);

         return void_result();
      } FC_CAPTURE_AND_RETHROW( (op) )
   }
   
   void_result ready_to_publish_evaluator::do_apply(const ready_to_publish_operation& op )
   {
      try {
         auto &idx = db().get_index_type<seeder_index>().indices().get<by_seeder>();
         const auto &sor = idx.find(op.seeder);
         if (sor == idx.end()) {
            db().create<seeder_object>([&](seeder_object &so) {
               so.seeder = op.seeder;
               so.free_space = op.space;
               so.pubKey = op.pubKey;
               so.price = asset(op.price_per_MByte);
            });
         } else {
            db().modify<seeder_object>(*sor, [&](seeder_object &so) {
               so.free_space = op.space;
               so.price = asset(op.price_per_MByte);
            });
         }

         return void_result();
      } FC_CAPTURE_AND_RETHROW( (op) )
   }
   
   void_result proof_of_custody_evaluator::do_evaluate(const proof_of_custody_operation& op )
   {
      try {
         //validate the proof - TODO_DECENT
         auto &idx = db().get_index_type<content_index>().indices().get<by_URI>();
         const auto &content = idx.find(op.URI);
         FC_ASSERT(content != idx.end());
         FC_ASSERT(content->expiration > db().head_block_time());

         return void_result();
      } FC_CAPTURE_AND_RETHROW( (op) )
   }
   
   void_result proof_of_custody_evaluator::do_apply(const proof_of_custody_operation& op )
   {
      try {
         //pay the seeder
         auto &idx = db().get_index_type<content_index>().indices().get<by_URI>();
         const auto &content = idx.find(op.URI);
         const auto &seeder = db().get<seeder_object>(op.seeder);
         auto last_proof = content->last_proof.find(op.seeder);
         if (last_proof == content->last_proof.end()) {
            //the inital proof, no payments yet
            db().modify<content_object>(*content, [&](content_object &co) {
               co.last_proof.emplace(std::make_pair(op.seeder, db().head_block_time()));
            });
         } else {
            fc::microseconds diff = db().head_block_time() - last_proof->second;
            if (diff > fc::days(1))
               diff = fc::days(1);
            uint64_t ratio = 100 * diff.count() / fc::days(1).count();
            uint64_t loss = (100 - ratio) / 4;
            uint64_t total_reward_ratio = (ratio * (100 - loss)) / 100;
            asset reward(seeder.price.amount * total_reward_ratio / 100);
            db().modify<content_object>(*content, [&](content_object &co) {
               co.last_proof[op.seeder] = db().head_block_time();
               co.publishing_fee_escrow -= reward;
            });
            db().adjust_balance(seeder.seeder, reward);
         }

         return void_result();
      } FC_CAPTURE_AND_RETHROW( (op) )
   }

}} // graphene::chain
