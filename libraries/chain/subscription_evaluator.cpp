/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
#include <graphene/chain/database.hpp>
#include <graphene/chain/subscription_evaluator.hpp>
#include <graphene/chain/subscription_object.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/transaction_detail_object.hpp>

namespace graphene { namespace chain {

void_result subscribe_evaluator::do_evaluate( const subscribe_operation& op )
{
   try {
      auto& idx = db().get_index_type<account_index>().indices().get<by_id>();
      const auto& from_account = idx.find( op.from );
      FC_ASSERT( from_account != idx.end() , "Account does not exist" );
      const auto& to_account = idx.find( op.to );
      FC_ASSERT( to_account != idx.end() , "Author does not exist" );
      FC_ASSERT( op.price <= db().get_balance( op.from, op.price.asset_id ) );
      FC_ASSERT( to_account->options.allow_subscription , "Author does not allow subscription" );

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

      FC_ASSERT( op.price >= dct_price );

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
      uint32_t subscription_period_in_secs = to_account->options.subscription_period * 24 * 3600;
      time_point_sec now = db().head_block_time();

      if (subscription != idx2.end())
      {
         db().modify<subscription_object>(*subscription, [&subscription_period_in_secs, &now](subscription_object &so)
         {
            if (so.expiration < now)
               so.expiration = now + subscription_period_in_secs;
            else
               so.expiration += subscription_period_in_secs;
         });
      }
      else
      {
         db().create<subscription_object>([&subscription_period_in_secs, &now, &op](subscription_object &so)
         {
            so.from = op.from;
            so.to = op.to;
            so.expiration = now + subscription_period_in_secs;
            so.automatic_renewal = false;
         });
      }

      db().adjust_balance( op.from, -op.price );
      db().adjust_balance( op.to, op.price );

      database& d = db();
      db().create<transaction_detail_object>([&d, &op, &to_account](transaction_detail_object& obj)
                                             {
                                                obj.m_operation_type = (uint8_t)transaction_detail_object::subscription;
                                                obj.m_from_account = op.from;
                                                obj.m_to_account = op.to;
                                                obj.m_transaction_amount = op.price;
                                                obj.m_transaction_fee = op.fee;
                                                string subscription_period = std::to_string(to_account->options.subscription_period) + " day";
                                                if( to_account->options.subscription_period > 1 )
                                                   subscription_period += "s";
                                                obj.m_str_description = subscription_period;
                                                obj.m_timestamp = d.head_block_time();
                                             });

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
      uint32_t subscription_period_in_secs = to_account->options.subscription_period * 24 * 3600;
      time_point_sec now = db().head_block_time();

      if (subscription != idx2.end())
      {
         db().modify<subscription_object>(*subscription, [&subscription_period_in_secs, &now](subscription_object &so)
         {
            if (so.expiration < now)
               so.expiration = now + subscription_period_in_secs;
            else
               so.expiration += subscription_period_in_secs;
         });
      }
      else
      {
         db().create<subscription_object>([&subscription_period_in_secs, &now, &op](subscription_object &so)
         {
            so.from = op.from;
            so.to = op.to;
            so.expiration = now + subscription_period_in_secs;
            so.automatic_renewal = false;
         });
      }

      database& d = db();
      db().create<transaction_detail_object>([&d, &op, &to_account](transaction_detail_object& obj)
                                             {
                                                obj.m_operation_type = (uint8_t)transaction_detail_object::subscription;
                                                obj.m_from_account = op.from;
                                                obj.m_to_account = op.to;
                                                obj.m_transaction_amount = asset();
                                                obj.m_transaction_fee = op.fee;
                                                string subscription_period = std::to_string(to_account->options.subscription_period) + " day";
                                                if( to_account->options.subscription_period > 1 )
                                                   subscription_period += "s";
                                                obj.m_str_description = subscription_period;
                                                obj.m_timestamp = d.head_block_time();
                                             });

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
