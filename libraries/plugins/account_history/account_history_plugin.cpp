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

#include <graphene/account_history/account_history_plugin.hpp>
#include <graphene/chain/protocol/fee_schedule.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/operation_history_object.hpp>
#include <graphene/chain/content_object.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/app/impacted.hpp>

namespace graphene { namespace account_history {

struct account_history_plugin::impl
{
      impl(account_history_plugin& _plugin) : _self( _plugin ) {}

      /** this method is called as a callback after a block is applied
       * and will process/index all operations that were applied in the block.
       */
      void update_account_histories( const graphene::chain::signed_block& b );

      graphene::chain::database& database()
      {
         return _self.database();
      }

      account_history_plugin& _self;
};

void account_history_plugin::impl::update_account_histories( const graphene::chain::signed_block& b )
{
   graphene::chain::database& db = database();
   for( const auto& o_op : db.get_applied_operations() )
   {
      // get the set of accounts this operation applies to
      fc::flat_set<graphene::chain::account_id_type> impacted;
      std::vector<graphene::chain::authority> other;
      operation_get_required_authorities( o_op.op, impacted, impacted, other );

      if( o_op.op.which() == graphene::chain::operation::tag<graphene::chain::account_create_operation>::value )
         impacted.insert( o_op.result.get<graphene::db::object_id_type>() );
      else if (o_op.op.which() == graphene::chain::operation::tag<graphene::chain::transfer_operation>::value ) {

         const graphene::chain::transfer_operation& tr2o = o_op.op.get<graphene::chain::transfer_operation>();
         if( tr2o.to.is<graphene::chain::account_id_type>() ) {
            impacted.insert( tr2o.from );
            impacted.insert( tr2o.to.as<graphene::chain::account_id_type>() );
         }
         else if ( tr2o.to.is<graphene::chain::content_id_type>() ) {
            auto& content_obj = db.get<graphene::chain::content_object>( tr2o.to.as<graphene::chain::content_id_type>() );

            impacted.insert( content_obj.author);
            for( auto& item : content_obj.co_authors )
               impacted.insert( item.first );
         }
      }
      else
         graphene::app::operation_get_impacted_accounts( o_op.op, impacted );

      for( auto& a : other )
         for( auto& item : a.account_auths )
            impacted.insert( item.first );

      auto accounts = graphene::chain::generic_evaluator::tracked_accounts_intersection(impacted.begin(), impacted.end());
      if(accounts.empty())
         return;

      const auto& oho = db.create<graphene::chain::operation_history_object>([&](graphene::chain::operation_history_object &h) {
         h.op           = o_op.op;
         h.result       = o_op.result;
         h.block_num    = o_op.block_num;
         h.trx_in_block = o_op.trx_in_block;
         h.op_in_trx    = o_op.op_in_trx;
         h.virtual_op   = o_op.virtual_op;
      });

      // for each operation this account applies to that is in the config link it into the history
      for( const auto& account_id : accounts )
      {
         // add history
         const auto& stats_obj = account_id(db).statistics(db);
         const auto& ath = db.create<graphene::chain::account_transaction_history_object>( [&]( graphene::chain::account_transaction_history_object& obj ){
               obj.operation_id = oho.id;
               obj.account = account_id;
               obj.sequence = stats_obj.total_ops+1;
               obj.next = stats_obj.most_recent_op;
         });
         db.modify( stats_obj, [&]( graphene::chain::account_statistics_object& obj ){
               obj.most_recent_op = ath.id;
               obj.total_ops = ath.sequence;
         });
      }
   }
}

account_history_plugin::account_history_plugin(graphene::app::application* app) : graphene::app::plugin(app)
{
   my.reset(new impl(*this));
}

account_history_plugin::~account_history_plugin()
{
}

std::string account_history_plugin::plugin_name()
{
   return "account_history";
}

void account_history_plugin::plugin_set_program_options(
   boost::program_options::options_description& cli,
   boost::program_options::options_description& cfg
   )
{
}

void account_history_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
   database().applied_block.connect( [&]( const graphene::chain::signed_block& b){ my->update_account_histories(b); } );
}

} }
