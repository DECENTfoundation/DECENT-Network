#pragma once

#include <vector>
#include <set>
#include <mutex>

namespace monitoring {
   struct counter_item
   {
      const char* name;
      volatile int64_t value;
      int64_t last_reset;
   };

   void monitoring_thread_function();

   class monitoring_counters_base
   {
   public:
      monitoring_counters_base() {}
      virtual ~monitoring_counters_base() {}
      
      static std::set<monitoring_counters_base*> registered_instances;
      static std::mutex registered_instances_mutex;
      static std::thread monitoring_thread;

      void register_instance()
      {
         std::lock_guard<std::mutex> lock(monitoring_counters_base::registered_instances_mutex);
         monitoring_counters_base::registered_instances.insert(this);
      }

      void unregister_instance()
      {
         std::lock_guard<std::mutex> lock(monitoring_counters_base::registered_instances_mutex);
         std::set<monitoring_counters_base*>::iterator it = registered_instances.find(this);
         if (it != registered_instances.end())
            registered_instances.erase(it);
      }

      void reset(const std::vector<std::string>& names)
      {
         std::lock_guard<std::mutex> lock(monitoring_counters_base::registered_instances_mutex);
         std::set<monitoring_counters_base*>::iterator it;
         for (it = registered_instances.begin(); it != registered_instances.end(); ++it)
         {
            (*it)->reset_local_counters(names);
         }
      }
   protected:
      virtual void reset_local_counters(const std::vector<std::string>& names) = 0;

   };



#define MONITORING_COUNTERS_CLASS_NAME(provider_class_name) monitoring_counters_##provider_class_name
#define PUBLIC_DERIVATION_FROM_COUNTERS_CLASS(provider_class_name) , public MONITORING_COUNTERS_CLASS_NAME(provider_class_name)
#define PUBLIC_DERIVATION_FROM_ONLY_COUNTERS_CLASS(provider_class_name) : public MONITORING_COUNTERS_CLASS_NAME(provider_class_name)

#define MONITORING_COUNTERS_BEGIN(provider_class_name) class MONITORING_COUNTERS_CLASS_NAME(provider_class_name) : public monitoring_counters_base \
{ \
public: \
MONITORING_COUNTERS_CLASS_NAME(provider_class_name) (){register_instance();} \
virtual ~MONITORING_COUNTERS_CLASS_NAME(provider_class_name) () {unregister_instance();} \
struct counters_array { \


#define MONITORING_COUNTERS_END() } _counters; \
protected: \
void reset_local_counters(const std::vector<std::string>& names) override { \
   counter_item* it = (counter_item*)&_counters; \
   if(names.size() == 0) { \
      for (int i = 0; i < sizeof(_counters) / sizeof(counter_item); i++) { \
         it->last_reset = 0LL; \
         it->value = 0LL; \
         it++; \
      } \
   } else \
   {} \
   } \
}; 

#define COUNTER_NAME_STR(name) #name
#define MONITORING_DEFINE_COUNTER(name) counter_item name = {COUNTER_NAME_STR(name), 0LL, 0LL};



}