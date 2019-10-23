/*
 * Copyright (c) 2018 oxarbitrage, and contributors.
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
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/miner_object.hpp>
#include <graphene/chain/proposal_object.hpp>
#include <graphene/chain/withdraw_permission_object.hpp>
#include <graphene/chain/vesting_balance_object.hpp>
#include <graphene/chain/non_fungible_token_object.hpp>
#include <graphene/chain/global_property_object.hpp>
#include <graphene/chain/chain_property_object.hpp>
#include <graphene/chain/miner_schedule_object.hpp>
#include <graphene/chain/budget_record_object.hpp>

namespace decent { namespace elasticsearch {

std::string get_object_name(const std::string &object_type_name);

fc::mutable_variant_object adapt_object(fc::mutable_variant_object obj);
fc::mutable_variant_object adapt_operation(const graphene::chain::operation& op, const graphene::chain::database& db, fc::mutable_variant_object obj);
fc::mutable_variant_object adapt_authority(const fc::variant &v, const graphene::chain::database &db);
fc::mutable_variant_object adapt_account_options(const fc::variant &v, const graphene::chain::database &db, const graphene::chain::account_options &options);
fc::mutable_variant_object adapt_asset_options(const fc::variant &v, const graphene::chain::database &db, const graphene::chain::asset_options &options);
fc::mutable_variant_object adapt_monitored_asset_options(const fc::variant &v, const graphene::chain::database &db);
fc::mutable_variant_object adapt_non_fungible_token_options(const fc::variant &v, const graphene::chain::database &db);
fc::mutable_variant_object adapt_account(const fc::variant &id, const graphene::chain::database &db);
fc::mutable_variant_object adapt_miner(const fc::variant &id, const graphene::chain::database &db);
fc::variant adapt_votes(const graphene::chain::database &db, const fc::flat_set<graphene::chain::vote_id_type> &votes);
void adapt_asset(fc::variant &obj, const graphene::chain::database &db);

struct type_name_visitor : fc::visitor<const char*>
{
   template<typename T>
   result_type operator()(const T& v) const
   {
      return fc::get_typename<T>::name();
   }
};

template<typename T>
fc::mutable_variant_object adapt_static_variant(const T &v, const fc::variants &ar, const char *type_name = nullptr)
{
   fc::mutable_variant_object obj;
   obj.set("type", get_object_name(type_name ? type_name : v.visit(type_name_visitor())));

   FC_ASSERT(ar.size() == 2 && ar[0].as_int64() == v.which());
   const fc::variant &value = ar[1];
   value.is_object() ? obj(value.get_object()) : obj.set("value", value);

   return obj;
}

fc::mutable_variant_object adapt_chain_parameters(const graphene::chain::chain_parameters& params, const graphene::chain::database& db, fc::mutable_variant_object obj)
{
   fc::mutable_variant_object fee_schedule(obj["current_fees"]);
   fc::variants &fees = fee_schedule["parameters"].get_array();

   graphene::chain::operation op;
   auto it = params.current_fees->parameters.begin();
   for(std::size_t i = 0; i < fees.size(); ++i, ++it) {
      const graphene::chain::fee_parameters &v = *it;
      op.set_which(v.which());
      fees[i] = adapt_static_variant(v, fees[i].get_array(), op.visit(type_name_visitor()));
   }

   obj.set("current_fees", fees);
   obj.erase("extensions");
   return obj;
}

template<typename T>
fc::mutable_variant_object adapt(const T &obj, const graphene::chain::database &db)
{
   fc::variant v;
   fc::to_variant(obj, v);
   return adapt_object(v.get_object());
}

template<>
fc::mutable_variant_object adapt(const graphene::chain::account_object &obj, const graphene::chain::database &db)
{
   fc::variant v;
   fc::to_variant(obj, v);

   fc::mutable_variant_object o(v);
   o.set("registrar", adapt_account(o["registrar"], db));
   o.set("owner", adapt_authority(o["owner"], db));
   o.set("active", adapt_authority(o["active"], db));
   o.set("options", adapt_account_options(o["options"], db, obj.options));

   o.erase("rights_to_publish");
   o.erase("id");
   return o;
}

template<>
fc::mutable_variant_object adapt(const graphene::chain::account_balance_object &obj, const graphene::chain::database &db)
{
   fc::variant v;
   fc::to_variant(obj, v);

   fc::mutable_variant_object o(v);
   const auto& asset = db.cast<graphene::chain::asset_object>(db.get_object(graphene::db::object_id_type(obj.asset_type)));
   o.set("symbol", asset.symbol);
   o.set("precision", asset.precision);
   o.set("owner", adapt_account(o["owner"], db));
   o.erase("id");
   return o;
}

template<>
fc::mutable_variant_object adapt(const graphene::chain::account_statistics_object &obj, const graphene::chain::database &db)
{
   fc::variant v;
   fc::to_variant(obj, v);

   fc::mutable_variant_object o(v);
   o.set("owner", adapt_account(o["owner"], db));
   o.set("votes", adapt_votes(db, obj.votes));
   o.erase("most_recent_op");
   o.erase("id");
   return o;
}

template<>
fc::mutable_variant_object adapt(const graphene::chain::asset_object &obj, const graphene::chain::database &db)
{
   fc::variant v;
   fc::to_variant(obj, v);

   fc::mutable_variant_object o(v);
   o.set("issuer", adapt_account(o["issuer"], db));
   o.set("options", adapt_asset_options(o["options"], db, obj.options));
   if(obj.monitored_asset_opts.valid())
      o.set("monitored_asset_opts", adapt_monitored_asset_options(o["monitored_asset_opts"], db));

   o.erase("id");
   return o;
}

template<>
fc::mutable_variant_object adapt(const graphene::chain::miner_object &obj, const graphene::chain::database &db)
{
   fc::variant v;
   fc::to_variant(obj, v);

   fc::mutable_variant_object o(v);
   o.set("miner_account", adapt_account(o["miner_account"], db));
   for(auto &vote : o["votes_gained"].get_array()) {
      const fc::variants &ar = vote.get_array();
      fc::mutable_variant_object acc;
      acc.set("voter", adapt_account(ar[0], db));
      acc.set("votes", ar[1]);
      vote = std::move(acc);
   }

   o.erase("id");
   return o;
}

template<>
fc::mutable_variant_object adapt(const graphene::chain::miner_schedule_object &obj, const graphene::chain::database &db)
{
   fc::variant v;
   fc::to_variant(obj, v);

   fc::mutable_variant_object o(v);
   for(auto &acc : o["current_shuffled_miners"].get_array())
      acc = adapt_miner(acc, db);

   o.erase("id");
   return o;
}

template<>
fc::mutable_variant_object adapt(const graphene::chain::vesting_balance_object &obj, const graphene::chain::database &db)
{
   fc::variant v;
   fc::to_variant(obj, v);

   fc::mutable_variant_object o(v);
   o.set("owner", adapt_account(o["owner"], db));
   adapt_asset(o["balance"], db);
   fc::mutable_variant_object policy;
   policy.set("type", get_object_name(obj.policy.visit(type_name_visitor())));
   policy(o["policy"].get_array()[1].get_object());
   o.set("policy", std::move(policy));
   o.erase("id");
   return o;
}

template<>
fc::mutable_variant_object adapt(const graphene::chain::non_fungible_token_object &obj, const graphene::chain::database &db)
{
   fc::variant v;
   fc::to_variant(obj, v);

   fc::mutable_variant_object o(v);
   o.set("options", adapt_non_fungible_token_options(o["options"], db));
   o.erase("id");
   return o;
}

template<>
fc::mutable_variant_object adapt(const graphene::chain::non_fungible_token_data_object &obj, const graphene::chain::database &db)
{
   fc::variant v;
   fc::to_variant(obj, v);

   fc::mutable_variant_object o(v);
   o.set("owner", adapt_account(o["owner"], db));
   const auto& nft = db.cast<graphene::chain::non_fungible_token_object>(db.get_object(graphene::db::object_id_type(obj.nft_id)));
   o.set("symbol", nft.symbol);
   o.erase("id");
   return o;
}

template<>
fc::mutable_variant_object adapt(const graphene::chain::global_property_object &obj, const graphene::chain::database &db)
{
   fc::variant v;
   fc::to_variant(obj, v);

   fc::mutable_variant_object o(v);
   o.set("parameters", adapt_chain_parameters(obj.parameters, db, o["parameters"].get_object()));
   if(obj.pending_parameters.valid())
      o.set("pending_parameters", adapt_chain_parameters(*obj.pending_parameters, db, o["pending_parameters"].get_object()));
   for(auto &acc : o["active_miners"].get_array())
      acc = adapt_miner(acc, db);

   o.erase("id");
   return o;
}

template<>
fc::mutable_variant_object adapt(const graphene::chain::dynamic_global_property_object &obj, const graphene::chain::database &db)
{
   fc::variant v;
   fc::to_variant(obj, v);

   fc::mutable_variant_object o(v);
   if(obj.current_miner.instance != 0)
      o.set("current_miner", adapt_miner(o["current_miner"], db));
   else
      o.erase("current_miner");

   o.erase("id");
   return o;
}

template<>
fc::mutable_variant_object adapt(const graphene::chain::signed_block_with_info &obj, const graphene::chain::database &db)
{
   fc::variant v;
   fc::to_variant(obj, v);

   fc::mutable_variant_object o(v);
   o.set("miner", adapt_miner(o["miner"], db));
   o.set("transactions_count", static_cast<uint64_t>(obj.transaction_ids.size()));
   o.erase("block_id");
   o.erase("timestamp");
   o.erase("extensions");
   o.erase("transactions");
   return o;
}

template<>
fc::mutable_variant_object adapt(const graphene::chain::processed_transaction &obj, const graphene::chain::database &db)
{
   fc::variant v;
   fc::to_variant(obj, v);

   fc::mutable_variant_object o(v);
   o.set("operations_count", static_cast<uint64_t>(obj.operations.size()));

   fc::variants &results = o["operation_results"].get_array();
   for(std::size_t i = 0; i < results.size(); ++i)
         results[i] = adapt_static_variant(obj.operation_results[i], results[i].get_array());

   fc::variants &operations = o["operations"].get_array();
   for(std::size_t i = 0; i < operations.size(); ++i)
         operations[i] =  adapt_operation(obj.operations[i], db, adapt_static_variant(obj.operations[i], operations[i].get_array()));

   o.erase("extensions");
   return o;
}

} } // decent::elasticsearch
