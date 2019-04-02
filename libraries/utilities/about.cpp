#include <decent/about.hpp>

#include <boost/version.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string.hpp>

#include <openssl/opensslv.h>
#include <cryptopp/config.h>

namespace decent {

   std::string get_boost_version()
   {
      return boost::replace_all_copy(std::string(BOOST_LIB_VERSION), "_", ".");
   }

   std::string get_openssl_version()
   {
      std::string openssl_version_text = std::string(OPENSSL_VERSION_TEXT);
      openssl_version_text = openssl_version_text.substr(0, openssl_version_text.length() - 11);
      return openssl_version_text;
   }

   std::string get_cryptopp_version()
   {
      unsigned int cryptopp_major_version = CRYPTOPP_VERSION / 100;
      unsigned int cryptopp_minor_version = CRYPTOPP_VERSION / 10 - cryptopp_major_version * 10;
      std::string cryptopp_version_text = std::to_string(cryptopp_major_version) + "." + std::to_string(cryptopp_minor_version) + "." + std::to_string(CRYPTOPP_VERSION % 10);
      return cryptopp_version_text;
   }

} //namespace decent
