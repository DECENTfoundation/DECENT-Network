#include <decent/about.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/version.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string.hpp>

#include <openssl/opensslv.h>
#include <cryptopp/config.h>

#include <fc/git_revision.hpp>
#include <fc/time.hpp>
#include <fc/variant_object.hpp>

#include <graphene/utilities/git_revision.hpp>
#include <decent/about.hpp>

#include <string>

using namespace std;

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

   fc::variant_object get_about()
   {
      string client_version( graphene::utilities::git_revision_description );
      const size_t pos = client_version.find( '/' );
      if( pos != string::npos && client_version.size() > pos )
         client_version = client_version.substr( pos + 1 );

      fc::mutable_variant_object result;
      result["client_version"]           = client_version;
      result["graphene_revision"]        = graphene::utilities::git_revision_sha;
      result["graphene_revision_age"]    = fc::get_approximate_relative_time_string( fc::time_point_sec( graphene::utilities::git_revision_unix_timestamp ) );
      result["fc_revision"]              = fc::git_revision_sha;
      result["fc_revision_age"]          = fc::get_approximate_relative_time_string( fc::time_point_sec( fc::git_revision_unix_timestamp ) );
      result["compile_date"]             = "compiled on " __DATE__ " at " __TIME__;
      result["boost_version"]            = decent::get_boost_version();
      result["openssl_version"]          = OPENSSL_VERSION_TEXT;
      result["cryptopp_version"]         = decent::get_cryptopp_version();

      std::string bitness = boost::lexical_cast<std::string>(8 * sizeof(int*)) + "-bit";
#if defined(__APPLE__)
      std::string os = "osx";
#elif defined(__linux__)
      std::string os = "linux";
#elif defined(_MSC_VER)
      std::string os = "win32";
#else
      std::string os = "other";
#endif
      result["build"] = os + " " + bitness;

      return result;
   }

} //namespace decent
