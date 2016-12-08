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
#include <decent/encrypt/encryptionutils.hpp>

namespace graphene { namespace chain {
  
   void_result content_submit_evaluator::do_evaluate(const content_submit_operation& o )
   {
      auto& idx = db().get_index_type<seeder_index>().indices().get<by_seeder>();
      asset total_price_per_day;
      for ( const auto &p : o.seeders ){
         const auto& itr = idx.find( p );
         FC_ASSERT(itr != idx.end(), "seeder does not exist" );
         total_price_per_day += itr-> price;
      }
      FC_ASSERT( o.seeders.size() == o.key_parts.size() );
      FC_ASSERT( db().head_block_time() <= o.expiration);
      fc::microseconds duration = (db().head_block_time() - o.expiration);
      uint64_t days = duration.to_seconds() / 3600 / 24;
      FC_ASSERT( days*total_price_per_day <= o.publishing_fee );
      //TODO_DECENT - URI check, synopsis check
      //TODO_DECENT - what if it is resubmit?
   }
   
   void_result content_submit_evaluator::do_apply(const content_submit_operation& o )
   {
      db().create<content_object>( [&](content_object& co){
         co.author = o.author;
         co.price = o.price;
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

         co.hash = o. hash;
         co.expiration = o.expiration;
         co.created = db().head_block_time();
         co.times_bought = 0;
         co.AVG_rating = 0;
         co.total_rating = 0;
      });
      db().adjust_balance(o.author,-o.publishing_fee);
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
   {
      const auto& buying = db().get<buying_object>(o.buying);
      auto& idx = db().get_index_type<content_index>().indices().get<by_URI>();
      const auto& content = idx.find( buying.URI );
      const auto& seeder = db().get<seeder_object>( o.seeder );
      const auto& seeder_pubKey = seeder.pubKey;
      const auto& buyer_pubKey = buying.pubKey;
      const auto& firstK = content->key_parts.at( o.seeder );
      const auto& secondK = o.key;
      const auto& proof = o.proof;
      FC_ASSERT( decent::crypto::verify_delivery_proof( proof, firstK, secondK, seeder_pubKey, buyer_pubKey) );
   }

   void_result deliver_keys_evaluator::do_apply(const deliver_keys_operation& o )
   {
      const auto& buying = db().get<buying_object>(o.buying);
      db().modify<buying_object>(buying, [&](buying_object& bo){
           bo.seeders_answered.push_back( o.seeder );
           //check if we have enough answers already
      });
   }

   void_result leave_rating_evaluator::do_evaluate(const leave_rating_operation& o )
   {
      //check in buying history if the object exists
   }
   
   void_result leave_rating_evaluator::do_apply(const leave_rating_operation& o )
   {
      //create rating object and adjust content statistics
   }
   
   void_result ready_to_publish_evaluator::do_evaluate(const ready_to_publish_operation& o )
   {
      //empty
   }
   
   void_result ready_to_publish_evaluator::do_apply(const ready_to_publish_operation& o )
   {
      //create seeding object
   }
   
   void_result proof_of_custody_evaluator::do_evaluate(const proof_of_custody_operation& o )
   {
      //validate the proof
   }
   
   void_result proof_of_custody_evaluator::do_apply(const proof_of_custody_operation& o )
   {
      //pay the seeder
   }
   

   
}} // graphene::chain
