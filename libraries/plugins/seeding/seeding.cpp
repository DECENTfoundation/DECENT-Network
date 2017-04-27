#include <graphene/seeding/seeding.hpp>
#include <graphene/chain/operation_history_object.hpp>
#include <graphene/chain/database.hpp>
#include <decent/encrypt/encryptionutils.hpp>
#include <graphene/chain/content_object.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/buying_object.hpp>
#include <graphene/utilities/key_conversion.hpp>
#include <graphene/package/package.hpp>
#include <fc/smart_ref_impl.hpp>
#include <algorithm>

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

void seeding_plugin_impl::handle_content_submit(const operation_history_object &op_obj)
{
   graphene::chain::database &db = database();

   // ilog("seeding_plugin_impl::handle_content_submit starting for operation ${o} from block ${b}, highest known block is ${h}, head is ${i}",
   //    ("o",op_obj)("b", op_obj.block_num)("h", db.highest_know_block_number())("i", db.head_block_num()) );

   //The oposite shall not happen, but better be sure.
   if( op_obj.op.which() == operation::tag<content_submit_operation>::value ) {
      ilog("seeding plugin:  handle_content_submit() handling new content");
      const content_submit_operation &cs_op = op_obj.op.get<content_submit_operation>();
      if(cs_op.expiration < fc::time_point::now())
         return;
      const auto &idx = db.get_index_type<my_seeder_index>().indices().get<by_seeder>();
      auto seeder_itr = idx.begin();
      while( seeder_itr != idx.end()) {
         //Check if the content is seeded by one of seeders managed by the plugin
         if( std::find(cs_op.seeders.begin(), cs_op.seeders.end(), (seeder_itr->seeder)) != cs_op.seeders.end()) {
            ilog("seeding plugin:  handle_content_submit() handling new content by seeder ${s}",("s",seeder_itr->seeder));

            //Get the key particle assigned to this seeder
            auto s = cs_op.seeders.begin();
            auto k = cs_op.key_parts.begin();
            while( *s != seeder_itr->seeder && s != cs_op.seeders.end()) {
               ++s;
               ++k;
            }

            const auto &sidx = db.get_index_type<my_seeding_index>().indices().get<by_URI>();
            const auto &sitr = sidx.find(cs_op.URI);
            if( sitr != sidx.end()) { //not a new content
               //TODO_DECENT - resubmit is not handled yet properly, planned in drop 2
               uint32_t new_space = cs_op.size; //we allocate the whole megabytes per content
               uint32_t diff_space = sitr->space - new_space;
               db.modify<my_seeding_object>(*sitr, [&](my_seeding_object &mso) {
                    mso.space -= diff_space;
               });
               db.modify<my_seeder_object>(*seeder_itr, [&](my_seeder_object &mso) {
                    mso.free_space -= diff_space;
               });
            } else { // new content case, create the object in DB and download the package
               const my_seeding_object& mso = db.create<my_seeding_object>([&](my_seeding_object &so) {
                    so.URI = cs_op.URI;
                    so.seeder = seeder_itr->seeder;
                    so._hash = cs_op.hash;
                    so.space = cs_op.size; //we allocate the whole megabytes per content
                    if( k != cs_op.key_parts.end())
                       so.key = *k;
                    so.expiration = cs_op.expiration;
                    so.cd = cs_op.cd;
               });
               auto so_id = mso.id;
               ilog("seeding plugin:  handle_content_submit() created new content_object ${s}",("s",mso));
               db.modify<my_seeder_object>(*seeder_itr, [&](my_seeder_object &mso) {
                    mso.free_space -= cs_op.size ; //we allocate the whole megabytes per content
               });
               //if we run this in main thread it can crash _push_block
               service_thread->async( [cs_op, this, mso](){
                    ilog("seeding plugin:  handle_content_submit() lambda called");
                    /*auto id = package_manager::instance().download_package(cs_op.URI, *this, empty_report_stats_listener::instance());
                    active_downloads[id] = so_id;
                    */
                    auto& pm = decent::package::PackageManager::instance();
                    auto package_handle = pm.get_package(mso.URI);
                    decent::package::event_listener_handle_t sl = std::make_shared<SeedingListener>(*this, mso , package_handle);
                    package_handle->add_event_listener(sl);
                    package_handle->download(false);
                    ilog("seeding plugin:  handle_content_submit() lambda ended");
               });
            }
         }
         ++seeder_itr;
      }
   }
}

