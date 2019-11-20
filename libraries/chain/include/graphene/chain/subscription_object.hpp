/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
#pragma once

#include <graphene/chain/protocol/types.hpp>
#include <graphene/db/object.hpp>
#include <graphene/db/generic_index.hpp>

#include <fc/reflect/reflect.hpp>
#include <fc/time.hpp>

#include <boost/multi_index/composite_key.hpp>

namespace graphene { namespace chain {

   /// Tracks subscription rom consumer to author
   class subscription_object : public graphene::db::abstract_object<implementation_ids, impl_subscription_object_type, subscription_object>
   {
   public:
      account_id_type from;
      account_id_type to;
      fc::time_point_sec expiration;
      bool automatic_renewal;
   };

   struct by_from;
   struct by_to;
   struct by_expiration;
   struct by_from_to;
   struct by_from_expiration;
   struct by_to_expiration;
   struct by_renewal;
   struct by_to_renewal;

   typedef boost::multi_index_container<
      subscription_object,
      db::mi::indexed_by<
         db::object_id_index,
         db::mi::ordered_non_unique<db::mi::tag<by_from>,
            db::mi::member<subscription_object, account_id_type, &subscription_object::from>
         >,
         db::mi::ordered_non_unique<db::mi::tag<by_to>,
            db::mi::member<subscription_object, account_id_type, &subscription_object::to>
         >,
         db::mi::ordered_non_unique<db::mi::tag<by_expiration>,
            db::mi::member<subscription_object, fc::time_point_sec, &subscription_object::expiration>, std::greater<fc::time_point_sec>
         >,
         db::mi::ordered_unique<db::mi::tag<by_from_to>,
            db::mi::composite_key<subscription_object,
               db::mi::member<subscription_object, account_id_type, &subscription_object::from>,
               db::mi::member<subscription_object, account_id_type, &subscription_object::to>
            >
         >,
         db::mi::ordered_non_unique<db::mi::tag<by_from_expiration>,
            db::mi::composite_key<subscription_object,
               db::mi::member<subscription_object, account_id_type, &subscription_object::from>,
               db::mi::member<subscription_object, fc::time_point_sec, &subscription_object::expiration>
            >,
            db::mi::composite_key_compare<
               std::less<account_id_type>,
               std::greater<fc::time_point_sec>
            >
         >,
         db::mi::ordered_non_unique<db::mi::tag<by_to_expiration>,
            db::mi::composite_key<subscription_object,
               db::mi::member<subscription_object, account_id_type, &subscription_object::to>,
               db::mi::member<subscription_object, fc::time_point_sec, &subscription_object::expiration>
            >,
            db::mi::composite_key_compare<
               std::less<account_id_type>,
                 std::greater<fc::time_point_sec>
               >
         >,
         db::mi::ordered_non_unique<db::mi::tag<by_renewal>,
            db::mi::member<subscription_object, bool, &subscription_object::automatic_renewal>, std::greater<bool>
         >,
         db::mi::ordered_non_unique<db::mi::tag<by_to_renewal>,
            db::mi::composite_key<subscription_object,
               db::mi::member<subscription_object, account_id_type, &subscription_object::to>,
               db::mi::member<subscription_object, bool, &subscription_object::automatic_renewal>
            >
         >
      >
   >subscription_object_multi_index_type;

typedef graphene::db::generic_index< subscription_object, subscription_object_multi_index_type > subscription_index;

} } // graphene::chain

FC_REFLECT_DERIVED(graphene::chain::subscription_object, (graphene::db::object), (from)(to)(expiration)(automatic_renewal) )
