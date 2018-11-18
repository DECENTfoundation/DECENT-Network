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

#include <graphene/chain/protocol/transaction.hpp>
#include <graphene/chain/transaction_evaluation_state.hpp>

#include <graphene/db/generic_index.hpp>

namespace graphene { namespace chain {


/**
 *  @brief tracks the approval of a partially approved transaction 
 *  @ingroup object
 *  @ingroup protocol
 */
class referendum_object : public abstract_object<referendum_object>
{
   public:
      static const uint8_t space_id = protocol_ids;
      static const uint8_t type_id = referendum_object_type;
	  account_id_type               proposer;
      time_point_sec                expiration_time;
      optional<time_point_sec>      review_period_time;
      transaction                   proposed_transaction;
	  share_type                  pledge=0;
	  fc::uint128_t               citizen_pledge = 0;
	  flat_set<address>     approved_key_approvals;
	  flat_set<address>     disapproved_key_approvals;
	  flat_set<address>     required_account_approvals;
	  bool                  finished = false;
      bool is_authorized_to_execute(database& db)const;
	  
};

struct by_expiration;
struct by_pledge;
typedef boost::multi_index_container<
   referendum_object,
   indexed_by<
      ordered_unique< tag< by_id >, member< object, object_id_type, &object::id > >,
      ordered_non_unique< tag< by_expiration >, member< referendum_object, time_point_sec, &referendum_object::expiration_time > >,
	ordered_non_unique < tag<by_pledge>,composite_key<referendum_object,
	                                 member<referendum_object, share_type, &referendum_object::pledge>,
	                                 member<referendum_object,fc::uint128_t,&referendum_object::citizen_pledge>,
	                                 member<object, object_id_type, &object::id>>,
	                                 composite_key_compare<std::less<share_type>,std::less<fc::uint128_t>,std::greater<object_id_type>>>
   >
> referendum_multi_index_container;
typedef generic_index<referendum_object, referendum_multi_index_container> referendum_index;

} } // graphene::chain

FC_REFLECT_DERIVED( graphene::chain::referendum_object, (graphene::chain::object),(proposer)
                    (expiration_time)(review_period_time)(proposed_transaction)(pledge)(citizen_pledge)(approved_key_approvals)(disapproved_key_approvals)(required_account_approvals)(finished))