void seeding_plugin_impl::handle_request_to_buy(const operation_history_object &op_obj)
{
   graphene::chain::database &db = database();
   const request_to_buy_operation &rtb_op = op_obj.op.get<request_to_buy_operation>();
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
   if(co.expiration < fc::time_point::now() ){
      //if the content expired let the PoR generation cycle, return. PoR cycle will take care of the cleaning up...
      return;
   }

   //Decrypt the key particle and encryt it with consumer key
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
   graphene::chain::database &db = database();

   if( op_obj.op.which() == operation::tag<request_to_buy_operation>::value ) {
      if( sync_mode ) {
         ilog("seeding_plugin_impl::handle_commited_operation exiting, not producing yet");
         return;
      }
      ilog("seeding plugin:  handle_commited_operation() handling request_to_buy");
      handle_request_to_buy(op_obj);
   }

   if( op_obj.op.which() == operation::tag<content_submit_operation>::value ) {
      ilog("seeding plugin:  handle_commited_operation() handling content_submit");
      //in case of content submit we don't really care if the sync has been finished or not...
      handle_content_submit(op_obj);
   }
}


void
seeding_plugin_impl::generate_por2(const my_seeding_object& mso, decent::package::package_handle_t package_handle)
{
   ilog("seeding plugin_impl:  generate_por() start");
   graphene::chain::database &db = database();

   //Collect data first...
   const auto &sidx = db.get_index_type<my_seeder_index>().indices().get<by_seeder>();
   const auto &sritr = sidx.find(mso.seeder);
   FC_ASSERT(sritr != sidx.end());
   const auto& cidx = db.get_index_type<content_index>().indices().get<graphene::chain::by_URI>();
   const auto& citr = cidx.find(mso.URI);

   ilog("seeding plugin_impl:  generate_por() processing content ${c}",("c", mso.URI));

   FC_ASSERT(citr != cidx.end());
   if( citr->expiration < fc::time_point::now() ){
      ilog("seeding plugin_impl:  generate_por() - content expired, cleaning up");
      auto& pm = decent::package::PackageManager::instance();
      package_handle->stop_seeding();
      pm.release_package(package_handle);
      return;
   }

   //calculate time when next PoR has to be sent out
   fc::time_point_sec generate_time;

   try {
      fc::time_point_sec last_proof_time = citr->last_proof.at(mso.seeder);
      generate_time = last_proof_time + fc::seconds(24*60*60) - fc::seconds(POR_WAKEUP_INTERVAL_SEC/2);
      if( generate_time > citr->expiration )
         generate_time = citr->expiration - fc::seconds(POR_WAKEUP_INTERVAL_SEC);
   }catch (std::out_of_range e){
      //no proof has been delivered by us yet...
      generate_time = fc::time_point::now();
   }

   ilog("seeding plugin_impl:  generate_por() - generate time for this content is planned at ${t}",("t", generate_time) );
   fc::time_point next_wakeup( fc::time_point::now() + fc::microseconds(POR_WAKEUP_INTERVAL_SEC * 1000000 ));
   //If we are about to generate PoR, generate it.
   if( fc::time_point(generate_time) - fc::seconds(POR_WAKEUP_INTERVAL_SEC) <= ( fc::time_point::now() ) )
   {
      ilog("seeding plugin_impl: generate_por() - generating PoR");

      decent::encrypt::CustodyProof proof;

      auto dyn_props = db.get_dynamic_global_properties();
      fc::ripemd160 b_id = dyn_props.head_block_id;
      uint32_t b_num = dyn_props.head_block_number;
      proof.reference_block = b_num;
      for( int i = 0; i < 5; i++ )
         proof.seed.data[i] = b_id._hash[i]; //use the block ID as source of entrophy

      if(package_handle->get_data_state() == decent::package::PackageInfo::DataState::CHECKED ) //files available on disk
         package_handle->create_proof_of_custody(mso.cd, proof);
      else{
         package_handle->download(true);
         package_handle->create_proof_of_custody(mso.cd, proof);
         package_handle->remove(true);
      }
      // issue PoR and plan periodic PoR generation
      proof_of_custody_operation op;

      op.seeder = mso.seeder;
      op.proof = proof;
      op.URI = mso.URI;

      signed_transaction tx;
      tx.operations.push_back(op);

      tx.set_reference_block(dyn_props.head_block_id);
      tx.set_expiration(dyn_props.time + fc::seconds(30));
      tx.validate();

      chain_id_type _chain_id = db.get_chain_id();

      tx.sign(sritr->privKey, _chain_id);
      idump((tx));

      main_thread->async([this, tx]() { database().push_transaction(tx); });

      ilog("broadcasting out PoR");
      _self.p2p_node().broadcast_transaction(tx);
   }

   ilog("seeding plugin_impl:  generate_por() - planning next wake-up at ${t}",("t", next_wakeup) );
   service_thread->schedule([this, mso, package_handle]() { generate_por2(mso, package_handle); }, next_wakeup,
                            "Seeding plugin PoR generate");

   ilog("seeding plugin_impl:  generate_por() end");
}
/*
void
seeding_plugin_impl::generate_por(my_seeding_id_type so_id, graphene::package::package_object downloaded_package)
{
   ilog("seeding plugin_impl:  generate_por() start");
   graphene::chain::database &db = database();

   //Collect data first...
   const my_seeding_object &mso = db.get<my_seeding_object>(so_id);
   const auto &sidx = db.get_index_type<my_seeder_index>().indices().get<by_seeder>();
   const auto &sritr = sidx.find(mso.seeder);
   FC_ASSERT(sritr != sidx.end());
   const auto& cidx = db.get_index_type<content_index>().indices().get<graphene::chain::by_URI>();
   const auto& citr = cidx.find(mso.URI);

   ilog("seeding plugin_impl:  generate_por() processing content ${c}",("c", mso.URI));

   FC_ASSERT(citr != cidx.end());
   if( citr->expiration < fc::time_point::now() ){
      ilog("seeding plugin_impl:  generate_por() - content expired, cleaning up");
      package_manager::instance().delete_package(downloaded_package.get_hash());
      main_thread->async([&]{ db.remove(mso);});
      return;
   }

   //calculate time when next PoR has to be sent out
   fc::time_point_sec generate_time;

   try {
      fc::time_point_sec last_proof_time = citr->last_proof.at(mso.seeder);
      generate_time = last_proof_time + fc::seconds(24*60*60) - fc::seconds(POR_WAKEUP_INTERVAL_SEC/2);
      if( generate_time > citr->expiration )
         generate_time = citr->expiration - fc::seconds(POR_WAKEUP_INTERVAL_SEC);
   }catch (std::out_of_range e){
      //no proof has been delivered by us yet...
      generate_time = fc::time_point::now();
   }

   ilog("seeding plugin_impl:  generate_por() - generate time for this content is planned at ${t}",("t", generate_time) );
   fc::time_point next_wakeup( fc::time_point::now() + fc::microseconds(POR_WAKEUP_INTERVAL_SEC * 1000000 ));
   //If we are about to generate PoR, generate it.
   if( fc::time_point(generate_time) - fc::seconds(POR_WAKEUP_INTERVAL_SEC) <= ( fc::time_point::now() ) )
   {
      ilog("seeding plugin_impl: generate_por() - generating PoR");

      decent::encrypt::CustodyProof proof;

      auto dyn_props = db.get_dynamic_global_properties();
      fc::ripemd160 b_id = dyn_props.head_block_id;
      uint32_t b_num = dyn_props.head_block_number;
      proof.reference_block = b_num;
      for( int i = 0; i < 5; i++ )
         proof.seed.data[i] = b_id._hash[i]; //use the block ID as source of entrophy

      downloaded_package.create_proof_of_custody(mso.cd, proof);
      // issue PoR and plan periodic PoR generation
      proof_of_custody_operation op;

      op.seeder = mso.seeder;
      op.proof = proof;
      op.URI = mso.URI;

      signed_transaction tx;
      tx.operations.push_back(op);

      tx.set_reference_block(dyn_props.head_block_id);
      tx.set_expiration(dyn_props.time + fc::seconds(30));
      tx.validate();

      chain_id_type _chain_id = db.get_chain_id();

      tx.sign(sritr->privKey, _chain_id);
      idump((tx));

      main_thread->async([this, tx]() { database().push_transaction(tx); });

      ilog("broadcasting out PoR");
      _self.p2p_node().broadcast_transaction(tx);
   }

   ilog("seeding plugin_impl:  generate_por() - planning next wake-up at ${t}",("t", next_wakeup) );
   service_thread->schedule([this, so_id, downloaded_package]() { generate_por(so_id, downloaded_package); }, next_wakeup,
                   "Seeding plugin PoR generate");

   ilog("seeding plugin_impl:  generate_por() end");
} //*/

