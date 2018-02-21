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

#ifndef DECENT_WALLET_PROPOSALS_H
#define DECENT_WALLET_PROPOSALS_H

/**
 * @brief Lists proposed transactions relevant to a user
 * @param account_or_id the name or id of the account
 * @return a list of proposed transactions
 * @ingroup WalletAPI_Proposals
 */
vector<proposal_object> get_proposed_transactions( const string& account_or_id ) const;

/**
 * @brief Encapsulates begin_builder_transaction(), add_operation_to_builder_transaction(),
 * propose_builder_transaction2(), set_fees_on_builder_transaction() functions for transfer operation.
 * @param proposer proposer
 * @param from the name or id of the account sending the funds
 * @param to the name or id of the account receiving the funds
 * @param amount the amount to send (in nominal units -- to send half of a DCT, specify 0.5)
 * @param asset_symbol the symbol or id of the asset to send
 * @param memo a memo to attach to the transaction.  The memo will be encrypted in the
 *             transaction and readable for the receiver. There is no length limit
 *             other than the limit imposed by maximum transaction size.
 * @param expiration expiration time
 * @ingroup WalletAPI_Proposals
 */
void propose_transfer(const string& proposer,
                      const string& from,
                      const string& to,
                      const string& amount,
                      const string& asset_symbol,
                      const string& memo,
                      time_point_sec expiration);

/**
 * @brief Creates a transaction to propose a parameter change.
 * Multiple parameters can be specified if an atomic change is
 * desired.
 * @param proposing_account the account paying the fee to propose the transaction
 * @param expiration_time timestamp specifying when the proposal will either take effect or expire
 * @param changed_values the values to change; all other chain parameters are filled in with default values
 * @param broadcast \c true if you wish to broadcast the transaction
 * @return the signed version of the transaction
 * @ingroup WalletAPI_Proposals
 */
signed_transaction propose_parameter_change(const string& proposing_account,
                                            fc::time_point_sec expiration_time,
                                            const variant_object& changed_values,
                                            bool broadcast = false);

/**
 * @brief Propose a fee change.
 * @param proposing_account the account paying the fee to propose the transaction
 * @param expiration_time timestamp specifying when the proposal will either take effect or expire
 * @param changed_values map of operation type to new fee.  Operations may be specified by name or ID
 *    The "scale" key changes the scale.  All other operations will maintain current values
 * @param broadcast \c true if you wish to broadcast the transaction
 * @return the signed version of the transaction
 * @ingroup WalletAPI_Proposals
 */
signed_transaction propose_fee_change(const string& proposing_account,
                                      fc::time_point_sec expiration_time,
                                      const variant_object& changed_values,
                                      bool broadcast = false);

/**
 * @brief Approve or disapprove a proposal.
 * @param fee_paying_account the account paying the fee for the operation
 * @param proposal_id the proposal to modify
 * @param delta members contain approvals to create or remove.  In JSON you can leave empty members undefined
 * @param broadcast \c true if you wish to broadcast the transaction
 * @return the signed version of the transaction
 * @ingroup WalletAPI_Proposals
 */
signed_transaction approve_proposal(const string& fee_paying_account,
                                    const string& proposal_id,
                                    const approval_delta& delta,
                                    bool broadcast /* = false */);

#endif //DECENT_WALLET_PROPOSALS_H
