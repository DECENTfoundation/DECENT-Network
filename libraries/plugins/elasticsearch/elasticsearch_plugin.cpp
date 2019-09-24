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

#include <graphene/elasticsearch/elasticsearch_plugin.hpp>
#include <curl/curl.h>
#include <condition_variable>
#include "adaptor.hpp"

namespace decent { namespace elasticsearch {

namespace {
   typedef void CurlHandler;

   struct CurlRequest {
      CurlHandler *handler;
      std::string url;
      std::string auth;
      const char* query = nullptr;
   };

   size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
   {
      ((std::string*)userp)->append((char*)contents, size * nmemb);
      return size * nmemb;
   }

   bool curl_call(const CurlRequest& req, const char* request, bool bulk = false)
   {
      curl_slist *headers = NULL;
      headers = curl_slist_append(headers, bulk ? "Content-Type: application/x-ndjson" : "Content-Type: application/json");

      curl_easy_setopt(req.handler, CURLOPT_HTTPHEADER, headers);
      curl_easy_setopt(req.handler, CURLOPT_URL, req.url.c_str());
      curl_easy_setopt(req.handler, CURLOPT_CUSTOMREQUEST, request);
      if(req.query) {
         curl_easy_setopt(req.handler, CURLOPT_POST, true);
         curl_easy_setopt(req.handler, CURLOPT_POSTFIELDS, req.query);
      }

      std::string result;
      curl_easy_setopt(req.handler, CURLOPT_WRITEFUNCTION, WriteCallback);
      curl_easy_setopt(req.handler, CURLOPT_WRITEDATA, (void *)&result);
      curl_easy_setopt(req.handler, CURLOPT_USERAGENT, "DCore");

      if(!req.auth.empty())
         curl_easy_setopt(req.handler, CURLOPT_USERPWD, req.auth.c_str());

      curl_easy_perform(req.handler);

      long http_code = 0;
      curl_easy_getinfo(req.handler, CURLINFO_RESPONSE_CODE, &http_code);

      if(http_code == 200) {
         if(bulk) {
            // all good, but check errors in response
            fc::variant v = fc::json::from_string(result);
            if(v["errors"].as_bool()) {
               elog("200 error: ${s}", ("s", fc::json::to_pretty_string(v)));
               return false;
            }
         }
      }
      else {
         elog("${c} error: ${s}", ("c", static_cast<int64_t>(http_code))("s", result));
         return false;
      }

      return true;
   }

   std::string prepare_bulk_header(std::string&& id, std::string&& index_name, const char* command)
   {
      fc::mutable_variant_object bulk_header;
      bulk_header["_id"] = id;
      bulk_header["_index"] = index_name;

      fc::mutable_variant_object final_bulk_header;
      final_bulk_header[command] = bulk_header;
      return fc::json::to_string(final_bulk_header);
   }

   std::string prepare_bulk_data(fc::mutable_variant_object obj, uint32_t block_number = 0, fc::time_point_sec block_time = fc::time_point_sec())
   {
      if(block_number) {
         obj["block_time"] = block_time;
         obj["block_number"] = block_number;
      }

      return fc::json::to_string(obj, fc::json::legacy_generator);
   }
}

class elasticsearch_thread
{
public:
   elasticsearch_thread(CURL *curl, const std::string &url, const std::string &basic_auth, std::size_t bulk_limit, uint32_t bulk_pause)
      : _req{curl, url + "/_bulk", basic_auth},
      _limit(bulk_limit),
      _pause(bulk_pause),
      _thread{ [this]() { run(); } }
   {
   }

   ~elasticsearch_thread()
   {
      if(_thread.joinable())
         _thread.join();
   }

   std::size_t limit() const { return _limit; }

   void stop()
   {
      std::unique_lock<std::mutex> lock(_mutex);
      _stop = true;
      _sync.notify_one();
   }

