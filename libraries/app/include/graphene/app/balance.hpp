/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */

#pragma once

#include <fc/container/flat.hpp>
#include <graphene/chain/protocol/operations.hpp>
#include <graphene/chain/protocol/transaction.hpp>
#include <graphene/chain/protocol/types.hpp>

namespace graphene { namespace app {

struct asset_array;

void operation_get_balance_history(const graphene::chain::operation& op, graphene::chain::account_id_type account, asset_array& result, graphene::chain::asset& fee_result );


} }