/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */

#include <fc/io/json.hpp>
#include <fc/smart_ref_impl.hpp>
#include <fc/variant.hpp>

#include <fc/variant_object.hpp>
#include <fc/crypto/hex.hpp>

#include <graphene/chain/protocol/protocol.hpp>

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>


using namespace graphene::chain;

using namespace std;
using namespace fc;
namespace bpo = boost::program_options;




fc::ecc::private_key derive_private_key( const std::string& prefix_string,
                                         int sequence_number )
{
   std::string sequence_string = std::to_string(sequence_number);
   fc::sha512 h = fc::sha512::hash(prefix_string + " " + sequence_string);
   fc::ecc::private_key derived_key = fc::ecc::private_key::regenerate(fc::sha256::hash(h));
   return derived_key;
}

string normalize_brain_key( string s )
{
   size_t i = 0, n = s.length();
   std::string result;
   char c;
   result.reserve( n );

   bool preceded_by_whitespace = false;
   bool non_empty = false;
   while( i < n )
   {
      c = s[i++];
      switch( c )
      {
      case ' ':  case '\t': case '\r': case '\n': case '\v': case '\f':
         preceded_by_whitespace = true;
         continue;

      case 'a': c = 'A'; break;
      case 'b': c = 'B'; break;
      case 'c': c = 'C'; break;
      case 'd': c = 'D'; break;
      case 'e': c = 'E'; break;
      case 'f': c = 'F'; break;
      case 'g': c = 'G'; break;
      case 'h': c = 'H'; break;
      case 'i': c = 'I'; break;
      case 'j': c = 'J'; break;
      case 'k': c = 'K'; break;
      case 'l': c = 'L'; break;
      case 'm': c = 'M'; break;
      case 'n': c = 'N'; break;
      case 'o': c = 'O'; break;
      case 'p': c = 'P'; break;
      case 'q': c = 'Q'; break;
      case 'r': c = 'R'; break;
      case 's': c = 'S'; break;
      case 't': c = 'T'; break;
      case 'u': c = 'U'; break;
      case 'v': c = 'V'; break;
      case 'w': c = 'W'; break;
      case 'x': c = 'X'; break;
      case 'y': c = 'Y'; break;
      case 'z': c = 'Z'; break;

      default:
         break;
      }
      if( preceded_by_whitespace && non_empty )
         result.push_back(' ');
      result.push_back(c);
      preceded_by_whitespace = false;
      non_empty = true;
   }
   return result;
}


int main( int argc, char** argv )
{
    try {
        boost::program_options::options_description opts;
        opts.add_options()
        ("verify", "Verify the signature" )
        ("sign", "Sign the buffer" )
        ("buffer", bpo::value<string>()->default_value(""), "Buffer to verify or sign.")
        ("brainkey", bpo::value<string>()->default_value(""), "Brain key.")
        ("publickey", bpo::value<string>()->default_value(""), "Public key.")
        ("signature", bpo::value<string>()->default_value(""), "Signature to verify.")
        ;

        bpo::variables_map options;
        bpo::store( bpo::parse_command_line(argc, argv, opts), options );
      

        if (options.count( "verify" ) && options.count( "sign" ) ) {
            std::cerr << "Either sign or verify please" << std::endl;
            return 1;
        }
        
        std::string str_buffer = options.at("buffer").as<string>();
        std::string str_brainkey = options.at("brainkey").as<string>();
        std::string str_publickey = options.at("publickey").as<string>();
        std::string str_signature = options.at("signature").as<string>();

        if ( options.count( "sign" ) ) {
            if (str_buffer.size() == 0 || str_brainkey.size() == 0) {
                std::cerr << "You need buffer and brainkey to sign" << std::endl;
                return 1;
            }
           
            string normalized_brain_key = normalize_brain_key( str_brainkey );
            fc::ecc::private_key privkey = derive_private_key( normalized_brain_key, 0 );

            sha256 digest(str_buffer);
            
            auto sign = privkey.sign_compact(digest);
           
           std::cout << to_hex((const char*)sign.begin(), sign.size()) << std::endl;

        } else {
            if (str_buffer.size() == 0 || str_publickey.size() == 0 || str_signature.size() == 0) {
                std::cerr << "You need buffer, public key and signature to verify" << std::endl;
                return 1;
            }
           
           fc::ecc::compact_signature signature;
           from_hex(str_signature, (char*)signature.begin(), signature.size());
           sha256 digest(str_buffer);

           fc::ecc::public_key pub_key(signature, digest);
           public_key_type provided_key(str_publickey);
           
           if (provided_key == pub_key) {
              cout << "Valid" << std::endl;
              return 0;
           } else {
              cerr << "Invalid signature" << std::endl;
              return 1;
           }
           
           
           
        }
      
        

    }
    catch ( const fc::exception& e ) { 
    }

   return 0;
}
