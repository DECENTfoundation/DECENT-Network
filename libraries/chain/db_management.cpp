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

#include <graphene/chain/database.hpp>
#include <graphene/chain/global_property_object.hpp>
#include <graphene/chain/protocol/fee_schedule.hpp>
#include <boost/filesystem.hpp>
#include <fc/io/fstream.hpp>
#include <fstream>
#include <functional>
#include <iostream>

namespace graphene { namespace chain {

database::database(const std::vector< uint8_t >& object_type_count)
: object_database(object_type_count)
{
   initialize_indexes();
   initialize_evaluators();
}

database::~database()
{
   clear_pending();
}

void database::reindex(boost::filesystem::path data_dir, const genesis_state_type& initial_allocation)
{ try {
      dlog("reindexing blockchain");
      wipe(data_dir, false);
      open(data_dir, [&initial_allocation] {return initial_allocation; });

      auto start = fc::time_point::now();
      auto last_block = _block_id_to_block.last();
      if (!last_block) {
         reindexing_progress(100);
         dlog("no last block");
         return;
      }

      const auto last_block_num = last_block->block_num();

      ilog("Replaying blocks...");
      _undo_db.disable();
      double reindexing_status = 0.0;
      double one_perc_step = last_block_num / 100.0;
      for (uint32_t i = 1; i <= last_block_num; ++i)
      {
         if (reindexing_status <= (i - 1))
         {
            // report progress done so far
            auto progress = static_cast<uint8_t>((i - 1) * 100.0 / last_block_num);
            reindexing_progress(progress);
            reindexing_status += one_perc_step;
            ilog("${p}%: ${i}/${t}", ("p", progress) ("i", i - 1) ("t", last_block_num));
         }

         fc::optional< signed_block > block = _block_id_to_block.fetch_by_number(i);
         if (!block.valid())
         {
            wlog("Reindexing terminated due to gap:  Block ${i} does not exist!", ("i", i));
            uint32_t dropped_count = 0;
            while (true)
            {
               fc::optional< block_id_type > last_id = _block_id_to_block.last_id();
               // this can trigger if we attempt to e.g. read a file that has block #2 but no block #1
               if (!last_id.valid())
                  break;
               // we've caught up to the gap
               if (block_header::num_from_id(*last_id) <= i)
                  break;
               _block_id_to_block.remove(*last_id);
               dropped_count++;
            }
            wlog("Dropped ${n} blocks from after the gap", ("n", dropped_count));
            break;
         }
         apply_block(*block, skip_miner_signature |
            skip_transaction_signatures |
            skip_transaction_dupe_check |
            skip_tapos_check |
            skip_miner_schedule_check |
            skip_authority_check);

      }
      ilog("100%: ${t}/${t}", ("t", last_block_num));
      ilog("Done reindexing, elapsed time: ${t} sec", ("t", double((fc::time_point::now() - start).count()) / 1000000.0));
      reindexing_progress(100);
      _undo_db.enable();
} FC_CAPTURE_AND_RETHROW( (data_dir) ) }

void database::wipe(const boost::filesystem::path& data_dir, bool include_blocks)
{
   ilog("Wiping database (including blocks: ${blocks})", ("blocks", include_blocks));
   close();
   object_database::wipe(data_dir);
   if( include_blocks )
      remove_all( data_dir / "database" );
}

void database::open(
   const boost::filesystem::path& data_dir,
   std::function<genesis_state_type()> genesis_loader)
{
   try
   {
      object_database::open(data_dir );

      _block_id_to_block.open(data_dir / "database" / "block_num_to_block");

      if( !find(global_property_id_type()) )
         init_genesis(genesis_loader());

      fc::optional<signed_block> last_block = _block_id_to_block.last();
      if( last_block.valid() )
      {
         _fork_db.start_block( *last_block );
         ddump((last_block->id())(last_block->block_num()));
         if( last_block->id() != head_block_id() )
         {
            ddump((last_block));
            ddump((get( dynamic_global_property_id_type() )));
            ddump((_fork_db.head()->data));
            ddump((_fork_db.head()->num));

            FC_ASSERT( head_block_num() == 0, "last block ID does not match current chain state" );
         }
      }
   }
   FC_CAPTURE_LOG_AND_RETHROW( (data_dir) )
}

void database::close(bool rewind)
{
   // TODO:  Save pending tx's on close()
   clear_pending();
   // pop all of the blocks that we can given our undo history, this should
   // throw when there is no more undo history to pop
   if( rewind )
   {
      try
      {
         uint32_t cutoff = get_dynamic_global_properties().last_irreversible_block_num;
         while( head_block_num() > cutoff )
         {
            block_id_type popped_block_id = head_block_id();
            pop_block();
            _fork_db.remove(popped_block_id); // doesn't throw on missing
            try
            {
               _block_id_to_block.remove(popped_block_id);
            }
            catch (const fc::key_not_found_exception&)
            {
            }
         }
      } catch (const fc::exception &){
         //elog("database::close Exception caught");
         //elog( "${details}", ("details",er.to_detail_string()) );
      } catch (...)
      {
         //elog("database::close Exception caught");
      }
   }

   // Since pop_block() will move tx's in the popped blocks into pending,
   // we have to clear_pending() after we're done popping to get a clean
   // DB state (issue #336).
   clear_pending();

   object_database::flush();
   object_database::close();

   if( _block_id_to_block.is_open() )
      _block_id_to_block.close();

   _fork_db.reset();
}

} }
