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
#include <graphene/chain/protocol/operations.hpp>
#include <graphene/chain/protocol/fee_schedule.hpp>
#include <fc/smart_ref_impl.hpp>

namespace graphene { namespace chain {


void referendum_create_operation::validate() const
{
   FC_ASSERT( !proposed_ops.empty() );
   FC_ASSERT(fee >= asset(1000* GRAPHENE_HXCHAIN_PRECISION));
   for( const auto& op : proposed_ops ) operation_validate( op.op );
}

void referendum_update_operation::validate()const
{
	FC_ASSERT(fee.amount >= 0);
}
void referendum_update_operation::get_required_authorities(vector<authority>& a) const
{
	authority auth;
	for (const auto& k : key_approvals_to_add)
		auth.address_auths[k] = 1;
	for (const auto& k : key_approvals_to_remove)
		auth.address_auths[k] = 1;
	auth.weight_threshold = auth.address_auths.size();

	a.emplace_back(std::move(auth));
}
void referendum_accelerate_pledge_operation::validate()const
{
	FC_ASSERT(fee >= asset(0));
}


} } // graphene::chain
