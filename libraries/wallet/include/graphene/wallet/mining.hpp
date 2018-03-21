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

#ifndef DECENT_WALLET_MINING_H
#define DECENT_WALLET_MINING_H

/**
 * @brief Lists all miners registered in the blockchain.
 * This returns a list of all account names that own miners, and the associated miner id,
 * sorted by name. This lists miners whether they are currently voted in or not.
 * Use the \c lowerbound and \c limit parameters to page through the list.  To retrieve all miners,
 * start by setting \c lowerbound to the empty string \c "", and then each iteration, pass
 * the last miner name returned as the \c lowerbound for the next \c list_miners() call.
 * @param lowerbound the name of the first miner to return.  If the named miner does not exist,
 *                   the list will start at the miner that comes after \c lowerbound
 * @param limit the maximum number of miners to return (max: 1000)
 * @return a list of miners mapping miner names to miner ids
 * @ingroup WalletAPI_Mining
 */
map<string,miner_id_type>  list_miners(const string& lowerbound, uint32_t limit);

/**
 * @brief Returns information about the given miner.
 * @param owner_account the name or id of the miner account owner, or the id of the miner
 * @return the information about the miner stored in the block chain
 * @ingroup WalletAPI_Mining
 */
miner_object get_miner(const string& owner_account);

/**
 * @brief Creates a miner object owned by the given account.
 * @note an account can have at most one miner object.
 * @param owner_account the name or id of the account which is creating the miner
 * @param url a URL to include in the miner record in the blockchain.  Clients may
 *            display this when showing a list of miners.  May be blank.
 * @param broadcast \c true to broadcast the transaction on the network
 * @return the signed transaction registering a miner
 * @ingroup WalletAPI_Mining
 */
signed_transaction create_miner(const string& owner_account,
                                const string& url,
                                bool broadcast = false);

/**
 * @brief Update a miner object owned by the given account.
 * @param miner_name The name of the miner's owner account. Also accepts the ID of the owner account or the ID of the miner.
 * @param url Same as for create_miner.  The empty string makes it remain the same.
 * @param block_signing_key the new block signing public key.  The empty string makes it remain the same
 * @param broadcast \c true if you wish to broadcast the transaction.
 * @ingroup WalletAPI_Mining
 */
signed_transaction update_miner(const string& miner_name,
                                const string& url,
                                const string& block_signing_key,
                                bool broadcast = false);

/**
 * @brief Get information about a vesting balance object.
 * @param account_name an account name, account ID, or vesting balance object ID.
 * @ingroup WalletAPI_Mining
 */
vector< vesting_balance_object_with_info > get_vesting_balances( const string& account_name );

/**
 * @brief Withdraw a vesting balance.
 * @param miner_name the account name of the miner, also accepts account ID or vesting balance ID type.
 * @param amount the amount to withdraw.
 * @param asset_symbol the symbol of the asset to withdraw
 * @param broadcast \c true if you wish to broadcast the transaction
 * @ingroup WalletAPI_Mining
 */
signed_transaction withdraw_vesting(const string& miner_name,
                                    const string& amount,
                                    const string& asset_symbol,
                                    bool broadcast = false);

/**
 * @brief Vote for a given miner.
 * An account can publish a list of all miners they approve of. This
 * command allows you to add or remove miners from this list.
 * Each account's vote is weighted according to the number of shares of the
 * core asset owned by that account at the time the votes are tallied.
 * @note You cannot vote against a miner, you can only vote for the miner
 *       or not vote for the miner.
 * @see \c list_miners()
 * @param voting_account the name or id of the account who is voting with their shares
 * @param miner the name or id of the miner' owner account
 * @param approve \c true if you wish to vote in favor of that miner, \c false to
 *                remove your vote in favor of that miner
 * @param broadcast \c true if you wish to broadcast the transaction
 * @return the signed transaction changing your vote for the given miner
 * @ingroup WalletAPI_Mining
 */
signed_transaction vote_for_miner(const string& voting_account,
                                  const string& miner,
                                  bool approve,
                                  bool broadcast = false);

/**
 * @brief Set the voting proxy for an account.
 * If a user does not wish to take an active part in voting, they can choose
 * to allow another account to vote their stake.
 * Setting a vote proxy does not remove your previous votes from the blockchain,
 * they remain there but are ignored. If you later null out your vote proxy,
 * your previous votes will take effect again.
 * This setting can be changed at any time.
 * @param account_to_modify the name or id of the account to update
 * @param voting_account the name or id of an account authorized to vote account_to_modify's shares,
 *                       or null to vote your own shares
 * @param broadcast \c true if you wish to broadcast the transaction
 * @return the signed transaction changing your vote proxy settings
 * @ingroup WalletAPI_Mining
 */
signed_transaction set_voting_proxy(const string& account_to_modify,
                                    optional<string> voting_account,
                                    bool broadcast = false);

/**
 * @brief Set your vote for the number of miners in the system.
 * Each account can voice their opinion on how many
 * miners there should be in the active miner list. These
 * are independent of each other. You must vote your approval of at least as many
 * miners as you claim there should be (you can't say that there should
 * be 20 miners but only vote for 10).
 * There are maximum values for each set in the blockchain parameters (currently
 * defaulting to 1001).
 * This setting can be changed at any time. If your account has a voting proxy
 * set, your preferences will be ignored.
 * @param account_to_modify the name or id of the account to update
 * @param desired_number_of_miners
 * @param broadcast \c true if you wish to broadcast the transaction
 * @return the signed transaction changing your vote proxy settings
 * @ingroup WalletAPI_Mining
 */
signed_transaction set_desired_miner_count(const string& account_to_modify,
                                           uint16_t desired_number_of_miners,
                                           bool broadcast = false);

/**
 * @brief
 * @param term search term
 * @param order order field. Available options are defined in 'database_api.cpp'
 * @param id the id of content object to start searching from
 * @param count maximum number of contents to fetch (must not exceed 1000)
 * @return the contents found
 * @ingroup WalletAPI_Mining
 */
vector<miner_voting_info> search_miner_voting(const string& term,
                                  const string& order,
                                  const string& account_id,
                                  const string& id,
                                  uint32_t count ) const;


#endif //DECENT_﻿WALLET_MINING_H
