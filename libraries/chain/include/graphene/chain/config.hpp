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

#define GRAPHENE_DEFAULT_BLOCK_INTERVAL      5 /* seconds */
#define GRAPHENE_BLOCKCHAIN_PRECISION        uint64_t( 100000000 )
#define GRAPHENE_BLOCKCHAIN_PRECISION_DIGITS 8

#define DECENT_BLOCK_REWARD_0 uint64_t(0)
#define DECENT_BLOCK_REWARD_1 uint64_t(37 * GRAPHENE_BLOCKCHAIN_PRECISION / 100)
#define DECENT_BLOCK_REWARD_2 uint64_t(DECENT_BLOCK_REWARD_1 / 2)
#define DECENT_BLOCK_REWARD_3 uint64_t(DECENT_BLOCK_REWARD_2 / 2)
#define DECENT_BLOCK_REWARD_4 uint64_t(DECENT_BLOCK_REWARD_3 / 2)
#define DECENT_BLOCK_REWARD_5 uint64_t(0)

#define DECENT_SPLIT_0    100000
#define DECENT_SPLIT_1  31653280
#define DECENT_SPLIT_2  63206560
#define DECENT_SPLIT_3  94777120
#define DECENT_SPLIT_4 126330400

#define GRAPHENE_SYMBOL "DCT"

#define GRAPHENE_MIN_ACCOUNT_NAME_LENGTH 5
#define GRAPHENE_MAX_ACCOUNT_NAME_LENGTH 63

#define GRAPHENE_MIN_ASSET_SYMBOL_LENGTH 3
#define GRAPHENE_MAX_ASSET_SYMBOL_LENGTH 16

#define DECENT_RTP_VALIDITY   7200 //2 hours

#define GRAPHENE_MAX_SHARE_SUPPLY      int64_t( 7319777577456890ll )
#define GRAPHENE_INITIAL_SHARE_SUPPLY  int64_t( 5130608937456900ll )
#define GRAPHENE_MAX_SIG_CHECK_DEPTH 2

#define DECENT_MAX_FILE_SIZE 10000 /// MBs, aka: 10 GBs
#define DECENT_MAX_COMMENT_SIZE 100 /// chars
#define DECENT_MAX_SUBSCRIPTION_PERIOD 365 /// days, aka: 1 year
#define DECENT_MAX_SEEDING_PRICE 100000000 /// 1 DCT per MB
/**
 * Don't allow the committee_members to publish a limit that would make the network unable to operate.
 */
#define GRAPHENE_MIN_TRANSACTION_SIZE_LIMIT 1024
#define GRAPHENE_MIN_BLOCK_INTERVAL   1 /* seconds */
#define GRAPHENE_MAX_BLOCK_INTERVAL  30 /* seconds */

#define GRAPHENE_DEFAULT_MAX_TRANSACTION_SIZE      4096
#define GRAPHENE_DEFAULT_MAX_BLOCK_SIZE            (GRAPHENE_DEFAULT_MAX_TRANSACTION_SIZE*GRAPHENE_DEFAULT_BLOCK_INTERVAL*200)
#define GRAPHENE_DEFAULT_MAX_TIME_UNTIL_EXPIRATION (60*60*24) // seconds,  aka: 1 day
#define GRAPHENE_DEFAULT_MAINTENANCE_INTERVAL      (60*60*24) // seconds, aka: 1 day
#define GRAPHENE_DEFAULT_MAINTENANCE_SKIP_SLOTS    3  // number of slots to skip for maintenance interval

#define GRAPHENE_MIN_UNDO_HISTORY 10
#define GRAPHENE_MAX_UNDO_HISTORY 10000

#define GRAPHENE_MIN_BLOCK_SIZE_LIMIT (GRAPHENE_MIN_TRANSACTION_SIZE_LIMIT*5) // 5 transactions per block

#define GRAPHENE_MAX_INSTANCE_ID (uint64_t(-1)>>16)
/** percentage fields are fixed point with a denominator of 10,000 */
#define GRAPHENE_100_PERCENT     10000
#define GRAPHENE_1_PERCENT       (GRAPHENE_100_PERCENT/100)

#define GRAPHENE_DEFAULT_PRICE_FEED_LIFETIME       (60*60*24) ///< 1 day
#define GRAPHENE_DEFAULT_MAX_AUTHORITY_MEMBERSHIP  10
#define GRAPHENE_DEFAULT_MAX_ASSET_FEED_PUBLISHERS 10

#define GRAPHENE_DEFAULT_MIN_MINER_COUNT                    (11)
#define GRAPHENE_DEFAULT_MAX_MINERS                         (1001) // SHOULD BE ODD
#define GRAPHENE_DEFAULT_MAX_PROPOSAL_LIFETIME_SEC          (60*60*24*7*4) // Four weeks
#define GRAPHENE_DEFAULT_MINER_PROPOSAL_REVIEW_PERIOD_SEC   (60*60*24*7*2) // Two weeks
#define GRAPHENE_DEFAULT_CASHBACK_VESTING_PERIOD_SEC        (60*60*24*365) ///< 1 year
#define GRAPHENE_DEFAULT_CASHBACK_VESTING_THRESHOLD         (GRAPHENE_BLOCKCHAIN_PRECISION*int64_t(100))
#define GRAPHENE_DEFAULT_MAX_ASSERT_OPCODE                  1

#define GRAPHENE_MAX_URL_LENGTH 127

#define GRAPHENE_DEFAULT_MINER_PAY_PER_BLOCK       (GRAPHENE_BLOCKCHAIN_PRECISION / int64_t( 10) )
#define GRAPHENE_DEFAULT_MINER_PAY_VESTING_SECONDS (60*60*24)

#define GRAPHENE_RECENTLY_MISSED_COUNT_INCREMENT 4
#define GRAPHENE_RECENTLY_MISSED_COUNT_DECREMENT 3

#define GRAPHENE_CURRENT_DB_VERSION "DCT1.5.0"

#define GRAPHENE_IRREVERSIBLE_THRESHOLD (70 * GRAPHENE_1_PERCENT)

/**
 *  Reserved Account IDs with special meaning
 */
///@{
/// Represents the current miners
#define GRAPHENE_MINER_ACCOUNT (graphene::chain::account_id_type(0))
/// Represents the canonical account with NO authority (nobody can access funds in null account)
#define GRAPHENE_NULL_ACCOUNT (graphene::chain::account_id_type(1))
/// Represents the canonical account with WILDCARD authority (anybody can access funds in temp account)
#define GRAPHENE_TEMP_ACCOUNT (graphene::chain::account_id_type(2))
/// Represents the canonical account for specifying you will vote directly (as opposed to a proxy)
#define GRAPHENE_PROXY_TO_SELF_ACCOUNT (graphene::chain::account_id_type(3))
/// DECENT maintenance account
/// Sentinel value used in the scheduler.
#define GRAPHENE_NULL_MINER (graphene::chain::miner_id_type(0))
///@}
