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

#include <graphene/chain/protocol/types.hpp>

namespace graphene { namespace chain {

struct configuration {
   std::string graphene_symbol;
   std::string graphene_address_prefix;
   uint32_t graphene_min_account_name_length;
   uint32_t graphene_max_account_name_length;
   uint32_t graphene_min_asset_symbol_length;
   uint32_t graphene_max_asset_symbol_length;
   int64_t graphene_max_share_supply;
   uint32_t graphene_max_pay_rate;
   uint32_t graphene_max_sig_check_depth;
   uint32_t graphene_min_transaction_size_limit;
   uint32_t graphene_min_block_interval;
   uint32_t graphene_max_block_interval;
   uint32_t graphene_default_block_interval;
   uint32_t graphene_default_max_transaction_size;
   uint32_t graphene_default_max_block_size;
   uint32_t graphene_default_max_time_until_expiration;
   uint32_t graphene_default_maintenance_interval;
   uint32_t graphene_default_maintenance_skip_slots;
   uint32_t graphene_min_undo_history;
   uint32_t graphene_max_undo_history;
   uint32_t graphene_min_block_size_limit;
   uint32_t graphene_min_transaction_expiration_limit;
   uint32_t graphene_blockchain_precision;
   uint32_t graphene_blockchain_precision_digits;
   uint32_t graphene_default_transfer_fee;
   uint64_t graphene_max_instance_id;
   uint32_t graphene_100_percent;
   uint32_t graphene_1_percent;
   uint32_t graphene_max_market_fee_percent;
   uint32_t graphene_default_force_settlement_delay;
   uint32_t graphene_default_force_settlement_offset;
   uint32_t graphene_default_force_settlement_max_volume;
   uint32_t graphene_default_price_feed_lifetime;
   uint32_t graphene_max_feed_producers;
   uint32_t graphene_default_max_authority_membership;
   uint32_t graphene_default_max_asset_whitelist_authorities;
   uint32_t graphene_default_max_asset_feed_publishers;
   uint32_t graphene_collateral_ratio_denom;
   uint32_t graphene_min_collateral_ratio;
   uint32_t graphene_max_collateral_ratio;
   uint32_t graphene_default_maintenance_collateral_ratio;
   uint32_t graphene_default_max_short_squeeze_ratio;
   uint32_t graphene_default_margin_period_sec;
   uint32_t graphene_default_max_miners;
   uint32_t graphene_default_max_proposal_lifetime_sec;
   uint32_t graphene_default_miner_proposal_review_period_sec;
   uint32_t graphene_default_network_percent_of_fee;
   uint32_t graphene_default_lifetime_referrer_percent_of_fee;
   uint32_t graphene_default_max_bulk_discount_percent;
   int64_t graphene_default_bulk_discount_threshold_min;
   int64_t graphene_default_bulk_discount_threshold_max;
   uint32_t graphene_default_cashback_vesting_period_sec;
   int64_t graphene_default_cashback_vesting_threshold;
   uint32_t graphene_default_burn_percent_of_fee;
   uint32_t graphene_miner_pay_percent_precision;
   uint32_t graphene_default_max_assert_opcode;
   uint64_t graphene_default_fee_liquidation_threshold;
   uint32_t graphene_default_accounts_per_fee_scale;
   uint32_t graphene_default_account_fee_scale_bitshifts;
   uint32_t graphene_max_worker_name_length;
   uint32_t graphene_max_url_length;
   uint64_t graphene_near_schedule_ctr_iv;
   uint64_t graphene_far_schedule_ctr_iv;
   uint32_t graphene_core_asset_cycle_rate;
   uint32_t graphene_core_asset_cycle_rate_bits;
   int64_t graphene_default_miner_pay_per_block;
   uint32_t graphene_default_miner_pay_vesting_seconds;
   uint16_t graphene_max_interest_apr;
   account_id_type graphene_miner_account;
   account_id_type graphene_null_account;
   account_id_type graphene_temp_account;
};

configuration get_configuration();

} } // graphene::chain

FC_REFLECT( graphene::chain::configuration,
            (graphene_symbol)
            (graphene_address_prefix)
            (graphene_min_account_name_length)
            (graphene_max_account_name_length)
            (graphene_min_asset_symbol_length)
            (graphene_max_asset_symbol_length)
            (graphene_max_share_supply)
            (graphene_max_pay_rate)
            (graphene_max_sig_check_depth)
            (graphene_min_transaction_size_limit)
            (graphene_min_block_interval)
            (graphene_max_block_interval)
            (graphene_default_block_interval)
            (graphene_default_max_transaction_size)
            (graphene_default_max_block_size)
            (graphene_default_max_time_until_expiration)
            (graphene_default_maintenance_interval)
            (graphene_default_maintenance_skip_slots)
            (graphene_min_undo_history)
            (graphene_max_undo_history)
            (graphene_min_block_size_limit)
            (graphene_min_transaction_expiration_limit)
            (graphene_blockchain_precision)
            (graphene_blockchain_precision_digits)
            (graphene_default_transfer_fee)
            (graphene_max_instance_id)
            (graphene_100_percent)
            (graphene_1_percent)
            (graphene_max_market_fee_percent)
            (graphene_default_force_settlement_delay)
            (graphene_default_force_settlement_offset)
            (graphene_default_force_settlement_max_volume)
            (graphene_default_price_feed_lifetime)
            (graphene_max_feed_producers)
            (graphene_default_max_authority_membership)
            (graphene_default_max_asset_whitelist_authorities)
            (graphene_default_max_asset_feed_publishers)
            (graphene_collateral_ratio_denom)
            (graphene_min_collateral_ratio)
            (graphene_max_collateral_ratio)
            (graphene_default_maintenance_collateral_ratio)
            (graphene_default_max_short_squeeze_ratio)
            (graphene_default_margin_period_sec)
            (graphene_default_max_miners)
            (graphene_default_max_proposal_lifetime_sec)
            (graphene_default_miner_proposal_review_period_sec)
            (graphene_default_network_percent_of_fee)
            (graphene_default_lifetime_referrer_percent_of_fee)
            (graphene_default_max_bulk_discount_percent)
            (graphene_default_bulk_discount_threshold_min)
            (graphene_default_bulk_discount_threshold_max)
            (graphene_default_cashback_vesting_period_sec)
            (graphene_default_cashback_vesting_threshold)
            (graphene_default_burn_percent_of_fee)
            (graphene_miner_pay_percent_precision)
            (graphene_default_max_assert_opcode)
            (graphene_default_fee_liquidation_threshold)
            (graphene_default_accounts_per_fee_scale)
            (graphene_default_account_fee_scale_bitshifts)
            (graphene_max_worker_name_length)
            (graphene_max_url_length)
            (graphene_near_schedule_ctr_iv)
            (graphene_far_schedule_ctr_iv)
            (graphene_core_asset_cycle_rate)
            (graphene_core_asset_cycle_rate_bits)
            (graphene_default_miner_pay_per_block)
            (graphene_default_miner_pay_vesting_seconds)
            (graphene_max_interest_apr)
            (graphene_miner_account)
            (graphene_null_account)
            (graphene_temp_account)
)
