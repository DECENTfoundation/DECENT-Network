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
#include <graphene/chain/miner_evaluator.hpp>
#include <graphene/chain/miner_object.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/protocol/vote.hpp>
#include <fc/smart_ref_impl.hpp>

namespace graphene { namespace chain {

void_result miner_create_evaluator::do_evaluate( const miner_create_operation& op )
{
   return void_result();
}

graphene::db::object_id_type miner_create_evaluator::do_apply( const miner_create_operation& op )
{ try {
   vote_id_type vote_id;
   db().modify(db().get_global_properties(), [&vote_id](global_property_object& p) {
      vote_id = get_next_vote_id(p, vote_id_type::miner);
   });

   const auto& new_miner_object = db().create<miner_object>( [&]( miner_object& obj ){
         obj.miner_account  = op.miner_account;
         obj.signing_key      = op.block_signing_key;
         obj.vote_id          = vote_id;
         obj.url              = op.url;
   });
   return new_miner_object.id;
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result miner_update_evaluator::do_evaluate( const miner_update_operation& op )
{ try {
   FC_ASSERT(db().get(op.miner).miner_account == op.miner_account);
   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result miner_update_evaluator::do_apply( const miner_update_operation& op )
{ try {
   database& _db = db();
   _db.modify(
      _db.get(op.miner),
      [&]( miner_object& wit )
      {
         if( op.new_url.valid() )
            wit.url = *op.new_url;
         if( op.new_signing_key.valid() )
            wit.signing_key = *op.new_signing_key;
      });
   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result miner_update_global_parameters_evaluator::do_evaluate(const miner_update_global_parameters_operation& o)
{ try {
      FC_ASSERT(trx_state->_is_proposed_trx);

      return void_result();
   } FC_CAPTURE_AND_RETHROW( (o) ) }

void_result miner_update_global_parameters_evaluator::do_apply(const miner_update_global_parameters_operation& o)
{ try {
      db().modify(db().get_global_properties(), [&o](global_property_object& p) {
         p.pending_parameters = o.new_parameters;
      });

      return void_result();
   } FC_CAPTURE_AND_RETHROW( (o) ) }

} } // graphene::chain
