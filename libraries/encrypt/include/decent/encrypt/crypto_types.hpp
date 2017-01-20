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

class d_integer;


class d_integer_string{
public:
   std::string s;
   d_integer_string(const d_integer& d);
   d_integer_string(const std::string& _s) { s = _s; };
   d_integer_string(d_integer_string& _s) {s = _s.s; };
   d_integer_string(const d_integer_string& _s) {s = _s.s; };
   d_integer_string();
   d_integer_string& operator=(const std::string& _s) { s=_s; return *this; };
   d_integer_string& operator=(const d_integer& d);
   d_integer_string& operator=(const d_integer_string& _s) { s = _s.s; return *this;};
};

class d_integer : public CryptoPP::Integer {
public:
   std::string to_string() const;
   //d_integer_string to_string() const{ d_integer_string tmp; tmp.s = to_string; return tmp; };

   static d_integer from_string(std::string from) ;
   static d_integer from_string(d_integer_string from) {return from_string(from.s);};


   d_integer(CryptoPP::Integer integer) : CryptoPP::Integer(integer) { };
   d_integer( const d_integer& integer ) : CryptoPP::Integer(integer) { };
   d_integer( const std::string& s ) : CryptoPP::Integer(from_string(s)){ };
   d_integer( const d_integer_string& s ) : CryptoPP::Integer(from_string(s.s)){ };

   d_integer& operator=(const d_integer& integer){ CryptoPP::Integer::operator=(integer); return *this; };

   d_integer() : CryptoPP::Integer(){};
   d_integer(std::string s): CryptoPP::Integer(s.c_str()){};
   friend class d_integer_string;
   d_integer& operator=(const d_integer_string& s){ *this = d_integer::from_string(s.s); return *this; };
};


typedef std::vector<unsigned char> valtype;

struct delivery_proof;
struct delivery_proof_string {
   d_integer_string G1;
   d_integer_string G2;
   d_integer_string G3;
   d_integer_string s;
   d_integer_string r;

   delivery_proof_string(d_integer g1, d_integer g2, d_integer g3, d_integer s,
         d_integer r) : G1(g1), G2(g2), G3(g3), s(s), r(r) {};
   delivery_proof_string( delivery_proof df );
   delivery_proof_string(){
      d_integer a=CryptoPP::Integer::Zero();
      G1=(a.to_string());
      G2=(a.to_string());
      G3=(a.to_string());
      s=(a.to_string());
      r=(a.to_string());
   };
};

struct delivery_proof {
   d_integer G1;
   d_integer G2;
   d_integer G3;
   d_integer s;
   d_integer r;

   delivery_proof(d_integer g1, d_integer g2, d_integer g3, d_integer s,
                  d_integer r) : G1(g1), G2(g2), G3(g3), s(s), r(r) {};
   delivery_proof(delivery_proof_string& ss){
      G1=d_integer::from_string(ss.G1.s);
      G2=d_integer::from_string(ss.G2.s);
      G3=d_integer::from_string(ss.G3.s);
      s=d_integer::from_string(ss.s.s);
      r=d_integer::from_string(ss.r.s);
   };
   delivery_proof(const delivery_proof_string& ss){
      G1=d_integer::from_string(ss.G1.s);
      G2=d_integer::from_string(ss.G2.s);
      G3=d_integer::from_string(ss.G3.s);
      s=d_integer::from_string(ss.s.s);
      r=d_integer::from_string(ss.r.s);
   };
   delivery_proof(){
      G1=CryptoPP::Integer::Zero();
      G2=CryptoPP::Integer::Zero();
      G3=CryptoPP::Integer::Zero();
      s=CryptoPP::Integer::Zero();
      r=CryptoPP::Integer::Zero();
   }

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

struct ciphertext;
struct ciphertext_string {
   d_integer_string C1;
   d_integer_string D1;
   ciphertext_string(ciphertext ct);
   ciphertext_string(){
      d_integer a=CryptoPP::Integer::Zero();
      C1=(a.to_string());
      D1=(a.to_string());
   }
};

struct ciphertext {
   d_integer C1 = decent::crypto::d_integer(CryptoPP::Integer::One());
   d_integer D1 = decent::crypto::d_integer(CryptoPP::Integer::One());
   ciphertext(ciphertext_string&s){C1=d_integer::from_string(s.C1.s);D1=d_integer::from_string(s.D1.s);};
   ciphertext(const ciphertext_string&s){C1=d_integer::from_string(s.C1.s);D1=d_integer::from_string(s.D1.s);};

   ciphertext(){};
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

FC_REFLECT(decent::crypto::d_integer_string, (s) )

FC_REFLECT(decent::crypto::aes_key, (key_byte))
FC_REFLECT(decent::crypto::delivery_proof, (G1)(G2)(G3)(s)(r))
FC_REFLECT(decent::crypto::ciphertext, (C1)(D1))
FC_REFLECT(decent::crypto::custody_data, (n)(u_seed)(pubKey))
FC_REFLECT(decent::crypto::custody_proof, (reference_block)(seed)(mus)(sigma))
FC_REFLECT(decent::crypto::delivery_proof_string, (G1)(G2)(G3)(s)(r))
FC_REFLECT(decent::crypto::ciphertext_string, (C1)(D1))