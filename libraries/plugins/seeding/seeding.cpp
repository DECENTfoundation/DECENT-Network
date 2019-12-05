/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */

#include <graphene/seeding/seeding.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/buying_object.hpp>
#include <graphene/chain/content_object.hpp>
#include <graphene/chain/global_property_object.hpp>
#include <graphene/chain/seeding_object.hpp>
#include <graphene/chain/hardfork.hpp>
#include <decent/encrypt/encryptionutils.hpp>
#include <graphene/utilities/key_conversion.hpp>
#include <decent/package/package_config.hpp>
#include <decent/package/package.hpp>
#include <decent/ipfs_check.hpp>
#include <ipfs/client.h>
#include <fc/thread/thread.hpp>

namespace bpo = boost::program_options;

namespace decent { namespace seeding {
namespace detail {

#define POR_WAKEUP_INTERVAL_SEC 300

struct seeding_plugin_startup_options
{
   graphene::chain::account_id_type seeder;
   decent::encrypt::DInteger content_private_key;
   graphene::chain::private_key_type seeder_private_key;
   uint64_t free_space;
   std::string seeding_price;
   std::string seeding_symbol;
   boost::filesystem::path packages_path;
   std::string region_code;
};

class SeedingListener : public decent::package::EventListenerInterface, public std::enable_shared_from_this<SeedingListener>
{
public:
   SeedingListener(seeding_plugin_impl &impl, const std::string &url, const decent::package::package_handle_t pi) : _url(url), _pi(pi), _my(&impl) {}
   virtual ~SeedingListener() {}

   virtual void package_download_error(const std::string &) override;
   virtual void package_download_complete() override;

private:
   std::string _url;
   decent::package::package_handle_t _pi;
   seeding_plugin_impl *_my;
   int failed = 0;
};

}// end namespace detail

/**
 * @class seeding_plugin_impl This class implements the seeder functionality.
 * @inherits package_transfer_interface::transfer_listener Integrates with package manager through this interface.
 */
struct seeding_plugin_impl /*: public package_transfer_interface::transfer_listener */{
   seeding_plugin_impl(seeding_plugin &_plugin) : _self(_plugin) {}
   ~seeding_plugin_impl() {}

   /**
    * Get DB instance
    * @return DB instance
    */
   graphene::chain::database &database();
   /**
    * Generates proof of retrievability of a package
    * @param so_id ID of the seeding_object
    * @param downloaded_package Downloaded package object
    */
   //void generate_por( my_seeding_id_type so_id, graphene::package::package_object downloaded_package );

   /**
    * Generates proof of retrievability of a package
    * @param so_id ID of the seeding_object
    * @param downloaded_package Downloaded package object
    */
   void generate_pors();

   void generate_por_int(const graphene::chain::seeding_object &so, decent::package::package_handle_t package_handle);
   /**
    * Process new content, from content_object
    * @param co Content object
    */
   void handle_new_content(const graphene::chain::content_object& co);

   /**
    * Delete data and database object related to a package. Called e.g. on package expiration
    * @param mso database object
    * @param package_handle package handle
    */
   void release_package(const graphene::chain::seeding_object &mso, decent::package::package_handle_t package_handle);
   /**
    * Process new content, from operation. If the content is managed by local seeder, it is downloaded, and meta are stored in local db.
    * @param cs_op
    */
   void handle_new_content(const graphene::chain::content_submit_operation& cs_op);
   /**
    * Handle newly submitted or resubmitted content. If it is content managed by one of our seeders, download it.
    * @param op_obj The operation wrapper carrying content submit operation
    */
   void handle_content_submit(const graphene::chain::content_submit_operation &op);

   /**
    * Handle request to buy. If it is concerning one of content seeded by the plugin, provide decryption key parts in deliver key
    * @param op_obj The operation wrapper carrying content request to buy operation
    */
   void handle_request_to_buy(const graphene::chain::request_to_buy_operation &op);

   /**
    * Called only after the highest known block has been applied. If it is request to buy or content submit, pass it to the corresponding handler
    * @param op_obj The operation wrapper
    * @param sync_mode
    */
   void handle_commited_operation(const graphene::chain::operation &op, bool sync_mode);

   /**
    * Restarts all downloads and seeding upon application start
    */
   void restore_state();

   /**
    * Resend all possibly missed keys
    */
   void resend_keys();

   /**
    * Generates and broadcasts RtP operation
    */
   void send_ready_to_publish();

