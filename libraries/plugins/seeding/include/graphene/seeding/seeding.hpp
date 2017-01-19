#pragma once

#include <graphene/app/plugin.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/db/object.hpp>
#include <graphene/db/generic_index.hpp>
#include <graphene/chain/protocol/types.hpp>
#include <graphene/package/package.hpp>

namespace graphene { namespace seeding {

using namespace chain;
using namespace graphene::package;

#ifndef SEEDING_PLUGIN_SPACE_ID
#define SEEDING_PLUGIN_SPACE_ID 123// ??? rrr
#endif

enum seeding_object_type
{
   seeder_object_type = 0,
   seeding_object_type = 1
};

class seeding_plugin;

class my_seeder_object : public graphene::db::abstract_object< my_seeder_object >
{
public:
   static const uint8_t space_id = SEEDING_PLUGIN_SPACE_ID;
   static const uint8_t type_id  = seeder_object_type;

   account_id_type seeder;
   d_integer content_privKey;
   fc::ecc::private_key privKey;
   uint64_t free_space;
   uint32_t price;
};

class my_seeding_object : public graphene::db::abstract_object< my_seeding_object >
{
public:
   static const uint8_t space_id = SEEDING_PLUGIN_SPACE_ID;
   static const uint8_t type_id  = seeding_object_type;

   string URI;
   fc::time_point_sec expiration;
   decent::crypto::custody_data cd;
   account_id_type seeder;
   ciphertext key;
   uint32_t space;
};

typedef graphene::chain::object_id< SEEDING_PLUGIN_SPACE_ID, seeding_object_type,  my_seeding_object>     my_seeding_id_type;
typedef graphene::chain::object_id< SEEDING_PLUGIN_SPACE_ID, seeding_object_type,  my_seeder_object>     my_seeder_id_type;


namespace detail {
class seeding_plugin_impl : public package_transfer_interface::transfer_listener {
public:
   seeding_plugin_impl(seeding_plugin &_plugin) : _self(_plugin) {}

   ~seeding_plugin_impl();

   virtual void on_download_finished(package_transfer_interface::transfer_id id, package_object downloaded_package){
      my_seeding_id_type so_id = active_downloads[id];
      active_downloads.erase(id);
      generate_por( so_id, downloaded_package   );
   };

   graphene::chain::database &database();
   void generate_por( my_seeding_id_type so_id, graphene::package::package_object downloaded_package );
   void on_operation(const operation_history_object &op_obj);

   virtual void on_download_started(package_transfer_interface::transfer_id id) {}
   virtual void on_download_progress(package_transfer_interface::transfer_id id, package_transfer_interface::transfer_progress progress) {}
   virtual void on_upload_started(package_transfer_interface::transfer_id id, const std::string& url) {}
   virtual void on_upload_finished(package_transfer_interface::transfer_id id) {}
   virtual void on_upload_progress(package_transfer_interface::transfer_id id, package_transfer_interface::transfer_progress progress) {}

   void send_ready_to_publish();
   seeding_plugin& _self;
   std::map<package_transfer_interface::transfer_id, my_seeding_id_type> active_downloads;
};
}



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
      ~seeding_plugin(){};

      virtual std::string plugin_name()const override;
      virtual void plugin_set_program_options(
              boost::program_options::options_description& cli,
              boost::program_options::options_description& cfg) override;
      void plugin_initialize(const boost::program_options::variables_map& options) override;
      void plugin_startup() override;


      friend class detail::seeding_plugin_impl;
      std::unique_ptr<detail::seeding_plugin_impl> my;
};

}}

FC_REFLECT_DERIVED( graphene::seeding::my_seeder_object, (graphene::db::object), (seeder)(content_privKey)(privKey)(free_space) );
FC_REFLECT_DERIVED( graphene::seeding::my_seeding_object, (graphene::db::object), (URI)(expiration)(seeder)(key)(space) );

