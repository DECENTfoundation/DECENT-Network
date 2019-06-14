/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */

#pragma once

#include <boost/filesystem/path.hpp>
#include <fc/optional.hpp>

namespace graphene { namespace utilities {
   
   class decent_path_finder {
   private:
      // Constructor may throw exceptions. Which is bad in general, but if it fails program execution must be terminated
      decent_path_finder();
      decent_path_finder(const decent_path_finder&) = delete;
      
   public:
      static decent_path_finder& instance() {
         static decent_path_finder theChoosenOne;
         return theChoosenOne;
      }
      
   public:
      boost::filesystem::path get_user_home()   const { return _user_home; }

      boost::filesystem::path get_decent_home() const { return _decent_home; }
      boost::filesystem::path get_decent_data() const { return _decent_data; }
      boost::filesystem::path get_decent_logs() const { return _decent_logs; }
      boost::filesystem::path get_decent_temp() const { return _decent_temp; }
      boost::filesystem::path get_ipfs_bin() const { return _ipfs_bin; }
      boost::filesystem::path get_ipfs_path() const { return _ipfs_path; }
      boost::filesystem::path get_decent_packages() const { return _packages_path.valid() ? *_packages_path : _decent_data / "packages"; }

      void set_decent_data_path(const boost::filesystem::path& path) { _decent_data = path; }
      void set_decent_logs_path(const boost::filesystem::path& path) { _decent_logs = path; }
      void set_decent_temp_path(const boost::filesystem::path& path) { _decent_temp = path; }
      void set_packages_path(const boost::filesystem::path& path) { _packages_path = path; }
      
   private:
      boost::filesystem::path _user_home;
      boost::filesystem::path _decent_home;
      boost::filesystem::path _decent_data;
      boost::filesystem::path _decent_logs;
      boost::filesystem::path _decent_temp;
      boost::filesystem::path _ipfs_bin;
      boost::filesystem::path _ipfs_path;
      fc::optional<boost::filesystem::path> _packages_path;
   };
   
} } // graphene::utilities
