#pragma once

#include <string>

#include <fc/variant_object.hpp>

namespace decent {

   std::string get_boost_version();
   std::string get_openssl_version();
   std::string get_cryptopp_version();
   fc::variant_object get_about();

}
