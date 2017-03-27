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
#include <fc/crypto/sha256.hpp>


#define DECENT_EL_GAMAL_GROUP_ELEMENT_SIZE 64 //bytes
#define DECENT_EL_GAMAL_CIPHERTEXT_SIZE (2 * DECENT_EL_GAMAL_GROUP_ELEMENT_SIZE) //bytes
#define DECENT_MESSAGE_SIZE 32 //bytes
#define DECENT_HASH_SIZE 32 //bytes
#define DECENT_PROOF_OF_DELIVERY_SIZE (DECENT_EL_GAMAL_GROUP_ELEMENT_SIZE * 5 + 2 * (DECENT_HASH_SIZE +1)) //bytes
const CryptoPP::Integer DECENT_EL_GAMAL_MODULUS_512(
      "11760620558671662461946567396662025495126946227619472274601251081547302009186313201119191293557856181195016058359990840577430081932807832465057884143546419.");
const CryptoPP::Integer DECENT_EL_GAMAL_GROUP_GENERATOR(3);

#ifdef DECENT_LONG_SHAMIR
const CryptoPP::Integer DECENT_SHAMIR_ORDER("15990255936946939907849515173384301065310112618668839741376715383284525444117462469424256726689912144767057646796171794911693894374339500336078721128840927");
#else
const CryptoPP::Integer DECENT_SHAMIR_ORDER("115792089237316195423570985008687907852837564279074904382605163141518161494337" );
#endif


namespace decent {
namespace encrypt {

enum encryption_results {
   ok,
   io_error,
   other_error,
};

/*
 * Class used to split and join the secret into particles
 */
class ShamirSecret{
public:
   DInteger secret=DInteger::Zero(); //<the secret
   std::vector<point> split; //<the secret particles

   /**
    *
    * Constructor. Used to fill the secret. Shall be followed by calculate_split method.
    * @param _quorum quorum needed to restore the key
    * @param _shares number of shares to generate
    * @param _secret the secret
    */
   ShamirSecret(uint16_t _quorum, uint16_t _shares, DInteger _secret) : secret(_secret), quorum(_quorum), shares(_shares){/*calculate_split();*/}
   /**
    * Restore the secret using shares
    * @param _quorum quorum needed to restore the key
    * @param _shares shares used to restore the secret
    */
   ShamirSecret(uint16_t _quorum, uint16_t _shares) : quorum(_quorum), shares(_shares){}

   /*
    * Add new share
    * @param X The new share
    */
   void add_point(point X){split.push_back(X);}
   /*
    * Verifies we have enough information to restore the secret
    */
   bool resolvable(){return split.size()>=quorum;}
   /**
    * Restore the secret
    */
   void calculate_secret();
   /**
    * Create split.
    */
   void calculate_split();

private:
   uint16_t quorum;
   uint16_t shares;
};

/**
 * Encrypt file with key
 * @param fileIn Input file
 * @param fileOut Output encrypted file
 * @param key Secret key
 * @return ok if successfull, or corresponding error code
 */
encryption_results AES_encrypt_file(const std::string &fileIn, const std::string &fileOut, const AesKey &key);

/*********************************************************
 *  Decrypt file wit key
 *********************************************************/

/**
 * Decrypt file with key
 * @param fileIn Input encrypted file
 * @param fileOut Output decrypted file
 * @param key Secret key
 * @return ok if successfull, or corresponding error code
 */
encryption_results AES_decrypt_file(const std::string &fileIn, const std::string &fileOut, const AesKey &key);

/**
 * Generate new el-gamal private key
 * @return New private key
 */
DInteger generate_private_el_gamal_key();
/**
 * Deterministically generate new el-gamal private key from the seed
 * @param secret Input seed
 * @return New private key
 */
DInteger generate_private_el_gamal_key_from_secret(fc::sha256 secret);

/**
 * Get public el-gamal key corresponding to the private one
 * @param privateKey Private key to which public one is calculated
 * @return Public el-gamal key
 */
DInteger get_public_el_gamal_key(const DInteger &privateKey);

/**
 * Encrypt message with el-gamal schema
 * @param message Message to encryt
 * @param publicKey Encryption key
 * @param result Encrypted message
 * @return ok if successfull, or corresponding error code
 */
encryption_results el_gamal_encrypt(const point &message, const DInteger &publicKey, Ciphertext &result);

/**
 * Decrypt message with el-gamal schema
 * @param input Encrypted message
 * @param privateKey Decryption key
 * @param plaintext Decrypted message
 * @return ok if successfull, or corresponding error code
 */
encryption_results el_gamal_decrypt(const Ciphertext &input, const DInteger &privateKey, point &plaintext);

/**
 * Encrypt message and generate delivery proof
 * @param message Message to encryt
 * @param privateKey Private key used to generate
 * @param destinationPublicKey Encryption key
 * @param incoming Message encrypted with public key corresponging to privateKey. el_gamal_decrypt(incoming, privateKey) must return message
 * @param outgoing Message encrypted with destinationPublicKey
 * @param proof Delivery proof
 * @return ok if successfull, or corresponding error code
 */
encryption_results
encrypt_with_proof(const point &message, const DInteger &privateKey, const DInteger &destinationPublicKey,
                   const Ciphertext &incoming, Ciphertext &outgoing, DeliveryProof &proof);


/**
 * Verify delivery proof
 * @param proof Delivery proof itself
 * @param first Message encrypted with seeder's key
 * @param second Message encrypted with consumer's key
 * @param firstPulicKey Seeder's public key
 * @param secondPublicKey Consumer's public key
 * @return ok if successfull, or corresponding error code
 */
bool
verify_delivery_proof(const DeliveryProof &proof, const Ciphertext &first, const Ciphertext &second,
                      const DInteger &firstPulicKey, const DInteger &secondPublicKey);

}//namespace encrypt
}//namespace decent


