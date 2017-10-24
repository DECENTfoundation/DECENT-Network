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
#include <graphene/chain/message_object.hpp>

namespace graphene{
namespace chain {


void message_receiver_index::object_inserted(const object &obj) {
   assert(dynamic_cast<const message_object *>(&obj)); // for debug only
   const message_object &a = static_cast<const message_object &>(obj);

   auto recipients = get_key_recipients(a);

   for( auto &item : recipients ) {
      message_to_receiver_memberships[ item ].insert(obj.id);
   }
}

void message_receiver_index::object_removed(const object &obj) {
   assert(dynamic_cast<const message_object *>(&obj)); // for debug only
   const message_object &a = static_cast<const message_object &>(obj);

   auto recipients = get_key_recipients(a);

   for( auto &item : recipients ) {
      message_to_receiver_memberships[ item ].erase(obj.id);
   }
}

void message_receiver_index::about_to_modify(const object &before) {
}

void message_receiver_index::object_modified(const object &after) {
}

set<account_id_type> message_receiver_index::get_key_recipients(const message_object &a) const {
   set<account_id_type> result;
   for( auto item : a.receivers_data )
      result.insert(item.receiver);

   return result;
}

}}//namespace