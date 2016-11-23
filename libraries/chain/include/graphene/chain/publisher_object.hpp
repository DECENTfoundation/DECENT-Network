#pragma once
#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/protocol/asset.hpp>
#include <graphene/db/object.hpp>

#include <stdint.h>

namespace graphene { namespace chain {
   
   class publisher_object : public graphene::db::abstract_object<publisher_object>
   {
      account_id_type publisher;
      uint64_t free_space;
      asset price;
   };
}}
