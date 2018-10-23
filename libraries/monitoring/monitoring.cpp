#include <decent/monitoring/monitoring.hpp>


namespace monitoring {

   std::set<monitoring_counters_base*> monitoring_counters_base::registered_instances;
   std::mutex monitoring_counters_base::registered_instances_mutex;
   std::thread monitoring_counters_base::monitoring_thread(monitoring_thread_function);

}