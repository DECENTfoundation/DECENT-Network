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

namespace graphene { namespace chain {

void custom_operation::validate()const
{
   FC_ASSERT( fee.amount > 0 );
}
share_type custom_operation::calculate_fee(const fee_parameters_type& k, fc::time_point_sec now)const
{
   if( now >= HARDFORK_5_TIME )
      return k.fee + calculate_data_fee( fc::raw::pack_size(data), k.price_per_kbyte );
   else
      return k.fee + calculate_data_fee( fc::raw::pack_size(*this), k.price_per_kbyte );
}

void custom_operation::get_required_active_authorities( boost::container::flat_set<account_id_type>& a )const
{
   for( const auto& i : required_auths ) a.insert(i);
}

message_payload_receivers_data::message_payload_receivers_data(const std::string &msg,
                                                               const private_key_type& priv,
                                                               const public_key_type& pub,
                                                               account_id_type id,
                                                               uint64_t _nonce)
   : to(id), pub_to(pub)
{
   if (!msg.empty())
   {
      if( priv != private_key_type() && pub != public_key_type() )
      {
         nonce = _nonce == 0 ? memo_data::generate_nonce() : _nonce;
         data = memo_data::encrypt_message(msg, priv, pub, nonce);
      }
      else
      {
         std::string text = memo_message(0, msg).serialize();
         data.insert(data.begin(), text.begin(), text.end());
      }
   }
}

std::string message_payload_receivers_data::get_message(const private_key_type& priv, const public_key_type& pub) const
{
   if ( priv != private_key_type() && pub != public_key_type() )
   {
      return memo_data::decrypt_message(data, priv, pub, nonce);
   }
   else
   {
      return memo_message::deserialize(std::string(data.begin(), data.end())).text;
   }
}

} }
