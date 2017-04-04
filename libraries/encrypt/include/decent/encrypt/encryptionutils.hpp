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
//const CryptoPP::Integer DECENT_SHAMIR_ORDER("15990255936946939907849515173384301065310112618668839741376715383284525444117462469424256726689912144767057646796171794911693894374339500336078721128840927");


namespace decent {
namespace encrypt {

enum encryption_results {
   ok,
   io_error,
   other_error,
   key_error,
};


class ShamirSecret{
public:
   DInteger secret=DInteger::Zero();
   std::vector<point> split;

   ShamirSecret(uint16_t _quorum, uint16_t _shares, DInteger _secret) : secret(_secret), quorum(_quorum), shares(_shares){/*calculate_split();*/}
   ShamirSecret(uint16_t _quorum, uint16_t _shares) : quorum(_quorum), shares(_shares){}
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
encryption_results AES_encrypt_file(const std::string &fileIn, const std::string &fileOut, const AesKey &key);

/*********************************************************
 *  Decrypt file wit key
 *********************************************************/
encryption_results AES_decrypt_file(const std::string &fileIn, const std::string &fileOut, const AesKey &key);

DInteger generate_private_el_gamal_key();

DInteger get_public_el_gamal_key(const DInteger &privateKey);

encryption_results el_gamal_encrypt(const point &message, const DInteger &publicKey, Ciphertext &result);

encryption_results el_gamal_decrypt(const Ciphertext &input, const DInteger &privateKey, point &plaintext);

encryption_results
encrypt_with_proof(const point &message, const DInteger &privateKey, const DInteger &destinationPublicKey,
                   const Ciphertext &incoming, Ciphertext &outgoing, DeliveryProof &proof);

bool
verify_delivery_proof(const DeliveryProof &proof, const Ciphertext &first, const Ciphertext &second,
                      const DInteger &firstPulicKey, const DInteger &secondPublicKey);

}
}//namespace


