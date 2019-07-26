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
#pragma once
#include <graphene/chain/node_property_object.hpp>
#include <graphene/chain/fork_database.hpp>
#include <graphene/chain/block_database.hpp>
#include <graphene/chain/genesis_state.hpp>
#include <graphene/chain/evaluator.hpp>

#include <graphene/db/object_database.hpp>
#include <graphene/db/object.hpp>
#include <fc/signals.hpp>

#include <fc/monitoring.hpp>
#include <fc/log/logger.hpp>

#include <map>

namespace graphene { namespace chain {
   class account_object;
   class asset_object;
   class global_property_object;
   class transaction_evaluation_state;

   struct miner_reward_input;
   struct real_supply;

   MONITORING_COUNTERS_BEGIN(database)
   MONITORING_DEFINE_COUNTER(blocks_applied)
   MONITORING_DEFINE_COUNTER(transactions_in_applied_blocks)
   MONITORING_COUNTERS_DEPENDENCIES
   MONITORING_COUNTERS_END

   /**
    *   @class database
    *   @brief tracks the blockchain state in an extensible manner
    */
   class database : public db::object_database PUBLIC_DERIVATION_FROM_MONITORING_CLASS(database)
   {
      public:
         //////////////////// db_management.cpp ////////////////////
         /**
          * @brief Constructor
          *
          * Opens a database in the specified directory. If no initialized database is found, genesis_loader is called
          * and its return value is used as the genesis state when initializing the new database
          *
          * @param object_type_count Counts of second level of two-dimensional array of database indexes
          */
         database(const std::vector< uint8_t >& object_type_count);
         ~database();
         bool is_undo_enabled(){ return _undo_db.enabled(); };

         enum validation_steps
         {
            skip_nothing                = 0,
            skip_miner_signature      = 1 << 0,  ///< used while reindexing
            skip_transaction_signatures = 1 << 1,  ///< used by non-miner nodes
            skip_transaction_dupe_check = 1 << 2,  ///< used while reindexing
            skip_fork_db                = 1 << 3,  ///< used while reindexing
            skip_block_size_check       = 1 << 4,  ///< used when applying locally generated transactions
            skip_tapos_check            = 1 << 5,  ///< used while reindexing -- note this skips expiration check as well
            skip_authority_check        = 1 << 6,  ///< used while reindexing -- disables any checking of authority on transactions
            skip_merkle_check           = 1 << 7,  ///< used while reindexing
            skip_assert_evaluation      = 1 << 8,  ///< used while reindexing
            skip_undo_history_check     = 1 << 9,  ///< used while reindexing
            skip_miner_schedule_check = 1 << 10,  ///< used while reindexing
            skip_validate               = 1 << 11 ///< used prior to checkpoint, skips validate() call on transaction
         };

         /**
          * @brief Open a database, creating a new one if necessary
          *
          * Opens a database in the specified directory. If no initialized database is found, genesis_loader is called
          * and its return value is used as the genesis state when initializing the new database
          *
          * genesis_loader will not be called if an existing database is found.
          *
          * @param data_dir Path to open or create database in
          * @param genesis_loader A callable object which returns the genesis state to initialize new databases on
          */
          void open(
             const boost::filesystem::path& data_dir,
             std::function<genesis_state_type()> genesis_loader );

         /**
          * @brief Rebuild object graph from block history and open detabase
          *
          * This method may be called after or instead of @ref database::open, and will rebuild the object graph by
          * replaying blockchain history. When this method exits successfully, the database will be open.
          */
         void reindex(boost::filesystem::path data_dir, const genesis_state_type& initial_allocation = genesis_state_type());

         /**
          * @brief wipe Delete database from disk, and potentially the raw chain as well.
          * @param data_dir
          * @param include_blocks If true, delete the raw chain as well as the database.
          *
          * Will close the database before wiping. Database will be closed when this function returns.
          */
         void wipe(const boost::filesystem::path& data_dir, bool include_blocks);
         void close(bool rewind = true);

         //////////////////// db_block.cpp ////////////////////

         /**
          *  @return true if the block is in our fork DB or saved to disk as
          *  part of the official chain, otherwise return false
          */
         bool                       is_known_block( const block_id_type& id )const;
         bool                       is_known_transaction( const transaction_id_type& id )const;
         block_id_type              get_block_id_for_num( uint32_t block_num )const;
         optional<signed_block>     fetch_block_by_id( const block_id_type& id )const;
         optional<signed_block>     fetch_block_by_number( uint32_t num )const;
         const signed_transaction&  get_recent_transaction( const transaction_id_type& trx_id )const;
         std::vector<block_id_type> get_block_ids_on_fork(block_id_type head_of_fork) const;

