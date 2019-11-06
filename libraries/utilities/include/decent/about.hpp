#pragma once
#include <fc/reflect/reflect.hpp>
#include <boost/program_options/parsers.hpp>

namespace decent {

   struct about_info {
      std::string version;
      std::string boost_version;
      std::string openssl_version;
      std::string cryptopp_version;
      std::string build;
   };

   about_info get_about_info();

   void dump_version_info();

   bool check_unrecognized(const boost::program_options::parsed_options& optparsed);

}

FC_REFLECT( decent::about_info,
            (version)
            (boost_version)
            (openssl_version)
            (cryptopp_version)
            (build)
          )
