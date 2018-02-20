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

#ifndef DECENT_WALLET_TRANSACTION_BUILDER_H
#define DECENT_WALLET_TRANSACTION_BUILDER_H

/**
 * @brief Allows creation of customized transactions and fill them with operation/s.
 * @return identifier allowing to construct several transactions in parallel and identify them
 * @ingroup WalletAPI_TransactionBuilder
 */
transaction_handle_type begin_builder_transaction();

/**
 * @brief Adds an operation to a transaction in transaction builder.
 * @see \c begin_builder_transaction()
 * @param transaction_handle the number indetifying transaction under construction process
 * @param op the operation
 * @ingroup WalletAPI_TransactionBuilder
 */
void add_operation_to_builder_transaction(transaction_handle_type transaction_handle, const operation& op);

/**
 * @brief Replace existing operation in specified transaction in transaction builder.
 * @see \c add_operation_to_builder_transaction()
 * @param handle the number identifying transaction under contruction process
 * @param operation_index index of the operation to replace
 * @param new_op the new operation replacing the existing one
 * @ingroup WalletAPI_TransactionBuilder
 */
void replace_operation_in_builder_transaction(transaction_handle_type handle,
                                              unsigned operation_index,
                                              const operation& new_op);

/**
 * @brief Set fees on all operations in a transaction
 * @see \c begin_builder_transaction()
 * @param handle the number identifying transaction under contruction process
 * @param fee_asset the asset in which fees are calculated
 * @return total fee in specified asset
 * @ingroup WalletAPI_TransactionBuilder
 */
asset set_fees_on_builder_transaction(transaction_handle_type handle, string fee_asset = GRAPHENE_SYMBOL);

/**
 * @brief Previews a transaction from transaction builder.
 * @see \c begin_builder_transaction()
 * @param handle the number identifying transaction under contruction process
 * @return the transaction to preview
 * @ingroup WalletAPI_TransactionBuilder
 */
transaction preview_builder_transaction(transaction_handle_type handle);

/**
 * @brief Signs a transaction from transaction builder
 * @see \c prewiev_builder_transaction()
 * @param transaction_handle the number identifying transaction under contruction process
 * @param broadcast \c true to broadcast the transaction on the network
 * @return the signed transaction
 * @ingroup WalletAPI_TransactionBuilder
 */
signed_transaction sign_builder_transaction(transaction_handle_type transaction_handle, bool broadcast = true);

/**
 * @brief Allows creation of a proposed transaction suitable for miner-account. Proposed transaction requires approval of multiple accounts in order to execute.
 * @param handle the number identifying transaction under contruction process
 * @param expiration the expiration time of the transaction
 * @param review_period_seconds the time reserved for reviewing the proposal transaction. It's not allowed to vote for the proposal when the transaction is under review
 * @param broadcast \c true to broadcast the transaction on the network
 * @return the signed transaction
 * @ingroup WalletAPI_TransactionBuilder
 */
signed_transaction propose_builder_transaction(transaction_handle_type handle,
                                               time_point_sec expiration = time_point::now() + fc::minutes(1),
                                               uint32_t review_period_seconds = 0,
                                               bool broadcast = true);

/**
 * @brief Allows creation of a proposed transaction. Proposed transaction requires approval of multiple accounts in order to execute.
 * @see \c propose_builder_transaction()
 * @param handle the number identifying transaction under contruction process
 * @param account_name_or_id the account which will pay the fee to propose the transaction
 * @param expiration the expiration time of the transaction
 * @param review_period_seconds the time reserved for reviewing the proposal transaction. It's not allowed to vote for the proposal when the transaction is under review
 * @param broadcast \c true to broadcast the transaction on the network
 * @return the signed transaction
 * @ingroup WalletAPI_TransactionBuilder
 */
signed_transaction propose_builder_transaction2(transaction_handle_type handle,
                                                string account_name_or_id,
                                                time_point_sec expiration = time_point::now() + fc::minutes(1),
                                                uint32_t review_period_seconds = 0,
                                                bool broadcast = true);

/**
 * @brief Removes a transaction from transaction builder
 * @param handle the number identifying transaction under contruction process
 * @ingroup WalletAPI_TransactionBuilder
 */
void remove_builder_transaction(transaction_handle_type handle);

// TODO: I don't see a broadcast_transaction() function, do we need one?
/**
 * @brief Converts a signed_transaction in JSON form to its binary representation.
 * @param tx the transaction to serialize
 * @return the binary form of the transaction.  It will not be hex encoded,
 *         this returns a raw string that may have null characters embedded in it
 * @ingroup WalletAPI_TransactionBuilder
 */
string serialize_transaction(signed_transaction tx) const;

/**
 * @brief Signs a transaction.
 * Given a fully-formed transaction that is only lacking signatures, this signs
 * the transaction with the necessary keys and optionally broadcasts the transaction
 * @param tx the unsigned transaction
 * @param broadcast \c true if you wish to broadcast the transaction
 * @return the signed version of the transaction
 * @ingroup WalletAPI_TransactionBuilder
 */
signed_transaction sign_transaction(signed_transaction tx, bool broadcast = false);

/**
 * @brief Returns an uninitialized object representing a given blockchain operation.
 * This returns a default-initialized object of the given type.
 * Any operation the blockchain supports can be created using the transaction builder's
 * \c add_operation_to_builder_transaction() , but to do that from the CLI you need to
 * know what the JSON form of the operation looks like. This will give you a template
 * you can fill in.
 * @param operation_type the type of operation to return, must be one of the
 *                       operations defined in `graphene/chain/operations.hpp`
 *                       (e.g., "global_parameters_update_operation")
 * @return a default-constructed operation of the given type
 * @ingroup WalletAPI_TransactionBuilder
 */
operation get_prototype_operation(string operation_type);


#endif //DECENT_TRANSACTION_BUILDER_H