   const graphene::chain::content_object& get_content(graphene::chain::database &db, const std::string &URI)const
   {
      const auto& cidx = db.get_index_type<graphene::chain::content_index>().indices().get<graphene::chain::by_URI>();
      const auto& citr = cidx.find(URI);
      FC_ASSERT(citr!=cidx.end());
      return *citr;
   }

   std::vector<detail::SeedingListener> listeners;
   seeding_plugin& _self;
//   std::map<package_transfer_interface::transfer_id, my_seeding_id_type> active_downloads; //<List of active downloads for whose we are expecting on_download_finished callback to be called
   std::shared_ptr<fc::thread> service_thread; //The thread where the computation shall happen
   fc::thread* main_thread; //The main thread, used mainly for DB modifications
   detail::seeding_plugin_startup_options seeding_options;
};

graphene::chain::database &seeding_plugin_impl::database()
{
   return _self.database();
}

void seeding_plugin_impl::handle_new_content(const graphene::chain::content_object& co)
{
   graphene::chain::content_submit_operation op;
   op.cd = co.cd;
   op.expiration = co.expiration;
   op.hash = co._hash;
   op.size = co.size;
   op.URI = co.URI;
   for( auto element : co.key_parts ){
      op.seeders.push_back(element.first);
      op.key_parts.push_back(element.second);
   }
   handle_new_content(op);
}

void seeding_plugin_impl::handle_new_content(const graphene::chain::content_submit_operation& cs_op)
{
   graphene::chain::database &db = database();
   //Check if the content is seeded by one of seeders managed by the plugin
   if( std::find(cs_op.seeders.begin(), cs_op.seeders.end(), (seeding_options.seeder)) != cs_op.seeders.end())
   {
      //Get the key particle assigned to this seeder
      auto s = cs_op.seeders.begin();
      auto k = cs_op.key_parts.begin();
      while( *s != seeding_options.seeder && s != cs_op.seeders.end())
      {
         ++s;
         ++k;
      }

      dlog("seeding plugin:  handle_content_submit() handling new content by seeder ${s}",("s",seeding_options.seeder));

      // new content case, create the object in DB and download the package
      const graphene::chain::seeding_object& mso = db.create<graphene::chain::seeding_object>([&](graphene::chain::seeding_object &so) {
            so.URI = cs_op.URI;
            so.seeder = seeding_options.seeder;
            so.size = cs_op.size; //we allocate the whole megabytes per content
            if( k != cs_op.key_parts.end())
               so.key = *k;
            so.expiration = cs_op.expiration;
            so.cd = cs_op.cd;
            so._hash = cs_op.hash;
      });
      dlog("seeding plugin:  handle_content_submit() created new seeding_object ${s}",("s",mso.id));
      seeding_options.free_space -= seeding_options.free_space > cs_op.size ? cs_op.size : seeding_options.free_space; //we allocate the whole megabytes per content
      //if we run this in main thread it can crash _push_block
      service_thread->async( [cs_op, this](){
            dlog("seeding plugin:  handle_content_submit() lambda called");
            auto& pm = decent::package::PackageManager::instance();
            auto package_handle = pm.get_package(cs_op.URI, cs_op.hash);
            decent::package::event_listener_handle_t sl = std::make_shared<detail::SeedingListener>(*this, cs_op.URI, package_handle);
            package_handle->remove_all_event_listeners();
            package_handle->add_event_listener(sl);
            package_handle->download(false);
      });
   }
}

void seeding_plugin_impl::handle_content_submit(const graphene::chain::content_submit_operation &cs_op)
{
   // dlog("seeding_plugin_impl::handle_content_submit starting for operation ${o} from block ${b}, highest known block is ${h}, head is ${i}",
   //    ("o",op_obj)("b", op_obj.block_num)("h", db.highest_know_block_number())("i", db.head_block_num()) );
   dlog("seeding plugin:  handle_content_submit() handling new content / resubmit");
   if(cs_op.expiration < fc::time_point::now())
      return;

   //bool is_resubmit = false;
   // check whether a content is resubmited. If so, unnecessary seeding_objects are expired
   graphene::chain::database &db = database();
   const auto &idx = db.get_index_type<graphene::chain::seeding_index>().indices().get<graphene::chain::by_URI>();
   if( idx.find(cs_op.URI) == idx.end() )
   {
      handle_new_content(cs_op);
   }
}

void seeding_plugin_impl::handle_request_to_buy(const graphene::chain::request_to_buy_operation &rtb_op)
{
   graphene::chain::database &db = database();
   const auto &idx = db.get_index_type<graphene::chain::seeding_index>().indices().get<graphene::chain::by_URI>();
   const auto &sitr = idx.find(rtb_op.URI);
   //Check if the content is handled by this plugin
   if( sitr == idx.end())
      return;

   dlog("seeding plugin: this is my content...");
   //ok, this is our seeding... we shall do some checks and generate deliver keys out of it...
   graphene::chain::account_object seeder_account = db.get<graphene::chain::account_object>(seeding_options.seeder);

   //get the content
   const auto &cidx = db.get_index_type<graphene::chain::content_index>().indices().get<graphene::chain::by_URI>();
   const auto &citr = cidx.find(rtb_op.URI);
   if( citr == cidx.end())
      FC_THROW("cannot find content by URI");
   const graphene::chain::content_object &co = *citr;
   if( co.expiration < fc::time_point::now() ){
      //if the content expired let the PoR generation cycle, return. PoR cycle will take care of the cleaning up...
      return;
   }

   //Decrypt the key particle and encrypt it with consumer key
   decent::encrypt::DInteger destPubKey = decent::encrypt::DInteger::from_string(rtb_op.pubKey);
   decent::encrypt::Ciphertext orig = co.key_parts.at(seeder_account.id);
   decent::encrypt::point message;
   auto result = decent::encrypt::el_gamal_decrypt(orig, seeding_options.content_private_key, message);
   FC_ASSERT(result == decent::encrypt::ok);
   decent::encrypt::Ciphertext key;
   decent::encrypt::DeliveryProof proof;
   result = decent::encrypt::encrypt_with_proof(message, seeding_options.content_private_key, destPubKey, orig, key, proof);

   //construct and send out the Deliver key operation
   graphene::chain::deliver_keys_operation op;
   op.key = key;
   op.proof = proof;
   const auto &bidx = db.get_index_type<graphene::chain::buying_index>().indices().get<graphene::chain::by_URI_consumer>();
   const auto &bitr = bidx.find(std::make_tuple( rtb_op.URI, rtb_op.consumer ));
   FC_ASSERT(bitr != bidx.end(), "no such buying_object for ${u}, ${c}",("u", rtb_op.URI )("c", rtb_op.consumer ));
   op.buying = bitr->id;

   op.seeder = seeder_account.id;

   graphene::chain::signed_transaction tx;
   tx.operations.push_back(op);

   auto dyn_props = db.get_dynamic_global_properties();
   tx.set_reference_block(dyn_props.head_block_id);
   tx.set_expiration(dyn_props.time + fc::seconds(30));
   tx.validate();

   graphene::chain::chain_id_type _chain_id = db.get_chain_id();

   tx.sign(seeding_options.seeder_private_key, _chain_id);
   //This method is called from main thread...
   database().push_transaction(tx);
   service_thread->async([this, tx]() { _self.app().p2p_node()->broadcast_transaction(tx); });
}

void seeding_plugin_impl::handle_commited_operation(const graphene::chain::operation &op, bool sync_mode)
{
   if( op.which() == graphene::chain::operation::tag<graphene::chain::request_to_buy_operation>::value ) {
      if( sync_mode ) {
         dlog("seeding_plugin_impl::handle_commited_operation exiting, not producing yet");
         return;
      }
      dlog("seeding plugin:  handle_commited_operation() handling request_to_buy");
      handle_request_to_buy(op.get<graphene::chain::request_to_buy_operation>());
   }

   if( op.which() == graphene::chain::operation::tag<graphene::chain::content_submit_operation>::value ) {
      dlog("seeding plugin:  handle_commited_operation() handling content_submit");
      //in case of content submit we don't really care if the sync has been finished or not...
      handle_content_submit(op.get<graphene::chain::content_submit_operation>());
   }
}

void seeding_plugin_impl::generate_por_int(const graphene::chain::seeding_object &mso, decent::package::package_handle_t package_handle)
{try{
   graphene::chain::database &db = database();
   dlog("seeding plugin_impl: generate_por() - Creating operation");
   graphene::chain::proof_of_custody_operation op;
   decent::encrypt::CustodyProof proof;

   auto dyn_props = db.get_dynamic_global_properties();

   if(mso.cd && !package_handle->is_virtual ){
      dlog("seeding plugin_impl: generate_por() - calculating full PoR");
      proof.reference_block = dyn_props.head_block_number;
      proof.seed = dyn_props.head_block_id; //use the block ID as source of entrophy

      if(package_handle->get_data_state() != decent::package::PackageInfo::DataState::CHECKED ) //files available on disk
         package_handle->download(true);

      FC_ASSERT(package_handle->create_proof_of_custody(*mso.cd, proof) == 0);
      op.proof = proof;
   }
   op.seeder = mso.seeder;

   op.URI = mso.URI;

   graphene::chain::signed_transaction tx;
   tx.operations.push_back(op);

   tx.set_reference_block(dyn_props.head_block_id);
   tx.set_expiration(dyn_props.time + fc::seconds(30));
   tx.validate();

   graphene::chain::chain_id_type _chain_id = db.get_chain_id();

   tx.sign(seeding_options.seeder_private_key, _chain_id);
   ddump((tx));

   main_thread->async([this, tx]() { database().push_transaction(tx); });

   dlog("broadcasting out PoR");
   _self.app().p2p_node()->broadcast_transaction(tx);
}FC_CAPTURE_AND_RETHROW((mso))}

void seeding_plugin_impl::release_package(const graphene::chain::seeding_object &mso, decent::package::package_handle_t package_handle)
{
   dlog("seeding plugin_impl:  release_package() - content expired, cleaning up");
   auto& pm = decent::package::PackageManager::instance();
   package_handle->stop_seeding("ipfs", true);
   package_handle->remove(true);
   pm.release_package(package_handle);
   database().modify<graphene::chain::seeding_object>(mso,[](graphene::chain::seeding_object& _mso){_mso.deleted = true;});
}

void
seeding_plugin_impl::generate_pors()
{try{
   /*
    * Generate_por just generates the POR. The checking when and if have to be perfomed at upper layer.
    */
   graphene::chain::database &db = database();
   const auto &seeding_idx = db.get_index_type<graphene::chain::seeding_index>().indices().get<graphene::db::by_id>();
   dlog("seeding plugin_impl:  generate_pors() start");
   auto& pm = decent::package::PackageManager::instance();

   for (const auto& mso : seeding_idx ) {
      //Collect data first...

      if(!mso.downloaded || mso.deleted )
         continue;
      dlog("seeding plugin_impl:  generate_pors() content ${c} downloaded, continue processing", ("c", mso.URI));
      auto package_handle = pm.get_package(mso.URI, mso._hash);
      package_handle->remove_all_event_listeners();

      const auto &content = get_content(db, mso.URI);
      if( content.expiration < fc::time_point::now()) {
         dlog("seeding plugin_impl:  generate_pors() content ${c} expired, clenaing up", ("c", mso.URI));
         release_package(mso, package_handle);
         dlog("seeding plugin_impl:  generate_pors() content cleaned, continue");
         continue;
      }
      dlog("seeding plugin_impl:  generate_pors() content is ok, processing");

      /*
       * calculate time when next PoR has to be sent out. The time shall be:
       * 1. now after submitting the new content (generate_por_int is called directly from the callback, so not handled here);
       * 2. 23:55 after the last PoR
       * 3. 5m before the expiration time
       */

      fc::time_point_sec generate_time;

      try {
         fc::time_point_sec last_proof_time = content.last_proof.at(mso.seeder);

         generate_time = std::min(last_proof_time + fc::seconds(24 * 60 * 60 - POR_WAKEUP_INTERVAL_SEC),
                                  content.expiration - fc::seconds(POR_WAKEUP_INTERVAL_SEC));
      } catch( const std::out_of_range& ) {
         //no proof has been delivered by us yet...
         generate_time = fc::time_point::now() + fc::seconds(1);
      }

      dlog("seeding plugin_impl:  generate_por() - generate time for this content is planned at ${t}",
           ("t", generate_time));
      //If we are about to generate PoR, generate it.
      if( fc::time_point(generate_time) < fc::time_point::now() + fc::seconds(POR_WAKEUP_INTERVAL_SEC) ){
         try {
            generate_por_int(mso, package_handle);
         }catch(...){}
      }
   }
   fc::time_point next_wakeup( fc::time_point::now() + fc::seconds(POR_WAKEUP_INTERVAL_SEC ));

   dlog("seeding plugin_impl:  generate_pors() - planning next wake-up at ${t}",("t", next_wakeup) );
   service_thread->schedule([this]() { generate_pors(); }, next_wakeup,
                            "Seeding plugin PoR generate");

   dlog("seeding plugin_impl:  generate_pors() end");
}FC_RETHROW()}

void seeding_plugin_impl::send_ready_to_publish()
{
   dlog("seeding plugin_impl: send_ready_to_publish() begin");
   try {
      ipfs::Client ipfs_client(decent::package::PackageManagerConfigurator::instance().get_ipfs_host(),
                              decent::package::PackageManagerConfigurator::instance().get_ipfs_port());
      ipfs::Json json;
      ipfs_client.Id(&json);

      const auto &assets_by_symbol = database().get_index_type<graphene::chain::asset_index>().indices().get<graphene::chain::by_symbol>();
      auto itr = assets_by_symbol.find(seeding_options.seeding_symbol);

      if( itr == assets_by_symbol.end() || !itr->is_monitored_asset() ||
            itr->monitored_asset_opts->current_feed.core_exchange_rate.is_null()) {
         itr = assets_by_symbol.find("DCT");
      }

      const graphene::chain::asset_object &ao = *itr;
      graphene::chain::asset dct_price = ao.amount_from_string(seeding_options.seeding_price);
      if( ao.id != graphene::chain::asset_id_type()) //core asset
         dct_price = database().price_to_dct( dct_price );

      if( dct_price.amount.value > DECENT_MAX_SEEDING_PRICE)
      {
         const auto& dct_ao = database().get(graphene::chain::asset_id_type());
         FC_THROW("Seeding price limit ${limit} DCT exceeded.",("limit",dct_ao.amount_to_string(DECENT_MAX_SEEDING_PRICE)));
      }

      graphene::chain::ready_to_publish_operation op;
      op.seeder = seeding_options.seeder;
      op.space = seeding_options.free_space;
      op.price_per_MByte = static_cast<uint32_t>(dct_price.amount.value);
      op.pubKey = get_public_el_gamal_key(seeding_options.content_private_key);
      op.ipfs_ID = json[ "ID" ];
      op.region_code = seeding_options.region_code;

      graphene::chain::signed_transaction tx;
      tx.operations.push_back(op);

      database().get_global_properties().parameters.current_fees->set_fee(tx.operations.back());
      auto dyn_props = database().get_dynamic_global_properties();
      tx.set_reference_block(dyn_props.head_block_id);
      tx.set_expiration(dyn_props.time + fc::seconds(30));

      graphene::chain::chain_id_type _chain_id = database().get_chain_id();

      tx.sign(seeding_options.seeder_private_key, _chain_id);
      ddump((tx));
      tx.validate();
      main_thread->async([ this, tx ]() {
            dlog("seeding plugin_impl:  send_ready_to_publish lambda - pushing transaction");
            database().push_transaction(tx);
      });
      dlog("seeding plugin_impl: send_ready_to_publish() broadcasting");
      _self.app().p2p_node()->broadcast_transaction(tx);
      //fc::usleep(fc::microseconds(1000000));
   } FC_LOG_AND_RETHROW()

   fc::time_point next_wakeup(fc::time_point::now() + fc::microseconds( (uint64_t) 1000000 * (DECENT_RTP_VALIDITY / 2 ))); //let's send PoR every hour
   dlog("seeding plugin_impl: planning next send_ready_to_publish at ${t}",("t",next_wakeup ));
   service_thread->schedule([=](){ send_ready_to_publish();}, next_wakeup, "Seeding plugin RtP generate" );
   dlog("seeding plugin_impl: send_ready_to_publish() end");
   //TODO_DECENT - hack, shall not be here
   resend_keys();
}

void seeding_plugin_impl::resend_keys()
{
   //Re-send all missing keys
   const auto& buying_range = database().get_index_type<graphene::chain::buying_index>().indices().get<graphene::chain::by_open_expiration>().equal_range( true );
   const auto& content_idx = database().get_index_type<graphene::chain::content_index>().indices().get<graphene::chain::by_URI>();
   const auto& cidx = database().get_index_type<graphene::chain::seeding_index>().indices().get<graphene::chain::by_URI>();
   std::for_each(buying_range.first, buying_range.second, [&](const graphene::chain::buying_object &buying_element)
   {
      const auto& content_itr = content_idx.find( buying_element.URI );
      if( cidx.find(buying_element.URI) != cidx.end() ) //in case that some reason we don't have this content in the internal database, e.g. it was deleted or it is fraud
      {
         if( content_itr != content_idx.end() && buying_element.expiration_time >= database().head_block_time() )
         {

            for( const auto& seeder_element : content_itr->key_parts )
            {
               if( seeder_element.first == seeding_options.seeder &&
                  std::find(buying_element.seeders_answered.begin(), buying_element.seeders_answered.end(), (seeding_options.seeder)) == buying_element.seeders_answered.end() )
               {
                  graphene::chain::request_to_buy_operation rtb_op;
                  rtb_op.URI = buying_element.URI;
                  rtb_op.consumer = buying_element.consumer;
                  rtb_op.pubKey = buying_element.pubKey;
                  rtb_op.price = buying_element.price;
                  rtb_op.region_code_from = buying_element.region_code_from;
                  dlog("seeding_plugin:  restore_state() processing unhandled request to buy ${s}",("s",rtb_op));
                  handle_request_to_buy( rtb_op );
                  break;
               }
            }
         }
      }
   });
}

void seeding_plugin_impl::restore_state()
{
   dlog("restoring state, main thread");
   service_thread->async([this]()
   {
      if( std::abs( (fc::time_point::now() - database().head_block_time()).count() ) > int64_t( 10000000 ) )
      {
         dlog("seeding plugin:  restoring state() waiting for sync");
         fc::usleep( fc::microseconds(1000000) );
      }
      dlog("restarting downloads, service thread");
      //start with rebuilding seeding_object database
      const auto& cidx = database().get_index_type<graphene::chain::seeding_index>().indices().get<graphene::chain::by_URI>();
      const auto& c_idx = database().get_index_type<graphene::chain::content_index>().indices().get<graphene::chain::by_expiration>();
      if( !c_idx.empty() )
      {
         // iterating backwards.
         // Content objects are ordered increasingly by expiration time.
         // This way we do not need to iterate over all ( expired ) objects
         auto content_itr = c_idx.rbegin();
         do
         {
            if( content_itr->expiration < database().head_block_time() )
               break;
            auto search_itr = content_itr->key_parts.find( seeding_options.seeder );
            if( search_itr != content_itr->key_parts.end() )
            {
               auto citr = cidx.find( content_itr->URI );
               if( citr == cidx.end() )
               {
                  database().create<graphene::chain::seeding_object>([&](graphene::chain::seeding_object &so) {
                        so.URI = content_itr->URI;
                        so.seeder = seeding_options.seeder;
                        so._hash = content_itr->_hash;
                        so.size = content_itr->size; //we allocate the whole megabytes per content
                        so.key = search_itr->second;
                        so.expiration = content_itr->expiration;
                        so.cd = content_itr->cd;
                  });
               }
               //we allocate the whole megabytes per content
               seeding_options.free_space -= seeding_options.free_space > content_itr->size ? content_itr->size : seeding_options.free_space;
            }
         }
         while( ++content_itr != c_idx.rend() );
      }

      //We need to rebuild the list of downloaded packages and compare it to the list of seeding_objects.
      //For the downloaded packages we can issue PoR right away, the others needs to be downloaded
      auto& pm = decent::package::PackageManager::instance();
      pm.recover_all_packages();
      for( decent::package::package_handle_t package : pm.get_all_known_packages() )
      {
         package->check(true);
         if ( package->get_data_state() != decent::package::PackageInfo::CHECKED )
            pm.release_package( package );
      }

      auto packages = pm.get_all_known_packages();
      for( const auto &so : cidx)
      {
         dlog("restarting downloads, dealing with package ${u}", ("u", so.URI));
         bool already_have = false;
         for( auto package : packages )
         {
            if( package->get_hash() == so._hash )
            {
               already_have = true;
               break;
            }
         }

         if(already_have)
         {
            database().modify<graphene::chain::seeding_object>(so, [](graphene::chain::seeding_object& so){so.downloaded = true;});
         }
         else if(so.expiration < database().head_block_time())
         {
            dlog("restarting downloads, re-downloading package ${u}", ("u", so.URI));
            auto package_handle = pm.get_package(so.URI, so._hash);
            decent::package::event_listener_handle_t sl = std::make_shared<detail::SeedingListener>(*this, so.URI, package_handle);
            package_handle->remove_all_event_listeners();
            package_handle->add_event_listener(sl);
            package_handle->download(false);
         }
      }

      dlog("restarting downloads, service thread end");
      generate_pors();
   });

   fc::time_point next_call = fc::time_point::now() + fc::seconds(30);
   dlog("RtP planned at ${t}", ("t",next_call) );
   service_thread->schedule([this](){ send_ready_to_publish(); }, next_call, "Seeding plugin RtP generate");
}

seeding_plugin::seeding_plugin(graphene::app::application* app) : graphene::app::plugin(app), my(nullptr)
{
}

seeding_plugin::~seeding_plugin()
{
}

void seeding_plugin::plugin_startup()
{
   if(!my)
      return;

   my->restore_state();
}

void seeding_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{try{
   // minimum required parameters to run seeding plugin
   if( !options.count("seeder-private-key") && !options.count("content-private-key") && !options.count("seeder") &&
       !options.count("free-space") && !options.count("seeding-price") )
   {
      return;
   }

   my.reset( new seeding_plugin_impl( *this) );

   if( options.count("seeder") ) {
      auto seeder = options["seeder"].as<std::string>();
      graphene::db::object_id_type account(seeder);
      FC_ASSERT( account.is<graphene::chain::account_id_type>(), "Invalid seeder account ${s}", ("s", seeder) );
      my->seeding_options.seeder = account;
   } else {
      FC_THROW("missing seeder parameter");
   }

   if( options.count("seeder-private-key") ) {
      auto wif_key = options["seeder-private-key"].as<std::string>();
      fc::optional<fc::ecc::private_key> private_key = graphene::utilities::wif_to_key(wif_key);
      if( !private_key.valid() ) {
         try {
            my->seeding_options.seeder_private_key = fc::variant(wif_key).as<fc::ecc::private_key>();
         }
         catch( const fc::exception & ) {
            FC_THROW("Invalid WIF-format seeder private key ${key_string}", ("key_string", wif_key));
         }
      }
      else {
         my->seeding_options.seeder_private_key = *private_key;
      }
   } else {
      FC_THROW("missing seeder-private-key parameter");
   }

   if( options.count("content-private-key") ) {
      auto content_key = options["content-private-key"].as<std::string>();
      my->seeding_options.content_private_key = decent::encrypt::DInteger::from_string(content_key);
      if( my->seeding_options.content_private_key.IsZero() ) {
         FC_THROW("Invalid content private key ${key_string}", ("key_string", content_key));
      }
   } else {
      FC_THROW("missing content-private-key parameter");
   }

   if( options.count("seeding-price") ) {
      my->seeding_options.seeding_price = options["seeding-price"].as<std::string>();
   } else{
      FC_THROW("missing seeding-price parameter");
   }

   if( options.count("seeding-symbol") ) {
      my->seeding_options.seeding_symbol = options["seeding-symbol"].as<std::string>();
   } else{
      my->seeding_options.seeding_symbol = "DCT";
   }

   if( options.count("free-space") )
      my->seeding_options.free_space = options["free-space"].as<int>();
   else
      FC_THROW("missing free-space parameter");

   if( options.count("packages-path") ) {
      try {
         my->seeding_options.packages_path = boost::filesystem::path(options["packages-path"].as<std::string>());
      } catch( ... ) {
         FC_THROW("Invalid packages path ${path_string}",
                  ("path_string", options["packages-path"].as<std::string>()));
      }
   } else {
      my->seeding_options.packages_path = boost::filesystem::path( );
   }

   const auto region_code_itr = graphene::chain::RegionCodes::s_mapNameToCode.find( options.count("region-code") ? options["region-code"].as<std::string>() : "" );

   if( region_code_itr != graphene::chain::RegionCodes::s_mapNameToCode.end() && region_code_itr->second != graphene::chain::RegionCodes::OO_all )
      my->seeding_options.region_code = region_code_itr->first;
   else
      FC_THROW("invalid country-code parameter");

   auto& pmc = decent::package::PackageManagerConfigurator::instance();
   try {
      fc::ip::endpoint api = fc::ip::endpoint::resolve_string( options["ipfs-api"].as<std::string>() ).back();
      pmc.set_ipfs_endpoint(api.get_address(), api.port());
   } catch( ... ) {
      FC_THROW_EXCEPTION(fc::unknown_host_exception, "Invalid IPFS daemon API address ${address}",
               ("address", options["ipfs-api"].as<std::string>()));
   }

   decent::check_ipfs_minimal_version(pmc.get_ipfs_host(), pmc.get_ipfs_port());

   database().add_index<graphene::db::primary_index<graphene::chain::seeding_index>>();

   auto& dir_helper = graphene::utilities::decent_path_finder::instance();
   if( my->seeding_options.packages_path != boost::filesystem::path( ) )
      dir_helper.set_packages_path( my->seeding_options.packages_path );
   else
      dir_helper.set_packages_path( dir_helper.get_decent_packages() / "seeding" );

   dlog("starting service thread");
   my->service_thread = std::make_shared<fc::thread>("seeding");
   my->main_thread = &fc::thread::current();

   database().on_new_commited_operation.connect( [&]( const graphene::chain::operation& op, uint32_t block_num ){ my->handle_commited_operation( op, false ); } );
   database().on_new_commited_operation_during_sync.connect( [&]( const graphene::chain::operation& op, uint32_t block_num ){ my->handle_commited_operation(op, true); } );

}FC_LOG_AND_RETHROW() }

std::string seeding_plugin::plugin_name()
{
   return "seeding";
}

void seeding_plugin::plugin_set_program_options(
        boost::program_options::options_description& cli,
        boost::program_options::options_description& cfg)
{
   graphene::db::object_id_type seeder_id_example = graphene::chain::account_id_type(15);
   cli.add_options()
         ("seeder", bpo::value<std::string>(), ("ID of account controlling this seeder, e.g. " + static_cast<std::string>(seeder_id_example)).c_str())
         ("seeder-private-key", bpo::value<std::string>(), "Private key of the account controlling this seeder")
         ("content-private-key", bpo::value<std::string>(), "El Gamal content private key")
         ("free-space", bpo::value<int>(), "Allocated disk space, in MegaBytes")
         ("packages-path", bpo::value<std::string>(), "Packages storage path")
         ("seeding-price", bpo::value<std::string>(), "Price amount per MegaBytes")
         ("seeding-symbol", bpo::value<std::string>()->default_value("DCT"), "Seeding price asset" )
         ("region-code", bpo::value<std::string>(), "Optional ISO 3166-1 alpha-2 two-letter region code")
         ("ipfs-api", bpo::value<std::string>()->default_value("127.0.0.1:5001"), "IPFS daemon API")
         ;

   cfg.add(cli);
}

void detail::SeedingListener::package_download_error(const std::string & error)
{
   elog("seeding plugin: package_download_error(): Failed downloading package ${s}, ${e}", ("s", _url)("e", error));
   failed++;
   decent::package::package_handle_t pi;

   pi = _pi;

   if(failed < 5) {
      //we want to restart the download; however, this method is being called from pi->_download_task::Task method, so we can't restart directly.
      // We will start asynchronously
      fc::thread::current().schedule([ pi ]() { pi->download(true); }, fc::time_point::now() + fc::seconds(60));
   }
   else{
      const auto& db = _my->database();
      const auto &mso_idx = db.get_index_type<graphene::chain::seeding_index>().indices().get<graphene::chain::by_URI>();
      const auto &mso_itr = mso_idx.find(_url);
      const auto& mso = *mso_itr;
      _my->release_package(mso, pi);
   }
}

void detail::SeedingListener::package_download_complete()
{
   dlog("seeding plugin: package_download_complete(): Finished downloading package${u}", ("u", _url));
   const auto& db = _my->database();
   const auto &mso_idx = db.get_index_type<graphene::chain::seeding_index>().indices().get<graphene::chain::by_URI>();
   const auto &mso_itr = mso_idx.find(_url);
   const auto& mso = *mso_itr;

   decent::package::package_handle_t pi = _pi;

   size_t size = (_pi->get_size() + 1024 * 1024 - 1) / (1024 * 1024);
   if( size > mso_itr->size ) {
      dlog("seeding plugin: package_download_complete(): Fraud detected: real content size is greater than propagated in blockchain; deleting...");
      //changing DB outside the main thread does not work properly, let's delete it from there
      _my->main_thread->async([ this, &mso, pi ]() { _my->release_package(mso, pi); });
      _pi.reset();
      return;
   }
   _pi->start_seeding("ipfs");
   //Don't block package manager thread for too long.
   _my->database().modify<graphene::chain::seeding_object>(mso, [](graphene::chain::seeding_object& so){so.downloaded = true;});
   _my->service_thread->async([ this, &mso, pi ]() { _my->generate_por_int(mso, pi); });
}

}}
