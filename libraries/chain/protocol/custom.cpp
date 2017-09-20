/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
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
#include <graphene/chain/protocol/custom.hpp>
#include <graphene/chain/protocol/memo.hpp>
#include <fc/crypto/aes.hpp>

namespace graphene { namespace chain {

void custom_operation::validate()const
{
   FC_ASSERT( fee.amount > 0 );
}
share_type custom_operation::calculate_fee(const fee_parameters_type& k)const
{
   return k.fee + calculate_data_fee( fc::raw::pack_size(*this), k.price_per_kbyte );
}

void message_payload::set_message(const fc::ecc::private_key& priv, const fc::ecc::public_key& pub,
   const string& msg, uint64_t custom_nonce)
{
   if (priv != fc::ecc::private_key() && public_key_type(pub) != public_key_type())
   {
      pub_from = priv.get_public_key();
      pub_to = pub;
      if (custom_nonce == 0)
      {
         uint64_t entropy = fc::sha224::hash(fc::ecc::private_key::generate())._hash[0];
         entropy <<= 32;
         entropy &= 0xff00000000000000;
         nonce = (fc::time_point::now().time_since_epoch().count() & 0x00ffffffffffffff) | entropy;
      }
      else
         nonce = custom_nonce;
      auto secret = priv.get_shared_secret(pub);
      auto nonce_plus_secret = fc::sha512::hash(fc::to_string(nonce) + secret.str());
      string text = memo_message(digest_type::hash(msg)._hash[0], msg).serialize();
      data = fc::aes_encrypt(nonce_plus_secret, std::vector<char>(text.begin(), text.end()));
   }
}

void message_payload::get_message(const fc::ecc::private_key& priv,
   const fc::ecc::public_key& pub, const std::vector<char>& data, std::string& text, uint64_t nonce)
{
   
   if ((public_key_type)pub != public_key_type())
   {
      auto secret = priv.get_shared_secret(pub);

      auto nonce_plus_secret = fc::sha512::hash(fc::to_string(nonce) + secret.str());
      auto plain_text = fc::aes_decrypt(nonce_plus_secret, data);
      
      auto result = memo_message::deserialize(string(plain_text.begin(), plain_text.end()));
      FC_ASSERT(result.checksum == uint32_t(digest_type::hash(result.text)._hash[0]));
     
      text = result.text;
      return;
   }
   
   return;
}

} }
