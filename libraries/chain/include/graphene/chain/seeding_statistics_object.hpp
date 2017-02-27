#pragma once
#include <graphene/chain/protocol/types.hpp>
#include <graphene/db/object.hpp>
#include <graphene/db/generic_index.hpp>

#include <fc/reflect/reflect.hpp>

#include <stdint.h>

namespace graphene { namespace chain {

      class seeding_statistics_object : public graphene::db::abstract_object<seeding_statistics_object>
      {
      public:
         static const uint8_t space_id = implementation_ids;
         static const uint8_t type_id  = impl_publisher_object_type;

         account_id_type seeder;
         share_type total_upload;
      };

      struct by_seeder;
      struct by_rating;

      typedef multi_index_container<
         seeding_statistics_object,
         indexed_by<
            ordered_unique< tag<by_id>,
               member< object, object_id_type, &object::id >
            >,
            ordered_unique< tag<by_seeder>,
               member<seeding_statistics_object, account_id_type, &seeding_statistics_object::seeder>,
               std::greater<uint64_t>
            >
         >
      >seeding_statistics_object_multi_index_type;

      typedef generic_index< seeding_statistics_object, seeding_statistics_object_multi_index_type > seeding_statistics_index;

   }}

FC_REFLECT_DERIVED(graphene::chain::seeding_statistics_object,
                   (graphene::db::object),
                   (seeder)(total_upload) )
