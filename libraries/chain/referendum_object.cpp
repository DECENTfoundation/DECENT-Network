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
#include <graphene/chain/database.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/referendum_object.hpp>
#include <graphene/chain/witness_object.hpp>
#include <boost/multiprecision/cpp_int.hpp>
namespace graphene { namespace chain {

bool referendum_object::is_authorized_to_execute(database& db) const
{
   transaction_evaluation_state dry_run_eval(&db);

   try {
	   if (finished == true)
		   return false;
	   auto& miner_idx = db.get_index_type<miner_index>().indices().get<by_account>();
	   auto& account_idx = db.get_index_type<account_index>().indices().get<by_address>();
	   boost::multiprecision::uint256_t total_weights = 0;
	   boost::multiprecision::uint256_t approved_key_weights = 0;
	   for (auto acc : required_account_approvals)
	   {
		   auto iter = miner_idx.find(account_idx.find(acc)->get_id());
		   auto temp_hi = boost::multiprecision::uint128_t(iter->pledge_weight.hi);
		   temp_hi <<= 64;
		   total_weights += temp_hi+boost::multiprecision::uint128_t(iter->pledge_weight.lo);
	   }
	   
	   for (auto acc : approved_key_approvals)
	   {
		   auto iter = miner_idx.find(account_idx.find(acc)->get_id());
		   auto temp_hi = boost::multiprecision::uint128_t(iter->pledge_weight.hi);
		   temp_hi <<= 64;
		   approved_key_weights += temp_hi + boost::multiprecision::uint128_t(iter->pledge_weight.lo);
	   }

	   return approved_key_weights >= (total_weights/3*2 + 1);
   } 
   catch ( const fc::exception& e )
   {
      //idump((available_active_approvals));
      //wlog((e.to_detail_string()));
      return false;
   }
   return true;
}




} } // graphene::chain
