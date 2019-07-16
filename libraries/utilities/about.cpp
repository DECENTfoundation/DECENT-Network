#include <decent/about.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/version.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/program_options/parsers.hpp>

#include <openssl/opensslv.h>
#include <cryptopp/config.h>

#include <fc/git_revision.hpp>
#include <fc/time.hpp>
#include <fc/variant_object.hpp>

#include <graphene/utilities/git_revision.hpp>

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

   about_info get_about_daemon()
   {
      std::string version( graphene::utilities::git_revision_description );
      const size_t pos = version.find( '/' );
      if( pos != std::string::npos && version.size() > pos )
         version = version.substr( pos + 1 );

      return {
         version,
         graphene::utilities::git_revision_sha,
         fc::get_approximate_relative_time_string( fc::time_point_sec( graphene::utilities::git_revision_unix_timestamp ) ),
         fc::git_revision_sha,
         fc::get_approximate_relative_time_string( fc::time_point_sec( fc::git_revision_unix_timestamp ) ),
         "compiled on " __DATE__ " at " __TIME__,
         decent::get_boost_version(),
         OPENSSL_VERSION_TEXT,
         decent::get_cryptopp_version(),
#if defined(__APPLE__)
         "osx "
#elif defined(__linux__)
         "linux "
#elif defined(_MSC_VER)
         "win32 "
#else
         "other "
#endif
         + boost::lexical_cast<std::string>(8 * sizeof(int*)) + "-bit"
      };
   }

   about_info get_about_wallet()
   {
      std::string version( graphene::utilities::git_revision_description );
      const size_t pos = version.find( '/' );
      if( pos != std::string::npos && version.size() > pos )
         version = version.substr( pos + 1 );

      return {
         version,
         graphene::utilities::git_revision_sha,
         fc::get_approximate_relative_time_string( fc::time_point_sec( graphene::utilities::git_revision_unix_timestamp ) ),
         fc::git_revision_sha,
         fc::get_approximate_relative_time_string( fc::time_point_sec( fc::git_revision_unix_timestamp ) ),
         "compiled on " __DATE__ " at " __TIME__,
         decent::get_boost_version(),
         OPENSSL_VERSION_TEXT,
         decent::get_cryptopp_version(),
#if defined(__APPLE__)
         "osx "
#elif defined(__linux__)
         "linux "
#elif defined(_MSC_VER)
         "win32 "
#else
         "other "
#endif
         + boost::lexical_cast<std::string>(8 * sizeof(int*)) + "-bit"
      };
   }

   bool check_unrecognized(const boost::program_options::parsed_options& optparsed)
   {
      std::vector<std::string> unrecognized_args = boost::program_options::collect_unrecognized(optparsed.options, boost::program_options::include_positional);

      for (auto const& item : unrecognized_args)
      {
         std::cout << "Unrecognized argument: " << item << std::endl;
         return true;
      }

      return false;
   }

} //namespace decent
