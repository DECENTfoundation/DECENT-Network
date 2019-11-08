#include <decent/about.hpp>
#include <graphene/utilities/git_revision.hpp>

#include <boost/algorithm/string/replace.hpp>

#include <openssl/opensslv.h>
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
#include <openssl/crypto.h>
#endif
#include <cryptopp/config.h>

#include <iostream>

namespace decent {

   std::string get_boost_version()
   {
      return boost::replace_all_copy(std::string(BOOST_LIB_VERSION), "_", ".");
   }

   std::string get_openssl_version()
   {
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
      return OpenSSL_version(OPENSSL_VERSION);
#else
      return OPENSSL_VERSION_TEXT;
#endif
   }

   std::string get_cryptopp_version()
   {
      return std::to_string(CRYPTOPP_VERSION / 100).append(1, '.')
            .append(std::to_string(CRYPTOPP_VERSION % 100 / 10)).append(1, '.')
            .append(std::to_string(CRYPTOPP_VERSION % 10));
   }

   about_info get_about_info()
   {
      return {
         graphene::utilities::git_version(),
         get_boost_version(),
         get_openssl_version(),
         get_cryptopp_version(),
#if defined(__APPLE__)
         "osx "
#elif defined(__linux__)
         "linux "
#elif defined(_MSC_VER)
         "win32 "
#else
         "other "
#endif
         + std::to_string(8 * sizeof(int*)) + "-bit"
      };
   }

   void dump_version_info(const char *caption)
   {
      std::cout << caption << ' ' << graphene::utilities::git_version();
#ifndef NDEBUG
      std::cout << " (debug)";
#endif
      std::cout << "\nBoost " << get_boost_version() << "\n" << get_openssl_version() << "\nCryptopp " << get_cryptopp_version() << std::endl;
   }

   bool check_unrecognized(const boost::program_options::parsed_options& optparsed)
   {
      std::vector<std::string> unrecognized_args = boost::program_options::collect_unrecognized(optparsed.options, boost::program_options::include_positional);
      if(unrecognized_args.empty())
         return false;

      std::cerr << "Unrecognized argument(s):";
      for(auto const& item : unrecognized_args)
         std::cerr << ' ' << item;

      std::cerr << std::endl;
      return true;
   }

} //namespace decent
