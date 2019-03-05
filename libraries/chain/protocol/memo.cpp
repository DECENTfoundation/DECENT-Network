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
#include <graphene/chain/protocol/memo.hpp>
#include <fc/crypto/aes.hpp>

namespace graphene { namespace chain {

memo_data::memo_data(const std::string& msg, const private_key_type& priv, const public_key_type& pub, uint64_t _nonce)
{
   if( priv != private_key_type() && pub != public_key_type() )
   {
      from = priv.get_public_key();
      to = pub;
      nonce = _nonce == 0 ? generate_nonce() : _nonce;
      message = encrypt_message(msg, priv, to, nonce);
   }
   else
   {
      auto text = memo_message(0, msg).serialize();
      message = vector<char>(text.begin(), text.end());
   }
}

std::string memo_data::get_message(const private_key_type& priv, const public_key_type& pub)const
{
   if( from != public_key_type() )
   {
      return decrypt_message(message, priv, pub, nonce);
   }
   else
   {
      return memo_message::deserialize(std::string(message.begin(), message.end())).text;
   }
}

memo_data::message_type memo_data::encrypt_message(const std::string &message, const private_key_type &priv, const public_key_type &pub, uint64_t nonce)
{
   auto secret = priv.get_shared_secret(pub);
   auto nonce_plus_secret = fc::sha512::hash(fc::to_string(nonce) + secret.str());
   std::string text = memo_message(static_cast<uint32_t>(digest_type::hash(message)._hash[0]), message).serialize();
   return fc::aes_encrypt( nonce_plus_secret, vector<char>(text.begin(), text.end()) );
}

std::string memo_data::decrypt_message(const message_type &message, const private_key_type &priv, const public_key_type &pub, uint64_t nonce)
{
   if( message.empty() )
      return std::string();

   auto secret = priv.get_shared_secret(pub);
   auto nonce_plus_secret = fc::sha512::hash(fc::to_string(nonce) + secret.str());
   auto plain_text = fc::aes_decrypt( nonce_plus_secret, message );
   auto result = memo_message::deserialize(std::string(plain_text.begin(), plain_text.end()));
   FC_ASSERT( result.checksum == uint32_t(digest_type::hash(result.text)._hash[0]) );
   return result.text;
}

uint64_t memo_data::generate_nonce()
{
   uint64_t entropy = fc::sha224::hash(private_key_type::generate())._hash[0];
   entropy <<= 32;
   entropy                                                    &= 0xff00000000000000;
   return (fc::time_point::now().time_since_epoch().count()   &  0x00ffffffffffffff) | entropy;
}

string memo_message::serialize() const
{
   auto serial_checksum = string(sizeof(checksum), ' ');
   (uint32_t&)(*serial_checksum.data()) = checksum;
   return serial_checksum + text;
}

memo_message memo_message::deserialize(const string& serial)
{
   if( serial.empty() )
      return memo_message();

   memo_message result;
   FC_ASSERT( serial.size() >= sizeof(result.checksum) );
   result.checksum = ((uint32_t&)(*serial.data()));
   result.text = serial.substr(sizeof(result.checksum));
   return result;
}

} } // graphene::chain
