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
#include <graphene/chain/content_object.hpp>
#include <graphene/app/impacted.hpp>
#include <graphene/chain/config.hpp>
#include <graphene/chain/database.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/thread/thread.hpp>

#include <algorithm>
#include <iterator>
#include <regex>

namespace graphene { namespace transaction_history {
namespace detail
{

   class transaction_history_plugin_impl
   {
   public:
      transaction_history_plugin_impl(transaction_history_plugin& _plugin)
         : _self( _plugin )
      { }
      virtual ~transaction_history_plugin_impl();


      /** this method is called as a callback after a block is applied
       * and will process/index all transactions that were applied in the block.
       */
      void update_transaction_id_history( const signed_block& b );

      graphene::chain::database& database()
      {
         return _self.database();
      }

      transaction_history_plugin& _self;
      flat_set<account_id_type> _tracked_accounts;
   };

   transaction_history_plugin_impl::~transaction_history_plugin_impl()
   {
      return;
   }

   void transaction_history_plugin_impl::update_transaction_id_history( const signed_block& b )
   {
      graphene::chain::database& db = database();

      uint16_t tx_num = 0;
      for( const auto& tx : b.transactions )
      {
         flat_set<account_id_type> impacted;
         vector<authority> other;
         vector<account_id_type> common_data;

         for( const auto& op : tx.operations )
         {
            operation_get_required_authorities( op, impacted, impacted, other );

            for( auto& a : other )
               for( auto& item : a.account_auths )
                  impacted.insert( item.first );
         }

         if( _tracked_accounts.empty() )
         {
            db.create<transaction_history_object>( [&]( transaction_history_object& obj ){
               obj.tx_id = tx.id();
               obj.block_num = b.block_num();
               obj.trx_in_block = tx_num;
            });
         }
         else
         {
            std::set_intersection( impacted.begin(), impacted.end(),
               _tracked_accounts.begin(), _tracked_accounts.end(), std::back_inserter( common_data));
            if( !common_data.empty() )
            {
               db.create<transaction_history_object>( [&]( transaction_history_object& obj ){
                  obj.tx_id = tx.id();
                  obj.block_num = b.block_num();
                  obj.trx_in_block = tx_num;
               });
            }
         }

         tx_num++;
      }
   }

} // end namespace detail

transaction_history_plugin::transaction_history_plugin(graphene::app::application* app) : graphene::app::plugin(app),
   my( new detail::transaction_history_plugin_impl(*this) ) {}

transaction_history_plugin::~transaction_history_plugin() {}

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
   ilog("transaction history plugin:  plugin_initialize() begin");
   database().add_index< primary_index< transaction_history_index > >();

   if( options.at("transaction-id-history").as<bool>() )
   {
      database().applied_block.connect( [&]( const signed_block& b){ my->update_transaction_id_history(b); } );
      if (options.count("track-account")) {

         const std::vector<std::string>& ops = options["track-account"].as<std::vector<std::string>>();
         const std::regex rx("^\"1\.2\.[0-9]{1,15}\"$");// account id, for example "1.2.18"
         for (size_t i = 0; i < ops.size(); i++) {
            bool matches_reg_expr = std::regex_match(ops[i], rx);
            FC_ASSERT(matches_reg_expr, "Invalid argument: track-account = ${value}", ("value", ops[i]));
         }
         std::transform(ops.begin(), ops.end(), std::inserter(my->_tracked_accounts, my->_tracked_accounts.end()), &graphene::app::dejsonify<graphene::chain::account_id_type>);
      }
      ilog( "tracking of transaction IDs is enabled" );
   }
   else
      ilog( "tracking of transaction IDs is disabled" );

   ilog("transaction history plugin:  plugin_initialize() end");
}

void transaction_history_plugin::plugin_startup()
{
   ilog("transaction history plugin:  plugin_startup()");
}

flat_set<account_id_type> transaction_history_plugin::tracked_accounts() const
{
   return my->_tracked_accounts;
}

} }