   bool push_bulk(std::deque<std::string> &lines, bool sync)
   {
      {
         std::unique_lock<std::mutex> lock(_mutex, std::try_to_lock);
         if(!lock)
            return false;

         // prepare bulk requests
         while(sync ? !lines.empty() : lines.size() >= _limit) {
            auto it = lines.begin();
            auto end = _limit == 0 || _limit > lines.size() ? lines.end() : it + _limit;

            _bulks.emplace_back(std::distance(it, end));
            std::for_each(it, end, [&query = _bulks.back().query](const std::string &line) { query.append(line).append(1, '\n'); });
            lines.erase(it, end);
         }
      }

      if(!_bulks.empty())
         _sync.notify_one();

      return true;
   }

private:
   struct Bulk {
      Bulk(std::size_t n = 0) { lines = n; total += n; }
      ~Bulk() { total -= lines; }

      std::string query;
      std::size_t lines = 0;
      static std::size_t total;
   };

   bool send_bulk()
   {
      {
         const Bulk &bulk = _bulks.front();
         _req.query = bulk.query.c_str();
         ilog("Sending data chunk of ${n} documents (total: ${t})", ("n", bulk.lines)("t", Bulk::total));
      }

      if(!curl_call(_req, "POST", true))
         return false;

      _req.query = nullptr;
      _bulks.pop_front();
      return true;
   }

   void run()
   {
      std::unique_lock<std::mutex> lock(_mutex);
      while(true) {
         _sync.wait(lock);
         if(_stop)
            return;

         while(!send_bulk() || !_bulks.empty())
            std::this_thread::sleep_for(std::chrono::seconds(_pause));
      }
   }

   CurlRequest _req;
   std::size_t _limit = 0;
   uint32_t _pause = 0;
   std::thread _thread;
   std::mutex _mutex;
   std::condition_variable _sync;
   std::list<Bulk> _bulks;
   bool _stop = false;
};

std::size_t elasticsearch_thread::Bulk::total = 0;

struct elasticsearch_plugin::impl
{
   impl(elasticsearch_plugin& _plugin, const std::string &_index_prefix);
   ~impl();

   template<std::size_t N, typename ...Types>
   struct object_inspector;

   template<std::size_t N>
   struct object_inspector<N>
   {
      static void inspect(std::size_t i, impl&, const graphene::db::object_id_type&)
      {
         FC_ASSERT(i < N, "Invalid index: ${i}", ("i", i));
      }
   };

   template<std::size_t N, typename T, typename ...Types>
   struct object_inspector<N, T, Types...>
   {
      static void inspect(std::size_t n, impl& instance, const graphene::db::object_id_type& id)
      {
         if(n == N)
            instance.inspect_obj<T>(id);
         else
            object_inspector<N + 1, Types...>::inspect(n, instance, id);
      }
   };

   template<typename ...Types>
   struct object_types {
      typedef std::tuple_size<std::tuple<Types...>> size;

      template<std::size_t N>
      using element_t = std::tuple_element_t<N, std::tuple<Types...>>;

      static void inspect(std::size_t n, impl& instance, const graphene::db::object_id_type& id)
      {
         object_inspector<0, Types...>::inspect(n, instance, id);
      }
   };

   using object_types_t = object_types<
      graphene::chain::account_object,
      graphene::chain::asset_object,
      graphene::chain::miner_object,
      graphene::chain::proposal_object,
      graphene::chain::withdraw_permission_object,
      graphene::chain::vesting_balance_object,
      graphene::chain::non_fungible_token_object,
      graphene::chain::non_fungible_token_data_object,
      graphene::chain::global_property_object,
      graphene::chain::dynamic_global_property_object,
      graphene::chain::asset_dynamic_data_object,
      graphene::chain::account_balance_object,
      graphene::chain::account_statistics_object,
      graphene::chain::chain_property_object,
      graphene::chain::miner_schedule_object,
      graphene::chain::budget_record_object
   >;

   void check(const std::string &url, const std::string &basic_auth, uint32_t bulk_limit, uint32_t bulk_pause);
   void stop();

   void reindex(uint8_t progress, const std::string &url, const std::string &basic_auth);
   void index_block(const graphene::chain::signed_block& block);
   bool index_objects(const std::vector<graphene::db::object_id_type>& ids);

