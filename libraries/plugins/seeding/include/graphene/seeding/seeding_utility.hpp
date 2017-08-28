/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
#pragma once
#include <fc/thread/future.hpp>
#include <graphene/chain/protocol/types.hpp>

namespace decent { namespace seeding {


//using namespace graphene::chain;

   struct seeding_plugin_startup_options
   {
      graphene::chain::account_id_type seeder;
      decent::encrypt::DInteger content_private_key;
      fc::ecc::private_key seeder_private_key;
      uint64_t free_space;
      uint32_t seeding_price;
      fc::path packages_path;
      std::string seeding_symbol;

   };

   extern fc::promise<seeding_plugin_startup_options>::ptr seeding_promise;
}
}

FC_REFLECT(decent::seeding::seeding_plugin_startup_options,
           (seeder)(content_private_key)(seeder_private_key)(free_space)(seeding_price)(packages_path))