         /**
          *  Calculate the percent of block production slots that were missed in the
          *  past 128 blocks, not including the current block.
          */
         uint32_t miner_participation_rate()const;

         void                              add_checkpoints( const flat_map<uint32_t,block_id_type>& checkpts );
         const flat_map<uint32_t,block_id_type> get_checkpoints()const { return _checkpoints; }
         bool before_last_checkpoint()const;

         bool push_block(const signed_block &b, uint32_t skip = skip_nothing, bool sync_mode = false );
         //bool ( const signed_block& b, uint32_t skip = skip_nothing );
         processed_transaction push_transaction( const signed_transaction& trx, uint32_t skip = skip_nothing );
         bool _push_block(const signed_block &b, bool sync_mode = false );
         processed_transaction _push_transaction( const signed_transaction& trx );

         ///@throws fc::exception if the proposed transaction fails to apply.
         processed_transaction push_proposal( const proposal_object& proposal );

         signed_block generate_block(
            const fc::time_point_sec when,
            miner_id_type miner_id,
            const fc::ecc::private_key& block_signing_private_key,
            uint32_t skip
            );
         signed_block _generate_block(
            const fc::time_point_sec when,
            miner_id_type miner_id,
            const fc::ecc::private_key& block_signing_private_key
            );

         void pop_block();
         void clear_pending();

         /**
          *  This method is used to track applied operations during the evaluation of a block, these
          *  operations should include any operation actually included in a transaction as well
          *  as any implied/virtual operations that resulted, such as filling an order.  The
          *  applied operations is cleared after applying each block and calling the block
          *  observers which may want to index these operations.
          *
          *  @return the op_id which can be used to set the result after it has finished being applied.
          */
         uint32_t  push_applied_operation( const operation& op );
         void      set_applied_operation_result( uint32_t op_id, const operation_result& r );
         const vector<optional< operation_history_object > >& get_applied_operations()const;

         string to_pretty_string( const asset& a )const;

         /**
          * This signal is emitted for plugins to process every operation
          */
         fc::signal<void(const operation_history_object&)> on_applied_operation;
         fc::signal<void(const operation_history_object&)> on_new_commited_operation;
         fc::signal<void(const operation_history_object&)> on_new_commited_operation_during_sync;

         /**
          *  This signal is emitted after all operations and virtual operation for a
          *  block have been applied but before the get_applied_operations() are cleared.
          *
          *  You may not yield from this callback because the blockchain is holding
          *  the write lock and may be in an "inconstant state" until after it is
          *  released.
          */
         fc::signal<void(const signed_block&)>           applied_block;

         /**
          * This signal is emitted any time a new transaction is added to the pending
          * block state.
          */
         fc::signal<void(const signed_transaction&)>     on_pending_transaction;

         /**
          *  Emitted After a block has been applied and committed.  The callback
          *  should not yield and should execute quickly.
          */
         fc::signal<void(const vector<graphene::db::object_id_type>&)> changed_objects;

         //////////////////// db_miner_schedule.cpp ////////////////////

         /**
          * @brief Get the miner scheduled for block production in a slot.
          *
          * slot_num always corresponds to a time in the future.
          *
          * If slot_num == 1, returns the next scheduled miner.
          * If slot_num == 2, returns the next scheduled miner after
          * 1 block gap.
          *
          * Use the get_slot_time() and get_slot_at_time() functions
          * to convert between slot_num and timestamp.
          *
          * Passing slot_num == 0 returns GRAPHENE_NULL_MINER
          */
         miner_id_type get_scheduled_miner(uint32_t slot_num)const;

         /**
          * Get the time at which the given slot occurs.
          *
          * If slot_num == 0, return time_point_sec().
          *
          * If slot_num == N for N > 0, return the Nth next
          * block-interval-aligned time greater than head_block_time().
          */
         fc::time_point_sec get_slot_time(uint32_t slot_num)const;

         /**
          * Get the last slot which occurs AT or BEFORE the given time.
          *
          * The return value is the greatest value N such that
          * get_slot_time( N ) <= when.
          *
          * If no such N exists, return 0.
          */
         uint32_t get_slot_at_time(fc::time_point_sec when)const;

         void update_miner_schedule();

         //////////////////// db_getter.cpp ////////////////////

