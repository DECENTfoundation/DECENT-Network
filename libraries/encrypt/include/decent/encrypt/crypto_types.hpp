/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
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


#define DECENT_SECTORS 100
#define DECENT_SECTORS_BIG 100

#define SHORT_CURVE 1
#ifdef SHORT_CURVE
#define DECENT_SIZE_OF_POINT_ON_CURVE_COMPRESSED 33
#define DECENT_SIZE_OF_MU 15
#else
#define DECENT_SIZE_OF_POINT_ON_CURVE_COMPRESSED 65
#define DECENT_SIZE_OF_MU 32
#endif



namespace decent{
namespace encrypt{

class DInteger;

/*****
 * Class DIntegerString
 * Stores big integers in string form. Used in objects that needs to be serialized/deserialized
 */
class DIntegerString{
public:
   /* Stores the big number in decadic representation */
   std::string s;

   /* Constructor, creates DIntegerString from DInteger */
   DIntegerString(const DInteger& d);
   /* Constructor, creates DIntegerString from string */
   DIntegerString(const std::string& _s) { s = _s; };
   /* copy constructor */
   DIntegerString(DIntegerString& _s) {s = _s.s; };
   /* copy constructor */
   DIntegerString(const DIntegerString& _s) {s = _s.s; };
   /* Default constuctor, initializes to zero value */
   DIntegerString();
   /* Assignment operator */
   DIntegerString& operator=(const std::string& _s) { s=_s; return *this; };
   /* Assignment operator */
   DIntegerString& operator=(const DInteger& d);
   /* Assignment operator */
   DIntegerString& operator=(const DIntegerString& _s) { s = _s.s; return *this;};
   /* Comparison operator */
   bool operator<(const DIntegerString& _s)const;
};

/*****
 * Class DInteger
 * Stores big integers.
 */
class DInteger : public CryptoPP::Integer {
public:

   std::string to_string() const;

   static DInteger from_string(std::string from) ;
   static DInteger from_string(DIntegerString from) {return from_string(from.s);};


   DInteger(CryptoPP::Integer integer) : CryptoPP::Integer(integer) { };
   DInteger( const DInteger& integer ) : CryptoPP::Integer(integer) { };
   DInteger( const std::string& s ) : CryptoPP::Integer(from_string(s)){ };
   DInteger( const DIntegerString& s ) : CryptoPP::Integer(from_string(s.s)){ };

   DInteger& operator=(const DInteger& integer){ CryptoPP::Integer::operator=(integer); return *this; };

   DInteger() : CryptoPP::Integer(){};
   DInteger(std::string s): CryptoPP::Integer(s.c_str()){};
   friend class DIntegerString;
   DInteger& operator=(const DIntegerString& s){ *this = DInteger::from_string(s.s); return *this; };
};


typedef std::vector<unsigned char> valtype;


struct DeliveryProof;

/*****
 * Class DeliveryProofString
 * Stores proof of key delivery, in string form. For field details refer to the whitepaper
 */
struct DeliveryProofString {
   DIntegerString G1;
   DIntegerString G2;
   DIntegerString G3;
   DIntegerString s;
   DIntegerString r;

   DeliveryProofString(DInteger g1, DInteger g2, DInteger g3, DInteger s,
         DInteger r) : G1(g1), G2(g2), G3(g3), s(s), r(r) {};
   DeliveryProofString( DeliveryProof df );
   DeliveryProofString(){
      DInteger a=CryptoPP::Integer::Zero();
      G1=(a.to_string());
      G2=(a.to_string());
      G3=(a.to_string());
      s=(a.to_string());
      r=(a.to_string());
   };
};

/*****
 * Class DeliveryProof
 * Stores proof of key delivery.
 */
struct DeliveryProof {
   DInteger G1;
   DInteger G2;
   DInteger G3;
   DInteger s;
   DInteger r;

