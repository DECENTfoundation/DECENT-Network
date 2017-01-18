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
#include <fc/array.hpp>

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
   fc::array<int8_t,16> u_seed; //generator for u's
   fc::array<uint8_t,DECENT_SIZE_OF_POINT_ON_CURVE_COMPRESSED> pubKey; //uploaders public key
};

struct custody_proof{
   uint32_t reference_block;
   fc::array<uint32_t,5> seed; //ripemd160._hash of the reference block
   std::vector<std::vector<unsigned char>> mus;
   fc::array<uint8_t,DECENT_SIZE_OF_POINT_ON_CURVE_COMPRESSED> sigma;
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
/*
template<typename K, size_t S >
inline void to_variant( const K (&var)[S], fc::variant& vo ) {
   std::vector<variant> vars(S);
   for(int i =0; i< S; i++ )
      vars[i] = var[i];
   vo = vars;
}

template<typename K, size_t S >
inline void from_variant( fc::variant& var, const K (&vo)[S] ) {
   const variants& vars = var.get_array();
   memset((char*)vo,0,sizeof(K)*S);
   int i = 0;
   for( auto itr = vars.begin(); itr != vars.end(); ++itr, ++i )
      vo[i]=itr->as<K>();
}

inline void to_variant( const decent::crypto::custody_data& cd, fc::variant& vo ){
   std::vector<variant> vars(1+16+DECENT_SIZE_OF_POINT_ON_CURVE_COMPRESSED);
   vars[0] = cd.n;
   for(int i = 0; i < 16; ++i )
      vars[i+1] = cd.u_seed[i];
   for(int i = 0; i < DECENT_SIZE_OF_POINT_ON_CURVE_COMPRESSED; ++i )
      vars[i+17] = cd.pubKey[i];
   vo = vars;
}

inline void from_variant(const fc::variant& var, decent::crypto::custody_data& cd ){
   const variants& vars = var.get_array();
   int i = 0;
   for( auto itr = vars.begin(); itr != vars.end(); ++itr, ++i ){
      if( i == 0 )
         cd.n = itr->as<uint32_t>();
      if( i >=1 && i <= 16)
         cd.u_seed[i-1] = itr->as<uint32_t>();
      if( i > 16 )
         cd.pubKey[i-17] = itr->as<uint8_t>();
   }
}

inline void to_variant( const decent::crypto::custody_proof& proof, fc::variant& vo ){
   std::vector<variant> vars(1+5+1+DECENT_SIZE_OF_POINT_ON_CURVE_COMPRESSED);
   vars[0] = proof.reference_block;
   for(int i = 0; i < 5; ++i )
      vars[i+1] = proof.seed[i];
   for(int i = 0; i < DECENT_SIZE_OF_POINT_ON_CURVE_COMPRESSED; ++i )
      vars[i+6] = proof.sigma[i];
   vars [6+DECENT_SIZE_OF_POINT_ON_CURVE_COMPRESSED] = proof.mus;
   vo = vars;
}

inline void from_variant(const fc::variant& var, decent::crypto::custody_proof& proof ){
   const variants& vars = var.get_array();
   int i = 0;
   for( auto itr = vars.begin(); itr != vars.end(); ++itr, ++i ){
      if( i == 0 )
         proof.reference_block = itr->as<uint32_t>();
      if( i >=1 && i <= 5 )
         proof.seed[i-1] = itr->as<int8_t>();
      if( i > 5 && i <= 5+DECENT_SIZE_OF_POINT_ON_CURVE_COMPRESSED )
         proof.sigma[i-6] = itr->as<uint8_t>();
      if( i == 6+DECENT_SIZE_OF_POINT_ON_CURVE_COMPRESSED )
         proof.mus = itr->as<std::vector<std::vector<unsigned char>>>();
   }
}*/

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
FC_REFLECT_EMPTY(decent::crypto::custody_data)
FC_REFLECT_EMPTY(decent::crypto::custody_proof)
