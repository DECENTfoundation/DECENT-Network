#pragma once
#include <graphene/chain/protocol/types.hpp>
#include <graphene/db/object.hpp>

#include <fc/string.hpp>
#include <fc/time.hpp>


namespace graphene { namespace chain {
   
   class buying_object : public graphene::db::abstract_object<buying_object>
   {
      
      static const uint8_t space_id = implementation_ids;
      static const uint8_t type_id  = impl_buying_object_type;
      
      account_id_type consumer;
      string URI;
      vector<account_id_type> seeders_answered;
      time_point_sec expiration_time;
   };
   
}}
