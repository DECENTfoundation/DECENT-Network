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
#include <graphene/db/object.hpp>
#include <graphene/db/generic_index.hpp>
#include <boost/multi_index/hashed_index.hpp>

namespace graphene { namespace chain {

   /**
    * @brief tracks the history of transactions
    * @ingroup object
    * @ingroup implementation
    *
    * @note By default these objects are not tracked, the transaction_history_plugin must
    * be loaded for these objects to be maintained.
    */
   class transaction_history_object : public abstract_object<transaction_history_object>
   {
   public:
      static const uint8_t space_id = implementation_ids;
      static const uint8_t type_id  = impl_transaction_history_object_type;

      transaction_id_type tx_id;
      uint32_t          block_num = 0;
      /** the transaction in the block */
      uint16_t          trx_in_block = 0;
   };


   struct by_id;
   struct by_tx_id;

   typedef multi_index_container<transaction_history_object,
      indexed_by<
         ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
         hashed_non_unique< tag<by_tx_id>, BOOST_MULTI_INDEX_MEMBER(transaction_history_object, transaction_id_type, tx_id), std::hash<transaction_id_type> >
      >
   > transaction_history_multi_index_type;

   typedef generic_index<transaction_history_object, transaction_history_multi_index_type> transaction_history_index;

   } } // graphene::chain

FC_REFLECT_DERIVED( graphene::chain::transaction_history_object, (graphene::chain::object),
                    (tx_id)(block_num)(trx_in_block) )
