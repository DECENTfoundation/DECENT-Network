#pragma once

#include <graphene/app/plugin.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/db/object.hpp>
#include <graphene/db/generic_index.hpp>
#include <graphene/chain/protocol/types.hpp>
#include <graphene/package/package.hpp>

namespace decent { namespace seeding {

using namespace graphene::chain;

#ifndef SEEDING_PLUGIN_SPACE_ID
#define SEEDING_PLUGIN_SPACE_ID 123
#endif

enum seeding_object_type
{
   seeder_object_type = 0,
   seeding_object_type = 1
};

class seeding_plugin;

/**
 * @Class my_seeder_object - Defines seeders managed by this plugin. ATM only one seeder is supported.
 */
class my_seeder_object : public graphene::db::abstract_object< my_seeder_object >
{
public:
   static const uint8_t space_id = SEEDING_PLUGIN_SPACE_ID;
   static const uint8_t type_id  = seeder_object_type;

   account_id_type seeder;
   decent::encrypt::DIntegerString content_privKey;

   fc::ecc::private_key privKey;
   uint64_t free_space;
   uint32_t price;
};

/**
 * @class my_seeding_object - Defines content pieces seeded by this plugin.
 */
class my_seeding_object : public graphene::db::abstract_object< my_seeding_object >
{
public:
   static const uint8_t space_id = SEEDING_PLUGIN_SPACE_ID;
   static const uint8_t type_id  = seeding_object_type;

   string URI; //<Content address
   fc::ripemd160 _hash; //<Content hash
   fc::time_point_sec expiration; //<Content expiration
   decent::encrypt::CustodyData cd; //<Content custody data
   account_id_type seeder; //<Seeder seeding this content managed by this plugin
   decent::encrypt::CiphertextString key; //<Decryption key part

   uint32_t space;
};

typedef graphene::chain::object_id< SEEDING_PLUGIN_SPACE_ID, seeding_object_type,  my_seeding_object>     my_seeding_id_type;
typedef graphene::chain::object_id< SEEDING_PLUGIN_SPACE_ID, seeding_object_type,  my_seeder_object>     my_seeder_id_type;



namespace detail {

class SeedingListener;



/**
 * @class seeding_plugin_impl This class implements the seeder functionality.
 * @inherits package_transfer_interface::transfer_listener Integrates with package manager through this interface.
 */
class seeding_plugin_impl /*: public package_transfer_interface::transfer_listener */{
public:
   seeding_plugin_impl(seeding_plugin &_plugin) : _self(_plugin) { }

   ~seeding_plugin_impl();

   /**
    * Called when package manager finishes download of the content. It is called from package manager thread.
    * @param id Download ID, used to map content
    * @param downloaded_package Downloaded package object
    */
/*   virtual void on_download_finished(package_transfer_interface::transfer_id id, package_object downloaded_package){
      ilog("seeding plugin: on_download_finished(): Finished downloading package");
      my_seeding_id_type so_id = active_downloads[id];
      active_downloads.erase(id);
      const auto& so = database().get<my_seeding_object>(so_id);
      ilog("seeding plugin: on_download_finished(): Package URL ${u}",("u",so.URI));

      size_t size = (downloaded_package.get_size() + 1024*1024 - 1) / (1024 * 1024);
      if ( size > so.space ) {
         ilog("seeding plugin: on_download_finished(): Fraud detected: real content size is greater than propagated in blockchain; deleting...");
         package_manager::instance().delete_package(downloaded_package.get_hash());
         //changing DB outside the main thread does not work properly, let's delete it from there
         main_thread->async([so_id, this](){ const auto& so = database().get<my_seeding_object>(so_id); database().remove(so); });
         return;
      }
      //Don't block package manager thread for too long.
      service_thread->async([this,so_id, downloaded_package](){generate_por( so_id, downloaded_package );});
      //hack to deal with ipfs until the package manager is finished
      fc::url download_url(package_manager::instance().get_transfer_url(id));
      if(download_url.proto() == "ipfs")
         package_manager::instance().upload_package(downloaded_package, "ipfs", *this);
   };*/

   /**
    * Get DB instance
    * @return DB instance
    */
   graphene::chain::database &database();
   /**
    * Generates proof of retrievability of a package
    * @param so_id ID of the my_seeding_object
    * @param downloaded_package Downloaded package object
    */
   void generate_por( my_seeding_id_type so_id, graphene::package::package_object downloaded_package );

   /**
    * Generates proof of retrievability of a package
    * @param so_id ID of the my_seeding_object
    * @param downloaded_package Downloaded package object
    */
   void generate_por2( const my_seeding_object& so, decent::package::package_handle_t package_handle );

   /**
    * Handle newly submitted content. If it is content managed by one of our seeders, download it.
    * @param op_obj The operation wrapper carrying content submit operation
    */
   void handle_content_submit(const operation_history_object &op_obj);

