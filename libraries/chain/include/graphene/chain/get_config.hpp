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
   uint32_t graphene_default_block_interval;
   uint32_t graphene_blockchain_precision;
   uint32_t graphene_blockchain_precision_digits;
   uint64_t decent_block_reward_0;
   uint64_t decent_block_reward_1;
   uint64_t decent_block_reward_2;
   uint64_t decent_block_reward_3;
   uint64_t decent_block_reward_4;
   uint64_t decent_block_reward_5;
   uint32_t decent_split_0;
   uint32_t decent_split_1;
   uint32_t decent_split_2;
   uint32_t decent_split_3;
   uint32_t decent_split_4;
   std::string graphene_symbol;
   uint32_t graphene_min_account_name_length;
   uint32_t graphene_max_account_name_length;
   uint32_t graphene_min_asset_symbol_length;
   uint32_t graphene_max_asset_symbol_length;
   uint32_t decent_rtp_validity;
   int64_t graphene_max_share_supply;
   int64_t graphene_initial_share_supply;
   uint32_t graphene_max_sig_check_depth;
   uint32_t decent_max_file_size;
   uint32_t decent_max_comment_size;
   uint32_t decent_max_description_size;
   uint32_t decent_max_subscription_period;
   uint32_t decent_max_seeding_price;
   uint32_t decent_max_content_synopsis_size;
   uint32_t decent_max_content_uri_size;
   uint32_t graphene_min_transaction_size_limit;
   uint32_t graphene_min_block_interval;
   uint32_t graphene_max_block_interval;
   uint32_t graphene_default_max_transaction_size;
   uint32_t graphene_default_max_block_size;
   uint32_t graphene_default_max_time_until_expiration;
   uint32_t graphene_default_maintenance_interval;
   uint32_t graphene_default_maintenance_skip_slots;
   uint32_t graphene_min_undo_history;
   uint32_t graphene_max_undo_history;
   uint32_t graphene_min_block_size_limit;
   uint64_t graphene_max_instance_id;
   uint32_t graphene_100_percent;
   uint32_t graphene_1_percent;
   uint32_t graphene_default_price_feed_lifetime;
   uint32_t graphene_default_max_authority_membership;
   uint32_t graphene_default_max_asset_feed_publishers;
   uint32_t grephene_default_min_miner_count;
   uint32_t graphene_default_max_miners;
   uint32_t graphene_default_max_proposal_lifetime_sec;
   uint32_t graphene_default_miner_proposal_review_period_sec;
   uint32_t graphene_default_cashback_vesting_period_sec;
   int64_t graphene_default_cashback_vesting_threshold;
   uint32_t graphene_default_max_assert_opcode;
   uint32_t graphene_max_url_length;
   int64_t graphene_default_miner_pay_per_block;
   uint32_t graphene_default_miner_pay_vesting_seconds;
   uint32_t graphene_recently_missed_count_increment;
   uint32_t graphene_recently_missed_count_decrement;
   std::string graphene_current_db_version;
   uint32_t graphene_irreversible_threshold;
   account_id_type graphene_miner_account;
   account_id_type graphene_null_account;
   account_id_type graphene_temp_account;
   account_id_type graphene_proxy_to_self_account;
   miner_id_type graphene_null_miner;
};

configuration get_configuration();

} } // graphene::chain

FC_REFLECT( graphene::chain::configuration,
            (graphene_default_block_interval)
            (graphene_blockchain_precision)
            (graphene_blockchain_precision_digits)
            (decent_block_reward_0)
            (decent_block_reward_1)
            (decent_block_reward_2)
            (decent_block_reward_3)
            (decent_block_reward_4)
            (decent_block_reward_5)
            (decent_split_0)
            (decent_split_1)
            (decent_split_2)
            (decent_split_3)
            (decent_split_4)
            (graphene_symbol)
            (graphene_min_account_name_length)
            (graphene_max_account_name_length)
            (graphene_min_asset_symbol_length)
            (graphene_max_asset_symbol_length)
            (decent_rtp_validity)
            (graphene_max_share_supply)
            (graphene_initial_share_supply)
            (graphene_max_sig_check_depth)
            (decent_max_file_size)
            (decent_max_comment_size)
            (decent_max_description_size)
            (decent_max_subscription_period)
            (decent_max_seeding_price)
            (decent_max_content_synopsis_size)
            (decent_max_content_uri_size)
            (graphene_min_transaction_size_limit)
            (graphene_min_block_interval)
            (graphene_max_block_interval)
            (graphene_default_max_transaction_size)
            (graphene_default_max_block_size)
            (graphene_default_max_time_until_expiration)
            (graphene_default_maintenance_interval)
            (graphene_default_maintenance_skip_slots)
            (graphene_min_undo_history)
            (graphene_max_undo_history)
            (graphene_min_block_size_limit)
            (graphene_max_instance_id)
            (graphene_100_percent)
            (graphene_1_percent)
            (graphene_default_price_feed_lifetime)
            (graphene_default_max_authority_membership)
            (graphene_default_max_asset_feed_publishers)
            (grephene_default_min_miner_count)
            (graphene_default_max_miners)
            (graphene_default_max_proposal_lifetime_sec)
            (graphene_default_miner_proposal_review_period_sec)
            (graphene_default_cashback_vesting_period_sec)
            (graphene_default_cashback_vesting_threshold)
            (graphene_default_max_assert_opcode)
            (graphene_max_url_length)
            (graphene_default_miner_pay_per_block)
            (graphene_default_miner_pay_vesting_seconds)
            (graphene_recently_missed_count_increment)
            (graphene_recently_missed_count_decrement)
            (graphene_current_db_version)
            (graphene_irreversible_threshold)
            (graphene_miner_account)
            (graphene_null_account)
            (graphene_temp_account)
            (graphene_proxy_to_self_account)
            (graphene_null_miner)

)
