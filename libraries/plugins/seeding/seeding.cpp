/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
#include <graphene/seeding/seeding.hpp>
#include <graphene/chain/operation_history_object.hpp>
#include <graphene/chain/database.hpp>
#include <decent/encrypt/encryptionutils.hpp>
#include <graphene/chain/content_object.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/buying_object.hpp>
#include <graphene/utilities/key_conversion.hpp>
#include <decent/package/package.hpp>
#include <decent/package/package_config.hpp>
#include <fc/smart_ref_impl.hpp>
#include <algorithm>
#include <ipfs/client.h>

namespace decent { namespace seeding {
namespace bpo = boost::program_options;
namespace detail {

#define POR_WAKEUP_INTERVAL_SEC 300

seeding_plugin_impl::~seeding_plugin_impl() {
   return;
}

graphene::chain::database &seeding_plugin_impl::database()
{
   return _self.database();
}

void seeding_plugin_impl::handle_new_content(const content_object& co){
   content_submit_operation op;
   op.cd = co.cd;
   op.expiration = co.expiration;
   op.hash = co._hash;
   op.size = co.size;
   op.size = co.size;
   op.URI = co.URI;
   for( auto element : co.key_parts ){
      op.seeders.push_back(element.first);
      op.key_parts.push_back(element.second);
   }
   handle_new_content(op);
}

void seeding_plugin_impl::handle_new_content(const content_submit_operation& cs_op){
   graphene::chain::database &db = database();
   const auto &idx = db.get_index_type<my_seeder_index>().indices().get<by_seeder>();
   auto seeder_itr = idx.begin();
   while( seeder_itr != idx.end() )
   {
      //Check if the content is seeded by one of seeders managed by the plugin
      if( std::find(cs_op.seeders.begin(), cs_op.seeders.end(), (seeder_itr->seeder)) != cs_op.seeders.end())
      {
         //Get the key particle assigned to this seeder
         auto s = cs_op.seeders.begin();
         auto k = cs_op.key_parts.begin();
         while( *s != seeder_itr->seeder && s != cs_op.seeders.end())
         {
            ++s;
            ++k;
         }

         ilog("seeding plugin:  handle_content_submit() handling new content by seeder ${s}",("s",seeder_itr->seeder));

         // new content case, create the object in DB and download the package
         const my_seeding_object& mso = db.create<my_seeding_object>([&](my_seeding_object &so) {
              so.URI = cs_op.URI;
              so.seeder = seeder_itr->seeder;
              so.space = cs_op.size; //we allocate the whole megabytes per content
              if( k != cs_op.key_parts.end())
                 so.key = *k;
              so.expiration = cs_op.expiration;
              so.cd = cs_op.cd;
              so._hash = cs_op.hash;
         });
         auto so_id = mso.id;
         ilog("seeding plugin:  handle_content_submit() created new my_seeding_object ${s}",("s",so_id));
         db.modify<my_seeder_object>(*seeder_itr, [&](my_seeder_object &mso) {
              mso.free_space -= cs_op.size ; //we allocate the whole megabytes per content
         });
         ilog("seeding plugin:  handle_content_submit() my_seeder_object modified ${s}",("s",seeder_itr->id));
         //if we run this in main thread it can crash _push_block
         service_thread->async( [cs_op, this, mso](){
              ilog("seeding plugin:  handle_content_submit() lambda called");
              auto& pm = decent::package::PackageManager::instance();
              auto package_handle = pm.get_package(cs_op.URI, mso._hash);
              decent::package::event_listener_handle_t sl = std::make_shared<SeedingListener>(*this, mso , package_handle);
              package_handle->remove_all_event_listeners();
              package_handle->add_event_listener(sl);
              package_handle->download(false);
         });
      }
      ++seeder_itr;
   }

}

void seeding_plugin_impl::handle_content_submit(const content_submit_operation &cs_op)
{
   graphene::chain::database &db = database();

   // ilog("seeding_plugin_impl::handle_content_submit starting for operation ${o} from block ${b}, highest known block is ${h}, head is ${i}",
   //    ("o",op_obj)("b", op_obj.block_num)("h", db.highest_know_block_number())("i", db.head_block_num()) );
   ilog("seeding plugin:  handle_content_submit() handling new content / resubmit");
      if(cs_op.expiration < fc::time_point::now())
         return;

      auto element = db.get_index_type<my_seeding_index>().indices().get<by_URI>().find(cs_op.URI);
      //bool is_resubmit = false;
      if( element !=  db.get_index_type<my_seeding_index>().indices().get<by_URI>().end() )  // check whether a content is resubmited. If so, unnecessary my_seeding_objects are expired
      {
         return;
      }
      handle_new_content(cs_op);
}

void seeding_plugin_impl::handle_request_to_buy(const request_to_buy_operation &rtb_op)
{
   graphene::chain::database &db = database();
   const auto &idx = db.get_index_type<my_seeding_index>().indices().get<by_URI>();
   const auto &sitr = idx.find(rtb_op.URI);
   //Check if the content is handled by this plugin
   if( sitr == idx.end())
      return;

   const auto &sidx = db.get_index_type<my_seeder_index>().indices().get<by_seeder>();
   const auto &sritr = sidx.find(sitr->seeder);
   FC_ASSERT(sritr != sidx.end());

   ilog("seeding plugin: this is my content...");
   //ok, this is our seeding... we shall do some checks and generate deliver keys out of it...
   account_object seeder_account = db.get<account_object>(sitr->seeder);

   //get the content
   const auto &cidx = db.get_index_type<content_index>().indices().get<graphene::chain::by_URI>();
   const auto &citr = cidx.find(rtb_op.URI);
   if( citr == cidx.end())
      FC_THROW("cannot find content by URI");
   const content_object &co = *citr;
   if( co.expiration < fc::time_point::now() ){
      //if the content expired let the PoR generation cycle, return. PoR cycle will take care of the cleaning up...
      return;
   }

   //Decrypt the key particle and encrypt it with consumer key
   DInteger destPubKey = decent::encrypt::DInteger::from_string(rtb_op.pubKey);
   decent::encrypt::Ciphertext orig = co.key_parts.at(seeder_account.id);
   decent::encrypt::point message;
   auto result = decent::encrypt::el_gamal_decrypt(orig, sritr->content_privKey, message);
   FC_ASSERT(result == decent::encrypt::ok);
   decent::encrypt::Ciphertext key;
   decent::encrypt::DeliveryProof proof;
   result = decent::encrypt::encrypt_with_proof(message, sritr->content_privKey, destPubKey, orig, key, proof);

   //construct and send out the Deliver key operation
   deliver_keys_operation op;
   op.key = key;
   op.proof = proof;
   const auto &bidx = db.get_index_type<graphene::chain::buying_index>().indices().get<graphene::chain::by_URI_consumer>();
   const auto &bitr = bidx.find(std::make_tuple( rtb_op.URI, rtb_op.consumer ));
   FC_ASSERT(bitr != bidx.end(), "no such buying_object for ${u}, ${c}",("u", rtb_op.URI )("c", rtb_op.consumer ));
   op.buying = bitr->id;

   op.seeder = seeder_account.id;

   signed_transaction tx;
   tx.operations.push_back(op);

   auto dyn_props = db.get_dynamic_global_properties();
   tx.set_reference_block(dyn_props.head_block_id);
   tx.set_expiration(dyn_props.time + fc::seconds(30));
   tx.validate();

   chain_id_type _chain_id = db.get_chain_id();

   tx.sign(sritr->privKey, _chain_id);
   //This method is called from main thread...
   database().push_transaction(tx);
   service_thread->async([this, tx]() { _self.p2p_node().broadcast_transaction(tx); });
}

void seeding_plugin_impl::handle_commited_operation(const operation_history_object &op_obj, bool sync_mode)
{
   if( op_obj.op.which() == operation::tag<request_to_buy_operation>::value ) {
      if( sync_mode ) {
         ilog("seeding_plugin_impl::handle_commited_operation exiting, not producing yet");
         return;
      }
      ilog("seeding plugin:  handle_commited_operation() handling request_to_buy");
      const request_to_buy_operation &rtb_op = op_obj.op.get<request_to_buy_operation>();
      handle_request_to_buy(rtb_op);
   }

   if( op_obj.op.which() == operation::tag<content_submit_operation>::value ) {
      ilog("seeding plugin:  handle_commited_operation() handling content_submit");
      //in case of content submit we don't really care if the sync has been finished or not...
      const content_submit_operation &cs_op = op_obj.op.get<content_submit_operation>();
      handle_content_submit(cs_op);
   }
}


void
seeding_plugin_impl::generate_por_int(const my_seeding_object &mso, decent::package::package_handle_t package_handle)
{try{
      const auto& sidx = database().get_index_type<my_seeder_index>().indices().get<by_seeder>();
      const auto& seeder = sidx.find(mso.seeder);
      generate_por_int(mso, package_handle, seeder->privKey);
}FC_CAPTURE_AND_RETHROW((mso))}

void
seeding_plugin_impl::generate_por_int(const my_seeding_object &mso, decent::package::package_handle_t package_handle, fc::ecc::private_key privKey)
{try {
   graphene::chain::database &db = database();
   ilog("seeding plugin_impl: generate_por() - Creating operation");
   proof_of_custody_operation op;
   decent::encrypt::CustodyProof proof;
   auto dyn_props = db.get_dynamic_global_properties();
   if(mso.cd){
      ilog("seeding plugin_impl: generate_por() - calculating full PoR");
      fc::ripemd160 b_id = dyn_props.head_block_id;
      uint32_t b_num = dyn_props.head_block_number;
      proof.reference_block = b_num;
      for( int i = 0; i < 5; i++ )
         proof.seed.data[i] = b_id._hash[i]; //use the block ID as source of entrophy

      if(package_handle->get_data_state() == decent::package::PackageInfo::DataState::CHECKED ) //files available on disk
         package_handle->create_proof_of_custody(*mso.cd, proof);
      else{
         package_handle->download(true);
         package_handle->create_proof_of_custody(*mso.cd, proof);
      }
   }
   op.seeder = mso.seeder;
   if(mso.cd)
      op.proof = proof;

   op.URI = mso.URI;

   signed_transaction tx;
   tx.operations.push_back(op);

   tx.set_reference_block(dyn_props.head_block_id);
   tx.set_expiration(dyn_props.time + fc::seconds(30));
   tx.validate();

   chain_id_type _chain_id = db.get_chain_id();

   tx.sign(privKey, _chain_id);
   idump((tx));

   main_thread->async([this, tx]() { database().push_transaction(tx); });

   ilog("broadcasting out PoR");
   _self.p2p_node().broadcast_transaction(tx);
}FC_CAPTURE_AND_RETHROW((mso))}

void seeding_plugin_impl::release_package(const my_seeding_object &mso, decent::package::package_handle_t package_handle){
   ilog("seeding plugin_impl:  generate_por() - content expired, cleaning up");
   auto& pm = decent::package::PackageManager::instance();
   package_handle->stop_seeding();
   package_handle->remove(true);
   pm.release_package(package_handle);
   database().remove(mso);
   return;
}

void
seeding_plugin_impl::generate_pors()
{try{
   /*
    * Generate_por just generates the POR. The checking when and if have to be perfomed at upper layer.
    */
   graphene::chain::database &db = database();
   const auto &sidx = db.get_index_type<my_seeder_index>().indices().get<by_seeder>();
   const auto &seeding_idx = db.get_index_type<my_seeding_index>().indices().get<by_id>();
   ilog("seeding plugin_impl:  generate_pors() start");
   auto& pm = decent::package::PackageManager::instance();

   for (const auto& mso : seeding_idx ) {
      //Collect data first...
      ilog("seeding plugin_impl:  generate_pors() processing content ${c}, object ${o}", ("c", mso.URI)("o",mso));
      if(!mso.downloaded)
         continue;
      ilog("seeding plugin_impl:  generate_pors() content ${c} downloaded, continue processing", ("c", mso.URI));
      auto package_handle = pm.get_package(mso.URI, mso._hash);
      package_handle->remove_all_event_listeners();

      const auto &sritr = sidx.find(mso.seeder);
      FC_ASSERT(sritr != sidx.end());
      const auto &content = mso.get_content(db);


      if( content.expiration < fc::time_point::now()) {
         ilog("seeding plugin_impl:  generate_pors() content ${c} expired, clenaing up", ("c", mso.URI));
         release_package(mso, package_handle);
         continue;
      }


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
      } catch( std::out_of_range e ) {
         //no proof has been delivered by us yet...
         generate_time = fc::time_point::now() + fc::seconds(1);
      }

      ilog("seeding plugin_impl:  generate_por() - generate time for this content is planned at ${t}",
           ("t", generate_time));
      //If we are about to generate PoR, generate it.
      if( fc::time_point(generate_time) < fc::time_point::now() + fc::seconds(POR_WAKEUP_INTERVAL_SEC) ){
         generate_por_int(mso, package_handle, sritr->privKey);
      }
   }
   fc::time_point next_wakeup( fc::time_point::now() + fc::seconds(POR_WAKEUP_INTERVAL_SEC ));

   ilog("seeding plugin_impl:  generate_pors() - planning next wake-up at ${t}",("t", next_wakeup) );
   service_thread->schedule([this]() { generate_pors(); }, next_wakeup,
                            "Seeding plugin PoR generate");

   ilog("seeding plugin_impl:  generate_pors() end");
}FC_CAPTURE_AND_RETHROW(())}


void seeding_plugin_impl::send_ready_to_publish()
{
   ilog("seeding plugin_impl: send_ready_to_publish() begin");
   const auto &sidx = database().get_index_type<my_seeder_index>().indices().get<by_seeder>();
   auto sritr = sidx.begin();
   ipfs::Client ipfs_client(decent::package::PackageManagerConfigurator::instance().get_ipfs_host(), decent::package::PackageManagerConfigurator::instance().get_ipfs_port());
   ipfs::Json json;
   ipfs_client.Id( &json );

   while(sritr != sidx.end() ){
      const auto& assets_by_symbol = database().get_index_type<asset_index>().indices().get<by_symbol>();
      auto itr = assets_by_symbol.find(sritr->symbol);

      if(itr == assets_by_symbol.end() || !itr->is_monitored_asset() || itr->monitored_asset_opts->current_feed.core_exchange_rate.is_null() ) {
         itr = assets_by_symbol.find("DCT");
      }

      const asset_object& ao = *itr;

      asset dct_price  = ao.amount_from_string(sritr->price);
      if ( ao.id != asset_id_type() ) //core asset
         dct_price = dct_price * ao.monitored_asset_opts->current_feed.core_exchange_rate;

      ready_to_publish_operation op;
      op.seeder = sritr->seeder;
      op.space = sritr->free_space;
      op.price_per_MByte = dct_price.amount.value;
      op.pubKey = get_public_el_gamal_key(sritr->content_privKey);
      op.ipfs_ID = json["ID"];
      signed_transaction tx;
      tx.operations.push_back(op);

      idump((op));

      auto dyn_props = database().get_dynamic_global_properties();
      tx.set_reference_block(dyn_props.head_block_id);
      tx.set_expiration(dyn_props.time + fc::seconds(30));

      chain_id_type _chain_id = database().get_chain_id();

      tx.sign(sritr->privKey, _chain_id);
      idump((tx));
      tx.validate();
      main_thread->async( [this, tx](){ilog("seeding plugin_impl:  send_ready_to_publish lambda - pushing transaction"); database().push_transaction(tx);} );
      ilog("seeding plugin_impl: send_ready_to_publish() broadcasting");
      _self.p2p_node().broadcast_transaction(tx);
      //fc::usleep(fc::microseconds(1000000));
      sritr++;
   }
   fc::time_point next_wakeup(fc::time_point::now() + fc::microseconds( (uint64_t) 1000000 * (60 * 60))); //let's send PoR every hour
   ilog("seeding plugin_impl: planning next send_ready_to_publish at ${t}",("t",next_wakeup ));
   service_thread->schedule([=](){ send_ready_to_publish();}, next_wakeup, "Seeding plugin RtP generate" );
   ilog("seeding plugin_impl: send_ready_to_publish() end");
   //TODO_DECENT - hack, shall not be here
   resend_keys();
}


void seeding_plugin_impl::resend_keys(){
   //Re-send all missing keys
   const auto& sidx = database().get_index_type<my_seeder_index>().indices().get<by_seeder>();
   const auto& buying_range = database().get_index_type<buying_index>().indices().get<by_open_expiration>().equal_range( true );
   const auto& content_idx = database().get_index_type<content_index>().indices().get<graphene::chain::by_URI>();
   const auto& cidx = database().get_index_type<my_seeding_index>().indices().get<by_URI>();
   auto sitr = sidx.begin();

   while( sitr != sidx.end() )
   {
      std::for_each(buying_range.first, buying_range.second, [&](const buying_object &buying_element)
      {
           const auto& content_itr = content_idx.find( buying_element.URI );
           if( cidx.find(buying_element.URI) != cidx.end() ) //in case that some reason we don't have this content in the internal database, e.g. it was deleted or it is fraud

              if( content_itr != content_idx.end() && buying_element.expiration_time >= database().head_block_time() )
              {

                 for( const auto& seeder_element : content_itr->key_parts )
                 {
                    if( seeder_element.first == sitr->seeder &&
                        std::find(buying_element.seeders_answered.begin(), buying_element.seeders_answered.end(), (sitr->seeder)) == buying_element.seeders_answered.end() )
                    {
                       request_to_buy_operation rtb_op;
                       rtb_op.URI = buying_element.URI;
                       rtb_op.consumer = buying_element.consumer;
                       rtb_op.pubKey = buying_element.pubKey;
                       rtb_op.price = buying_element.price;
                       rtb_op.region_code_from = buying_element.region_code_from;
                       ilog("seeding_plugin:  restore_state() processing unhandled request to buy ${s}",("s",rtb_op));
                       handle_request_to_buy( rtb_op );
                       break;
                    }
                 }
              }
      });
      sitr++;
   }

}

void seeding_plugin_impl::restore_state(){

   elog("restoring state, main thread");
   service_thread->async([this](){
        if( std::abs( (fc::time_point::now() - database().head_block_time()).count() ) > int64_t( 10000000 ) )
        {
           ilog("seeding plugin:  restoring state() waiting for sync");
           fc::usleep( fc::microseconds(1000000) );
        }
        elog("restarting downloads, service thread");
        //start with rebuilding my_seeding_object database
        const auto& sidx = database().get_index_type<my_seeder_index>().indices().get<by_seeder>();
        const auto& cidx = database().get_index_type<my_seeding_index>().indices().get<by_URI>();


        const auto& c_idx = database().get_index_type<content_index>().indices().get<by_expiration>();
        auto sitr = sidx.begin();
        {//remove all existing entries and start over
           const auto &sidx = database().get_index_type<my_seeding_index>();
           sidx.inspect_all_objects([ & ](const object &o) {
                database().remove(o);
           });
        }
        while( sitr != sidx.end() )
        {
           auto content_itr = c_idx.end();
           while( content_itr != c_idx.begin() )
              // iterating backwards.
              // Content objects are ordered increasingly by expiration time.
              // This way we do not need to iterate over all ( expired ) objects
           {
              content_itr--;
              if( content_itr->expiration < database().head_block_time() )
                 break;
              auto search_itr = content_itr->key_parts.find( sitr->seeder );
              if( search_itr != content_itr->key_parts.end() )
              {

                 auto citr = cidx.find( content_itr->URI );
                 if( citr == cidx.end() )
                 {
                    const my_seeding_object& mso = database().create<my_seeding_object>([&](my_seeding_object &so) {
                         so.URI = content_itr->URI;
                         so.seeder = sitr->seeder;
                         so._hash = content_itr->_hash;
                         so.space = content_itr->size; //we allocate the whole megabytes per content
                         so.key = search_itr->second;
                         so.expiration = content_itr->expiration;
                         so.cd = content_itr->cd;
                    });
                    ilog("seeding_plugin:  restore_state() creating my_seeding_object for unhandled content submit ${s}",("s",mso));
                    database().modify<my_seeder_object>(*sitr, [&](my_seeder_object &mso) {
                         mso.free_space -= content_itr->size ; //we allocate the whole megabytes per content
                    });
                 }
              }
           }
           sitr++;
        }

        //We need to rebuild the list of downloaded packages and compare it to the list of my_seeding_objects.
        //For the downloaded packages we can issue PoR right away, the others needs to be downloaded
        auto& pm = decent::package::PackageManager::instance();
        pm.recover_all_packages();
        auto packages = pm.get_all_known_packages();
        for( decent::package::package_handle_t package : packages ){
           package->check(true);
           if ( package->get_data_state() != decent::package::PackageInfo::CHECKED )
              pm.release_package( package );
        }

        packages = pm.get_all_known_packages();

        auto citr = cidx.begin();
        while(citr!=cidx.end()) {
           elog("restarting downloads, dealing with package ${u}", ("u", citr->URI));
           bool already_have = false;
           decent::package::package_handle_t package_handle(0);
           for( auto package : packages )
              if( package->get_hash() == citr->_hash ) {
                 already_have = true;
                 package_handle = package;
              }

           if(already_have){
              database().modify<my_seeding_object>(*citr, [](my_seeding_object& so){
                   elog("setting object ${o} to downloaded",("o", so));
                   so.downloaded = true;
                   elog("setting object ${o} to downloaded",("o", so));

              });
           }else{
              elog("restarting downloads, re-downloading package ${u}", ("u", citr->URI));
              package_handle = pm.get_package(citr->URI, citr->_hash);
              decent::package::event_listener_handle_t sl = std::make_shared<SeedingListener>(*this, *citr , package_handle);
              package_handle->remove_all_event_listeners();
              package_handle->add_event_listener(sl);
              package_handle->download(false);
           }
           ++citr;
        }
        elog("restarting downloads, service thread end");
        generate_pors();
   });
}
}// end namespace detail


seeding_plugin::seeding_plugin():my(nullptr) {}

void seeding_plugin::plugin_startup()
{
   if(!my)
      return;

   ilog("seeding plugin:  plugin_startup() start");


   my->restore_state();
   fc::time_point next_call = fc::time_point::now()  + fc::microseconds(30000000);
   elog("RtP planned at ${t}", ("t",next_call) );
   my->service_thread->schedule([this](){elog("generating first ready to publish");my->send_ready_to_publish(); }, next_call, "Seeding plugin RtP generate");
   ilog("seeding plugin:  plugin_startup() end");
}

void seeding_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{try{
   ilog("seeding plugin:  plugin_initialize() start");

   database().add_index< primary_index < my_seeding_index > >();
   database().add_index< primary_index < my_seeder_index > >();

   fc::optional<fc::ecc::private_key> private_key;
   seeding_plugin_startup_options seeding_options;

   if( options.count("seeder-private-key") || options.count("content-private-key") || options.count("seeder")
       || options.count("free-space") || options.count("seeding-price") ) { // minimum required parameters to run seeding plugin
      if( options.count("seeder-private-key")) {
         private_key = graphene::utilities::wif_to_key(options["seeder-private-key"].as<std::string>());
         if( !private_key )
            try {
               private_key = fc::variant(options["seeder-private-key"].as<string>()).as<fc::ecc::private_key>();
            }
            catch( const fc::exception & ) {
               FC_THROW("Invalid WIF-format seeder private key ${key_string}",
                        ("key_string", options["seeder-private-key"].as<string>()));
            }
      } else {
         FC_THROW("missing seeder-private-key parameter");
      }
      seeding_options.seeder_private_key = *private_key;

      if( options.count("content-private-key")) {
         try {
            seeding_options.content_private_key = decent::encrypt::DInteger::from_string(options["content-private-key"].as<string>());
         } catch( ... ) {
            FC_THROW("Invalid content private key ${key_string}",
                     ("key_string", options["content-private-key"].as<string>()));
         }
      } else {
         FC_THROW("missing content-private-key parameter");
      }

      if( options.count("seeding-price")) {
         seeding_options.seeding_price = options["seeding-price"].as<string>();
      } else{
         FC_THROW("missing seeding-price parameter");
      }

      if( options.count("seeding-symbol")) {
         seeding_options.seeding_symbol = options["seeding-symbol"].as<string>();
      } else{
         seeding_options.seeding_symbol = "DCT";
      }

      if( options.count("seeder"))
         seeding_options.seeder = fc::variant(options["seeder"].as<string>()).as<account_id_type>();
      else
         FC_THROW("missing seeder parameter");

      if( options.count("free-space"))
         seeding_options.free_space = options["free-space"].as<int>();
      else
         FC_THROW("missing free-space parameter");

      if( options["packages-path"].as<string>() != "" ) {
         try {
            seeding_options.packages_path = boost::filesystem::path(options["packages-path"].as<string>());
         } catch( ... ) {
            FC_THROW("Invalid packages path ${path_string}",
                     ("path_string", options["packages-path"].as<string>()));
         }
      } else {
         seeding_options.packages_path = fc::path( "" );
      }

      plugin_pre_startup( seeding_options );
   }

   ilog("seeding plugin:  plugin_initialize() end");
}FC_LOG_AND_RETHROW() }

void seeding_plugin::plugin_pre_startup( const seeding_plugin_startup_options& seeding_options )
{
   if( my )
      return;

   ilog("seeding plugin:  plugin_pre_startup() start");
   auto& dir_helper = graphene::utilities::decent_path_finder::instance();
   if( seeding_options.packages_path != fc::path( ) )
      dir_helper.set_packages_path( seeding_options.packages_path );
   else
      dir_helper.set_packages_path( dir_helper.get_decent_packages() / "seeding" );
   
   ilog("starting service thread");
   my = unique_ptr<detail::seeding_plugin_impl>( new detail::seeding_plugin_impl( *this) );
   my->service_thread = std::make_shared<fc::thread>("seeding");
   my->main_thread = &fc::thread::current();

   database().on_new_commited_operation.connect( [&]( const operation_history_object& b ){ my->handle_commited_operation( b, false ); } );
   database().on_new_commited_operation_during_sync.connect( [&]( const operation_history_object& b ){
      my->handle_commited_operation(b, true); } );

   ilog("seeding plugin:  plugin_pre_startup() seeder prepared");
   try {
      {//remove all existing entries and start over
         const auto &sidx = database().get_index_type<my_seeder_index>();
         sidx.inspect_all_objects([ & ](const object &o) {
              database().remove(o);
         });
      }

      database().create<my_seeder_object>([&seeding_options](my_seeder_object &mso) {
         mso.seeder = seeding_options.seeder;
         mso.free_space = seeding_options.free_space;
         mso.content_privKey = seeding_options.content_private_key;
         mso.privKey = seeding_options.seeder_private_key;
         mso.price = seeding_options.seeding_price;
         mso.symbol = seeding_options.seeding_symbol;
      });
   }catch(...){}
   ilog("seeding plugin:  plugin_pre_startup() end");
}

std::string seeding_plugin::plugin_name()const
{
   return "seeding";
}

void seeding_plugin::plugin_set_program_options(
        boost::program_options::options_description& cli,
        boost::program_options::options_description& cfg)
{
   cli.add_options()
         ("seeder", bpo::value<string>(), "ID of account controlling this seeder, quotes are required, may specify multiple times)")
         ("content-private-key", bpo::value<string>(), "El Gamal content private key")
         ("seeder-private-key", bpo::value<string>(), "Private key of the account controlling this seeder")
         ("free-space", bpo::value<int>(), "Allocated disk space, in MegaBytes")
         ("packages-path", bpo::value<string>()->default_value(""), "Packages storage path")
         ("seeding-price", bpo::value<string>(), "Price amount per MegaBytes")
         ("seeding-symbol", bpo::value<string>()->default_value("DCT"), "Seeding price asset, e.g. DCT" )
         ;
}

void detail::SeedingListener::package_download_error(const std::string & error) {
   elog("seeding plugin: package_download_error(): Failed downloading package ${s}, ${e}", ("s", _url)("e", error));
   decent::package::package_handle_t pi;
   auto& pm = decent::package::PackageManager::instance();

   pi = _pi;
   //we want to restart the download; however, this method is being called from pi->_download_task::Task method, so we can't restart directly.
   // We will start asynchronously
   fc::thread::current().schedule([pi](){ pi->download(true);}, fc::time_point::now() + fc::seconds(60) );
};

void detail::SeedingListener::package_download_complete() {
   ilog("seeding plugin: package_download_complete(): Finished downloading package${u}", ("u", _url));
   auto &pm = decent::package::PackageManager::instance();
   const auto& db = _my->database();
   const auto &mso_idx = db.get_index_type<my_seeding_index>().indices().get<by_URI>();
   const auto &mso_itr = mso_idx.find(_url);
   const auto& mso = *mso_itr;

   decent::package::package_handle_t pi = _pi;

   size_t size = (_pi->get_size() + 1024 * 1024 - 1) / (1024 * 1024);
   if( size > mso_itr->space ) {
      ilog("seeding plugin: package_download_complete(): Fraud detected: real content size is greater than propagated in blockchain; deleting...");
      //changing DB outside the main thread does not work properly, let's delete it from there
      _my->main_thread->async([ & ]() { _my->release_package(mso, pi); });
      _pi.reset();
      return;
   }
   _pi->start_seeding();
   //Don't block package manager thread for too long.
   seeding_plugin_impl *my = _my;
   _my->database().modify<my_seeding_object>(mso, [](my_seeding_object& so){so.downloaded = true;});
   _my->service_thread->async([ & ]() { _my->generate_por_int(mso, pi); });
};


}}
