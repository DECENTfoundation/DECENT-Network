#pragma once
#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/protocol/asset.hpp>
#include <graphene/db/object.hpp>

#include <fc/io/json.hpp>
#include <fc/string.hpp>
#include <fc/crypto/ripemd160.hpp>

#include <stdint.h>
#include <vector>

namespace graphene { namespace chain {
   
   class content_object : public graphene::db::abstract_object<content_object>
   {
      account_id_type author;
      price Price;
      fc::json synopsis;
      string URI;
      vector<account_id_type> seeders;
      fc::ripemd160 hash;
      uint64_t AVG_rating;
      uint32_t total_rating;
      uint32_t times_bought;
      asset publishing_fee_escrow;
   };
   
}}
