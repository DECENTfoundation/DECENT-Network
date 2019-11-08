#include <decent/about.hpp>
#include <graphene/utilities/git_revision.hpp>

#include <boost/algorithm/string/replace.hpp>

#include <openssl/opensslv.h>
#include <cryptopp/config.h>

#include <iostream>

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

   about_info get_about_info()
   {
      return {
         graphene::utilities::git_version(),
         get_boost_version(),
         OPENSSL_VERSION_TEXT,
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
