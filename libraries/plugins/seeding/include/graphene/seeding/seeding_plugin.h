#pragma once

#include <graphene/app/plugin.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/db/object.hpp>
#include <graphene/db/generic_index.hpp>
#include <graphene/chain/protocol/types.hpp>

namespace graphene { namespace seeding {

using namespace chain;

#ifndef SEEDING_PLUGIN_SPACE_ID
#define SEEDING_PLUGIN_SPACE_ID 123// ??? rrr
#endif

enum seeding_object_type
{
   key_account_object_type = 0,
   bucket_object_type  = 1,
   seeding_object_type = 2
};

namespace detail {
   class seeding_plugin_impl;
}


class my_seeding_object : public abstract_object< my_seeding_object >
{
   public:
      static const uint8_t space_id = SEEDING_PLUGIN_SPACE_ID;
      static const uint8_t type_id  = seeding_object_type;

      string URI;
      account_id_type consumer;

      account_id_type seeder;
      delivery_proof proof;
      ciphertext key;
};

struct by_id;
struct by_URI_consumer;
struct by_consumer_URI;

using namespace boost::multi_index_container;

typedef multi_index_container<
   my_seeding_object,
   indexed_by<
      ordered_unique< tag<by_id>, member< object, object_id_type, &object::id >>,
      ordered_unique< tag< by_URI_consumer>,
         composite_key< rating_object,
            member<my_seeding_object, string, &my_seeding_object::URI>,
            member<my_seeding_object, account_id_type, &my_seeding_object::consumer>
         >
      >,
      ordered_unique< tag< by_consumer_URI>,
         composite_key< rating_object,
            member<my_seeding_object, account_id_type, &my_seeding_object::consumer>,
            member<my_seeding_object, string, &my_seeding_object::URI>
         >
      >
   >
>my_seeding_object_multi_index_type;

typedef generic_index< my_seeding_object, my_seeding_object_multi_index_type > my_seeding_index;


class seeding_plugin : public graphene::app::plugin
{
   public:
      seeding_plugin();
      virtual ~seeding_plugin(){}

      virtual std::string plugin_name()const override;
      virtual void plugin_set_program_options(
              boost::program_options::options_description& cli,
              boost::program_options::options_description& cfg) override;
      virtual void plugin_initialize(const boost::program_options::variables_map& options) override;
      virtual void plugin_startup() override;

      friend class detail::seeding_plugin_impl;
      std::unique_ptr<detail::seeding_plugin_impl> my;
};

}}

FC_REFLECT_DERIVED( graphene::seeding::my_seeding_object, (graphene::db::abstract_object), (URI)(consumer)(seeder)(proof)(key) );