void seeding_plugin_impl::send_ready_to_publish()
{
   ilog("seeding plugin_impl: send_ready_to_publish() begin");
   const auto &sidx = database().get_index_type<my_seeder_index>().indices().get<by_seeder>();
   auto sritr = sidx.begin();
   graphene::chain::database &db = database();

   while(sritr != sidx.end() ){
      ready_to_publish_operation op;
      op.seeder = sritr->seeder;
      op.space = sritr->free_space;
      op.price_per_MByte = sritr->price;
      op.pubKey = get_public_el_gamal_key(sritr->content_privKey);
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
      sritr++;
   }
   fc::time_point next_wakeup(fc::time_point::now() + fc::microseconds( (uint64_t) 1000000 * (60 * 60)));
   ilog("seeding plugin_impl: planning next send_ready_to_publish at ${t}",("t",next_wakeup ));
   service_thread->schedule([=](){ send_ready_to_publish();}, next_wakeup, "Seeding plugin RtP generate" );
   ilog("seeding plugin_impl: send_ready_to_publish() end");
}

void seeding_plugin_impl::restart_downloads(){
   elog("restarting downloads, main thread");
   service_thread->async([this](){
        elog("restarting downloads, service thread");
        /*const auto& cidx = database().get_index_type<my_seeding_index>().indices().get<by_URI>();
        auto citr = cidx.begin();

        while(citr!=cidx.end()){
           if( citr->expiration > fc::time_point_sec( fc::time_point::now() ) ) {
              active_downloads[ package_manager::instance().download_package(citr->URI, *this,
                                                                             empty_report_stats_listener::instance()) ] = citr->id;

           }
           ++citr;

        }*/

        //We need to rebuild the list of downloaded packages and compare it to the list of my_seeding_objects.
        // However, we can't rely on the
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
        const auto& cidx = database().get_index_type<my_seeding_index>().indices().get<by_URI>();
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
              generate_por2( *citr, package_handle );
           }else{
              elog("restarting downloads, re-downloading package ${u}", ("u", citr->URI));
              package_handle = pm.get_package(citr->URI);
              decent::package::event_listener_handle_t sl = std::make_shared<SeedingListener>(*this, *citr , package_handle);
              package_handle->add_event_listener(sl);
              package_handle->download(false);
           }
           ++citr;
        }
        elog("restarting downloads, service thread end");
   });
}
}// end namespace detail


