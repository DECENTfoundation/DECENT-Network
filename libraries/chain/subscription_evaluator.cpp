#include <graphene/chain/database.hpp>
#include <graphene/chain/subscription_evaluator.hpp>
#include <graphene/chain/subscription_object.hpp>
#include <graphene/chain/account_object.hpp>

namespace graphene { namespace chain {

void_result subscribe_evaluator::do_evaluate( const subscribe_operation& op )
{
   try {
      auto& idx = db().get_index_type<account_index>().indices().get<by_id>();
      const auto& from_account = idx.find( op.from );
      FC_ASSERT( from_account != idx.end() , "Account does not exist" );
      const auto& to_account = idx.find( op.to );
      FC_ASSERT( to_account != idx.end() , "Author does not exist" );

      FC_ASSERT( to_account->options.allow_subscription , "Author does not allow subscription" );
      period_count = op.duration / to_account->options.subscription_period;
      FC_ASSERT( period_count >= 1 );

      auto price = to_account->options.price_per_subscribe;
      asset dct_price;
      auto ao = db().get( price.asset_id );
      FC_ASSERT( price.asset_id == asset_id_type(0) || ao.is_monitored_asset() );

      //if the price is in fiat, calculate price in DCT with current exchange rate...
      if( ao.is_monitored_asset() ){
         auto rate = ao.monitored_asset_opts->current_feed.core_exchange_rate;
         FC_ASSERT(!rate.is_null(), "No price feed for this asset");
         dct_price = price * rate;
      }else{
         dct_price = price;
      }

      FC_ASSERT( op.price >= period_count * dct_price );

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

      // head_block_time rounded up to midnight
      uint32_t head_block_time_rounded_to_days = db().head_block_time().sec_since_epoch() / ( 24 * 3600 );
      head_block_time_rounded_to_days += 24 * 3600;

      if (subscription != idx2.end())
      {
         if (subscription->expiration < db().head_block_time())
            db().modify<subscription_object>(*subscription, [&](subscription_object &so)
            {
               so.expiration = time_point_sec( head_block_time_rounded_to_days ) + period_count * to_account->options.subscription_period * 24 * 3600; // seconds
            });
         else
            db().modify<subscription_object>(*subscription, [&](subscription_object &so)
            {
               so.expiration += period_count * to_account->options.subscription_period * 24 * 3600; // seconds
            });
      }
      else
      {
         db().create<subscription_object>([&](subscription_object &so)
         {
            so.from = op.from;
            so.to = op.to;
            so.expiration = time_point_sec( head_block_time_rounded_to_days ) + period_count * to_account->options.subscription_period * 24 * 3600; // seconds
            so.automatic_renewal = false;
         });
      }

      return void_result();
   } FC_CAPTURE_AND_RETHROW( (op) )
}

void_result subscribe_by_author_evaluator::do_evaluate( const subscribe_by_author_operation& op )
{
   try {
      auto &idx = db().get_index_type<account_index>().indices().get<by_id>();
      const auto &from_account = idx.find(op.from);
      FC_ASSERT(from_account != idx.end(), "Account does not exist");
      const auto &to_account = idx.find(op.to);
      FC_ASSERT(to_account != idx.end(), "Author does not exist");

      FC_ASSERT(to_account->options.allow_subscription, "Author does not allow subscription");
      period_count = op.duration / to_account->options.subscription_period;
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

      // head_block_time rounded up to midnight
      uint32_t head_block_time_rounded_to_days = db().head_block_time().sec_since_epoch() / ( 24 * 3600 );
      head_block_time_rounded_to_days += 24 * 3600;

      if (subscription != idx2.end())
      {
         if (subscription->expiration < db().head_block_time())
            db().modify<subscription_object>(*subscription, [&](subscription_object &so)
            {
               so.expiration = time_point_sec( head_block_time_rounded_to_days ) + period_count * to_account->options.subscription_period * 24 * 3600; // seconds
            });
         else
            db().modify<subscription_object>(*subscription, [&](subscription_object &so)
            {
               so.expiration += period_count * to_account->options.subscription_period * 24 * 3600; // seconds
            });
      }
      else
      {
         db().create<subscription_object>([&](subscription_object &so)
         {
            so.from = op.from;
            so.to = op.to;
            so.expiration = time_point_sec( head_block_time_rounded_to_days ) + period_count * to_account->options.subscription_period * 24 * 3600; // seconds
            so.automatic_renewal = false;
         });
      }

      return void_result();
   } FC_CAPTURE_AND_RETHROW( (op) )
}

void_result automatic_renewal_of_subscription_evaluator::do_evaluate(const automatic_renewal_of_subscription_operation& op )
{
   try {
      auto &idx = db().get_index_type<account_index>().indices().get<by_id>();
      const auto &from_account = idx.find(op.consumer);
      FC_ASSERT(from_account != idx.end(), "Account does not exist");

      auto &idx2 = db().get_index_type<subscription_index>().indices().get<by_id>();
      const auto &subscription = idx2.find(op.subscription);
      FC_ASSERT(subscription != idx2.end(), "subscription does not exist");

      FC_ASSERT(subscription->from == op.consumer, "Subscriber and subscription object doesn't match");
      FC_ASSERT(subscription->automatic_renewal != op.automatic_renewal, "Automatic renewal option is already set to ${bool_value}", ("bool_value", op.automatic_renewal));

      const auto &to_account = idx.find(subscription->to);
      FC_ASSERT(to_account != idx.end(), "Account does not exist");

      FC_ASSERT( to_account->options.price_per_subscribe <= db().get_balance( op.consumer, to_account->options.price_per_subscribe.asset_id ), "Insufficient account balance" );

      return void_result();
   } FC_CAPTURE_AND_RETHROW( (op) )
}

void_result automatic_renewal_of_subscription_evaluator::do_apply(const automatic_renewal_of_subscription_operation& op )
{
   try {
      auto &idx2 = db().get_index_type<subscription_index>().indices().get<by_id>();
      const auto &subscription = idx2.find(op.subscription);

      db().modify<subscription_object>(*subscription, [&](subscription_object &so)
      {
         so.automatic_renewal = op.automatic_renewal;
      });

      return void_result();
   } FC_CAPTURE_AND_RETHROW( (op) )
}

void_result disallow_automatic_renewal_of_subscription_evaluator::do_evaluate(const disallow_automatic_renewal_of_subscription_operation& op )
{
   void_result result;
   return result;
}

void_result disallow_automatic_renewal_of_subscription_evaluator::do_apply(const disallow_automatic_renewal_of_subscription_operation& op )
{
   void_result result;
   return result;
}

void_result renewal_of_subscription_evaluator::do_evaluate(const renewal_of_subscription_operation& op )
{
   void_result result;
   return result;
}

void_result renewal_of_subscription_evaluator::do_apply(const renewal_of_subscription_operation& op )
{
   void_result result;
   return result;
}

} } // graphene::chain