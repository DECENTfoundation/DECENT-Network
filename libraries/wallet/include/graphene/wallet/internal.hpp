//
// Created by Milan Franc on 26/01/2018.
//

#ifndef DECENT_INTERNAL_H
#define DECENT_INTERNAL_H

#include <string>

using namespace std;

namespace graphene { namespace wallet { namespace internal {


   /**
    * @brief Transforms a brain key to reduce the chance of errors when re-entering the key from memory.
    *
    * This takes a user-supplied brain key and normalizes it into the form used
    * for generating private keys.  In particular, this upper-cases all ASCII characters
    * and collapses multiple spaces into one.
    * @param s the brain key as supplied by the user
    * @returns the brain key in its normalized form
    * @ingroup WalletCLI
    */
//   std::string normalize_brain_key(std::string s) const;

   /**
    * @brief Sign a buffer
    * @param str_buffer The buffer to be signed
    * @param str_brainkey Derives the private key used for signature
    * @return The signed buffer
    * @ingroup WalletCLI
    */
   std::string sign_buffer(std::string const& str_buffer,
                           std::string const& str_brainkey);

   /**
    * @brief Verify if the signature is valid
    * @param str_buffer The original buffer
    * @param str_publickey The public key used for verification
    * @param str_signature The signed buffer
    * @return true if valid, otherwise false
    * @ingroup WalletCLI
    */
   bool verify_signature(std::string const& str_buffer,
                         std::string const& str_publickey,
                         std::string const& str_signature);



   /**
    *
    * @param creator
    * @param symbol
    * @ingroup WalletCLI
    */
   void dbg_make_mia(string creator, string symbol);

   /**
    *
    * @param src_filename
    * @param count
    * @ingroup WalletCLI
    */
   void dbg_push_blocks( std::string src_filename, uint32_t count );

   /**
    *
    * @param debug_wif_key
    * @param count
    * @ingroup WalletCLI
    */
   void dbg_generate_blocks( std::string debug_wif_key, uint32_t count );

   /**
    *
    * @param filename
    * @ingroup WalletCLI
    */
   void dbg_stream_json_objects( const std::string& filename );

   /**
    *
    * @param update
    * @ingroup WalletCLI
    */
   void dbg_update_object( fc::variant_object update );


} } }

#endif //DECENT_INTERNAL_H
