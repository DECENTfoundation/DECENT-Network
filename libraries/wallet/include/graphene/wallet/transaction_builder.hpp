#ifndef DECENT_WALLET_TRANSACTION_BUILDER_H
#define DECENT_WALLET_TRANSACTION_BUILDER_H

/**
 * @ingroup Transaction Builder API
 * @ingroup WalletCLI
 */
transaction_handle_type begin_builder_transaction();

/**
 *
 * @param transaction_handle
 * @param op
 * @ingroup Transaction Builder API
 * @ingroup WalletCLI
 */
void add_operation_to_builder_transaction(transaction_handle_type transaction_handle, const operation& op);

/**
 *
 * @param handle
 * @param operation_index
 * @param new_op
 * @ingroup Transaction Builder API
 * @ingroup WalletCLI
 */
void replace_operation_in_builder_transaction(transaction_handle_type handle,
                                              unsigned operation_index,
                                              const operation& new_op);
/**
 *
 * @param handle
 * @param fee_asset
 * @return
 * @ingroup Transaction Builder API
 * @ingroup WalletCLI
 */
asset set_fees_on_builder_transaction(transaction_handle_type handle, string fee_asset = GRAPHENE_SYMBOL);

/**
 * @param handle
 * @ingroup Transaction Builder API
 * @ingroup WalletCLI
 */
transaction preview_builder_transaction(transaction_handle_type handle);

/**
 *
 * @param transaction_handle
 * @param broadcast true to broadcast the transaction on the network
 * @return
 * @ingroup Transaction Builder API
 * @ingroup WalletCLI
 */
signed_transaction sign_builder_transaction(transaction_handle_type transaction_handle, bool broadcast = true);

/**
 *
 * @param handle
 * @param expiration
 * @param review_period_seconds
 * @param broadcast true to broadcast the transaction on the network
 * @return
 * @ingroup Transaction Builder API
 * @ingroup WalletCLI
 */
signed_transaction propose_builder_transaction(
   transaction_handle_type handle,
   time_point_sec expiration = time_point::now() + fc::minutes(1),
   uint32_t review_period_seconds = 0,
   bool broadcast = true
);

/**
 *
 * @param handle
 * @param account_name_or_id
 * @param expiration
 * @param review_period_seconds
 * @param broadcast true to broadcast the transaction on the network
 * @return
 * @ingroup WalletCLI
 */
signed_transaction propose_builder_transaction2(
   transaction_handle_type handle,
   string account_name_or_id,
   time_point_sec expiration = time_point::now() + fc::minutes(1),
   uint32_t review_period_seconds = 0,
   bool broadcast = true
);

/**
 *
 * @param handle
 * @ingroup Transaction Builder API
 * @ingroup WalletCLI
 */
void remove_builder_transaction(transaction_handle_type handle);

/**
 * @brief Converts a signed_transaction in JSON form to its binary representation.
 *
 * TODO: I don't see a broadcast_transaction() function, do we need one?
 *
 * @param tx the transaction to serialize
 * @returns the binary form of the transaction.  It will not be hex encoded,
 *          this returns a raw string that may have null characters embedded
 *          in it
 * @ingroup WalletCLI
 */
string serialize_transaction(signed_transaction tx) const;

/**
 * @brief Signs a transaction.
 *
 * Given a fully-formed transaction that is only lacking signatures, this signs
 * the transaction with the necessary keys and optionally broadcasts the transaction
 * @param tx the unsigned transaction
 * @param broadcast true if you wish to broadcast the transaction
 * @return the signed version of the transaction
 * @ingroup WalletCLI
 */
signed_transaction sign_transaction(signed_transaction tx, bool broadcast = false);

/**
 * @brief Returns an uninitialized object representing a given blockchain operation.
 *
 * This returns a default-initialized object of the given type; it can be used
 * during early development of the wallet when we don't yet have custom commands for
 * creating all of the operations the blockchain supports.
 *
 * Any operation the blockchain supports can be created using the transaction builder's
 * \c add_operation_to_builder_transaction() , but to do that from the CLI you need to
 * know what the JSON form of the operation looks like.  This will give you a template
 * you can fill in.  It's better than nothing.
 *
 * @param operation_type the type of operation to return, must be one of the
 *                       operations defined in `graphene/chain/operations.hpp`
 *                       (e.g., "global_parameters_update_operation")
 * @return a default-constructed operation of the given type
 * @ingroup WalletCLI
 */
operation get_prototype_operation(string operation_type);


#endif //DECENT_TRANSACTION_BUILDER_H
