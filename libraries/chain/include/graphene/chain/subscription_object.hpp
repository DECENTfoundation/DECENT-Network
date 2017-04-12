#pragma once

#include <graphene/chain/protocol/types.hpp>
#include <graphene/db/object.hpp>
#include <graphene/db/generic_index.hpp>

#include <fc/reflect/reflect.hpp>
#include <fc/time.hpp>

#include <boost/multi_index/composite_key.hpp>

namespace graphene { namespace chain {

   /// Tracks subscription rom consumer to author
   class subscription_object : public graphene::db::abstract_object< subscription_object >
   {
   public:
      static const uint8_t space_id = implementation_ids;
      static const uint8_t type_id  = impl_subscription_object_type;

      account_id_type from;
      account_id_type to;
      time_point_sec expiration;
      bool automatic_renewal;
   };

   struct by_from;
   struct by_to;
   struct by_expiration;
   struct by_from_to;
   struct by_from_expiration;
   struct by_to_expiration;
   struct by_renewal;

   typedef multi_index_container<
      subscription_object,
         indexed_by<
            ordered_unique< tag< by_id>,
               member< object, object_id_type, &object::id >
            >,
            ordered_non_unique< tag< by_from>,
               member<subscription_object, account_id_type, &subscription_object::from>
            >,
            ordered_non_unique< tag< by_to>,
               member<subscription_object, account_id_type, &subscription_object::to>
            >,
            ordered_non_unique< tag< by_expiration>,
               member<subscription_object, time_point_sec, &subscription_object::expiration>, std::greater< time_point_sec >
            >,
            ordered_unique< tag< by_from_to>,
               composite_key< subscription_object,
                  member<subscription_object, account_id_type, &subscription_object::from>,
                  member<subscription_object, account_id_type, &subscription_object::to>
               >
            >,
            ordered_unique< tag< by_from_expiration>,
               composite_key< subscription_object,
                  member<subscription_object, account_id_type, &subscription_object::from>,
                  member<subscription_object, time_point_sec, &subscription_object::expiration>
               >,
               composite_key_compare<
                  std::less< account_id_type >,
                  std::greater< time_point_sec >
               >
            >,
            ordered_unique< tag< by_to_expiration>,
               composite_key< subscription_object,
                  member<subscription_object, account_id_type, &subscription_object::from>,
                  member<subscription_object, time_point_sec, &subscription_object::expiration>
               >,
               composite_key_compare<
                  std::less< account_id_type >,
                  std::greater< time_point_sec >
                  >
            >,
            ordered_non_unique< tag< by_renewal>,
               member<subscription_object, bool, &subscription_object::automatic_renewal>, std::greater<bool>
            >
         >
   >subscription_object_multi_index_type;

typedef generic_index< subscription_object, subscription_object_multi_index_type > subscription_index;

} } // graphene::chain

FC_REFLECT_DERIVED(graphene::chain::subscription_object, (graphene::db::object), (from)(to)(expiration)(automatic_renewal) )