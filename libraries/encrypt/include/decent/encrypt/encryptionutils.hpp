//
// Created by Josef Sevcik on 25/11/2016.
//
#pragma once


#include <cstdlib>
#include <string>
#include <vector>

#include <decent/encrypt/crypto_types.hpp>


#include <cryptopp/osrng.h>


using CryptoPP::AutoSeededRandomPool;

#include <cryptopp/secblock.h>

using CryptoPP::SecByteBlock;

#include <cryptopp/elgamal.h>

using CryptoPP::ElGamal;
using CryptoPP::ElGamalKeys;

#include <cryptopp/cryptlib.h>

using CryptoPP::DecodingResult;
using CryptoPP::PublicKey;

#include <cryptopp/modarith.h>

using CryptoPP::ModularArithmetic;

#include <cryptopp/pubkey.h>


#define DECENT_EL_GAMAL_GROUP_ELEMENT_SIZE 64 //bytes
#define DECENT_EL_GAMAL_CIPHERTEXT_SIZE (2 * DECENT_EL_GAMAL_GROUP_ELEMENT_SIZE) //bytes
#define DECENT_MESSAGE_SIZE 32 //bytes
#define DECENT_HASH_SIZE 32 //bytes
#define DECENT_PROOF_OF_DELIVERY_SIZE (DECENT_EL_GAMAL_GROUP_ELEMENT_SIZE * 5 + 2 * (DECENT_HASH_SIZE +1)) //bytes
const CryptoPP::Integer DECENT_EL_GAMAL_MODULUS_512(
      "11760620558671662461946567396662025495126946227619472274601251081547302009186313201119191293557856181195016058359990840577430081932807832465057884143546419.");
const CryptoPP::Integer DECENT_EL_GAMAL_GROUP_GENERATOR(3);
const CryptoPP::Integer DECENT_SHAMIR_ORDER("115792089237316195423570985008687907852837564279074904382605163141518161494337" );

namespace decent {
namespace crypto {

enum encryption_results {
   ok,
   io_error,
   other_error,
};


class shamir_secret{
public:
   d_integer secret=d_integer::Zero();
   std::vector<point> split;

   shamir_secret(uint16_t _quorum, uint16_t _shares, d_integer _secret) : secret(_secret), quorum(_quorum), shares(_shares){/*calculate_split();*/}
   shamir_secret(uint16_t _quorum, uint16_t _shares) : quorum(_quorum), shares(_shares){}
   void add_point(point X){split.push_back(X);}
   bool resolvable(){return split.size()>=quorum;}
   void calculate_secret();
   void calculate_split();

private:
   uint16_t quorum;
   uint16_t shares;
};



/*********************************************************
 *  Encrypt file wit key
 *********************************************************/
encryption_results AES_encrypt_file(const std::string &fileIn, const std::string &fileOut, const aes_key &key);

/*********************************************************
 *  Decrypt file wit key
 *********************************************************/
encryption_results AES_decrypt_file(const std::string &fileIn, const std::string &fileOut, const aes_key &key);

d_integer generate_private_el_gamal_key();

d_integer get_public_el_gamal_key(const d_integer &privateKey);

encryption_results el_gamal_encrypt(const point &message, const d_integer &publicKey, ciphertext &result);

encryption_results el_gamal_decrypt(const ciphertext &input, const d_integer &privateKey, point &plaintext);

encryption_results
encrypt_with_proof(const point &message, const d_integer &privateKey, const d_integer &destinationPublicKey,
                   const ciphertext &incoming, ciphertext &outgoing, delivery_proof &proof);

bool
verify_delivery_proof(const delivery_proof &proof, const ciphertext &first, const ciphertext &second,
                      const d_integer &firstPulicKey, const d_integer &secondPublicKey);

}
}//namespace


