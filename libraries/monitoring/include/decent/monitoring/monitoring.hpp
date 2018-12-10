#pragma once
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

#include <vector>
#include <set>
#include <thread>
#include <condition_variable>
#include <string>

namespace monitoring {

   struct counter_item
   {
      std::string name;
      int64_t value;
      uint32_t last_reset;
      bool persistent;
   };

   struct counter_item_dependency
   {
      std::string name;
      std::string depends_on_name;
   };

   class monitoring_counters_base
   {
   public:
      monitoring_counters_base() : _counters_initialized(false) {}
      virtual ~monitoring_counters_base() {}
      
      static void reset_counters(const std::vector<std::string>& names);
      static void get_counters(const std::vector<std::string>& names, std::vector<counter_item>& result);

      static std::thread& start_monitoring_thread();
      static void stop_monitoring_thread();

   protected:
      static bool _end_thread;
      static std::thread* _monitoring_thread;
      static std::mutex wait_mutex;
      static std::set<monitoring_counters_base*> registered_instances;
      static std::mutex registered_instances_mutex;
      static std::condition_variable cv;
      static std::vector<counter_item> _initializing_cache;
      static bool _cache_is_loaded;
      bool _counters_initialized;
      static std::vector<counter_item> _pending_save;

      void register_instance();
      void unregister_instance();

      void reset_local_counters_internal(uint32_t seconds, counter_item* first_counter, int size, counter_item_dependency* first_dep, int dep_size, const std::vector<std::string>& names);
      void get_local_counters_internal(const counter_item* first_counter, int size, const std::vector<std::string>& names, bool only_persistent, std::vector<monitoring::counter_item>& result) const;
      bool load_local_counters_internal(counter_item* first_counter, int size);
      void save_local_counters_internal(counter_item* first_counter, int size);

      virtual void reset_local_counters(uint32_t seconds, const std::vector<std::string>& names) = 0;
      virtual void get_local_counters(const std::vector<std::string>& names, std::vector<monitoring::counter_item>& result, bool only_persistent) const = 0;
      virtual void load_local_counters() = 0;
      virtual void save_local_counters() = 0;
      virtual counter_item* get_first_counter() const = 0;
      virtual int get_counters_size() const = 0;

     
   private:
      static bool _thread_is_running;
      static void store_counters();
      static void save_to_disk(const std::vector<counter_item>& counters);
      static void read_from_disk(std::vector<counter_item>& counters);

      static void initialize_existing_instances();
      static void monitoring_thread_function();
      
   };
}

#define MONITORING_COUNTERS_CLASS_NAME(provider_class_name) monitoring_counters_##provider_class_name
#define PUBLIC_DERIVATION_FROM_MONITORING_CLASS(provider_class_name) , public MONITORING_COUNTERS_CLASS_NAME(provider_class_name)
#define PUBLIC_DERIVATION_FROM_ONLY_MONITORING_CLASS(provider_class_name) : public MONITORING_COUNTERS_CLASS_NAME(provider_class_name)

#define MONITORING_COUNTERS_BEGIN(provider_class_name) class MONITORING_COUNTERS_CLASS_NAME(provider_class_name) : public monitoring::monitoring_counters_base \
{ \
public: \
MONITORING_COUNTERS_CLASS_NAME(provider_class_name) () {std::lock_guard<std::mutex> lock(monitoring_counters_base::registered_instances_mutex); \
   load_local_counters(); register_instance();} \
virtual ~MONITORING_COUNTERS_CLASS_NAME(provider_class_name) () {std::lock_guard<std::mutex> lock(monitoring_counters_base::registered_instances_mutex); \
   save_local_counters();unregister_instance();} \
struct counters_array { 


#define MONITORING_COUNTERS_DEPENDENCIES } _counters; struct counters_dependencies_array { \


#define MONITORING_COUNTERS_END } _counters_dependencies; \
protected: \
void reset_local_counters(uint32_t seconds, const std::vector<std::string>& names) override { \
   monitoring_counters_base::reset_local_counters_internal(seconds, (monitoring::counter_item*)&_counters, sizeof(_counters)/sizeof(monitoring::counter_item), \
   (monitoring::counter_item_dependency*)&_counters_dependencies, sizeof(_counters_dependencies)/sizeof(monitoring::counter_item_dependency), names); \
} \
void get_local_counters(const std::vector<std::string>& names, std::vector<monitoring::counter_item>& result, bool only_persistent) const override { \
   monitoring_counters_base::get_local_counters_internal((const monitoring::counter_item*)&_counters, sizeof(_counters)/sizeof(monitoring::counter_item), \
   names, only_persistent, result); \
} \
void load_local_counters() override \
{ \
   _counters_initialized = monitoring_counters_base::load_local_counters_internal((monitoring::counter_item*)&_counters, sizeof(_counters)/sizeof(monitoring::counter_item)); \
} \
void save_local_counters() override \
{ \
   monitoring_counters_base::save_local_counters_internal((monitoring::counter_item*)&_counters, sizeof(_counters)/sizeof(monitoring::counter_item)); \
} \
monitoring::counter_item* get_first_counter() const override \
{ \
   return (monitoring::counter_item*)&_counters; \
} \
int get_counters_size() const override \
{ \
   return sizeof(_counters) / sizeof(monitoring::counter_item); \
} \
}; 

#define COUNTER_NAME_STR(name) #name
#define MONITORING_DEFINE_COUNTER(name) monitoring::counter_item name = {COUNTER_NAME_STR(name), 0LL, 0LL, true};
#define MONITORING_DEFINE_TRANSIENT_COUNTER(name) monitoring::counter_item name = {COUNTER_NAME_STR(name), 0LL, 0LL, false};
#define MONITORING_COUNTER_VALUE(name) _counters.name.value

#define MONITORING_COUNTER_DEPENDENCY(name,depends_on) monitoring::counter_item_dependency name = {COUNTER_NAME_STR(name), COUNTER_NAME_STR(depends_on)};





