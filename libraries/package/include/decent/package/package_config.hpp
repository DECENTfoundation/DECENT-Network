/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
#pragma once
#include <string>

namespace decent { namespace package {

class PackageManagerConfigurator {
   PackageManagerConfigurator() = default;
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

   void set_ipfs_endpoint(const std::string &host, uint32_t port) { _ipfs_host = host; _ipfs_port = port; }

   uint32_t get_ipfs_port() const { return _ipfs_port; }
   const std::string& get_ipfs_host() const { return _ipfs_host; }

   PackageManagerConfigurator(const PackageManagerConfigurator&)             = delete;
   PackageManagerConfigurator(PackageManagerConfigurator&&)                  = delete;
   PackageManagerConfigurator& operator=(const PackageManagerConfigurator&)  = delete;
   PackageManagerConfigurator& operator=(PackageManagerConfigurator&&)       = delete;
};

}} //namespace decent::package
