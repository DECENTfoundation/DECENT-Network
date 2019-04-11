#pragma once

#include <string>

#include <boost/program_options.hpp>

#include <fc/variant_object.hpp>

namespace bpo = boost::program_options;

namespace decent {

   std::string get_boost_version();
   std::string get_openssl_version();
   std::string get_cryptopp_version();
   fc::variant_object get_about();

   bool check_unrecognized(bpo::parsed_options optparsed);

}
