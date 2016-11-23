#pragma once
#include <graphene/chain/protocol/types.hpp>
#include <graphene/db/object.hpp>

#include <fc/string.hpp>



namespace graphene { namespace chain {
   
   class rating_object : public graphene::db::abstract_object<rating_object>
   {
      account_id_type consumer;
      string URI;
      uint64_t rating;
      buying_id_type buying;
   };
   
}}
