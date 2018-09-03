
#include <ipfs/client.h>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/lexical_cast.hpp>

namespace decent {

const unsigned int g_minimalIpfsVersion = 0x00040c00;  //minimal version 0.4.12

bool check_ipfs_minimal_version(const std::string& host, int port)
{
   ipfs::Client client(host, port);

   ipfs::Json version_info;
   client.Version(&version_info);
   const std::string& ver_text = version_info["Version"];

   std::vector<std::string> data_split;
   boost::split( data_split, ver_text, boost::is_any_of("."), boost::token_compress_off);
   if (data_split.size() != 3)
      throw std::runtime_error("invalid IPFS version number");

   unsigned int version = 0;
   version |= (boost::lexical_cast<short>(data_split[0])) << 24;
   version |= (boost::lexical_cast<short>(data_split[1])) << 16;
   version |= (boost::lexical_cast<short>(data_split[2])) << 8;

   if (version >= g_minimalIpfsVersion)
      return true;

   return false;
}

}





