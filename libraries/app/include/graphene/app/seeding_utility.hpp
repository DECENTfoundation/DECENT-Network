/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
#pragma once

#include <fc/thread/future.hpp>

#include <graphene/chain/protocol/types.hpp>

namespace decent { namespace seeding {

   struct seeding_plugin_startup_options
   {
      graphene::chain::account_id_type seeder;
      decent::encrypt::DInteger content_private_key;
      graphene::chain::private_key_type seeder_private_key;
      uint64_t free_space;
      std::string seeding_price;
      std::string seeding_symbol;
      fc::path packages_path;
      std::string region_code;
   };

   extern fc::promise<seeding_plugin_startup_options>::ptr seeding_promise;
}
}

FC_REFLECT(decent::seeding::seeding_plugin_startup_options,
           (seeder)(content_private_key)(seeder_private_key)(free_space)(seeding_price)(packages_path)(region_code))