   bool push_bulk(bool sync)
   {
      return (sync ? bulk_lines.empty() : bulk_lines.size() < worker->limit()) || worker->push_bulk(bulk_lines, sync);
   }

   template<typename T>
   std::string get_index_name() const
   {
      return index_prefix + get_object_name(fc::get_typename<T>::name());
   }

   template<typename T>
   void prepare_remove(const graphene::db::object_id_type& id)
   {
      bulk_lines.push_back(prepare_bulk_header(std::string(id), get_index_name<T>(), "delete"));
   }

   template<typename T>
   void prepare_bulk(const T& obj)
   {
      bulk_lines.push_back(prepare_bulk_header(std::string(obj.id), get_index_name<T>(), "index").append(1, '\n').append(prepare_bulk_data(adapt(obj, _db))));
   }

   template<typename T>
   void inspect_obj(const graphene::db::object_id_type& id)
   {
      const T* obj = _db.find<T>(id);
      FC_ASSERT(id.is<T>(), "Object ${id} can not be cast to ${t}", ("id", std::string(id))("t", typeid(T).name()));
      if(obj)
         prepare_bulk(*obj);
      else
         prepare_remove<T>(id);
   }

   template<typename T>
   void inspect_idx()
   {
      const graphene::db::index &idx = _db.get_index<T>();
      idx.inspect_all_objects([=](const graphene::db::object &o) {
         inspect_obj<T>(o.id);
      });
   }

   template<std::size_t... Idx>
   void inspect_all(std::index_sequence<Idx...>)
   {
      auto l = { (inspect_idx<object_types_t::element_t<Idx>>(), 0)... };
      (void)l;
   }

