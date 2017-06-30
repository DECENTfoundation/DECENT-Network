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

#include <graphene/chain/protocol/authority.hpp>
#include <graphene/app/impacted.hpp>

namespace graphene { namespace app {

using namespace fc;
using namespace graphene::chain;

// TODO:  Review all of these, especially no-ops
struct get_impacted_account_visitor
{
   flat_set<account_id_type>& _impacted;
   get_impacted_account_visitor( flat_set<account_id_type>& impact ):_impacted(impact) {}
   typedef void result_type;

   void operator()( const transfer_operation& op )
   {
      _impacted.insert( op.to );
   }

   void operator()( const account_create_operation& op )
   {
      _impacted.insert( op.registrar );
      add_authority_accounts( _impacted, op.owner );
      add_authority_accounts( _impacted, op.active );
   }

   void operator()( const account_update_operation& op )
   {
      _impacted.insert( op.account );
      if( op.owner )
         add_authority_accounts( _impacted, *(op.owner) );
      if( op.active )
         add_authority_accounts( _impacted, *(op.active) );
   }

   void operator()( const asset_create_operation& op ) {}
   void operator()( const asset_update_operation& op )
   {
      if( op.new_issuer )
         _impacted.insert( *(op.new_issuer) );
   }

   void operator()( const asset_publish_feed_operation& op ) {}
   void operator()( const miner_create_operation& op )
   {
      _impacted.insert( op.miner_account );
   }
   void operator()( const miner_update_operation& op )
   {
      _impacted.insert( op.miner_account );
   }
   void operator()( const miner_update_global_parameters_operation& op ){}

   void operator()( const proposal_create_operation& op )
   {
      vector<authority> other;
      for( const auto& proposed_op : op.proposed_ops )
         operation_get_required_authorities( proposed_op.op, _impacted, _impacted, other );
      for( auto& o : other )
         add_authority_accounts( _impacted, o );
   }

   void operator()( const proposal_update_operation& op ) {}
   void operator()( const proposal_delete_operation& op ) {}

   void operator()( const withdraw_permission_create_operation& op )
   {
      _impacted.insert( op.authorized_account );
   }

   void operator()( const withdraw_permission_update_operation& op )
   {
      _impacted.insert( op.authorized_account );
   }

   void operator()( const withdraw_permission_claim_operation& op )
   {
      _impacted.insert( op.withdraw_from_account );
   }

   void operator()( const withdraw_permission_delete_operation& op )
   {
      _impacted.insert( op.authorized_account );
   }

   void operator()( const vesting_balance_create_operation& op )
   {
      _impacted.insert( op.owner );
   }

   void operator()( const vesting_balance_withdraw_operation& op ) {}
   void operator()( const custom_operation& op ) {}
   void operator()( const assert_operation& op ) {}
   void operator()( const set_publishing_manager_operation& op ) { _impacted.insert( op.from ); }
   void operator()( const set_publishing_right_operation& op ) { _impacted.insert( op.from ); }
   void operator()( const content_submit_operation& op) { _impacted.insert(op.author); }
   void operator()( const content_cancellation_operation& op) { _impacted.insert(op.author); }
   void operator()( const request_to_buy_operation& op) { _impacted.insert(op.consumer); }
   void operator()( const leave_rating_and_comment_operation& op) { _impacted.insert(op.consumer);}
   void operator()( const ready_to_publish_operation& op) { _impacted.insert(op.seeder); }
   void operator()( const proof_of_custody_operation& op) { _impacted.insert(op.seeder);}
   void operator()( const deliver_keys_operation& op) {  _impacted.insert(op.seeder);}
   void operator()( const subscribe_operation& op) { _impacted.insert(op.from); _impacted.insert(op.to); }
   void operator()( const subscribe_by_author_operation& op) { _impacted.insert(op.from); _impacted.insert(op.to); }
   void operator()( const automatic_renewal_of_subscription_operation& op) { _impacted.insert(op.consumer); }
   void operator()( const disallow_automatic_renewal_of_subscription_operation& op) { _impacted.insert(op.consumer); }
   void operator()( const renewal_of_subscription_operation& op) { _impacted.insert(op.consumer); }
   void operator()( const return_escrow_submission_operation& op) {  _impacted.insert(op.author);}
   void operator()( const return_escrow_buying_operation& op) {  _impacted.insert(op.consumer);}
   void operator()( const report_stats_operation& op) { _impacted.insert(op.consumer);}
   void operator()( const pay_seeder_operation& op) { _impacted.insert(op.author); _impacted.insert(op.seeder); };
   void operator()( const finish_buying_operation& op) { _impacted.insert(op.author); };
};

void operation_get_impacted_accounts( const operation& op, flat_set<account_id_type>& result )
{
   get_impacted_account_visitor vtor = get_impacted_account_visitor( result );
   op.visit( vtor );
}

void transaction_get_impacted_accounts( const transaction& tx, flat_set<account_id_type>& result )
{
   for( const auto& op : tx.operations )
      operation_get_impacted_accounts( op, result );
}

} }
