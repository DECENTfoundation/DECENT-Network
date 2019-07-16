#pragma once

#include <boost/program_options.hpp>

#include <fc/reflect/reflect.hpp>

namespace decent {

   std::string get_boost_version();
   std::string get_openssl_version();
   std::string get_cryptopp_version();

   struct about_info {
      std::string version;
      std::string graphene_revision;
      std::string graphene_revision_age;
      std::string fc_revision;
      std::string fc_revision_age;
      std::string compile_date;
      std::string boost_version;
      std::string openssl_version;
      std::string cryptopp_version;
      std::string build;
   };

   struct about_info_wallet {
      about_info daemon_info;
      about_info wallet_info;
   };

   about_info get_about_daemon();
   about_info get_about_wallet();

   bool check_unrecognized(const boost::program_options::parsed_options& optparsed);

}

FC_REFLECT( decent::about_info,
            (version)
            (graphene_revision)
            (graphene_revision_age)
            (fc_revision)
            (fc_revision_age)
            (compile_date)
            (boost_version)
            (openssl_version)
            (cryptopp_version)
            (build)
          )

FC_REFLECT( decent::about_info_wallet,
            (daemon_info)
            (wallet_info)
          )
