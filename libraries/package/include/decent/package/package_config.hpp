#pragma once
#include <string>

namespace decent { namespace package {

class PackageManagerConfigurator{
private:
   explicit PackageManagerConfigurator() { };
   std::string _ipfs_host = "localhost";
   uint32_t    _ipfs_port = 5001;

public:
   /**
     * Returns singleton instance. The instance is created when the method is called the first time
     * @return singleton instance
     */
   static PackageManagerConfigurator& instance() {
      static PackageManagerConfigurator the_configurator;
      return the_configurator;
   }

   void set_ipfs_endpoint(std::string host, uint32_t port){ _ipfs_host = host; _ipfs_port = port; };

   uint32_t get_ipfs_port(){ return _ipfs_port; };
   std::string get_ipfs_host(){ return _ipfs_host; };


   PackageManagerConfigurator(const PackageManagerConfigurator&)             = delete;
   PackageManagerConfigurator(PackageManagerConfigurator&&)                  = delete;
   PackageManagerConfigurator& operator=(const PackageManagerConfigurator&)  = delete;
   PackageManagerConfigurator& operator=(PackageManagerConfigurator&&)       = delete;

   ~PackageManagerConfigurator() {};


};





}} //namespace decent::package