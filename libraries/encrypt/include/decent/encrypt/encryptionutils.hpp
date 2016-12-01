//
// Created by Josef Sevcik on 25/11/2016.
//
#pragma once


#include <cstdlib>
#include <string>
#include <vector>

#include <fc/reflect/variant.hpp>
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
#include <cryptopp/sha.h>

#define EL_GAMAL_GROUP_ELEMENT_SIZE 64 //bytes
#define EL_GAMAL_CIPHERTEXT_SIZE (2 * EL_GAMAL_GROUP_ELEMENT_SIZE) //bytes
#define MESSAGE_SIZE 32 //bytes
#define HASH_SIZE 32 //bytes
#define PROOF_OF_DELIVERY_SIZE (EL_GAMAL_GROUP_ELEMENT_SIZE * 5 + 2 * (HASH_SIZE +1)) //bytes
const CryptoPP::Integer EL_GAMAL_MODULUS_512(
      "11760620558671662461946567396662025495126946227619472274601251081547302009186313201119191293557856181195016058359990840577430081932807832465057884143546419.");
const CryptoPP::Integer EL_GAMAL_GROUP_GENERATOR(3);
const CryptoPP::Integer SHAMIR_ORDER("115792089237316195423570985008687907852837564279074904382605163141518161494337" );

namespace decent {
namespace crypto {
struct aes_key {
   unsigned char key_byte[CryptoPP::AES::MAX_KEYLENGTH];
};

enum encryption_results {
   ok,
   io_error,
   other_error,
};

class d_integer : public CryptoPP::Integer {
public:
   std::string to_string() const;

   d_integer from_string(std::string from) const;

   d_integer(CryptoPP::Integer integer) : CryptoPP::Integer(integer) {}
};

typedef std::vector<unsigned char> valtype;

struct delivery_proof {
   d_integer G1;
   d_integer G2;
   d_integer G3;
   d_integer s;
   d_integer r;

   delivery_proof(d_integer g1, d_integer g2, d_integer g3, d_integer s,
                  d_integer r) : G1(g1), G2(g2), G3(g3), s(s), r(r) {}
};

struct ciphertext {
   d_integer C1 = decent::crypto::d_integer(CryptoPP::Integer::One());
   d_integer D1 = decent::crypto::d_integer(CryptoPP::Integer::One());
};


typedef std::pair<d_integer, d_integer> point;
class shamir_secret{
public:

   d_integer secret=d_integer::Zero();
   std::vector<point> split;


   shamir_secret(uint16_t _quorum, uint16_t _shares, d_integer _secret):quorum(_quorum),shares(_shares),secret(_secret){
      calculate_split();
   };
   shamir_secret(uint16_t _quorum, uint16_t _shares):quorum(_quorum), shares(_shares){};
   void add_point(point X){split.push_back(X);};
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

encryption_results el_gamal_encrypt(const valtype &message, d_integer &publicKey, ciphertext &result);

encryption_results el_gamal_decrypt(const ciphertext &input, d_integer &privateKey, valtype &plaintext);

encryption_results
encrypt_with_proof(const valtype &message, const d_integer &privateKey, const d_integer &destinationPublicKey,
                   const ciphertext &incoming, ciphertext &outgoing, delivery_proof &proof);

bool
verify_delivery_proof(const delivery_proof &proof, const ciphertext &first, const ciphertext &second,
                      const d_integer &firstPulicKey, const d_integer &secondPublicKey);






}
}//namespace

FC_REFLECT_EMPTY(decent::crypto::d_integer)

FC_REFLECT(decent::crypto::aes_key, (key_byte))
FC_REFLECT(decent::crypto::delivery_proof, (G1)(G2)(G3)(s)(r))
FC_REFLECT(decent::crypto::ciphertext, (C1)(D1))