   /**
    * Handle request to buy. If it is concerning one of content seeded by the plugin, provide decryption key parts in deliver key
    * @param op_obj The operation wrapper carrying content request to buy operation
    */
   void handle_request_to_buy(const operation_history_object &op_obj);

   /**
    * Called only after the highest known block has been applied. If it is request to buy or content submit, pass it to the corresponding handler
    * @param op_obj The operation wrapper
    * @param sync_mode
    */
   void handle_commited_operation(const operation_history_object &op_obj, bool sync_mode);

/*   virtual void on_download_started(package_transfer_interface::transfer_id id) {}
   virtual void on_download_progress(package_transfer_interface::transfer_id id, package_transfer_interface::transfer_progress progress) {}
   virtual void on_upload_started(package_transfer_interface::transfer_id id, const std::string& url) {}
   virtual void on_upload_finished(package_transfer_interface::transfer_id id) {}
   virtual void on_upload_progress(package_transfer_interface::transfer_id id, package_transfer_interface::transfer_progress progress) {}
   virtual void on_error(package::package_transfer_interface::transfer_id, std::string error){} */

   /**
    * Restarts all downloads and seeding upon application start
    */
   void restart_downloads();

   /**
    * Generates and broadcasts RtP operation
    */
   void send_ready_to_publish();

   std::vector<SeedingListener> listeners;
   seeding_plugin& _self;
//   std::map<package_transfer_interface::transfer_id, my_seeding_id_type> active_downloads; //<List of active downloads for whose we are expecting on_download_finished callback to be called
   std::shared_ptr<fc::thread> service_thread; //The thread where the computation shall happen
   fc::thread* main_thread; //The main thread, used mainly for DB modifications
};

class SeedingListener : public decent::package::EventListenerInterface, public std::enable_shared_from_this<SeedingListener>{
private:
   const my_seeding_object & _mso;
   decent::package::package_handle_t _pi;
   seeding_plugin_impl * _my;
public:
   SeedingListener(seeding_plugin_impl& impl, const my_seeding_object & mso, const decent::package::package_handle_t pi):_mso(mso),_pi(pi){ _my = &impl;};
   ~SeedingListener(){};

   virtual void package_download_error(const std::string&){
      //In case the download fails, delete the package and seeding objects - TODO_DECENT
      //_my->database().remove(mso);
      elog("seeding plugin: package_download_error(): Failed downloading package ${s}",("s",_mso.URI));
      auto& pm = decent::package::PackageManager::instance();
      pm.release_package(_pi);
   };

   virtual void package_download_complete(){
      ilog("seeding plugin: package_download_complete(): Finished downloading package${u}",("u",_mso.URI));
      auto& pm = decent::package::PackageManager::instance();

      size_t size = (_pi->get_size() + 1024*1024 - 1) / (1024 * 1024);
      if ( size > _mso.space ) {
         ilog("seeding plugin: package_download_complete(): Fraud detected: real content size is greater than propagated in blockchain; deleting...");
         pm.release_package(_pi);
         _pi = 0;
         //changing DB outside the main thread does not work properly, let's delete it from there
         _my->main_thread->async( [=](){ database().remove(_mso); } );
         return;
      }
      _pi->start_seeding();
      //Don't block package manager thread for too long.
      fc::url download_url( _mso.URI );
      if(download_url.proto() == "ipfs")
         _my->service_thread->async( [=](){ _my->generate_por2( _mso, _pi ); _pi->remove(false); });
      else
         _my->service_thread->async( [=](){ _my->generate_por2( _mso, _pi ); });
   };

};


} //namespace detail



struct by_id;
struct by_URI;
struct by_hash;
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
      ordered_unique< tag< by_URI >, member< my_seeding_object, string, &my_seeding_object::URI> >,
      ordered_unique< tag< by_hash >, member< my_seeding_object, fc::ripemd160, &my_seeding_object::_hash> >
   >
>my_seeding_object_multi_index_type;

typedef generic_index< my_seeding_object, my_seeding_object_multi_index_type > my_seeding_index;


class seeding_plugin : public graphene::app::plugin
{
   public:
      seeding_plugin();
      ~seeding_plugin(){};

      virtual std::string plugin_name()const override;
      /**
       * Extend program options with our option list
       * @param cli CLI parameters
       * @param cfg Config file parameters
       */
      virtual void plugin_set_program_options(
              boost::program_options::options_description& cli,
              boost::program_options::options_description& cfg) override;

      /**
       * Initialize plugin based on config parameters
       * @param options
       */
      void plugin_initialize(const boost::program_options::variables_map& options) override;
      /**
       * Start the plugin and begin work.
       */
      void plugin_startup() override;


      friend class detail::seeding_plugin_impl;
      std::unique_ptr<detail::seeding_plugin_impl> my;
};

}}

FC_REFLECT_DERIVED( decent::seeding::my_seeder_object, (graphene::db::object), (seeder)(content_privKey)(privKey)(free_space) );
FC_REFLECT_DERIVED( decent::seeding::my_seeding_object, (graphene::db::object), (URI)(expiration)(cd)(seeder)(key)(space) );

