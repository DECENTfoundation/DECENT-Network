#ifndef DECENT_WALLET_PROPOSALS_H
#define DECENT_WALLET_PROPOSALS_H

/**
 * Get list of proposed transactions
 */
vector<proposal_object> get_proposed_transactions( string account_or_id )const;

/**
 * @brief Encapsulates begin_builder_transaction(), add_operation_to_builder_transaction(),
 * propose_builder_transaction2(), set_fees_on_builder_transaction() functions for transfer operation
 *
 * @param proposer Proposer
 * @param from the name or id of the account sending the funds
 * @param to the name or id of the account receiving the funds
 * @param amount the amount to send (in nominal units -- to send half of a BTS, specify 0.5)
 * @param asset_symbol the symbol or id of the asset to send
 * @param memo a memo to attach to the transaction.  The memo will be encrypted in the
 *             transaction and readable for the receiver.  There is no length limit
 *             other than the limit imposed by maximum transaction size, but transaction
 *             increase with transaction size
 * @param expiration expiration time
 * @ingroup WalletCLI
 */
 void propose_transfer(string proposer,
                       string from,
                       string to,
                       string amount,
                       string asset_symbol,
                       string memo,
                       time_point_sec expiration
 );


 /**
  * @brief Creates a transaction to propose a parameter change.
  *
  * Multiple parameters can be specified if an atomic change is
  * desired.
  *
  * @param proposing_account The account paying the fee to propose the tx
  * @param expiration_time Timestamp specifying when the proposal will either take effect or expire.
  * @param changed_values The values to change; all other chain parameters are filled in with default values
  * @param broadcast true if you wish to broadcast the transaction
  * @return the signed version of the transaction
  * @ingroup WalletCLI
  */
 signed_transaction propose_parameter_change(
    const string& proposing_account,
    fc::time_point_sec expiration_time,
    const variant_object& changed_values,
    bool broadcast = false);

 /**
  * @brief Propose a fee change.
  *
  * @param proposing_account The account paying the fee to propose the tx
  * @param expiration_time Timestamp specifying when the proposal will either take effect or expire.
  * @param changed_values Map of operation type to new fee.  Operations may be specified by name or ID.
  *    The "scale" key changes the scale.  All other operations will maintain current values.
  * @param broadcast true if you wish to broadcast the transaction
  * @return the signed version of the transaction
  * @ingroup WalletCLI
  */
 signed_transaction propose_fee_change(
    const string& proposing_account,
    fc::time_point_sec expiration_time,
    const variant_object& changed_values,
    bool broadcast = false);

 /**
  *
  * @brief Approve or disapprove a proposal.
  *
  * @param fee_paying_account The account paying the fee for the op.
  * @param proposal_id The proposal to modify.
  * @param delta Members contain approvals to create or remove.  In JSON you can leave empty members undefined.
  * @param broadcast true if you wish to broadcast the transaction
  * @return the signed version of the transaction
  * @ingroup WalletCLI
  */
 signed_transaction approve_proposal(
    const string& fee_paying_account,
    const string& proposal_id,
    const approval_delta& delta,
    bool broadcast /* = false */);


#endif //DECENT_WALLET_PROPOSALS_H
