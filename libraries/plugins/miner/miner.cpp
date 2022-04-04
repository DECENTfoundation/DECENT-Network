/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <graphene/miner/miner.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/miner_object.hpp>
#include <graphene/chain/protocol/fee_schedule.hpp>
#include <graphene/utilities/time.hpp>
#include <graphene/utilities/key_conversion.hpp>

#include <fc/thread/thread.hpp>

#include <iostream>

namespace bpo = boost::program_options;

namespace graphene { namespace miner_plugin {

void new_chain_banner( const graphene::chain::database& db )
{
   std::cerr << "\n"
      "********************************\n"
      "*                              *\n"
      "*   ------- NEW CHAIN ------   *\n"
      "*   -  Welcome to DECENT!  -   *\n"
      "*   ------------------------   *\n"
      "*                              *\n"
      "********************************\n"
      "\n";
   if( db.get_slot_at_time( graphene::utilities::now() ) > 200 )
   {
      std::cerr << "Your genesis seems to have an old timestamp\n"
         "Please consider using the --genesis-timestamp option to give your genesis a recent timestamp\n"
         "\n"
         ;
   }
}

miner_plugin::miner_plugin(graphene::app::application* app) : graphene::app::plugin(app) {}

miner_plugin::~miner_plugin()
{
   try {
      if( _block_production_task.valid() )
         _block_production_task.cancel_and_wait(__FUNCTION__);
   } catch(fc::canceled_exception&) {
      //Expected exception. Move along.
   } catch(fc::exception& e) {
      edump((e.to_detail_string()));
   }
}

void miner_plugin::plugin_set_program_options(
   boost::program_options::options_description& command_line_options,
   boost::program_options::options_description& config_file_options)
{
   auto default_priv_key = fc::ecc::private_key::regenerate(fc::sha256::hash(std::string("nathan")));
   graphene::db::object_id_type miner_id_example = chain::miner_id_type(5);
   command_line_options.add_options()
         ("enable-stale-production", bpo::bool_switch(), "Enable block production, even if the chain is stale.")
         ("required-miners-participation", bpo::value<uint32_t>()->default_value(33), "Percent of miners (0-99) that must be participating in order to produce blocks")
         ("miner-id,m", bpo::value<std::vector<std::string>>()->composing()->multitoken(),
          ("ID of miner controlled by this node (may specify multiple times), e.g. " + static_cast<std::string>(miner_id_example)).c_str())
         ("miner-private-key,k", bpo::value<std::vector<std::string>>()->composing()->multitoken(),
          ("Miner WIF private key (may specify multiple times), e.g. " + graphene::utilities::key_to_wif(default_priv_key)).c_str())
         ;
   config_file_options.add(command_line_options);
}

std::string miner_plugin::plugin_name()
{
   return "miner";
}

void miner_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{ try {
   if( options.count("enable-stale-production") )
   {
      _production_enabled = options["enable-stale-production"].as<bool>();
   }

   if( options.count("required-miners-participation") )
   {
      _required_miner_participation = std::min(options["required-miners-participation"].as<uint32_t>(), 99u) * GRAPHENE_1_PERCENT;
   }

   if( options.count("miner-id") )
   {
      const std::vector<std::string>& miners = options["miner-id"].as<std::vector<std::string>>();
      std::for_each(miners.begin(), miners.end(), [this](const std::string &miner) {
         try {
            graphene::db::object_id_type account(miner);
            FC_ASSERT( account.is<graphene::chain::miner_id_type>(), "Invalid miner account ${s}", ("s", miner) );
            _miners.insert(account);
         } FC_RETHROW_EXCEPTIONS(error, "Invalid argument: miner-id = ${s}", ("s", miner));
      });

      FC_ASSERT( options.count("miner-private-key"), "Missing miner private key" );
      const std::vector<std::string>& keys = options["miner-private-key"].as<std::vector<std::string>>();
      std::for_each(keys.begin(), keys.end(), [this](const std::string &key) {
         fc::optional<fc::ecc::private_key> private_key = graphene::utilities::wif_to_key(key);
         FC_ASSERT( private_key.valid(), "Invalid miner private key" );
         _private_keys[private_key->get_public_key()] = *private_key;
      });
   }
} FC_LOG_AND_RETHROW() }

void miner_plugin::plugin_startup()
{ try {
   chain::database& d = database();
   //Start NTP time client
   graphene::utilities::now();

   if( !_miners.empty() )
   {
      ilog("Launching block production for ${n} miners.", ("n", _miners.size()));
      app().set_block_production(true);
      if( _production_enabled )
      {
         if( d.head_block_num() == 0 )
            new_chain_banner(d);
         //_production_skip_flags |= graphene::chain::database::skip_undo_history_check;
         _production_skip_flags |= graphene::chain::database::skip_transaction_signatures;
      }
      schedule_production_loop();
   } else
      dlog("No miners configured! Please add miner IDs and private keys to configuration.");
} FC_RETHROW() }

void miner_plugin::plugin_shutdown()
{ try {
   graphene::utilities::shutdown_ntp_time();
} FC_RETHROW() }

void miner_plugin::schedule_production_loop()
{
   //Schedule for the next second's tick regardless of chain state
   // If we would wait less than 50ms, wait for the whole second.
   fc::time_point ntp_now = graphene::utilities::now();
   fc::time_point fc_now = fc::time_point::now();
   int64_t time_to_next_second = 1000000 - (ntp_now.time_since_epoch().count() % 1000000);
   if( time_to_next_second < 50000 )      // we must sleep for at least 50ms
       time_to_next_second += 1000000;

   fc::time_point next_wakeup( fc_now + fc::microseconds( time_to_next_second ) );

   //wdump( (now.time_since_epoch().count())(next_wakeup.time_since_epoch().count()) );
   _block_production_task = fc::schedule([this]{block_production_loop();},
                                         next_wakeup, "Miner Block Production");
}

block_production_condition::block_production_condition_enum miner_plugin::block_production_loop()
{
   block_production_condition::block_production_condition_enum result;
   fc::mutable_variant_object capture;
   try
   {
      result = maybe_produce_block(capture);
   }
   catch( const fc::canceled_exception& )
   {
      //We're trying to exit. Go ahead and let this one out.
      throw;
   }
   catch( const fc::exception& e )
   {
      elog("Got exception while generating block:\n${e}", ("e", e.to_detail_string()));
      result = block_production_condition::exception_producing_block;
   }

   switch( result )
   {
      case block_production_condition::produced:
         //ilog("Generated block #${n} with timestamp ${t} at time ${c}", (capture));
         ilog("Generated block #${n} with timestamp ${t} at time ${c} processed transactions ${trx_diff} total ${trx_total}", (capture));
         break;
      case block_production_condition::not_synced:
         ilog("Not producing block because production is disabled until we receive a recent block (see: --enable-stale-production)");
         break;
      case block_production_condition::not_my_turn:
         //ilog("Not producing block because it isn't my turn");
         break;
      case block_production_condition::not_time_yet:
         // ilog("Not producing block because slot has not yet arrived");
         break;
      case block_production_condition::no_private_key:
         ilog("Not producing block because I don't have the private key for ${scheduled_key}", (capture) );
         break;
      case block_production_condition::low_participation:
         elog("Not producing block because node appears to be on a minority fork with only ${pct}% miner participation", (capture) );
         break;
      case block_production_condition::lag:
         elog("Not producing block because node didn't wake up within 500ms of the slot time.");
         break;
      case block_production_condition::consecutive:
         elog("Not producing block because the last block was generated by the same miner.\nThis node is probably disconnected from the network so block production has been disabled.\nDisable this check with --allow-consecutive option.");
         break;
      case block_production_condition::exception_producing_block:
         break;
   }

   schedule_production_loop();
   return result;
}

block_production_condition::block_production_condition_enum miner_plugin::maybe_produce_block( fc::mutable_variant_object& capture )
{
   chain::database& db = database();
   fc::time_point now_fine = graphene::utilities::now();
   fc::time_point_sec now = now_fine + fc::microseconds( 500000 );

   // If the next block production opportunity is in the present or future, we're synced.
   if( !_production_enabled )
   {
      if( db.get_slot_time(1) >= now )
         _production_enabled = true;
      else
         return block_production_condition::not_synced;
   }

   // is anyone scheduled to produce now or one second in the future?
   uint32_t slot = db.get_slot_at_time( now );
   if( slot == 0 )
   {
      capture("next_time", db.get_slot_time(1));
      return block_production_condition::not_time_yet;
   }

   //
   // this assert should not fail, because now <= db.head_block_time()
   // should have resulted in slot == 0.
   //
   // if this assert triggers, there is a serious bug in get_slot_at_time()
   // which would result in allowing a later block to have a timestamp
   // less than or equal to the previous block
   //
   assert( now > db.head_block_time() );

   graphene::chain::miner_id_type scheduled_miner = db.get_scheduled_miner( slot );
   // we must control the miner scheduled to produce the next block.
   if( _miners.find( scheduled_miner ) == _miners.end() )
   {
      capture("scheduled_miner", scheduled_miner);
      return block_production_condition::not_my_turn;
   }

   fc::time_point_sec scheduled_time = db.get_slot_time( slot );
   graphene::chain::public_key_type scheduled_key = scheduled_miner( db ).signing_key;
   auto private_key_itr = _private_keys.find( scheduled_key );

   if( private_key_itr == _private_keys.end() )
   {
      capture("scheduled_key", scheduled_key);
      return block_production_condition::no_private_key;
   }

   uint32_t prate = db.miner_participation_rate();
   if( prate < _required_miner_participation )
   {
      capture("pct", prate / GRAPHENE_1_PERCENT);
      return block_production_condition::low_participation;
   }

   if( llabs((scheduled_time - now).count()) > fc::milliseconds( 500 ).count() )
   {
      capture("scheduled_time", scheduled_time)("now", now);
      return block_production_condition::lag;
   }

   auto block = db.generate_block(
      scheduled_time,
      scheduled_miner,
      private_key_itr->second,
      _production_skip_flags
      );
   //capture("n", block.block_num())("t", block.timestamp)("c", now);
   uint64_t trx_diff = (uint64_t)block.transactions.size();
   MONITORING_COUNTER_VALUE(transactions_in_generated_blocks) += trx_diff;
   MONITORING_COUNTER_VALUE(blocks_generated)++;
   capture("n", block.block_num())("t", block.timestamp)("c", now)("trx_diff", trx_diff)("trx_total", MONITORING_COUNTER_VALUE(transactions_in_generated_blocks));
   fc::async( [this,block](){ app().p2p_node()->broadcast(net::block_message(block)); } );

   return block_production_condition::produced;
}

} } // graphene::miner_plugin
