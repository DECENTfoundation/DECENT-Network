//
// Created by Josef Sevcik on 25/11/2016.
//
#pragma once


#include <cstdlib>
#include <string>
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
#define MESSAGE_SIZE 33 //bytes
#define HASH_SIZE 32 //bytes
#define PROOF_OF_DELIVERY_SIZE (EL_GAMAL_GROUP_ELEMENT_SIZE * 5 + 2 * (HASH_SIZE +1)) //bytes
const CryptoPP::Integer EL_GAMAL_MODULUS_512("11760620558671662461946567396662025495126946227619472274601251081547302009186313201119191293557856181195016058359990840577430081932807832465057884143546419.");
const CryptoPP::Integer EL_GAMAL_GROUP_GENERATOR(3);

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
            std::string to_string()const;
            d_integer from_string(std::string from)const;

            d_integer(CryptoPP::Integer integer):CryptoPP::Integer(integer){};
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

/*********************************************************
 *  Encrypt file wit key
 *********************************************************/
        static encryption_results AES_encrypt_file(const std::string &fileIn, const std::string &fileOut, const aes_key &key);

/*********************************************************
 *  Decrypt file wit key
 *********************************************************/
        static encryption_results AES_decrypt_file(const std::string &fileIn, const std::string &fileOut, const aes_key &key);

        static d_integer generate_private_el_gamal_key();

        static d_integer get_public_el_gamal_key(const d_integer &privateKey);

        static encryption_results el_gamal_encrypt(const valtype &message, d_integer &publicKey, valtype &ciphertext);

        static encryption_results el_gamal_decrypt(const valtype &ciphertext, d_integer &privateKey, valtype &plaintext);

        static bool
        encrypt_with_proof(const valtype &message, const valtype privateKey, const valtype &destinationPublicKey,
                           const valtype &incomingCiphertext, valtype &outcomingCiphertext, valtype &proofOfDelivery);

        static bool
        verify_delivery_proof(const valtype &proof, const valtype &firstCiphertext, const valtype &secondCiphertext,
                              const valtype &firstPulicKey, const valtype &secondPublicKey);
    }
}

FC_REFLECT_EMPTY(decent::crypto::d_integer)

FC_REFLECT(decent::crypto::aes_key,(key_byte))
FC_REFLECT(decent::crypto::delivery_proof,(G1)(G2)(G3)(s)(r))
