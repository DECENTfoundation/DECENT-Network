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

#include <graphene/app/impacted.hpp>

#include <graphene/chain/account_evaluator.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/content_object.hpp>
#include <graphene/chain/config.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/operation_history_object.hpp>
#include <graphene/chain/transaction_evaluation_state.hpp>
#include <graphene/db/simple_index.hpp>
#include <fc/smart_ref_impl.hpp>
#include <fc/thread/thread.hpp>

#include <regex>

namespace graphene { namespace account_history {

namespace detail
{


class account_history_plugin_impl
{
   public:
      account_history_plugin_impl(account_history_plugin& _plugin)
         : _self( _plugin )
      { }
      virtual ~account_history_plugin_impl();


      /** this method is called as a callback after a block is applied
       * and will process/index all operations that were applied in the block.
       */
      void update_account_histories( const signed_block& b );

      graphene::chain::database& database()
      {
         return _self.database();
      }

      account_history_plugin& _self;
      flat_set<account_id_type> _tracked_accounts;
};

account_history_plugin_impl::~account_history_plugin_impl()
{
   return;
}

void account_history_plugin_impl::update_account_histories( const signed_block& b )
{
   graphene::chain::database& db = database();
   const vector<optional< operation_history_object > >& hist = db.get_applied_operations();
   for( const optional< operation_history_object >& o_op : hist )
   {
      // add to the operation history index
      const auto& oho = db.create<operation_history_object>( [&]( operation_history_object& h )
      {
         if( o_op.valid() )
            h = *o_op;
      } );

      if( !o_op.valid() )
      {
         ilog( "removing failed operation with ID: ${id}", ("id", oho.id) );
         db.remove( oho );
         continue;
      }

      const operation_history_object& op = *o_op;

      // get the set of accounts this operation applies to
      flat_set<account_id_type> impacted;
      vector<authority> other;
      operation_get_required_authorities( op.op, impacted, impacted, other );

      if( op.op.which() == operation::tag< account_create_operation >::value )
         impacted.insert( oho.result.get<graphene::db::object_id_type>() );
      else if (op.op.which() == operation::tag< transfer_operation >::value ) {

         const transfer_operation& tr2o = op.op.get<transfer_operation>();
         if( tr2o.to.is<account_id_type>() ) {
            impacted.insert( tr2o.from );
            impacted.insert( tr2o.to.as<graphene::chain::account_id_type>() );
         }
         else if ( tr2o.to.is<content_id_type>() ) {
            auto& content_obj = db.get<content_object>( tr2o.to.as<content_id_type>() );

            impacted.insert( content_obj.author);
            for( auto& item : content_obj.co_authors )
               impacted.insert( item.first );
         }
      }
      else
         graphene::app::operation_get_impacted_accounts( op.op, impacted );

      for( auto& a : other )
         for( auto& item : a.account_auths )
            impacted.insert( item.first );

      // for each operation this account applies to that is in the config link it into the history
      if( _tracked_accounts.empty() )
      {
         for( auto& account_id : impacted )
         {
            // we don't do index_account_keys here anymore, because
            // that indexing now happens in observers' post_evaluate()

            // add history
            const auto& stats_obj = account_id(db).statistics(db);
            const auto& ath = db.create<account_transaction_history_object>( [&]( account_transaction_history_object& obj ){
                obj.operation_id = oho.id;
                obj.account = account_id;
                obj.sequence = stats_obj.total_ops+1;
                obj.next = stats_obj.most_recent_op;
            });
            db.modify( stats_obj, [&]( account_statistics_object& obj ){
                obj.most_recent_op = ath.id;
                obj.total_ops = ath.sequence;
            });
         }
      }
      else
      {
         for( auto account_id : _tracked_accounts )
         {
            if( impacted.find( account_id ) != impacted.end() )
            {
               // add history
               const auto& stats_obj = account_id(db).statistics(db);
               const auto& ath = db.create<account_transaction_history_object>( [&]( account_transaction_history_object& obj ){
                  obj.operation_id = oho.id;
                  obj.account = account_id;
                  obj.sequence = stats_obj.total_ops+1;
                  obj.next = stats_obj.most_recent_op;
               });
               db.modify( stats_obj, [&]( account_statistics_object& obj ){
                   obj.most_recent_op = ath.id;
                   obj.total_ops = ath.sequence;
               });
            }
         }
      }
   }
}
} // end namespace detail

class param_validator_account_history
{
public:
   param_validator_account_history(const std::string param_name)
      : _name(param_name)
   {
   }

   void check_reg_expr(const std::regex& rx, const std::string& val)
   {
      bool matches_reg_expr = std::regex_match(val, rx);
      if(!matches_reg_expr)
         FC_THROW_EXCEPTION(fc::parse_error_exception, "Invalid argument: ${name} = ${value}", ("name", _name)("value", val));
   }

   void check_reg_expr(const std::regex& rx, const std::vector<std::string>& val)
   {
      for (size_t i = 0; i < val.size(); i++) {
         bool matches_reg_expr = std::regex_match(val[i], rx);
         if(!matches_reg_expr)
            FC_THROW_EXCEPTION(fc::parse_error_exception, "Invalid argument: ${name} = ${value}", ("name", _name)("value", val[i]));
      }
   }

   void operator()(const std::string& val)
   {
   }
   void operator()(const std::vector<std::string>& val)
   {
      if (_name == "track-account")
      {
         const std::regex rx("^\"1\\x2E2\\x2E[0-9]{1,15}\"$");// account id, for example "1.2.18"
         check_reg_expr(rx, val);
      } else
      if (_name == "miner-id")
      {
         const std::regex rx("^\"1\\x2E4\\x2E[0-9]{1,15}\"$");// miner id, for example "1.4.18"
         check_reg_expr(rx, val);
      }
   }

   std::string _name;
};



account_history_plugin::account_history_plugin(graphene::app::application* app) : graphene::app::plugin(app),
   my( new detail::account_history_plugin_impl(*this) )
{
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
   cli.add_options()
         ("track-account", boost::program_options::value<std::vector<std::string>>()->composing()->multitoken()->notifier(param_validator_account_history("track-account")), "Account ID to track history for (may specify multiple times)")
         ;
   cfg.add(cli);
}

void account_history_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
   database().applied_block.connect( [&]( const signed_block& b){ my->update_account_histories(b); } );
   database().add_index< graphene::db::primary_index< graphene::db::simple_index< operation_history_object > > >();
   database().add_index< graphene::db::primary_index< account_transaction_history_index > >();

   if (options.count("track-account")) {     
      const std::vector<std::string>& ops = options["track-account"].as<std::vector<std::string>>(); 
      std::transform(ops.begin(), ops.end(), std::inserter(my->_tracked_accounts, my->_tracked_accounts.end()), &graphene::app::dejsonify<graphene::chain::account_id_type>);
   }
}

void account_history_plugin::plugin_startup()
{
}

flat_set<account_id_type> account_history_plugin::tracked_accounts() const
{
   return my->_tracked_accounts;
}

} }
