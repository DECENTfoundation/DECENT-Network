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
   seeder_object_type = 0,
   seeding_object_type = 1
};

namespace detail {
   class seeding_plugin_impl;
}

class my_seeder_object : public graphene::db::abstract_object< my_seeder_object >
{
   public:
      static const uint8_t space_id = SEEDING_PLUGIN_SPACE_ID;
      static const uint8_t type_id  = seeder_object_type;

      account_id_type seeder;
      d_integer content_privKey;
      fc::ecc::private_key privKey;
      uint32_t free_space;
};

class my_seeding_object : public graphene::db::abstract_object< my_seeding_object >
{
   public:
      static const uint8_t space_id = SEEDING_PLUGIN_SPACE_ID;
      static const uint8_t type_id  = seeding_object_type;

      string URI;
      fc::time_point_sec expiration;
      account_id_type seeder;
      ciphertext key;
      uint32_t space;
};

struct by_id;
struct by_URI;
struct by_seeder;

typedef multi_index_container<
   my_seeder_object,
   indexed_by<
      ordered_unique< tag<by_id>, member< object, object_id_type, &object::id >>,
      ordered_unique< tag< by_seeder >, member< my_seeder_object, account_id_type, &my_seeder_object::seeder> >
   >
>my_seeder_object_multi_index_type;

typedef generic_index< my_seeder_object, my_seeder_object_multi_index_type > my_seeder_index;

typedef multi_index_container<
   my_seeding_object,
   indexed_by<
      ordered_unique< tag<by_id>, member< object, object_id_type, &object::id >>,
      ordered_unique< tag< by_URI >, member< my_seeding_object, string, &my_seeding_object::URI> >  
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

FC_REFLECT_DERIVED( graphene::seeding::my_seeder_object, (graphene::db::object), (seeder)(content_privKey)(privKey)(free_space) );
FC_REFLECT_DERIVED( graphene::seeding::my_seeding_object, (graphene::db::object), (URI)(expiration)(seeder)(key)(space) );

