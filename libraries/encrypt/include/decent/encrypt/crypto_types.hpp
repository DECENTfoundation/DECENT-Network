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

namespace decent{
namespace crypto{

class d_integer : public CryptoPP::Integer {
public:
   std::string to_string() const;

   d_integer from_string(std::string from) const;

   d_integer(CryptoPP::Integer integer) : CryptoPP::Integer(integer) {};
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

struct ciphertext {
   d_integer C1 = decent::crypto::d_integer(CryptoPP::Integer::One());
   d_integer D1 = decent::crypto::d_integer(CryptoPP::Integer::One());
};


typedef std::pair<d_integer, d_integer> point;

struct aes_key {
   unsigned char key_byte[CryptoPP::AES::MAX_KEYLENGTH];
};
}}

FC_REFLECT_EMPTY(decent::crypto::d_integer)

FC_REFLECT(decent::crypto::aes_key, (key_byte))
FC_REFLECT(decent::crypto::delivery_proof, (G1)(G2)(G3)(s)(r))
FC_REFLECT(decent::crypto::ciphertext, (C1)(D1))