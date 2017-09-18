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
#include <graphene/chain/protocol/operations.hpp>
#include <graphene/db/generic_index.hpp>
#include <boost/multi_index/composite_key.hpp>

namespace graphene {
   namespace chain {

   class message_object : public graphene::db::abstract_object<message_object>
   {
   public:
      static const uint8_t space_id = implementation_ids;
      static const uint8_t type_id = impl_messaging_object_type;

      fc::time_point_sec created;
      account_id_type sender;
      account_id_type receiver;
      std::string text;

   };

   struct by_sender;
   struct by_receiver;
   struct by_created;
   
   typedef multi_index_container<
      message_object,
      indexed_by<
      ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
      ordered_non_unique< tag<by_created>, member< message_object, time_point_sec, &message_object::created > >,
      ordered_non_unique< tag<by_sender>, member< message_object, account_id_type, &message_object::sender > >,
      ordered_non_unique< tag<by_receiver>, member< message_object, account_id_type, &message_object::receiver > >
      >
   > message_multi_index_type;

   typedef generic_index<message_object, message_multi_index_type> message_index;
   }
} // namespaces

FC_REFLECT_DERIVED(
   graphene::chain::message_object,
   (graphene::db::object),
   (created)
   (sender)
   (receiver)
   (text)
)