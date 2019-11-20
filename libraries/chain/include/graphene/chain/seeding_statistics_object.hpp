/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
#pragma once
#include <graphene/chain/protocol/types.hpp>
#include <graphene/db/object.hpp>
#include <graphene/db/generic_index.hpp>

#include <fc/reflect/reflect.hpp>

#include <stdint.h>

namespace graphene { namespace chain {

      class seeding_statistics_object : public graphene::db::abstract_object<implementation_ids, impl_seeding_statistics_object_type, seeding_statistics_object>
      {
      public:
         account_id_type seeder;
         // total bytes uploaded to consumers through ipfs
         uint64_t total_upload = 0;
         // increased with every request_to_buy op and decreased with every deliver_key_op, reset at maint to 0
         uint32_t missed_delivered_keys = 0;
         // increased with every deliver_key_op, reset at maint to 0
         uint32_t total_delivered_keys = 0;
         // 1.000.000 = 1 MB, increased with every initial PoR operation by the size of content / no. of seeders,
         // decreased inside maintenance for every expired content
         uint64_t total_content_seeded = 0;
         // increased with every initial PoR operation, decreased inside maint for every expired content
         uint32_t num_of_content_seeded = 0;
         // increased with every content_submit operation, decreased with every initial PoR
         uint32_t total_content_requested_to_seed = 0;
         // not initial ones, reset at every maint to 0
         uint32_t num_of_pors = 0;
         // total_upload at maint
         uint64_t uploaded_till_last_maint = 0;
         // 30%*(total_delivery_keys + missed_delivered_keys) / number_of_content + 70%*avg_buying_ratio
         uint32_t avg_buying_ratio = 0;
         // 30%* upload_to_data_recent / avg_buying_ratio + 70%*seeding_ratio
         uint32_t seeding_rel_ratio = 0;
         // 30% * upload_to_data_recent + 70% * seeding_abs_ratio
         uint32_t seeding_abs_ratio = 0;
         // 30% ( content_requested_to_seed_in_given_MT / number_of_content ) + 70%*missed_ratio
         uint32_t missed_ratio = 0;
      };

      struct by_seeder;
      struct by_upload;

      typedef boost::multi_index_container<
         seeding_statistics_object,
         db::mi::indexed_by<
            db::object_id_index,
            db::mi::ordered_unique<db::mi::tag<by_seeder>,
               db::mi::member<seeding_statistics_object, account_id_type, &seeding_statistics_object::seeder>
            >,
            db::mi::ordered_non_unique<db::mi::tag<by_upload>,
               db::mi::member<seeding_statistics_object, uint64_t , &seeding_statistics_object::total_upload>, std::greater<uint64_t>
            >
         >
      >seeding_statistics_object_multi_index_type;

      typedef graphene::db::generic_index< seeding_statistics_object, seeding_statistics_object_multi_index_type > seeding_statistics_index;

   }}

FC_REFLECT_DERIVED(graphene::chain::seeding_statistics_object,
                   (graphene::db::object),
                   (seeder)(total_upload)(missed_delivered_keys)(total_delivered_keys)(total_content_seeded)
                   (num_of_content_seeded)(total_content_requested_to_seed)(num_of_pors)(uploaded_till_last_maint)
                   (avg_buying_ratio)(seeding_rel_ratio)(seeding_abs_ratio)(missed_ratio))