seeding_plugin::seeding_plugin():my(nullptr) {}

void seeding_plugin::plugin_startup()
{
   if(!my)
      return;
   const auto& sidx = database().get_index_type<my_seeder_index>().indices().get<by_seeder>();
   auto sitr = sidx.begin();
   while(sitr!=sidx.end()){
      idump((*sitr));
      ++sitr;
   }
   const auto& cidx = database().get_index_type<my_seeding_index>().indices().get<by_URI>();
   auto citr = cidx.begin();
   while(citr!=cidx.end()){
      idump((*citr));
      ++citr;
   }
   ilog("seeding plugin:  plugin_startup() start");
   my->restart_downloads();
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
   DInteger content_key;
   account_id_type seeder;
   uint64_t free_space;
   uint32_t price;
   if( options.count("seeder-private-key") || options.count("content-private-key") || options.count("seeder") || options.count("free-space") ) {
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


      if( options.count("content-private-key")) {
         try {
            content_key = decent::encrypt::DInteger::from_string(options["content-private-key"].as<string>());
         } catch( ... ) {
            FC_THROW("Invalid content private key ${key_string}",
                     ("key_string", options["content-private-key"].as<string>()));
         }
      } else {
         FC_THROW("missing content-private-key parameter");
      }
/*      if( options.count("packages-path")) {
         try {
            boost::filesystem::path master_path = boost::filesystem::path(options["packages-path"].as<string>());
            package_manager::instance().set_packages_path(master_path);
         } catch( ... ) {
            FC_THROW("Invalid packages path ${path_string}",
                     ("path_string", options["packages-path"].as<string>()));
         }
      } else {
         FC_THROW("missing packages-path parameter");
      } */
      if( options.count("seeding-price")) {
         price = options["seeding-price"].as<int>();
      } else{
         FC_THROW("missing seeding-price parameter");
      }
      if( options.count("seeder"))
         seeder = fc::variant(options["seeder"].as<string>()).as<account_id_type>();
      else
         FC_THROW("missing seeder parameter");

      if( options.count("free-space"))
         free_space = options["free-space"].as<int>();
      else
         FC_THROW("missing free-space parameter");
      
      ilog("starting service thread");
      my = unique_ptr<detail::seeding_plugin_impl>( new detail::seeding_plugin_impl( *this) );
      my->service_thread = std::make_shared<fc::thread>("seeding");
      my->main_thread = &fc::thread::current();

      database().on_new_commited_operation.connect( [&]( const operation_history_object& b ){ my->handle_commited_operation( b, false ); } );
      database().on_new_commited_operation_during_sync.connect( [&]( const operation_history_object& b ){
           my->handle_commited_operation(b, true); } );

      ilog("seeding plugin:  plugin_initialize() seeder prepared");
      try {
         database().create<my_seeder_object>([&](my_seeder_object &mso) {
              mso.seeder = seeder;
              mso.free_space = free_space;
              mso.content_privKey = content_key;
              mso.privKey = *private_key;
              mso.price = price;
         });
      }catch(...){}
   }
   ilog("seeding plugin:  plugin_initialize() end");
}FC_LOG_AND_RETHROW() }

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
         ("packages-path", bpo::value<string>(), "Packages storage path")
         ("seeding-price", bpo::value<int>(), "price per MegaBytes")
         ;
}

}}