         const chain_id_type&                   get_chain_id()const;
         const asset_object&                    get_core_asset()const;
         const chain_property_object&           get_chain_properties()const;
         const global_property_object&          get_global_properties()const;
         const dynamic_global_property_object&  get_dynamic_global_properties()const;
         const node_property_object&            get_node_properties()const;
         const fee_schedule&                    current_fee_schedule()const;

         time_point_sec   head_block_time()const;
         uint32_t         head_block_num()const;
         block_id_type    head_block_id()const;
         miner_id_type    head_block_miner()const;

         decltype( chain_parameters::block_interval ) block_interval( )const;

         node_property_object& node_properties();


         uint32_t last_non_undoable_block_num() const;
         //////////////////// db_init.cpp ////////////////////

         void initialize_evaluators();
         /// Reset the object graph in-memory
         void initialize_indexes();
         void init_genesis(const genesis_state_type& genesis_state = genesis_state_type());

         template<typename EvaluatorType>
         void register_evaluator()
         {
            _operation_evaluators[
               operation::tag<typename EvaluatorType::operation_type>::value].reset( new op_evaluator_impl<EvaluatorType>() );
         }

         const vector< unique_ptr<op_evaluator> > & get_operation_evaluators() const
         {       
            return _operation_evaluators;
         }

         //////////////////// db_balance.cpp ////////////////////

         /**
          * @brief Retrieve a particular account's balance in a given asset
          * @param owner Account whose balance should be retrieved
          * @param asset_id ID of the asset to get balance in
          * @return owner's balance in asset
          */
         asset get_balance(account_id_type owner, asset_id_type asset_id)const;
         /// This is an overloaded method.
         asset get_balance(const account_object& owner, const asset_object& asset_obj)const;

         /**
          * @brief Adjust a particular account's balance in a given asset by a delta
          * @param account ID of account whose balance should be adjusted
          * @param delta Asset ID and amount to adjust balance by
          */
         void adjust_balance(account_id_type account, asset delta);

         /**
          * @brief Helper to make lazy deposit to CDD VBO.
          *
          * If the given optional VBID is not valid(),
          * or it does not have a CDD vesting policy,
          * or the owner / vesting_seconds of the policy
          * does not match the parameter, then credit amount
          * to newly created VBID and return it.
          *
          * Otherwise, credit amount to ovbid.
          * 
          * @return ID of newly created VBO, but only if VBO was created.
          */
         optional< vesting_balance_id_type > deposit_lazy_vesting(
            const optional< vesting_balance_id_type >& ovbid,
            share_type amount,
            uint32_t req_vesting_seconds,
            account_id_type req_owner,
            bool require_vesting );

         // helper to handle cashback rewards
         void deposit_cashback(const account_object& acct, share_type amount, bool require_vesting = true);
         // helper to handle miner pay
         void deposit_miner_pay(const miner_object& wit, share_type amount);

         /**
          * @brief Converts asset into DCT, using actual price feed.
          * @param price asset in DCT, monitored asset or user issued asset
          * @return price in DCT
          */
         asset price_to_dct( asset price );

         /**
          * @brief Tests whether two assets are exchangeable
          * This method does not check balance of asset pools
          * @param payment Asset
          * @param price Price of a content
          * @return true if the assets are exhangeable, false otherwise
          */
         bool are_assets_exchangeable( const asset_object& payment, const asset_object& price );

         //////////////////// db_decent.cpp ////////////////////

         /**
          * @brief Returns unused escrow from expired buying to consumer
          * @param buying Expired buying object
          */
         void buying_expire(const buying_object& buying);
         /**
          * @brief Returns unused publishing fee to author
          * @param content Expired content object
          */
         void content_expire(const content_object& content);
         /**
          * @brief Renewal of expired subscription
          * @param subscription Expired subscription object
          * @param subscription_period Extension of subscription, in days
          * @param price Price for subscription
          */
         void renew_subscription(const subscription_object& subscription, const uint32_t subscription_period, const asset price);
         /**
          * @brief Disallows automatic renewal of subscription if consumer doesn't have enought balance to renew
          * expired subscription
          * @param subscription Expired subscription object
          */
         void disallow_automatic_renewal_of_subscription(const subscription_object& subscription);
         void set_and_reset_seeding_stats();
         void decent_housekeeping();
         share_type get_new_asset_per_block();
         share_type get_asset_per_block_by_block_num(uint32_t block_num);
         miner_reward_input get_time_to_maint_by_block_time(fc::time_point_sec block_time);
         share_type get_miner_budget(uint32_t blocks);
         bool is_reward_switch_in_interval(uint64_t a, uint64_t b)const;
         uint64_t get_next_reward_switch_block(uint64_t start)const;