   template<std::size_t... Idx>
   void prepare_object_ids(std::index_sequence<Idx...>)
   {
      object_ids = { std::make_pair(object_types_t::element_t<Idx>::space_id, object_types_t::element_t<Idx>::type_id)... };
   }

private:
   graphene::chain::database& _db;
   std::string index_prefix;
   CURL *curl = nullptr;
   bool reindexing = false;
   uint32_t block_number = 0;
   fc::time_point_sec block_time;
   std::deque<std::string> bulk_lines;
   std::vector<std::pair<uint8_t, uint8_t>> object_ids;
   std::unique_ptr<elasticsearch_thread> worker;
};

elasticsearch_plugin::impl::impl(elasticsearch_plugin& _plugin, const std::string &_index_prefix)
   : _db(_plugin.database()), index_prefix(_index_prefix)
{
   curl = curl_easy_init();
   prepare_object_ids(std::make_index_sequence<object_types_t::size::value>());
}

elasticsearch_plugin::impl::~impl()
{
   if(curl) {
      curl_easy_cleanup(curl);
      curl = nullptr;
   }
}

void elasticsearch_plugin::impl::check(const std::string &url, const std::string &basic_auth, uint32_t bulk_limit, uint32_t bulk_pause)
{
   CurlRequest req{curl, url + "/_nodes", basic_auth};
   if(!curl_call(req, "GET"))
      FC_THROW_EXCEPTION(fc::exception, "Elasticsearch service is not up in url ${url}", ("url", url));

   worker.reset(new elasticsearch_thread(curl, url, basic_auth, bulk_limit, bulk_pause));
}

void elasticsearch_plugin::impl::stop()
{
   if(worker)
      worker->stop();

   worker.reset();
}

void elasticsearch_plugin::impl::reindex(uint8_t progress, const std::string &url, const std::string &basic_auth)
{
   reindexing = progress != 100;

   if(progress == 0) {
      CurlRequest req{curl, url + "/" + index_prefix + "*_object", basic_auth};
      if(!curl_call(req, "DELETE"))
         FC_THROW_EXCEPTION(fc::exception, "Error deleting object indices in url ${url}", ("url", url));
   }
   else if(progress == 100) {
      ilog("Inspecting object database");
      inspect_all(std::make_index_sequence<object_types_t::size::value>());
      if(!worker->push_bulk(bulk_lines, true))
         FC_THROW_EXCEPTION(fc::exception, "Error populating object indices.");
   }
}

void elasticsearch_plugin::impl::index_block(const graphene::chain::signed_block& block)
{
   block_number = block.block_num();
   block_time = block.timestamp;
   if(reindexing)
      return;

   bulk_lines.push_back(prepare_bulk_header(std::string(block.id()), "block_info", "index").append(1, '\n')
                  .append(prepare_bulk_data(adapt(_db.get_signed_block_with_info(block), _db), block_number, block_time)));

   for(const auto &trx : block.transactions)
      bulk_lines.push_back(prepare_bulk_header(std::string(trx.id()), "transaction_info", "index").append(1, '\n')
                     .append(prepare_bulk_data(adapt(trx, _db), block_number, block_time)));
}

bool elasticsearch_plugin::impl::index_objects(const std::vector<graphene::db::object_id_type>& ids)
{
   auto beg = object_ids.cbegin();
   auto end = object_ids.cend();
   for(const graphene::db::object_id_type& id : ids) {
      auto range = std::equal_range(beg, end, std::make_pair(id.space(), id.type()));
      if(range.first != end && std::distance(range.first, range.second) == 1)
         object_types_t::inspect(range.first - beg, *this, id);
   }

   auto max_sync_offset = fc::seconds(2 * _db.get_global_properties().parameters.block_interval);
   return push_bulk(fc::time_point::now() - block_time < max_sync_offset);
}

elasticsearch_plugin::elasticsearch_plugin(graphene::app::application* app) : graphene::app::plugin(app)
{
}

elasticsearch_plugin::~elasticsearch_plugin()
{
}

std::string elasticsearch_plugin::plugin_name()
{
   return "elasticsearch";
}

void elasticsearch_plugin::plugin_set_program_options(
   boost::program_options::options_description& cli,
   boost::program_options::options_description& cfg
   )
{
   cli.add_options()
         ("elasticsearch-api", boost::program_options::value<std::string>()->implicit_value("http://127.0.0.1:9200"), "Elasticsearch service endpoint")
         ("elasticsearch-bulk-limit", boost::program_options::value<uint32_t>()->default_value(16000), "Maximum number of documents to index in bulk")
         ("elasticsearch-bulk-pause", boost::program_options::value<uint32_t>()->default_value(2), "Pause between subsequent bulk requests (in seconds)")
         ("elasticsearch-basic-auth", boost::program_options::value<std::string>(), "Basic auth for Elasticsearch service")
         ("elasticsearch-index-prefix", boost::program_options::value<std::string>(), "Custom prefix for Elasticsearch database index")
         ;
   cfg.add(cli);
}

void elasticsearch_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
   if(!options.count("elasticsearch-api"))
      return;

   std::string url = options["elasticsearch-api"].as<std::string>();
   std::string basic_auth = options.count("elasticsearch-basic-auth") ? options["elasticsearch-basic-auth"].as<std::string>() : std::string();

   my.reset(new impl(*this, options.count("elasticsearch-index-prefix") ? options["elasticsearch-index-prefix"].as<std::string>() : std::string()));
   my->check(url, basic_auth, options["elasticsearch-bulk-limit"].as<uint32_t>(), options["elasticsearch-bulk-pause"].as<uint32_t>());

   database().reindexing_progress.connect([=](uint8_t progress) {
      my->reindex(progress, url, basic_auth);
   } );

   database().applied_block.connect([this](const graphene::chain::signed_block& block) {
      my->index_block(block);
   });

   database().changed_objects.connect([this](const std::vector<graphene::db::object_id_type>& ids) {
      if(!my->index_objects(ids))
         dlog("Elasticsearch worker is busy.");
   });
}

void elasticsearch_plugin::plugin_shutdown()
{
   if(!my)
      return;

   while(!my->push_bulk(true)) {
      wlog("Elasticsearch worker is busy.");
      std::this_thread::sleep_for(std::chrono::seconds(1));
   }

   my->stop();
}

} } // decent::elasticsearch
