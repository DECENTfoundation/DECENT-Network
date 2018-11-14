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

#include <decent/monitoring/monitoring.hpp>

#include <fc/variant.hpp>
#include <fc/io/json.hpp>

#include <algorithm>



namespace monitoring {

   std::set<monitoring_counters_base*> monitoring_counters_base::registered_instances;
   std::mutex monitoring_counters_base::registered_instances_mutex;
   std::shared_ptr<std::thread> monitoring_counters_base::_monitoring_thread(nullptr);
   bool monitoring_counters_base::_end_thread = false;
   std::condition_variable monitoring_counters_base::cv;
   std::mutex  monitoring_counters_base::wait_mutex;
   bool monitoring_counters_base::_thread_is_running = false;
   std::vector<counter_item> monitoring_counters_base::_initializing_cache;
   bool monitoring_counters_base::_cache_is_loaded = false;
   std::vector<counter_item> monitoring_counters_base::_pending_save;


   void monitoring_counters_base::store_counters()
   {
      try {
         std::vector<counter_item> result;
         std::for_each(registered_instances.begin(), registered_instances.end(), [&](const monitoring_counters_base* this_ptr) {
            const std::vector<std::string> names;

            this_ptr->get_local_counters(names, result);
         });

         if (_pending_save.size()) {
            std::move(_pending_save.begin(), _pending_save.end(), result.end());
            _pending_save.clear();
         }

         save_to_disk(result);
      }
      catch (...) {
         // not clean if to handle this exceptions
      }
   }

   void monitoring_counters_base::initialize_existing_instances()
   {
      std::set<monitoring_counters_base*>::iterator existing_iter;
      std::lock_guard<std::mutex> lock(monitoring_counters_base::registered_instances_mutex);
      for (existing_iter = registered_instances.begin(); existing_iter != registered_instances.end(); ++existing_iter)
      {
         if ((*existing_iter)->_counters_initialized == false) {
            monitoring::counter_item* it = (*existing_iter)->get_first_counter();

            for (int i = 0; i < (*existing_iter)->get_counters_size(); i++) {
               std::for_each(_initializing_cache.begin(), _initializing_cache.end(), [&](monitoring::counter_item& item) {
                  if (it->name == item.name) {
                     (*it) = item;
                  }
               });
               it++;
            }
            (*existing_iter)->_counters_initialized = true;
         }
      }
   }

   void monitoring_counters_base::monitoring_thread_function()
   {
      read_from_disk(_initializing_cache);
      _cache_is_loaded = true;
      initialize_existing_instances();
      while (true) {
         std::unique_lock<std::mutex> lck(wait_mutex);
         if (cv.wait_for(lck, std::chrono::milliseconds(1000), [] {return _end_thread == true; }) == true)
            break;

         std::lock_guard<std::mutex> lock(monitoring_counters_base::registered_instances_mutex);
         store_counters();
      }
   }

   std::thread& monitoring_counters_base::start_monitoring_thread()
   {
      _monitoring_thread = std::make_shared<std::thread>(monitoring_thread_function);
      _thread_is_running = true;
      return *_monitoring_thread;
   }

   void monitoring_counters_base::stop_monitoring_thread()
   {
      if (!_thread_is_running)
         return;

      std::unique_lock<std::mutex> lck(wait_mutex);
      monitoring_counters_base::_end_thread = true;
      lck.unlock();
      cv.notify_one();

      _monitoring_thread->join();
   }

   void monitoring_counters_base::register_instance()
   {
      //std::lock_guard<std::mutex> lock(monitoring_counters_base::registered_instances_mutex); moved to constructor
      monitoring_counters_base::registered_instances.insert(this);
   }

   void monitoring_counters_base::unregister_instance()
   {
      std::lock_guard<std::mutex> lock(monitoring_counters_base::registered_instances_mutex);
      std::set<monitoring_counters_base*>::iterator it = registered_instances.find(this);
      if (it != registered_instances.end())
         registered_instances.erase(it);
   }

   void monitoring_counters_base::reset_counters(const std::vector<std::string>& names)
   {
      std::lock_guard<std::mutex> lock(monitoring_counters_base::registered_instances_mutex);
      std::set<monitoring_counters_base*>::iterator it;
      std::chrono::system_clock::time_point tp = std::chrono::system_clock::now();
       uint32_t seconds = std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch()).count();

      for (it = registered_instances.begin(); it != registered_instances.end(); ++it)
      {
         (*it)->reset_local_counters(seconds, names);
      }
   }

   void monitoring_counters_base::reset_local_counters_internal(uint32_t seconds, counter_item* first_counter, int size, const std::vector<std::string>& names)
   {
      monitoring::counter_item* it = (monitoring::counter_item*)first_counter;
      if (names.size() == 0) {

         for (int i = 0; i < size; i++) {
            it->last_reset = seconds;
            it->value = 0LL;
            it++;
         }
      }
      else
      {
         for (int i = 0; i < size; i++) {

            std::string val(it->name);
            std::vector<std::string>::const_iterator iter = std::find(names.begin(), names.end(), val);
            if (iter != names.end()) {

               it->last_reset = seconds;
               it->value = 0LL;
            }
            it++;
         }
      }
   }

   void monitoring_counters_base::get_counters(const std::vector<std::string>& names, std::vector<counter_item>& result)
   {
      std::lock_guard<std::mutex> lock(monitoring_counters_base::registered_instances_mutex);
      std::set<monitoring_counters_base*>::iterator it;
      for (it = registered_instances.begin(); it != registered_instances.end(); ++it)
      {
         (*it)->get_local_counters(names, result);
      }
   }

   void monitoring_counters_base::get_local_counters_internal(const counter_item* first_counter, int size, const std::vector<std::string>& names, std::vector<monitoring::counter_item>& result) const
   {
      const monitoring::counter_item* it = (monitoring::counter_item*)first_counter;
      if (names.size() == 0) {
         for (int i = 0; i < size; i++) {
            result.push_back(*it);
            it++;
         }
      }
      else {
         for (int i = 0; i < size; i++) {
            std::string val(it->name);
            std::vector<std::string>::const_iterator iter = std::find(names.begin(), names.end(), val);
            if (iter != names.end()) {
               result.push_back(*it);
            }
            it++;
         }
      }
   }

   bool monitoring_counters_base::load_local_counters_internal(counter_item* first_counter, int size)
   {
      monitoring::counter_item* it = (monitoring::counter_item*)first_counter;

      if (_cache_is_loaded == false)
         return false;

      for (int i = 0; i < size; i++) {
         std::for_each(_initializing_cache.begin(), _initializing_cache.end(), [&](monitoring::counter_item& item) {
            if (it->name == item.name) {
               (*it) = item;
            }
         });
         it++;
      }
      return true;
   }

   void monitoring_counters_base::save_local_counters_internal(counter_item* first_counter, int size)
   {
      monitoring::counter_item* it = (monitoring::counter_item*)first_counter;

      for (int i = 0; i < size; i++) {
         _pending_save.push_back(*it);
         it++;
      }
   }
}