   DeliveryProof(DInteger g1, DInteger g2, DInteger g3, DInteger s,
                  DInteger r) : G1(g1), G2(g2), G3(g3), s(s), r(r) {};
   DeliveryProof(DeliveryProofString& ss){
      G1=DInteger::from_string(ss.G1.s);
      G2=DInteger::from_string(ss.G2.s);
      G3=DInteger::from_string(ss.G3.s);
      s=DInteger::from_string(ss.s.s);
      r=DInteger::from_string(ss.r.s);
   };
   DeliveryProof(const DeliveryProofString& ss){
      G1=DInteger::from_string(ss.G1.s);
      G2=DInteger::from_string(ss.G2.s);
      G3=DInteger::from_string(ss.G3.s);
      s=DInteger::from_string(ss.s.s);
      r=DInteger::from_string(ss.r.s);
   };
   DeliveryProof(){
      G1=CryptoPP::Integer::Zero();
      G2=CryptoPP::Integer::Zero();
      G3=CryptoPP::Integer::Zero();
      s=CryptoPP::Integer::Zero();
      r=CryptoPP::Integer::Zero();
   }

};

/*****
 * Class CustodyData
 * Stores blockchain part of custody data.
 */
struct CustodyData{
   uint32_t n; //<number of signatures
   fc::array<int8_t,16> u_seed; //<generator for u's
   fc::array<uint8_t,DECENT_SIZE_OF_POINT_ON_CURVE_COMPRESSED> pubKey; //<uploaders public key

   bool operator==( const CustodyData& other)const
   {
      return n == other.n && u_seed == other.u_seed && pubKey == other.pubKey;
   }
};


/*****
 * Class CustodyProof
 * Represent proof of retrievability
 */
struct CustodyProof{
   uint32_t reference_block; //<Block used to get entrophy from. Must be recent
   fc::array<uint32_t,5> seed; //<ripemd160._hash of the reference block
   std::vector<std::string> mus; //<Mju-s
   fc::array<uint8_t,DECENT_SIZE_OF_POINT_ON_CURVE_COMPRESSED> sigma; //<sigma
};


struct Ciphertext;

/*****
 * Class CiphertextString
 * Stores encrypted key particle, in string format.
 */
struct CiphertextString {
   DIntegerString C1;
   DIntegerString D1;
   CiphertextString(Ciphertext ct);
   CiphertextString(){
      DInteger a=CryptoPP::Integer::Zero();
      C1=(a.to_string());
      D1=(a.to_string());
   }
};

/*****
 * Class Ciphertext
 * Stores encrypted key particle.
 */
struct Ciphertext {
   DInteger C1 = decent::encrypt::DInteger(CryptoPP::Integer::One());
   DInteger D1 = decent::encrypt::DInteger(CryptoPP::Integer::One());
   Ciphertext(CiphertextString&s){C1=DInteger::from_string(s.C1.s);D1=DInteger::from_string(s.D1.s);};
   Ciphertext(const CiphertextString&s){C1=DInteger::from_string(s.C1.s);D1=DInteger::from_string(s.D1.s);};
   Ciphertext(){};
};


// Point on the elyptic curve
typedef std::pair<DInteger, DInteger> point;


struct AesKey {
   unsigned char key_byte[CryptoPP::AES::MAX_KEYLENGTH];
};
}}


namespace fc {

inline void to_variant(const decent::encrypt::DInteger &var, fc::variant &vo) {
   vo = var.to_string();
}

inline void from_variant(const fc::variant &var, decent::encrypt::DInteger &vo) {
   vo = decent::encrypt::DInteger::from_string(var.as_string());
}


namespace raw {
template<typename Stream>

inline void pack(Stream &s, const decent::encrypt::DInteger &tp) {
   fc::raw::pack(s, tp.to_string());
}

template<typename Stream>
inline void unpack(Stream &s, decent::encrypt::DInteger &tp) {
   std::string p;
   fc::raw::unpack(s, p);
   tp = decent::encrypt::DInteger::from_string(p);
}

} //namespace raw
} //namespace fc

FC_REFLECT_EMPTY(decent::encrypt::DInteger)

FC_REFLECT(decent::encrypt::DIntegerString, (s) )

FC_REFLECT(decent::encrypt::AesKey, (key_byte))
FC_REFLECT(decent::encrypt::DeliveryProof, (G1)(G2)(G3)(s)(r))
FC_REFLECT(decent::encrypt::DeliveryProofString, (G1)(G2)(G3)(s)(r))
FC_REFLECT(decent::encrypt::Ciphertext, (C1)(D1))
FC_REFLECT(decent::encrypt::CiphertextString, (C1)(D1))
FC_REFLECT(decent::encrypt::CustodyData, (n)(u_seed)(pubKey))
FC_REFLECT(decent::encrypt::CustodyProof, (reference_block)(seed)(mus)(sigma))


