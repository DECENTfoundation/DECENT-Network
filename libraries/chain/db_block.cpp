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
#include <graphene/chain/db_with.hpp>
#include <graphene/chain/hardfork.hpp>

#include <graphene/chain/account_object.hpp>
#include <graphene/chain/block_summary_object.hpp>
#include <graphene/chain/global_property_object.hpp>
#include <graphene/chain/proposal_object.hpp>
#include <graphene/chain/transaction_object.hpp>
#include <graphene/chain/miner_object.hpp>
#include <graphene/chain/protocol/fee_schedule.hpp>
#include <graphene/chain/exceptions.hpp>

#include <fc/smart_ref_impl.hpp>

namespace graphene { namespace chain {

bool database::is_known_block( const block_id_type& id )const
{
   return _fork_db.is_known_block(id) || _block_id_to_block.contains(id);
}
/**
 * Only return true *if* the transaction has not expired or been invalidated. If this
 * method is called with a VERY old transaction we will return false, they should
 * query things by blocks if they are that old.
 */
bool database::is_known_transaction( const transaction_id_type& id )const
{
   const auto& trx_idx = get_index_type<transaction_index>().indices().get<by_trx_id>();
   return trx_idx.find( id ) != trx_idx.end();
}

block_id_type  database::get_block_id_for_num( uint32_t block_num )const
{ try {
   return _block_id_to_block.fetch_block_id( block_num );
} FC_CAPTURE_AND_RETHROW( (block_num) ) }

optional<signed_block> database::fetch_block_by_id( const block_id_type& id )const
{
   auto b = _fork_db.fetch_block( id );
   if( !b )
      return _block_id_to_block.fetch_optional(id);
   return b->data;
}

optional<signed_block> database::fetch_block_by_number( uint32_t num )const
{
   auto results = _fork_db.fetch_block_by_number(num);
   if( results.size() == 1 )
      return results[0]->data;
   else
      return _block_id_to_block.fetch_by_number(num);
}

const signed_transaction& database::get_recent_transaction(const transaction_id_type& trx_id) const
{
   auto& index = get_index_type<transaction_index>().indices().get<by_trx_id>();
   auto itr = index.find(trx_id);
   FC_ASSERT(itr != index.end());
   return itr->trx;
}

std::vector<block_id_type> database::get_block_ids_on_fork(block_id_type head_of_fork) const
{
  pair<fork_database::branch_type, fork_database::branch_type> branches = _fork_db.fetch_branch_from(head_block_id(), head_of_fork);
  if( !((branches.first.back()->previous_id() == branches.second.back()->previous_id())) )
  {
     edump( (head_of_fork)
            (head_block_id())
            (branches.first.size())
            (branches.second.size()) );
     assert(branches.first.back()->previous_id() == branches.second.back()->previous_id());
  }
  std::vector<block_id_type> result;
  for (const item_ptr& fork_block : branches.second)
    result.emplace_back(fork_block->id);
  result.emplace_back(branches.first.back()->previous_id());
  return result;
}

signed_block_with_info database::get_signed_block_with_info(const signed_block& block) const
{
   signed_block_with_info result;
   reinterpret_cast<signed_block&>(result) = block;
   result.block_id = block.id();
   result.signing_key = block.signee();
   result.transaction_ids.reserve( block.transactions.size() );
   for( const processed_transaction& tx : block.transactions )
      result.transaction_ids.push_back( tx.id() );

   share_type miner_pay_from_fees = get_miner_pay_from_fees_by_block_time(block.timestamp);
   share_type miner_pay_from_reward = get_asset_per_block_by_block_num(block.block_num());

   //this should never happen, but better check.
   if (miner_pay_from_fees < share_type(0))
      miner_pay_from_fees = share_type(0);

   result.miner_reward = miner_pay_from_fees + miner_pay_from_reward;
   return result;
}

/**
 * Push block "may fail" in which case every partial change is unwound.  After
 * push block is successful the block is appended to the chain database on disk.
 *
 * @return true if we switched forks as a result of this push.
 */
bool database::push_block(const signed_block &new_block, uint32_t skip, bool sync_mode)
{
   //idump((new_block.block_num())(new_block.id())(new_block.timestamp)(new_block.previous));
   bool result;
   detail::with_skip_flags( *this, skip, [&]()
   {
      detail::without_pending_transactions( *this, std::move(_pending_tx),
      [&]()
      {
         result = _push_block( new_block, sync_mode );
      });
   });
   return result;
}

bool database::_push_block(const signed_block &new_block, bool sync_mode)
{ try {
   uint32_t skip = get_node_properties().skip_flags;
   if( !(skip&skip_fork_db) )
   {
      /// TODO: if the block is greater than the head block and before the next maitenance interval
      // verify that the block signer is in the current set of active miners.

      shared_ptr<fork_item> new_head = _fork_db.push_block(new_block);
      //If the head block from the longest chain does not build off of the current head, we need to switch forks.
      if( new_head->data.previous != head_block_id() )
      {
         //If the newly pushed block is the same height as head, we get head back in new_head
         //Only switch forks if new_head is actually higher than head
         if( new_head->data.block_num() > head_block_num() )
         {
            wlog( "Switching to fork: ${id}", ("id",new_head->data.id()) );
            auto branches = _fork_db.fetch_branch_from(new_head->data.id(), head_block_id());

            // pop blocks until we hit the forked block
            while( head_block_id() != branches.second.back()->data.previous )
               pop_block();

            // push all blocks on the new fork
            for( auto ritr = branches.first.rbegin(); ritr != branches.first.rend(); ++ritr )
            {
                ilog( "pushing blocks from fork ${n} ${id}", ("n",(*ritr)->data.block_num())("id",(*ritr)->data.id()) );
                optional<fc::exception> except;
                try {
                   graphene::db::undo_database::session session = _undo_db.start_undo_session();
                   apply_block( (*ritr)->data, skip );
                   _block_id_to_block.store( (*ritr)->id, (*ritr)->data );
                   session.commit();
                }
                catch ( const fc::exception& e ) { except = e; }
                if( except )
                {
                   wlog( "exception thrown while switching forks ${e}", ("e",except->to_detail_string() ) );
                   // remove the rest of branches.first from the fork_db, those blocks are invalid
                   while( ritr != branches.first.rend() )
                   {
                      _fork_db.remove( (*ritr)->data.id() );
                      ++ritr;
                   }
                   _fork_db.set_head( branches.second.front() );

                   // pop all blocks from the bad fork
                   while( head_block_id() != branches.second.back()->data.previous )
                      pop_block();

                   // restore all blocks from the good fork
                   for( auto ritr = branches.second.rbegin(); ritr != branches.second.rend(); ++ritr )
                   {
                      auto session = _undo_db.start_undo_session();
                      apply_block( (*ritr)->data, skip );
                      _block_id_to_block.store( new_block.id(), (*ritr)->data );
                      session.commit();
                   }
                   throw *except;
                }
            }
            return true;
         }
         else return false;
      }
   }

   try {
      auto session = _undo_db.start_undo_session();
      apply_block(new_block, skip);
      _block_id_to_block.store(new_block.id(), new_block);
      session.commit();
      //we will notify after session commit, since we want to be sure that seeding plugin works and generated tx will refer to commited block_objects

   } catch ( const fc::exception& e ) {
      elog("Failed to push new block:\n${e}", ("e", e.to_detail_string()));
      _fork_db.remove(new_block.id());
      throw;
   }
   auto session = _undo_db.start_undo_session();
   try {
      uint32_t block_num = new_block.block_num();
      for( const auto &trx : new_block.transactions ) {
         for( const auto &op : trx.operations ) {
            if(sync_mode)
               on_new_commited_operation_during_sync(op, block_num);
            else
               on_new_commited_operation(op, block_num);
         }
      }
      session.merge();
   } catch ( const fc::exception& e ){
      // The plugin database might become inconsisten with the chain in case of exception.
      // However, the recovery of previous state is not an option as the block was accepted ok and we would desync with the rest of the network.
      // All we can do is to report and let the plugin get corrected.
      // The pop_block expects two commits in the _push_block() in all cases. TODO_DECENT
      session.merge();
      elog("Failed to notify listeners on commited operation:\n${e}", ("e", e.to_detail_string()));
   }
   return false;
} FC_CAPTURE_AND_RETHROW( (new_block) ) }

/**
 * Attempts to push the transaction into the pending queue
 *
 * When called to push a locally generated transaction, set the skip_block_size_check bit on the skip argument. This
 * will allow the transaction to be pushed even if it causes the pending block size to exceed the maximum block size.
 * Although the transaction will probably not propagate further now, as the peers are likely to have their pending
 * queues full as well, it will be kept in the queue to be propagated later when a new block flushes out the pending
 * queues.
 */
processed_transaction database::push_transaction( const signed_transaction& trx, uint32_t skip )
{ try {
   processed_transaction result;
   size_t tx_size = fc::raw::pack_size( trx );
   auto maximum_tx_size = get_global_properties().parameters.maximum_transaction_size;

   if(tx_size > maximum_tx_size)
      elog("Tx too big");
   FC_ASSERT(tx_size <= maximum_tx_size, "Transaction size is too big");
   detail::with_skip_flags( *this, skip, [&]()
   {
      result = _push_transaction( trx );
   } );
   return result;
} FC_CAPTURE_AND_RETHROW( (trx) ) }

processed_transaction database::_push_transaction( const signed_transaction& trx )
{
   // If this is the first transaction pushed after applying a block, start a new undo session.
   // This allows us to quickly rewind to the clean state of the head block, in case a new block arrives.
   if( !_pending_tx_session.valid() )
      _pending_tx_session = _undo_db.start_undo_session();

   // Create a temporary undo session as a child of _pending_tx_session.
   // The temporary session will be discarded by the destructor if
   // _apply_transaction fails.  If we make it to merge(), we
   // apply the changes.

   auto temp_session = _undo_db.start_undo_session();
   auto processed_trx = _apply_transaction( trx );
   _pending_tx.push_back(processed_trx);

   notify_changed_objects();
   // The transaction applied successfully. Merge its changes into the pending block session.
   temp_session.merge();

   // notify anyone listening to pending transactions
   on_pending_transaction( trx );
   return processed_trx;
}

processed_transaction database::validate_transaction( const signed_transaction& trx )
{
   auto session = _undo_db.start_undo_session();
   return _apply_transaction( trx );
}

processed_transaction database::push_proposal(const proposal_object& proposal)
{ try {
   transaction_evaluation_state eval_state(this);
   eval_state._is_proposed_trx = true;

   eval_state.operation_results.reserve(proposal.proposed_transaction.operations.size());
   processed_transaction ptrx(proposal.proposed_transaction);
   eval_state._trx = &ptrx;
   size_t old_applied_ops_size = _applied_ops.size();

   try {
      auto session = _undo_db.start_undo_session(true);
      for( auto& op : proposal.proposed_transaction.operations )
         eval_state.operation_results.emplace_back(apply_operation(eval_state, op));
      remove(proposal);
      session.merge();
   } catch ( const fc::exception& e ) {
      _applied_ops.resize( old_applied_ops_size );
      elog("Failed to push proposal:\n${e}", ("e", e.to_detail_string()));
      throw;
   }

   ptrx.operation_results = std::move(eval_state.operation_results);
   return ptrx;
} FC_CAPTURE_AND_RETHROW( (proposal) ) }

signed_block database::generate_block(
   fc::time_point_sec when,
   miner_id_type miner_id,
   const fc::ecc::private_key& block_signing_private_key,
   uint32_t skip /* = 0 */
   )
{ try {
   signed_block result;
   detail::with_skip_flags( *this, skip, [&]()
   {
      result = _generate_block( when, miner_id, block_signing_private_key );
   } );
   return result;
} FC_RETHROW() }

signed_block database::_generate_block(
   fc::time_point_sec when,
   miner_id_type miner_id,
   const fc::ecc::private_key& block_signing_private_key
   )
{
   try {
   uint32_t skip = get_node_properties().skip_flags;
   uint32_t slot_num = get_slot_at_time( when );
   FC_ASSERT( slot_num > 0 );
   miner_id_type scheduled_miner = get_scheduled_miner( slot_num );
   FC_ASSERT( scheduled_miner == miner_id );

   const auto& miner_obj = miner_id(*this);

   if( !(skip & skip_miner_signature) )
      FC_ASSERT( miner_obj.signing_key == block_signing_private_key.get_public_key() );

   static const size_t max_block_header_size = fc::raw::pack_size( signed_block_header() ) + 4;
   auto maximum_block_size = get_global_properties().parameters.maximum_block_size;
   size_t total_block_size = max_block_header_size;

   signed_block pending_block;

   //
   // The following code throws away existing pending_tx_session and
   // rebuilds it by re-applying pending transactions.
   //
   // This rebuild is necessary because pending transactions' validity
   // and semantics may have changed since they were received, because
   // time-based semantics are evaluated based on the current block
   // time.  These changes can only be reflected in the database when
   // the value of the "when" variable is known, which means we need to
   // re-apply pending transactions in this method.
   //
   _pending_tx_session.reset();
   _pending_tx_session = _undo_db.start_undo_session();

   uint64_t postponed_tx_count = 0;
   // pop pending state (reset to head block state)
   for( const processed_transaction& tx : _pending_tx )
   {
      size_t tx_size = fc::raw::pack_size( tx );
      size_t new_total_size = total_block_size + tx_size;

      // postpone transaction if it would make block too big
      if( new_total_size >= maximum_block_size )
      {
         postponed_tx_count++;
         continue;
      }

      try
      {
         auto temp_session = _undo_db.start_undo_session();
         processed_transaction ptx = _apply_transaction( tx );
         temp_session.merge();

         // We have to recompute pack_size(ptx) because it may be different
         // than pack_size(tx) (i.e. if one or more results increased
         // their size)
         total_block_size += fc::raw::pack_size( ptx );
         pending_block.transactions.push_back( ptx );
      }
      catch ( const fc::exception& e )
      {
         // Do nothing, transaction will not be re-applied
         wlog( "Transaction was not processed while generating block due to ${e}", ("e", e) );
         wlog( "The transaction was ${t}", ("t", tx) );
      }
   }
   if( postponed_tx_count > 0 )
   {
      wlog( "Postponed ${n} transactions due to block size limit", ("n", postponed_tx_count) );
   }

   _pending_tx_session.reset();

   // We have temporarily broken the invariant that
   // _pending_tx_session is the result of applying _pending_tx, as
   // _pending_tx now consists of the set of postponed transactions.
   // However, the push_block() call below will re-create the
   // _pending_tx_session.

   pending_block.previous = head_block_id();
   pending_block.timestamp = when;
   pending_block.transaction_merkle_root = pending_block.calculate_merkle_root();
   pending_block.miner = miner_id;

   if( !(skip & skip_miner_signature) )
      pending_block.sign( block_signing_private_key );

   // TODO:  Move this to _push_block() so session is restored.
   if( !(skip & skip_block_size_check) )
   {
      FC_ASSERT( fc::raw::pack_size(pending_block) <= get_global_properties().parameters.maximum_block_size );
   }

   push_block( pending_block, skip );

   return pending_block;
} FC_CAPTURE_AND_RETHROW( (miner_id) ) }

/**
 * Removes the most recent block from the database and
 * undoes any changes it made.
 */
void database::pop_block()
{ try {
   _pending_tx_session.reset();
   auto head_id = head_block_id();
   optional<signed_block> head_block = fetch_block_by_id( head_id );
   FC_VERIFY_AND_THROW( head_block.valid(), pop_empty_chain_exception, "there are no blocks to pop" );

   _fork_db.pop_block();
   _block_id_to_block.remove( head_id );
   pop_undo();

   _popped_tx.insert( _popped_tx.begin(), head_block->transactions.begin(), head_block->transactions.end() );

} FC_RETHROW() }

void database::clear_pending()
{ try {
   assert( (_pending_tx.size() == 0) || _pending_tx_session.valid() );
   _pending_tx.clear();
   _pending_tx_session.reset();
} FC_RETHROW() }

uint32_t database::push_applied_operation( const operation& op )
{
   _applied_ops.emplace_back(_current_op_info, op);
   return static_cast<uint32_t>(_applied_ops.size() - 1);
}
void database::set_applied_operation_result( uint32_t op_id, const operation_result& result )
{
   assert( op_id < _applied_ops.size() );
   _applied_ops[op_id].result = result;
}

//////////////////// private methods ////////////////////

void database::apply_block( const signed_block& next_block, uint32_t skip )
{
   auto block_num = next_block.block_num();
   if( _checkpoints.size() && _checkpoints.rbegin()->second != block_id_type() )
   {
      auto itr = _checkpoints.find( block_num );
      if( itr != _checkpoints.end() )
         FC_ASSERT( next_block.id() == itr->second, "Block did not match checkpoint", ("checkpoint",*itr)("block_id",next_block.id()) );

      if( _checkpoints.rbegin()->first >= block_num )
         skip = ~0;// WE CAN SKIP ALMOST EVERYTHING
   }

   detail::with_skip_flags( *this, skip, [&]()
   {
      _apply_block( next_block );
   } );
   return;
}

void database::_apply_block( const signed_block& next_block )
{ try {
   uint32_t next_block_num = next_block.block_num();
   uint32_t skip = get_node_properties().skip_flags;
   _applied_ops.clear();

   FC_ASSERT( (skip & skip_merkle_check) || next_block.transaction_merkle_root == next_block.calculate_merkle_root(), "", ("next_block.transaction_merkle_root",next_block.transaction_merkle_root)("calc",next_block.calculate_merkle_root())("next_block",next_block)("id",next_block.id()) );

   const miner_object& signing_miner = validate_block_header(skip, next_block);
   const auto& dynamic_global_props = get<dynamic_global_property_object>(dynamic_global_property_id_type());
   bool maint_needed = (dynamic_global_props.next_maintenance_time <= next_block.timestamp)  ;

   _current_op_info.block_num    = next_block_num;
   _current_op_info.trx_in_block = 0;

   for( const auto& trx : next_block.transactions )
   {
      /* We do not need to push the undo state for each transaction
       * because they either all apply and are valid or the
       * entire block fails to apply.  We only need an "undo" state
       * for transactions when validating broadcast transactions or
       * when building a block.
       */
      apply_transaction( trx, skip | skip_transaction_signatures );
      ++_current_op_info.trx_in_block;
   }

   update_global_dynamic_data(next_block);
   update_signing_miner(signing_miner, next_block);
   update_last_irreversible_block();

   // Are we at the maintenance interval?
   if( maint_needed )
      perform_chain_maintenance(next_block);

   create_block_summary(next_block);
   clear_expired_transactions();
   clear_expired_proposals();
   update_expired_feeds();
   update_withdraw_permissions();

   // n.b., update_maintenance_flag() happens this late
   // because get_slot_time() / get_slot_at_time() is needed above
   // TODO:  figure out if we could collapse this function into
   // update_global_dynamic_data() as perhaps these methods only need
   // to be called for header validation?
   update_maintenance_flag( maint_needed );
   update_miner_schedule();

   // notify observers that the block has been applied
   applied_block( next_block ); //emit

   MONITORING_COUNTER_VALUE(blocks_applied)++;
   MONITORING_COUNTER_VALUE(transactions_in_applied_blocks) += _current_op_info.trx_in_block;

   _applied_ops.clear();

   notify_changed_objects();
} FC_CAPTURE_AND_RETHROW( (next_block.block_num()) )  }

void database::notify_changed_objects()
{ try {
   if( _undo_db.enabled() )
   {
      const auto& head_undo = _undo_db.head();
      vector<graphene::db::object_id_type> changed_ids;
      changed_ids.reserve(head_undo.old_values.size() + head_undo.new_ids.size() + head_undo.removed.size());
      for( const auto& item : head_undo.old_values ) changed_ids.push_back(item.first);
      for( const auto& item : head_undo.new_ids ) changed_ids.push_back(item);
      for( const auto& item : head_undo.removed ) changed_ids.push_back(item.first);
      changed_objects(changed_ids);
   }
} FC_RETHROW() }

processed_transaction database::apply_transaction(const signed_transaction& trx, uint32_t skip)
{
   processed_transaction result;
   detail::with_skip_flags( *this, skip, [&]()
   {
      result = _apply_transaction(trx);
   });
   return result;
}

processed_transaction database::_apply_transaction(const signed_transaction& trx)
{ try {
   uint32_t skip = get_node_properties().skip_flags;

   if( true || !(skip&skip_validate) )   /* issue #505 explains why this skip_flag is disabled */
      trx.validate();

   auto& trx_idx = get_mutable_index_type<transaction_index>();
   const chain_id_type& chain_id = get_chain_id();
   auto trx_id = trx.id();
   FC_ASSERT( (skip & skip_transaction_dupe_check) ||
              trx_idx.indices().get<by_trx_id>().find(trx_id) == trx_idx.indices().get<by_trx_id>().end() );
   transaction_evaluation_state eval_state(this);
   const chain_parameters& chain_parameters = get_global_properties().parameters;
   eval_state._trx = &trx;

   if( !(skip & (skip_transaction_signatures | skip_authority_check) ) )
   {
      auto get_active = [&]( account_id_type id ) { return &id(*this).active; };
      auto get_owner  = [&]( account_id_type id ) { return &id(*this).owner;  };
      trx.verify_authority( trx.get_signature_keys(chain_id), get_active, get_owner, get_global_properties().parameters.max_authority_depth );
   }

   //Skip all manner of expiration and TaPoS checking if we're on block 1; It's impossible that the transaction is
   //expired, and TaPoS makes no sense as no blocks exist.
   if( BOOST_LIKELY(head_block_num() > 0) )
   {
      if( !(skip & skip_tapos_check) )
      {
         const auto& tapos_block_summary = block_summary_id_type( trx.ref_block_num )(*this);

         //Verify TaPoS block summary has correct ID prefix, and that this block's time is not past the expiration
         FC_ASSERT( trx.ref_block_prefix == tapos_block_summary.block_id._hash[1] );
      }

      fc::time_point_sec now = head_block_time();

      FC_ASSERT( trx.expiration <= now + chain_parameters.maximum_time_until_expiration, "",
                 ("trx.expiration",trx.expiration)("now",now)("max_til_exp",chain_parameters.maximum_time_until_expiration));
      FC_ASSERT( now <= trx.expiration, "", ("now",now)("trx.exp",trx.expiration) );
   }

   //Insert transaction into unique transactions database.
   if( !(skip & skip_transaction_dupe_check) )
   {
      create<transaction_object>([&](transaction_object& transaction) {
         transaction.trx_id = trx_id;
         transaction.trx = trx;
      });
   }

   eval_state.operation_results.reserve(trx.operations.size());

   //Finally process the operations
   processed_transaction ptrx(trx);
   _current_op_info.op_in_trx = 0;
   for( const auto& op : ptrx.operations )
   {
      eval_state.operation_results.emplace_back(apply_operation(eval_state, op));
      ++_current_op_info.op_in_trx;
   }
   ptrx.operation_results = std::move(eval_state.operation_results);

   //Make sure the temp account has no non-zero balances
   const auto& index = get_index_type<account_balance_index>().indices().get<by_account_asset>();
   auto range = index.equal_range( boost::make_tuple( GRAPHENE_TEMP_ACCOUNT ) );
   std::for_each(range.first, range.second, [](const account_balance_object& b) { FC_ASSERT(b.balance == 0); });

   return ptrx;
} FC_CAPTURE_AND_RETHROW( (trx) ) }

operation_result database::apply_operation(transaction_evaluation_state& eval_state, const operation& op)
{ try {
   int i_which = op.which();
   uint64_t u_which = uint64_t( i_which );
   if( i_which < 0 )
      assert( "Negative operation tag" && false );
   if( u_which >= _operation_evaluators.size() )
      assert( "No registered evaluator for this operation" && false );
   unique_ptr<op_evaluator>& eval = _operation_evaluators[ u_which ];
   if( !eval )
      assert( "No registered evaluator for this operation" && false );
   auto op_id = push_applied_operation( op );
   auto result = eval->evaluate( eval_state, op, true );
   set_applied_operation_result( op_id, result );
   return result;
} FC_CAPTURE_AND_RETHROW( (op) ) }

const miner_object& database::validate_block_header( uint32_t skip, const signed_block& next_block )const
{
   FC_ASSERT( head_block_id() == next_block.previous, "", ("head_block_id",head_block_id())("next.prev",next_block.previous) );
   FC_ASSERT( head_block_time() < next_block.timestamp, "", ("head_block_time",head_block_time())("next",next_block.timestamp)("blocknum",next_block.block_num()) );
   const miner_object& miner = next_block.miner(*this);

   if( !(skip&skip_miner_signature) )
      FC_ASSERT( next_block.validate_signee( miner.signing_key ) );

   if( !(skip&skip_miner_schedule_check) )
   {
      uint32_t slot_num = get_slot_at_time( next_block.timestamp );
      FC_ASSERT( slot_num > 0 );

      miner_id_type scheduled_miner = get_scheduled_miner( slot_num );

      FC_ASSERT( next_block.miner == scheduled_miner, "Miner produced block at wrong time",
                 ("block miner",next_block.miner)("scheduled",scheduled_miner)("slot_num",slot_num) );
   }

   return miner;
}

void database::create_block_summary(const signed_block& next_block)
{
   block_summary_id_type sid(next_block.block_num() & 0xffff );
   modify( sid(*this), [&](block_summary_object& p) {
         p.block_id = next_block.id();
   });
}

void database::add_checkpoints( const flat_map<uint32_t,block_id_type>& checkpts )
{
   for( const auto& i : checkpts )
      _checkpoints[i.first] = i.second;
}

bool database::before_last_checkpoint()const
{
   return (_checkpoints.size() > 0) && (_checkpoints.rbegin()->first >= head_block_num());
}

} }
