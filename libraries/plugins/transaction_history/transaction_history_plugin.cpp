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

#include <graphene/transaction_history/transaction_history_plugin.hpp>
#include <graphene/chain/protocol/fee_schedule.hpp>
#include <graphene/chain/transaction_history_object.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/app/impacted.hpp>

namespace graphene { namespace transaction_history {

   struct transaction_history_plugin::impl
   {
      impl(transaction_history_plugin& _plugin) : _self( _plugin ) {}

      /** this method is called as a callback after a block is applied
       * and will process/index all transactions that were applied in the block.
       */
      void update_transaction_id_history(const graphene::chain::signed_block& b);

      graphene::chain::database& database()
      {
         return _self.database();
      }

      transaction_history_plugin& _self;
   };

   void transaction_history_plugin::impl::update_transaction_id_history(const graphene::chain::signed_block& b)
   {
      graphene::chain::database& db = database();

      uint16_t tx_num = 0;
      for( const auto& tx : b.transactions )
      {
         fc::flat_set<graphene::chain::account_id_type> impacted;
         std::vector<graphene::chain::authority> other;

         for( const auto& op : tx.operations )
         {
            operation_get_required_authorities( op, impacted, impacted, other );

            for( auto& a : other )
               for( auto& item : a.account_auths )
                  impacted.insert( item.first );
         }

         if( std::any_of(impacted.begin(), impacted.end(), graphene::chain::generic_evaluator::is_account_tracked) )
         {
            db.create<graphene::chain::transaction_history_object>( [&]( graphene::chain::transaction_history_object& obj ){
               obj.tx_id = tx.id();
               obj.block_num = b.block_num();
               obj.trx_in_block = tx_num;
            });
         }

         tx_num++;
      }
   }

transaction_history_plugin::transaction_history_plugin(graphene::app::application* app) : graphene::app::plugin(app)
{
   my.reset(new impl(*this));
}

transaction_history_plugin::~transaction_history_plugin()
{
}

std::string transaction_history_plugin::plugin_name()
{
   return "transaction_id_history";
}

void transaction_history_plugin::plugin_set_program_options(
   boost::program_options::options_description& cli,
   boost::program_options::options_description& cfg
)
{
   cli.add_options()
      ("transaction-id-history",boost::program_options::bool_switch(), "Enable transaction lookup by transaction ID")
      ;
   cfg.add(cli);
}

void transaction_history_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
   if( options.at("transaction-id-history").as<bool>() )
   {
      database().applied_block.connect( [&](const graphene::chain::signed_block& b){ my->update_transaction_id_history(b); } );
      dlog( "tracking of transaction IDs is enabled" );
   }
   else
      dlog( "tracking of transaction IDs is disabled" );
}

} }
