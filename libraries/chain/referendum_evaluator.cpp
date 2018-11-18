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
#include <graphene/chain/referendum_evaluator.hpp>
#include <graphene/chain/referendum_object.hpp>
#include <graphene/chain/proposal_object.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/protocol/fee_schedule.hpp>
#include <graphene/chain/exceptions.hpp>
#include <fc/smart_ref_impl.hpp>

namespace graphene { namespace chain {

void_result referendum_create_evaluator::do_evaluate(const referendum_create_operation& o)
{ try {
   const database& d = db();
   const auto& global_parameters = d.get_global_properties().parameters;
   //proposer has to be a formal guard
   auto  proposer = o.proposer;
   auto& citizen_idx = d.get_index_type<miner_index>().indices().get<by_account>();
   auto iter = citizen_idx.find(proposer);
   FC_ASSERT(iter != citizen_idx.end(), "referendum proposer must be a citizen.");
   _pledge = iter->pledge_weight;
   const auto& dynamic_obj = d.get_dynamic_global_properties();
   FC_ASSERT(!dynamic_obj.referendum_flag || (dynamic_obj.referendum_flag && (d.head_block_time() < dynamic_obj.next_vote_time)));
   const auto& proposal_idx = d.get_index_type<proposal_index>().indices().get<by_id>();
   for (const auto& proposal : proposal_idx)
   {
	   for (const auto& op :proposal.proposed_transaction.operations)
	   {
		   FC_ASSERT(op.which() != operation::tag<guard_member_update_operation>::value, "there is other proposal for senator election.");
	   }
   }
   for (const auto& op : o.proposed_ops)
   {
	   GRAPHENE_ASSERT(op.op.which() == operation::tag<citizen_referendum_senator_operation>::value, proposal_create_invalid_proposals,"invalid referendum");
   }
   for (const op_wrapper& op : o.proposed_ops)
   {
	   _proposed_trx.operations.push_back(op.op);

   }
   _proposed_trx.validate();
   transaction_evaluation_state eval_state(&db());
   eval_state.operation_results.reserve(_proposed_trx.operations.size());

   auto ptrx = processed_transaction(_proposed_trx);
   eval_state._trx = &ptrx;

   for (const auto& op : _proposed_trx.operations)
   {
	   unique_ptr<op_evaluator>& eval = db().get_evaluator(op);
	   eval->evaluate(eval_state, op, false);
   }
   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

object_id_type referendum_create_evaluator::do_apply(const referendum_create_operation& o)
{ try {
   database& d = db();
   const auto& ref_obj= d.create<referendum_object>([&](referendum_object& referendum) {
	   _proposed_trx.expiration = d.head_block_time()+ fc::seconds(HX_REFERENDUM_PACKING_PERIOD + HX_REFERENDUM_VOTING_PERIOD);
	   referendum.proposed_transaction = _proposed_trx;
	   referendum.expiration_time = d.head_block_time() + fc::seconds(HX_REFERENDUM_PACKING_PERIOD + HX_REFERENDUM_VOTING_PERIOD);
       referendum.proposer = o.proposer;
       //proposal should only be approved by guard or miners
	   const auto& acc = d.get_index_type<account_index>().indices().get<by_id>();
	   const auto& iter = d.get_index_type<miner_index>().indices().get<by_account>();
	   std::for_each(iter.begin(), iter.end(), [&](const miner_object& a)
	   {
		   referendum.required_account_approvals.insert(acc.find(a.miner_account)->addr);
	   });
	   referendum.citizen_pledge = _pledge;
	   referendum.pledge = o.fee.amount ;
   });
   return ref_obj.id;
} FC_CAPTURE_AND_RETHROW( (o) ) }

void referendum_create_evaluator::pay_fee()
{
	FC_ASSERT(core_fees_paid.asset_id == asset_id_type());
	db().modify(db().get(asset_id_type()).dynamic_asset_data_id(db()), [this](asset_dynamic_data_object& d) {
		d.current_supply -= this->core_fees_paid.amount;
	});
}
void_result referendum_update_evaluator::do_evaluate(const referendum_update_operation& o)
{
	try
	{
		database& d = db();
		_referendum = &o.referendum(d);
		auto next_vote_time = d.get_dynamic_global_properties().next_vote_time;
		FC_ASSERT(d.head_block_time() >= next_vote_time,"the referendum vote is in its packing period.");
		FC_ASSERT(_referendum->finished == false, "the referendum has been finished.");
		return void_result();
	}
	FC_CAPTURE_AND_RETHROW((o))
}
void_result referendum_update_evaluator::do_apply(const referendum_update_operation& o)
{
	try {
		database& d = db();
		d.modify(*_referendum, [&o, &d](referendum_object& p) {
			//FC_ASSERT(p.required_account_approvals(););

			auto is_miner_or_guard = [&](const address& addr)->bool {
				return p.required_account_approvals.find(addr) != p.required_account_approvals.end();
			};

			for (const auto& addr : o.key_approvals_to_add)
			{
				if (!is_miner_or_guard(addr))
					continue;
				p.approved_key_approvals.insert(addr);
				p.disapproved_key_approvals.erase(addr);
			}

			for (const auto& addr : o.key_approvals_to_remove)
			{
				if (!is_miner_or_guard(addr))
					continue;
				p.disapproved_key_approvals.insert(addr);
				p.approved_key_approvals.erase(addr);
			}

		});

		if (_referendum->review_period_time)
			return void_result();

		if (_referendum->is_authorized_to_execute(d))
		{
			// All required approvals are satisfied. Execute!
			_executed_referendum = true;
			try {
				_processed_transaction = d.push_referendum(*_referendum);
			}
			catch (fc::exception& e) {
				wlog("Proposed transaction ${id} failed to apply once approved with exception:\n----\n${reason}\n----\nWill try again when it expires.",
					("id", o.referendum)("reason", e.to_detail_string()));
				db().remove(*_referendum);
				_referendum_failed = true;
			}
		}
		return void_result();
	}FC_CAPTURE_AND_RETHROW((o))
}
void_result referendum_accelerate_pledge_evaluator::do_evaluate(const referendum_accelerate_pledge_operation& o)
{
	try {
		const auto& referendum = db().get(o.referendum_id);
		FC_ASSERT(db().get(referendum.proposer).addr == o.fee_paying_account,"the referendum was not created by this account.");
	}FC_CAPTURE_AND_RETHROW((o))
}

void_result referendum_accelerate_pledge_evaluator::do_apply(const referendum_accelerate_pledge_operation& o)
{
	try
	{
		auto & referendum = db().get(o.referendum_id);
		db().modify(referendum, [&](referendum_object& obj) {
			obj.pledge += o.fee.amount;
		});
	}FC_CAPTURE_AND_RETHROW((o))
}

void referendum_accelerate_pledge_evaluator::pay_fee()
{
	FC_ASSERT(core_fees_paid.asset_id == asset_id_type());
	db().modify(db().get(asset_id_type()).dynamic_asset_data_id(db()), [this](asset_dynamic_data_object& d) {
		d.current_supply -= this->core_fees_paid.amount;
	});
}

} } // graphene::chain