         real_supply get_real_supply()const;

         bool is_reward_switch_time() const;
         struct votes_gained{
            string account_name;
            uint64_t votes;
         };
         vector<votes_gained> get_actual_votes() const;

         /**
          *  This method validates transactions without adding it to the pending state.
          *  @return true if the transaction would validate
          */
         processed_transaction validate_transaction( const signed_transaction& trx );


         /** when popping a block, the transactions that were removed get cached here so they
          * can be reapplied at the proper time */
         std::deque< signed_transaction >       _popped_tx;

         uint32_t highest_know_block_number(){
            auto fhd = _fork_db.head();
            if(fhd)
               return fhd->data.block_num();
            return 0;
         }
         
         double get_reindexing_percent() { return _reindexing_percent; } // helper for retrieving reindexing progress - gui can use it
         void set_no_need_reindexing() { _reindexing_percent = 100; } // called from main when there is no need to reindex
         /**
          * @}
          */
   protected:
         //Mark pop_undo() as protected -- we do not want outside calling pop_undo(); it should call pop_block() instead
         void pop_undo() { object_database::pop_undo(); }
         void notify_changed_objects();

      private:
         optional<db::undo_database::session>   _pending_tx_session;
         vector< unique_ptr<op_evaluator> >     _operation_evaluators;

         template<class Index>
         vector<std::reference_wrapper<const typename Index::object_type>> sort_votable_objects()const;

         //////////////////// db_block.cpp ////////////////////

       public:
         // these were formerly private, but they have a fairly well-defined API, so let's make them public
         void                  apply_block( const signed_block& next_block, uint32_t skip = skip_nothing );
         processed_transaction apply_transaction( const signed_transaction& trx, uint32_t skip = skip_nothing );
         operation_result      apply_operation( transaction_evaluation_state& eval_state, const operation& op );
      private:
         void                  _apply_block( const signed_block& next_block );
         processed_transaction _apply_transaction( const signed_transaction& trx );

         ///Steps involved in applying a new block
         ///@{

         const miner_object& validate_block_header( uint32_t skip, const signed_block& next_block )const;
         const miner_object& _validate_block_header( const signed_block& next_block )const;
         void create_block_summary(const signed_block& next_block);

         //////////////////// db_update.cpp ////////////////////
         void update_global_dynamic_data( const signed_block& b );
         void update_signing_miner(const miner_object& signing_miner, const signed_block& new_block);
         void update_last_irreversible_block();
         void clear_expired_transactions();
         void clear_expired_proposals();
         void update_expired_feeds();
         void update_maintenance_flag( bool new_maintenance_flag );
         void update_withdraw_permissions();
         bool check_for_blackswan( const asset_object& mia, bool enable_black_swan = true );

         ///Steps performed only at maintenance intervals
         ///@{

         //////////////////// db_maint.cpp ////////////////////

         void process_budget();
         void perform_chain_maintenance(const signed_block& next_block, const global_property_object& global_props);
         void update_active_miners();

         template<class... Types>
         void perform_account_maintenance(std::tuple<Types...> helpers);
         ///@}
         ///@}

         vector< processed_transaction >        _pending_tx;
         fork_database                          _fork_db;

         /**
          *  Note: we can probably store blocks by block num rather than
          *  block id because after the undo window is past the block ID
          *  is no longer relevant and its number is irreversible.
          *
          *  During the "fork window" we can cache blocks in memory
          *  until the fork is resolved.  This should make maintaining
          *  the fork tree relatively simple.
          */
         block_database   _block_id_to_block;

         /**
          * Contains the set of ops that are in the process of being applied from
          * the current block.  It contains real and virtual operations in the
          * order they occur and is cleared after the applied_block signal is
          * emited.
          */
         vector<optional<operation_history_object> >  _applied_ops;

         uint32_t                          _current_block_num    = 0;
         uint16_t                          _current_trx_in_block = 0;
         uint16_t                          _current_op_in_trx    = 0;
         uint16_t                          _current_virtual_op   = 0;

         vector<uint64_t>                  _vote_tally_buffer;
         vector<uint64_t>                  _miner_count_histogram_buffer;
         uint64_t                          _total_voting_stake;

         flat_map<uint32_t,block_id_type>  _checkpoints;

         node_property_object              _node_property_object;
         double                            _reindexing_percent = 0;
   };

} }

FC_REFLECT(graphene::chain::database::votes_gained, (account_name)(votes))
