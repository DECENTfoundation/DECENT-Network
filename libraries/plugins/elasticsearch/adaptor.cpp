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
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/content_object.hpp>
#include <graphene/chain/miner_object.hpp>
#include <graphene/chain/non_fungible_token_object.hpp>
#include <graphene/chain/protocol/fee_schedule.hpp>
#include <algorithm>

namespace decent { namespace elasticsearch {

std::string get_object_name(const std::string &object_type_name)
{
   std::string::size_type i = object_type_name.find_last_of(':');
   return i == std::string::npos ? object_type_name : object_type_name.substr(i + 1);
}

bool check_malformed(const fc::variants &v)
{
   // guess array of static_variants, map, flat_map, etc.
   auto it = std::find_if_not(v.begin(), v.end(), [](const fc::variant &i) {
      return i.is_array() && i.get_array().size() == 2;
   });
   return it == v.end();
}

void adapt_array(fc::variants &v);

fc::mutable_variant_object adapt_object(fc::mutable_variant_object obj)
{
   for(fc::mutable_variant_object::entry &e : obj) {
      fc::variant &element = e.value();
      if(element.is_object())
         element = adapt_object(element.get_object());
      else if(element.is_array()) {
         fc::variants& va = element.get_array();
         if(va.size() == 2 && va[0].is_int64()) // guess static variant
            element = adapt_object(fc::mutable_variant_object(va[0].as_string(), va[1]));
         else
            adapt_array(va);
      }
      else if(e.key() == "nonce" || e.key() == "space")
         element = element.as_string();
   }

   obj.erase("id");
   return obj;
}

void adapt_array(fc::variants &v)
{
   bool malformed = check_malformed(v);
   for(fc::variant &element : v) {
      if(element.is_object())
         element = adapt_object(element.get_object());
      else if(element.is_array()) {
         fc::variants& va = element.get_array();
         if(malformed) {
            if(va[0].is_int64()) // guess array of static variants
               element = adapt_object(fc::mutable_variant_object(va[0].as_string(), va[1]));
            else { // guess map, flat_map, etc.
               fc::mutable_variant_object obj;
               obj("first", va[0])("second", va[1]);
               element = adapt_object(std::move(obj));
            }
         }
         else
            adapt_array(va);
      }
   }
}

void adapt_asset(fc::variant &obj, const graphene::chain::database &db)
{
   if(obj.is_object() && obj.get_object().contains("asset_id")) {
      fc::mutable_variant_object mo(obj.get_object());
      const auto& asset = db.cast<graphene::chain::asset_object>(db.get_object(graphene::db::object_id_type(mo["asset_id"].as_string())));
      mo.set("symbol", asset.symbol);
      mo.set("precision", asset.precision);
      obj = mo;
   }
}

fc::mutable_variant_object adapt_price(const fc::variant &v, const graphene::chain::database &db)
{
   fc::mutable_variant_object price(v);
   adapt_asset(price["base"], db);
   adapt_asset(price["quote"], db);
   return price;
}

fc::mutable_variant_object adapt_account(const fc::variant &id, const graphene::chain::database &db)
{
   const auto& account = db.cast<graphene::chain::account_object>(db.get_object(graphene::db::object_id_type(id.as_string())));
   return fc::mutable_variant_object("id", id)("name", account.name);
}

fc::mutable_variant_object adapt_miner(const fc::variant &id, const graphene::chain::database &db)
{
   const auto& miner = db.cast<graphene::chain::miner_object>(db.get_object(graphene::db::object_id_type(id.as_string())));
   const auto& account = db.cast<graphene::chain::account_object>(db.get_object(graphene::db::object_id_type(miner.miner_account)));
   return fc::mutable_variant_object("miner_id", id)("id", std::string(account.id))("name", account.name);
}

fc::mutable_variant_object adapt_authority(const fc::variant &v, const graphene::chain::database &db)
{
   fc::mutable_variant_object auth(v);
   for(fc::variant &item : auth["account_auths"].get_array()) {
      fc::mutable_variant_object o;
      const fc::variants &ar = item.get_array();
      o.set("account", adapt_account(ar[0], db));
      o.set("weight", ar[1]);
      item = std::move(o);
   }

   for(fc::variant &item : auth["key_auths"].get_array()) {
      fc::mutable_variant_object o;
      const fc::variants &ar = item.get_array();
      o.set("key", ar[0]);
      o.set("weight", ar[1]);
      item = std::move(o);
   }

   return auth;
}

fc::variant adapt_votes(const graphene::chain::database &db, const fc::flat_set<graphene::chain::vote_id_type> &votes)
{
   fc::variants v;
   const auto& miner_idx = db.get_index_type<graphene::chain::miner_index>().indices().get<graphene::chain::by_vote_id>();
   for(graphene::chain::vote_id_type vote_id : votes) {
      if(vote_id.type() == graphene::chain::vote_id_type::miner) {
         auto it = miner_idx.find(vote_id);
         if(it != miner_idx.end())
            v.push_back(adapt_miner(std::string(graphene::db::object_id_type(it->get_id())), db));
      }
   }

   return v;
}

fc::mutable_variant_object adapt_account_options(const fc::variant &v, const graphene::chain::database &db, const graphene::chain::account_options &options)
{
   fc::mutable_variant_object opt(v);
   opt.set("voting_account", adapt_account(opt["voting_account"], db));
   adapt_asset(opt["price_per_subscribe"], db);
   opt.set("votes", adapt_votes(db, options.votes));
   opt.erase("extensions");
   return opt;
}

fc::mutable_variant_object adapt_asset_options(const fc::variant &v, const graphene::chain::database &db, const graphene::chain::asset_options &options)
{
   fc::mutable_variant_object opt(v);
   opt.set("core_exchange_rate", adapt_price(opt["core_exchange_rate"], db));
   auto it = options.extensions.find(graphene::chain::asset_options::fixed_max_supply_struct());
   if(it != options.extensions.end())
      opt.set("is_fixed_max_supply", it->get<graphene::chain::asset_options::fixed_max_supply_struct>().is_fixed_max_supply);

   opt.erase("extensions");
   return opt;
}

fc::mutable_variant_object adapt_monitored_asset_options(const fc::variant &v, const graphene::chain::database &db)
{
   fc::mutable_variant_object opt(v);
   for(fc::variant &feed : opt["feeds"].get_array()) {
      const fc::variants& ar = feed.get_array();
      const fc::variants &ar2 = ar[1].get_array();
      fc::mutable_variant_object element;
      element.set("account", adapt_account(ar[0], db));
      element.set("published", ar2[0]);
      element.set("price", adapt_price(ar2[1], db));
      feed = std::move(element);
   }

   opt.set("current_feed", adapt_price(opt["current_feed"].get_object()["core_exchange_rate"], db));
   return opt;
}

fc::mutable_variant_object adapt_non_fungible_token_options(const fc::variant &v, const graphene::chain::database &db)
{
   fc::mutable_variant_object opt(v);
   opt.set("issuer", adapt_account(opt["issuer"], db));
   return opt;
}

fc::mutable_variant_object adapt_memo(const fc::variant &v)
{
   fc::mutable_variant_object memo(v);
   memo.set("nonce", memo["nonce"].as_string());
   return memo;
}

template<typename T>
fc::mutable_variant_object adapt_operation_type(const T& op, const graphene::chain::database& db, fc::mutable_variant_object& obj)
{
   obj.erase("extensions");
   return obj;
}

template<>
fc::mutable_variant_object adapt_operation_type(const graphene::chain::transfer_obsolete_operation& op, const graphene::chain::database& db, fc::mutable_variant_object& obj)
{
   obj.set("from", adapt_account(obj["from"], db));
   obj.set("to", adapt_account(obj["to"], db));
   adapt_asset(obj["amount"], db);
   if(op.memo.valid())
      obj.set("memo", adapt_memo(obj["memo"]));

   obj.erase("extensions");
   return obj;
}

template<>
fc::mutable_variant_object adapt_operation_type(const graphene::chain::account_create_operation& op, const graphene::chain::database& db, fc::mutable_variant_object& obj)
{
   obj.set("registrar", adapt_account(obj["registrar"], db));
   obj.set("owner", adapt_authority(obj["owner"], db));
   obj.set("active", adapt_authority(obj["active"], db));
   obj.set("options", adapt_account_options(obj["options"], db, op.options));
   obj.erase("extensions");
   return obj;
}

template<>
fc::mutable_variant_object adapt_operation_type(const graphene::chain::account_update_operation& op, const graphene::chain::database& db, fc::mutable_variant_object& obj)
{
   obj.set("account", adapt_account(obj["account"], db));
   if(op.owner.valid())
      obj.set("owner", adapt_authority(obj["owner"], db));
   if(op.active.valid())
      obj.set("active", adapt_authority(obj["active"], db));
   if(op.new_options.valid()) {
      obj.set("options", adapt_account_options(obj["new_options"], db, *op.new_options));
      obj.erase("new_options");
   }
   obj.erase("extensions");
   return obj;
}

template<>
fc::mutable_variant_object adapt_operation_type(const graphene::chain::asset_create_operation& op, const graphene::chain::database& db, fc::mutable_variant_object& obj)
{
   obj.set("issuer", adapt_account(obj["issuer"], db));
   obj.set("options", adapt_asset_options(obj["options"], db, op.options));
   if(op.monitored_asset_opts.valid())
      obj.set("monitored_asset_opts", adapt_monitored_asset_options(obj["monitored_asset_opts"], db));

   obj.erase("is_exchangeable");
   obj.erase("extensions");
   return obj;
}

template<>
fc::mutable_variant_object adapt_operation_type(const graphene::chain::asset_issue_operation& op, const graphene::chain::database& db, fc::mutable_variant_object& obj)
{
   obj.set("issuer", adapt_account(obj["issuer"], db));
   adapt_asset(obj["asset_to_issue"], db);
   if(op.memo.valid())
      obj.set("memo", adapt_memo(obj["memo"]));

   obj.erase("extensions");
   return obj;
}

template<>
fc::mutable_variant_object adapt_operation_type(const graphene::chain::asset_publish_feed_operation& op, const graphene::chain::database& db, fc::mutable_variant_object& obj)
{
   obj.set("publisher", adapt_account(obj["publisher"], db));
   const auto& asset = db.cast<graphene::chain::asset_object>(db.get_object(graphene::db::object_id_type(op.asset_id)));
   obj.set("symbol", asset.symbol);
   obj.set("precision", asset.precision);
   obj.erase("extensions");
   return obj;
}

template<>
fc::mutable_variant_object adapt_operation_type(const graphene::chain::miner_create_operation& op, const graphene::chain::database& db, fc::mutable_variant_object& obj)
{
   obj.set("miner_account", adapt_account(obj["miner_account"], db));
   return obj;
}

template<>
fc::mutable_variant_object adapt_operation_type(const graphene::chain::miner_update_operation& op, const graphene::chain::database& db, fc::mutable_variant_object& obj)
{
   obj.set("miner", adapt_miner(obj["miner"], db));
   obj.set("miner_account", adapt_account(obj["miner_account"], db));
   return obj;
}

template<>
fc::mutable_variant_object adapt_operation_type(const graphene::chain::proposal_create_operation& op, const graphene::chain::database& db, fc::mutable_variant_object& obj)
{
   obj.set("fee_paying_account", adapt_account(obj["fee_paying_account"], db));
   obj.erase("proposed_ops");
   obj.erase("extensions");
   return obj;
}

template<>
fc::mutable_variant_object adapt_operation_type(const graphene::chain::proposal_update_operation& op, const graphene::chain::database& db, fc::mutable_variant_object& obj)
{
   obj.set("fee_paying_account", adapt_account(obj["fee_paying_account"], db));
   for(auto &acc : obj["active_approvals_to_add"].get_array())
      acc = adapt_account(acc, db);
   for(auto &acc : obj["active_approvals_to_remove"].get_array())
      acc = adapt_account(acc, db);
   for(auto &acc : obj["owner_approvals_to_add"].get_array())
      acc = adapt_account(acc, db);
   for(auto &acc : obj["owner_approvals_to_remove"].get_array())
      acc = adapt_account(acc, db);
   obj.erase("extensions");
   return obj;
}

template<>
fc::mutable_variant_object adapt_operation_type(const graphene::chain::proposal_delete_operation& op, const graphene::chain::database& db, fc::mutable_variant_object& obj)
{
   obj.set("fee_paying_account", adapt_account(obj["fee_paying_account"], db));
   obj.erase("extensions");
   return obj;
}

template<>
fc::mutable_variant_object adapt_operation_type(const graphene::chain::withdraw_permission_create_operation& op, const graphene::chain::database& db, fc::mutable_variant_object& obj)
{
   obj.set("withdraw_from_account", adapt_account(obj["withdraw_from_account"], db));
   obj.set("authorized_account", adapt_account(obj["authorized_account"], db));
   adapt_asset(obj["withdrawal_limit"], db);
   return obj;
}

template<>
fc::mutable_variant_object adapt_operation_type(const graphene::chain::withdraw_permission_update_operation& op, const graphene::chain::database& db, fc::mutable_variant_object& obj)
{
   obj.set("withdraw_from_account", adapt_account(obj["withdraw_from_account"], db));
   obj.set("authorized_account", adapt_account(obj["authorized_account"], db));
   adapt_asset(obj["withdrawal_limit"], db);
   return obj;
}

template<>
fc::mutable_variant_object adapt_operation_type(const graphene::chain::withdraw_permission_claim_operation& op, const graphene::chain::database& db, fc::mutable_variant_object& obj)
{
   obj.set("withdraw_from_account", adapt_account(obj["withdraw_from_account"], db));
   obj.set("withdraw_to_account", adapt_account(obj["withdraw_to_account"], db));
   adapt_asset(obj["amount_to_withdraw"], db);
   if(op.memo.valid())
      obj.set("memo", adapt_memo(obj["memo"]));
   return obj;
}

template<>
fc::mutable_variant_object adapt_operation_type(const graphene::chain::withdraw_permission_delete_operation& op, const graphene::chain::database& db, fc::mutable_variant_object& obj)
{
   obj.set("withdraw_from_account", adapt_account(obj["withdraw_from_account"], db));
   obj.set("authorized_account", adapt_account(obj["authorized_account"], db));
   return obj;
}

template<>
fc::mutable_variant_object adapt_operation_type(const graphene::chain::vesting_balance_create_operation& op, const graphene::chain::database& db, fc::mutable_variant_object& obj)
{
   obj.set("creator", adapt_account(obj["creator"], db));
   obj.set("owner", adapt_account(obj["owner"], db));
   adapt_asset(obj["amount"], db);
   return obj;
}

template<>
fc::mutable_variant_object adapt_operation_type(const graphene::chain::vesting_balance_withdraw_operation& op, const graphene::chain::database& db, fc::mutable_variant_object& obj)
{
   obj.set("owner", adapt_account(obj["owner"], db));
   adapt_asset(obj["amount"], db);
   return obj;
}

template<>
fc::mutable_variant_object adapt_operation_type(const graphene::chain::custom_operation& op, const graphene::chain::database& db, fc::mutable_variant_object& obj)
{
   obj.set("payer", adapt_account(obj["payer"], db));
   for(auto &acc : obj["required_auths"].get_array())
      acc = adapt_account(acc, db);

   if(op.id == graphene::chain::custom_operation::custom_operation_subtype_messaging) {
      fc::mutable_variant_object msg(fc::json::from_string(std::string(op.data.begin(), op.data.end())));
      msg.set("from", adapt_account(msg["from"].as_string(), db));
      for(auto &rec : msg["receivers_data"].get_array()) {
         fc::mutable_variant_object mo(rec);
         mo.set("to", adapt_account(mo["to"].as_string(), db));
         mo.set("nonce", mo["nonce"].as_string());
         rec = std::move(mo);
      }

      obj.set("message", msg);
      obj.erase("data");
   }

   return obj;
}

template<>
fc::mutable_variant_object adapt_operation_type(const graphene::chain::assert_operation& op, const graphene::chain::database& db, fc::mutable_variant_object& obj)
{
   obj.set("fee_paying_account", adapt_account(obj["fee_paying_account"], db));
   for(auto &acc : obj["required_auths"].get_array())
      acc = adapt_account(acc, db);

   return obj;
}

template<>
fc::mutable_variant_object adapt_operation_type(const graphene::chain::content_submit_operation& op, const graphene::chain::database& db, fc::mutable_variant_object& obj)
{
   obj.set("author", adapt_account(obj["author"], db));
   for(auto &co : obj["co_authors"].get_array()) {
      const fc::variants &ar = co.get_array();
      fc::mutable_variant_object mo;
      mo.set("co_author", adapt_account(ar[0].as_string(), db));
      mo.set("points", ar[1]);
      co = std::move(mo);
   }

   for(auto &acc : obj["seeders"].get_array())
      acc = adapt_account(acc, db);

   adapt_asset(obj["publishing_fee"], db);
   return obj;
}

template<>
fc::mutable_variant_object adapt_operation_type(const graphene::chain::request_to_buy_operation& op, const graphene::chain::database& db, fc::mutable_variant_object& obj)
{
   obj.set("consumer", adapt_account(obj["consumer"], db));
   adapt_asset(obj["price"], db);
   return obj;
}

template<>
fc::mutable_variant_object adapt_operation_type(const graphene::chain::leave_rating_and_comment_operation& op, const graphene::chain::database& db, fc::mutable_variant_object& obj)
{
   obj.set("consumer", adapt_account(obj["consumer"], db));
   return obj;
}

template<>
fc::mutable_variant_object adapt_operation_type(const graphene::chain::ready_to_publish_obsolete_operation& op, const graphene::chain::database& db, fc::mutable_variant_object& obj)
{
   obj.set("seeder", adapt_account(obj["seeder"], db));
   obj.set("space", obj["space"].as_string());
   return obj;
}

template<>
fc::mutable_variant_object adapt_operation_type(const graphene::chain::proof_of_custody_operation& op, const graphene::chain::database& db, fc::mutable_variant_object& obj)
{
   obj.set("seeder", adapt_account(obj["seeder"], db));
   return obj;
}

template<>
fc::mutable_variant_object adapt_operation_type(const graphene::chain::deliver_keys_operation& op, const graphene::chain::database& db, fc::mutable_variant_object& obj)
{
   obj.set("seeder", adapt_account(obj["seeder"], db));
   return obj;
}

template<>
fc::mutable_variant_object adapt_operation_type(const graphene::chain::subscribe_operation& op, const graphene::chain::database& db, fc::mutable_variant_object& obj)
{
   obj.set("from", adapt_account(obj["from"], db));
   obj.set("to", adapt_account(obj["to"], db));
   adapt_asset(obj["price"], db);
   return obj;
}

template<>
fc::mutable_variant_object adapt_operation_type(const graphene::chain::subscribe_by_author_operation& op, const graphene::chain::database& db, fc::mutable_variant_object& obj)
{
   obj.set("from", adapt_account(obj["from"], db));
   obj.set("to", adapt_account(obj["to"], db));
   return obj;
}

template<>
fc::mutable_variant_object adapt_operation_type(const graphene::chain::automatic_renewal_of_subscription_operation& op, const graphene::chain::database& db, fc::mutable_variant_object& obj)
{
   obj.set("consumer", adapt_account(obj["consumer"], db));
   return obj;
}

template<>
fc::mutable_variant_object adapt_operation_type(const graphene::chain::report_stats_operation& op, const graphene::chain::database& db, fc::mutable_variant_object& obj)
{
   obj.set("consumer", adapt_account(obj["consumer"], db));
   for(auto &stat : obj["stats"].get_array()) {
      const fc::variants &ar = stat.get_array();
      fc::mutable_variant_object mo;
      mo.set("seeder", adapt_account(ar[0].as_string(), db));
      mo.set("size", ar[1]);
      stat = std::move(mo);
   }

   return obj;
}

template<>
fc::mutable_variant_object adapt_operation_type(const graphene::chain::set_publishing_manager_operation& op, const graphene::chain::database& db, fc::mutable_variant_object& obj)
{
   obj.set("from", adapt_account(obj["from"], db));
   for(auto &acc : obj["to"].get_array())
      acc = adapt_account(acc, db);

   return obj;
}

template<>
fc::mutable_variant_object adapt_operation_type(const graphene::chain::set_publishing_right_operation& op, const graphene::chain::database& db, fc::mutable_variant_object& obj)
{
   obj.set("from", adapt_account(obj["from"], db));
   for(auto &acc : obj["to"].get_array())
      acc = adapt_account(acc, db);

   return obj;
}

template<>
fc::mutable_variant_object adapt_operation_type(const graphene::chain::content_cancellation_operation& op, const graphene::chain::database& db, fc::mutable_variant_object& obj)
{
   obj.set("author", adapt_account(obj["author"], db));
   return obj;
}

template<>
fc::mutable_variant_object adapt_operation_type(const graphene::chain::asset_fund_pools_operation& op, const graphene::chain::database& db, fc::mutable_variant_object& obj)
{
   obj.set("from_account", adapt_account(obj["from_account"], db));
   adapt_asset(obj["uia_asset"], db);
   adapt_asset(obj["dct_asset"], db);
   obj.erase("extensions");
   return obj;
}

template<>
fc::mutable_variant_object adapt_operation_type(const graphene::chain::asset_reserve_operation& op, const graphene::chain::database& db, fc::mutable_variant_object& obj)
{
   obj.set("payer", adapt_account(obj["payer"], db));
   adapt_asset(obj["amount_to_reserve"], db);
   obj.erase("extensions");
   return obj;
}

template<>
fc::mutable_variant_object adapt_operation_type(const graphene::chain::asset_claim_fees_operation& op, const graphene::chain::database& db, fc::mutable_variant_object& obj)
{
   obj.set("issuer", adapt_account(obj["issuer"], db));
   adapt_asset(obj["uia_asset"], db);
   adapt_asset(obj["dct_asset"], db);
   obj.erase("extensions");
   return obj;
}

template<>
fc::mutable_variant_object adapt_operation_type(const graphene::chain::update_user_issued_asset_operation& op, const graphene::chain::database& db, fc::mutable_variant_object& obj)
{
   obj.set("issuer", adapt_account(obj["issuer"], db));
   const auto& asset = db.cast<graphene::chain::asset_object>(db.get_object(graphene::db::object_id_type(op.asset_to_update)));
   obj.set("symbol", asset.symbol);
   obj.set("precision", asset.precision);
   obj.erase("extensions");
   return obj;
}

template<>
fc::mutable_variant_object adapt_operation_type(const graphene::chain::update_monitored_asset_operation& op, const graphene::chain::database& db, fc::mutable_variant_object& obj)
{
   obj.set("issuer", adapt_account(obj["issuer"], db));
   const auto& asset = db.cast<graphene::chain::asset_object>(db.get_object(graphene::db::object_id_type(op.asset_to_update)));
   obj.set("symbol", asset.symbol);
   obj.set("precision", asset.precision);
   obj.erase("extensions");
   return obj;
}

template<>
fc::mutable_variant_object adapt_operation_type(const graphene::chain::ready_to_publish_operation& op, const graphene::chain::database& db, fc::mutable_variant_object& obj)
{
   obj.set("seeder", adapt_account(obj["seeder"], db));
   obj.set("space", obj["space"].as_string());
   obj.erase("extensions");
   return obj;
}

template<>
fc::mutable_variant_object adapt_operation_type(const graphene::chain::transfer_operation& op, const graphene::chain::database& db, fc::mutable_variant_object& obj)
{
   obj.set("from", adapt_account(obj["from"], db));
   if(op.to.is<graphene::chain::content_id_type>()) {
      const auto& content = db.cast<graphene::chain::content_object>(db.get_object(graphene::db::object_id_type(obj["to"].as_string())));
      obj.set("to", adapt_account(std::string(graphene::db::object_id_type(content.author)), db));
   }
   else
      obj.set("to", adapt_account(obj["to"], db));

   adapt_asset(obj["amount"], db);
   if(op.memo.valid())
      obj.set("memo", adapt_memo(obj["memo"]));

   obj.erase("extensions");
   return obj;
}

template<>
fc::mutable_variant_object adapt_operation_type(const graphene::chain::update_user_issued_asset_advanced_operation& op, const graphene::chain::database& db, fc::mutable_variant_object& obj)
{
   obj.set("issuer", adapt_account(obj["issuer"], db));
   const auto& asset = db.cast<graphene::chain::asset_object>(db.get_object(graphene::db::object_id_type(op.asset_to_update)));
   obj.set("symbol", asset.symbol);
   obj.set("precision", asset.precision);
   obj.erase("extensions");
   return obj;
}

template<>
fc::mutable_variant_object adapt_operation_type(const graphene::chain::non_fungible_token_create_definition_operation& op, const graphene::chain::database& db, fc::mutable_variant_object& obj)
{
   obj.set("options", adapt_non_fungible_token_options(obj["options"], db));
   obj.erase("extensions");
   return obj;
}

template<>
fc::mutable_variant_object adapt_operation_type(const graphene::chain::non_fungible_token_update_definition_operation& op, const graphene::chain::database& db, fc::mutable_variant_object& obj)
{
   obj.set("current_issuer", adapt_account(obj["current_issuer"], db));
   const auto& nft = db.cast<graphene::chain::non_fungible_token_object>(db.get_object(graphene::db::object_id_type(op.nft_id)));
   obj.set("symbol", nft.symbol);
   obj.set("options", adapt_non_fungible_token_options(obj["options"], db));
   obj.erase("extensions");
   return obj;
}

template<>
fc::mutable_variant_object adapt_operation_type(const graphene::chain::non_fungible_token_issue_operation& op, const graphene::chain::database& db, fc::mutable_variant_object& obj)
{
   obj.set("issuer", adapt_account(obj["issuer"], db));
   obj.set("to", adapt_account(obj["to"], db));
   const auto& nft = db.cast<graphene::chain::non_fungible_token_object>(db.get_object(graphene::db::object_id_type(op.nft_id)));
   obj.set("symbol", nft.symbol);
   obj.set("nft_values", obj["data"]);
   obj.erase("data");
   if(op.memo.valid())
      obj.set("memo", adapt_memo(obj["memo"]));
   obj.erase("extensions");
   return obj;
}

template<>
fc::mutable_variant_object adapt_operation_type(const graphene::chain::non_fungible_token_transfer_operation& op, const graphene::chain::database& db, fc::mutable_variant_object& obj)
{
   obj.set("from", adapt_account(obj["from"], db));
   obj.set("to", adapt_account(obj["to"], db));
   const auto& nft_data = db.cast<graphene::chain::non_fungible_token_data_object>(db.get_object(graphene::db::object_id_type(op.nft_data_id)));
   const auto& nft = db.cast<graphene::chain::non_fungible_token_object>(db.get_object(graphene::db::object_id_type(nft_data.nft_id)));
   obj.set("symbol", nft.symbol);
   if(op.memo.valid())
      obj.set("memo", adapt_memo(obj["memo"]));
   obj.erase("extensions");
   return obj;
}

template<>
fc::mutable_variant_object adapt_operation_type(const graphene::chain::non_fungible_token_update_data_operation& op, const graphene::chain::database& db, fc::mutable_variant_object& obj)
{
   obj.set("modifier", adapt_account(obj["modifier"], db));
   const auto& nft_data = db.cast<graphene::chain::non_fungible_token_data_object>(db.get_object(graphene::db::object_id_type(op.nft_data_id)));
   const auto& nft = db.cast<graphene::chain::non_fungible_token_object>(db.get_object(graphene::db::object_id_type(nft_data.nft_id)));
   obj.set("symbol", nft.symbol);

   for(auto &data : obj["data"].get_array()) {
      const fc::variants &ar = data.get_array();
      fc::mutable_variant_object value;
      value.set("name", ar[0]);
      value.set("value", ar[1].as_string());
      data = std::move(value);
   }

   obj.set("nft_data", obj["data"]);
   obj.erase("data");
   obj.erase("extensions");
   return obj;
}

struct adaptor_visitor : fc::visitor<fc::mutable_variant_object>
{
   adaptor_visitor(const graphene::chain::database &db, fc::mutable_variant_object &obj) : _db(db), _obj(obj) {}

   template<typename T>
   result_type operator()(const T& v) const
   {
      return adapt_operation_type(v, _db, _obj);
   }

   const graphene::chain::database& _db;
   fc::mutable_variant_object& _obj;
};

fc::mutable_variant_object adapt_operation(const graphene::chain::operation& op, const graphene::chain::database& db, fc::mutable_variant_object obj)
{
   auto it = obj.find("fee");
   if(it != obj.end())
      adapt_asset(it->value(), db);

   return op.visit(adaptor_visitor(db, obj));
}

} } // decent::elasticsearch
