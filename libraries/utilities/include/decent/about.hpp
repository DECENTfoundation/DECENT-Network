#pragma once

#include <string>

#include <fc/io/json.hpp>

using namespace std;

namespace decent {

   std::string get_boost_version();
   std::string get_openssl_version();
   std::string get_cryptopp_version();
   fc::variant_object get_about();

}
