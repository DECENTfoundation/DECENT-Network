#include <graphene/chain/database.hpp>
#include <graphene/chain/subscription_evaluator.hpp>
#include <graphene/chain/subscription_object.hpp>
#include <graphene/chain/account_object.hpp>

namespace graphene { namespace chain {

void_result subscribe_evaluator::do_evaluate( const subscribe_operation& op )
{
   try {
      auto& idx = db().get_index_type<account_index>().indices().get<by_id>();
      const auto& to_account = idx.find( op.to );
      FC_ASSERT( to_account != idx.end() , "author does not exist" );

      FC_ASSERT( to_account->allow_subscription , "account does not allow subscription" );
      FC_ASSERT( to_account->subscription_period > 0 ); // division by 0 is undefined
      period_count = op.duration / to_account->subscription_period;
      FC_ASSERT( period_count >= 1 );
      FC_ASSERT( op.price >= period_count * to_account->price_per_subscribe );

      return void_result();
   } FC_CAPTURE_AND_RETHROW( (op) )
}

void_result subscribe_evaluator::do_apply( const subscribe_operation& op )
{
   try {
      auto &idx = db().get_index_type<account_index>().indices().get<by_id>();
      const auto &to_account = idx.find(op.to);

      auto &idx2 = db().get_index_type<subscription_index>().indices().get<by_from_to>();
      const auto &subscription = idx2.find(boost::make_tuple(op.from, op.to));
      if (subscription != idx2.end()) {
         if (subscription->expiration < db().head_block_time())
            db().modify<subscription_object>(*subscription, [&](subscription_object &so) {
               so.expiration =
                       db().head_block_time() + period_count * to_account->subscription_period * 24 * 3600; // seconds
            });
         else
            db().modify<subscription_object>(*subscription, [&](subscription_object &so) {
               so.expiration += period_count * to_account->subscription_period * 24 * 3600; // seconds
            });
      } else {
         db().create<subscription_object>([&](subscription_object &so) {
            so.from = op.from;
            so.to = op.to;
            so.expiration =
                    db().head_block_time() + period_count * to_account->subscription_period * 24 * 3600; // seconds
         });
      }

      return void_result();
   } FC_CAPTURE_AND_RETHROW( (op) )
}

void_result subscribe_by_author_evaluator::do_evaluate( const subscribe_by_author_operation& op )
{
   try {
      auto &idx = db().get_index_type<account_index>().indices().get<by_id>();
      const auto &to_account = idx.find(op.to);
      FC_ASSERT(to_account != idx.end(), "author does not exist");

      FC_ASSERT(to_account->allow_subscription, "account does not allow subscription");
      FC_ASSERT(to_account->subscription_period > 0); // division by 0 is undefined
      period_count = op.duration / to_account->subscription_period;
      FC_ASSERT(period_count >= 1);

      return void_result();
   } FC_CAPTURE_AND_RETHROW( (op) )
}

void_result subscribe_by_author_evaluator::do_apply( const subscribe_by_author_operation& op )
{
   try {
      auto &idx = db().get_index_type<account_index>().indices().get<by_id>();
      const auto &to_account = idx.find(op.to);

      auto &idx2 = db().get_index_type<subscription_index>().indices().get<by_from_to>();
      const auto &subscription = idx2.find(boost::make_tuple(op.from, op.to));
      if (subscription != idx2.end()) {
         if (subscription->expiration < db().head_block_time())
            db().modify<subscription_object>(*subscription, [&](subscription_object &so) {
               so.expiration = db().head_block_time() + op.duration * 24 * 3600; // seconds
            });
         else
            db().modify<subscription_object>(*subscription, [&](subscription_object &so) {
               so.expiration += op.duration * 24 * 3600; // seconds
            });
      } else {
         db().create<subscription_object>([&](subscription_object &so) {
            so.from = op.from;
            so.to = op.to;
            so.expiration =
                    db().head_block_time() + period_count * to_account->subscription_period * 24 * 3600; // seconds
         });
      }

      return void_result();
   } FC_CAPTURE_AND_RETHROW( (op) )
}

void_result allow_subscription_evaluator::do_evaluate( const allow_subscription_operation& op )
{
   try {
      FC_ASSERT(op.duration > 0);
      FC_ASSERT(op.price.amount >= 0);
      auto &idx = db().get_index_type<account_index>().indices().get<by_id>();
      const auto &account = idx.find(op.account);
      FC_ASSERT(account != idx.end(), "account does not exist");

      return void_result();
   } FC_CAPTURE_AND_RETHROW( (op) )
}

void_result allow_subscription_evaluator::do_apply( const allow_subscription_operation& op )
{
   try {
      auto &idx = db().get_index_type<account_index>().indices().get<by_id>();
      const auto &account = idx.find(op.account);

      db().modify<account_object>(*account, [&](account_object &ao) {
         ao.allow_subscription = op.allow_subscription;
         ao.subscription_period = op.duration;
         ao.price_per_subscribe = op.price;
      });

      return void_result();
   } FC_CAPTURE_AND_RETHROW( (op) )
}

} } // graphene::chain