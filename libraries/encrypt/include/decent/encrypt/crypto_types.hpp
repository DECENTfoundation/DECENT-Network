/*
 * Copyright (c) 2015 Decent Sluzby, o.z., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once
#include <cryptopp/integer.h>
#include <cryptopp/aes.h>
#include <string>
#include <fc/reflect/variant.hpp>
#include <fc/log/logger.hpp>
#include <fc/io/raw_variant.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/io/datastream.hpp>
#include <fc/io/varint.hpp>
#include <fc/optional.hpp>
#include <fc/fwd.hpp>
#include <fc/smart_ref_fwd.hpp>
#include <fc/array.hpp>
#include <fc/time.hpp>
#include <fc/filesystem.hpp>
#include <fc/exception/exception.hpp>
#include <fc/safe.hpp>
#include <fc/io/raw_fwd.hpp>

#define DECENT_SIZE_OF_POINT_ON_CURVE_COMPRESSED 65
#define DECENT_SECTORS 32


namespace decent{
namespace crypto{

class d_integer : public CryptoPP::Integer {
public:
   std::string to_string() const;

   static d_integer from_string(std::string from) ;

   d_integer(CryptoPP::Integer integer) : CryptoPP::Integer(integer) { };
   d_integer( const d_integer& integer ) : CryptoPP::Integer(integer) { };
   d_integer& operator=(const d_integer& integer){ CryptoPP::Integer::operator=(integer); return *this; }

   d_integer() : CryptoPP::Integer(){};
   d_integer(std::string s): CryptoPP::Integer(s.c_str()){};
};

typedef std::vector<unsigned char> valtype;

struct delivery_proof {
   d_integer G1;
   d_integer G2;
   d_integer G3;
   d_integer s;
   d_integer r;

   delivery_proof(d_integer g1, d_integer g2, d_integer g3, d_integer s,
                  d_integer r) : G1(g1), G2(g2), G3(g3), s(s), r(r) {};
   delivery_proof(){};
};

struct custody_data{
   uint32_t n; //number of signatures
   char u_seed[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; //generator for u's
   unsigned char pubKey[DECENT_SIZE_OF_POINT_ON_CURVE_COMPRESSED]; //uploaders public key
};

struct custody_proof{
   uint32_t reference_block;
   uint32_t seed[5]; //ripemd160 of the reference block

   std::vector<std::vector<unsigned char>> mus;
   uint8_t sigma[DECENT_SIZE_OF_POINT_ON_CURVE_COMPRESSED];
};

struct ciphertext {
   d_integer C1 = decent::crypto::d_integer(CryptoPP::Integer::One());
   d_integer D1 = decent::crypto::d_integer(CryptoPP::Integer::One());
};


typedef std::pair<d_integer, d_integer> point;

struct aes_key {
   unsigned char key_byte[CryptoPP::AES::MAX_KEYLENGTH];
};
}}


namespace fc {
inline void to_variant( const decent::crypto::d_integer& var,  fc::variant& vo ) {
   vo = var.to_string();
}
inline void from_variant( const fc::variant& var, decent::crypto::d_integer& vo ) {
   vo = decent::crypto::d_integer::from_string( var.as_string() );
}

namespace raw {
template<typename Stream>
inline void pack( Stream& s, const decent::crypto::d_integer& tp )
{
   fc::raw::pack( s, tp.to_string() );
}

template<typename Stream>
inline void unpack( Stream& s, decent::crypto::d_integer& tp )
{
   std::string p;
   fc::raw::unpack( s, p );
   tp = decent::crypto::d_integer::from_string(p);
}

}
}


FC_REFLECT_EMPTY(decent::crypto::d_integer)

FC_REFLECT(decent::crypto::aes_key, (key_byte))
FC_REFLECT(decent::crypto::delivery_proof, (G1)(G2)(G3)(s)(r))
FC_REFLECT(decent::crypto::ciphertext, (C1)(D1))
