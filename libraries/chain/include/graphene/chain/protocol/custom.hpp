/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
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

#pragma once
#include <graphene/chain/protocol/base.hpp>
#include <graphene/chain/protocol/asset.hpp>
#include <fc/io/json.hpp>

namespace graphene { namespace chain {

   struct message_payload_receivers_data
   {
      message_payload_receivers_data() = default;

      /**
       * @brief Construct encrypted message
       * @param msg the message to encrypt
       * @param priv the private key of sender
       * @param pub the public key of receiver
       * @param id the account id of receiver
       * @param nonce the salt number to use for message encryption (will be generated if zero)
       */
      message_payload_receivers_data(const std::string &msg, const private_key_type& priv, const public_key_type& pub, account_id_type id, uint64_t nonce = 0);

      /**
       * @brief Decrypt message
       * @param priv the private key of sender/receiver
       * @param pub the public key of receiver/sender
       * @return decrypted message
       */
      std::string get_message(const private_key_type& priv, const public_key_type& pub) const;

      account_id_type to;
      public_key_type pub_to;
      uint64_t nonce = 0;
      std::vector<char> data;
   };

   struct message_payload {
      account_id_type from;
      public_key_type pub_from;

      std::vector<message_payload_receivers_data> receivers_data;
   };

   enum custom_operation_subtype : int;
   /**
    * @brief provides a generic way to add higher level protocols on top of miner consensus
    * @ingroup operations
    *
    * There is no validation for this operation other than that required auths are valid and a fee
    * is paid that is appropriate for the data contained.
    */
   struct custom_operation : public base_operation<false>
   {
      struct fee_parameters_type {
         uint64_t fee = GRAPHENE_BLOCKCHAIN_PRECISION/1000;
         uint32_t price_per_kbyte = 10;
      };

      enum subtype : uint16_t
      {
         custom_operation_subtype_undefined = 0,
         custom_operation_subtype_messaging
      };

      asset                     fee;
      account_id_type           payer;
      boost::container::flat_set<account_id_type> required_auths;
      uint16_t                  id = 0;
      std::vector<char>         data;

      account_id_type   fee_payer()const { return payer; }
      void              validate()const;
      share_type        calculate_fee(const fee_parameters_type& k)const;

      void get_messaging_payload(message_payload& pl) const
      {
         FC_ASSERT(data.size());
         fc::variant tmp = fc::json::from_string(std::string(data.begin(), data.end()));
         fc::from_variant(tmp, pl);
      }

      void set_messaging_payload(const message_payload& pl)
      {
         fc::variant tmp;
         fc::to_variant(pl, tmp);
         std::string s = fc::json::to_string(tmp);
         data = std::vector<char>(s.begin(), s.end());
      }
   };

} } // namespace graphene::chain

FC_REFLECT( graphene::chain::message_payload_receivers_data, (to)(pub_to)(nonce)(data) )
FC_REFLECT( graphene::chain::message_payload, (from)(pub_from)(receivers_data) )
FC_REFLECT( graphene::chain::custom_operation::fee_parameters_type, (fee)(price_per_kbyte) )
FC_REFLECT( graphene::chain::custom_operation, (fee)(payer)(required_auths)(id)(data) )
