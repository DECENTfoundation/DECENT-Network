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

#ifndef DECENT_WALLET_ACCOUNT_H
#define DECENT_WALLET_ACCOUNT_H

/**
 * @brief Returns the number of accounts registered on the blockchain.
 * @return the number of registered accounts
 * @ingroup WalletAPI_Account
 */
uint64_t                          get_account_count() const;

/**
 * @brief Lists all accounts registered in the blockchain.
 * This returns a list of all account names and their account ids, sorted by account name.
 * Use the \c lowerbound and \c limit parameters to page through the list.  To retrieve all accounts,
 * start by setting \c lowerbound to the empty string \c "", and then each iteration, pass
 * the last account name returned as the \c lowerbound for the next \c list_accounts() call.
 * @param lowerbound the name of the first account to return.  If the named account does not exist,
 *                   the list will start at the account that comes after \c lowerbound
 * @param limit the maximum number of accounts to return (max: 1000)
 * @return a list of accounts mapping account names to account ids
 * @ingroup WalletAPI_Account
 */
map<string,account_id_type>       list_accounts(const string& lowerbound, uint32_t limit);

/**
 * @brief Get registered accounts that match search term
 * @param term will try to partially match account name or id
 * @param limit maximum number of results to return ( must not exceed 1000 )
 * @param order sort data by field
 * @param id object_id to start searching from
 * @return map of account names to corresponding IDs
 * @ingroup WalletAPI_Account
 */
vector<account_object>       search_accounts(const string& term, const string& order, const string& id, uint32_t limit);

/**
 * @brief List the balances of an account.
 * Each account can have multiple balances, one for each type of asset owned by that
 * account.
 * @param id the name or id of the account whose balances you want
 * @return a list of the given account's balances
 * @ingroup WalletAPI_Account
 */
vector<asset>                     list_account_balances(const string& id);

/**
 * @brief Returns the operations on the named account.
 * This returns a list of transaction detail objects, which describe past the past activity on the account.
 * @param account_name the name or id of the account
 * @param order sort data by field
 * @param id object_id to start searching from
 * @param limit the number of entries to return (starting from the most recent) (max 100)
 * @return a list of transaction detail objects
 * @ingroup WalletAPI_Account
 */
vector<class transaction_detail_object> search_account_history(const string& account_name,
                                                               const string& order,
                                                               const string& id,
                                                               int limit) const;


/**
 * @brief Returns the most recent operations on the named account.
 * This returns a list of operation history objects, which describe activity on the account.
 * @note this API doesn't give a way to retrieve more than the most recent 100 transactions
 * @param name the name or id of the account
 * @param limit the number of entries to return (starting from the most recent)
 * @return a list of operation history objects
 * @ingroup WalletAPI_Account
 */
vector<operation_detail>  get_account_history(const string& name, int limit) const;

/**
 * @brief Get operations relevant to the specified account referenced
 * by an event numbering specific to the account. The current number of operations
 * for the account can be found in the account statistics (or use 0 for start).
 * @param name The account whose history should be queried
 * @param stop Sequence number of earliest operation. 0 is default and will
 * query 'limit' number of operations.
 * @param limit Maximum number of operations to retrieve (must not exceed 100)
 * @param start Sequence number of the most recent operation to retrieve.
 * 0 is default, which will start querying from the most recent operation.
 * @return A list of operations performed by account, ordered from most recent to oldest.
 * @ingroup WalletAPI_Account
 */
vector<operation_detail>  get_relative_account_history(const string& name,
                                                       uint32_t stop,
                                                       int limit,
                                                       uint32_t start) const;

/**
 * @brief Returns the most recent balance operations on the named account.
 * This returns a list of operation history objects, which describe activity on the account.
 * @param account_name the name or id of the account
 * @param assets_list list of asset names to filter or empty for all assets
 * @param partner_account partner account_id to filter transfers to speccific account or empty
 * @param from_block filtering parameter, starting block number (can be determined by from time) or zero when not used
 * @param to_block filtering parameter, ending block number or zero when not used
 * @param start_offset starting offset from zero
 * @param limit the number of entries to return (starting from the most recent)
 * @return a list of balance operation history objects
 * @ingroup WalletAPI_Account
 */
vector<balance_operation_detail>  search_account_balance_history(const string& account_name,
                                                                 const flat_set<string>& assets_list,
                                                                 const string& partner_account,
                                                                 uint32_t from_block, uint32_t to_block,
                                                                 uint32_t start_offset,
                                                                 int limit) const;

/**
 * @brief Returns the most recent balance operations on the named account.
 * @param account_name the name or id of the account
 * @param operation_history_id the operation_history_id to search for
 * @return returns balance_operation_detail or empty when not found
 * @ingroup WalletAPI_Account
 */
fc::optional<balance_operation_detail> get_account_balance_for_transaction(const string& account_name,
                                                                           operation_history_id_type operation_history_id);

/**
 * @brief Returns information about the given account.
 *
 * @param account_name_or_id the name or id of the account to provide information about
 * @return the public account data stored in the blockchain
 * @ingroup WalletAPI_Account
 */
account_object                    get_account(const string& account_name_or_id) const;

/**
 * @brief Derive private key from given prefix and sequence.
 * @param prefix_string
 * @param sequence_number
 * @return derived private key
 * @ingroup WalletAPI_Account
 */
std::string derive_private_key(const std::string& prefix_string, int sequence_number) const;

/**
 * @brief Get public key from given private key.
 * @param wif_private_key the private key in wallet import format
 * @return corresponding public key
 * @ingroup WalletAPI_Account
 */
graphene::chain::public_key_type get_public_key( const std::string& wif_private_key ) const;

/**
 * @brief Suggests a safe brain key to use for creating your account.
 * \c create_account_with_brain_key() requires you to specify a brain key,
 * a long passphrase that provides enough entropy to generate cyrptographic
 * keys.  This function will suggest a suitably random string that should
 * be easy to write down (and, with effort, memorize).
 * @return a suggested brain key
 * @ingroup WalletAPI_Account
 */
brain_key_info suggest_brain_key() const;

/**
 * @brief Calculates the private key and public key corresponding to any brain key
 * @param brain_key the brain key to be used for calculation
 * @return the corresponding \c brain_key_info
 * @ingroup WalletAPI_Account
 */
brain_key_info get_brain_key_info(const string& brain_key) const;

/**
 * @brief Suggests a safe brain key to use for creating your account. This funcion also
 * generates \c el_gamal_key_pair corresponding to the brain key.
 * @note \c create_account_with_brain_key() requires you to specify a brain key,
 * a long passphrase that provides enough entropy to generate cyrptographic
 * keys.  This function will suggest a suitably random string that should
 * be easy to write down (and, with effort, memorize).
 * @return a suggested brain key and corresponding El Gamal key pair
 * @ingroup WalletAPI_Account
 */
pair<brain_key_info, el_gamal_key_pair> generate_brain_key_el_gamal_key() const;

/**
 * @brief Registers a third party's account on the blockckain.
 * This function is used to register an account for which you do not own the private keys.
 * When acting as a registrar, an end user will generate their own private keys and send
 * you the public keys.  The registrar will use this function to register the account
 * on behalf of the end user.
 * @note The owner key represents absolute control over the account. Generally, the only time the owner key is required
 * is to update the active key.
 * @note The active key represents the hot key of the account. This key has control over nearly all
 * operations the account may perform.
 * @note The memo key is the key this account will typically use to encrypt/sign transaction memos and other non-
   validated account activities.
 * @see suggest_brain_key()
 * @param name the name of the account, must be unique on the blockchain and contains at least 5 characters
 * @param owner the owner key for the new account
 * @param active the active key for the new account
 * @param memo the memo key for the new account
 * @param registrar_account the account which will pay the fee to register the user
 * @param broadcast \c true to broadcast the transaction on the network
 * @return the signed transaction registering the account
 * @ingroup WalletAPI_Account
 */
signed_transaction register_account_with_keys(const string& name,
                                              public_key_type owner,
                                              public_key_type active,
                                              public_key_type memo,
                                              const string& registrar_account,
                                              bool broadcast = false);

/**
 * @brief Registers a third party's account on the blockckain.
 * This function is used to register an account for which you do not own the private keys.
 * When acting as a registrar, an end user will generate their own private keys and send
 * you the public keys.  The registrar will use this function to register the account
 * on behalf of the end user.
 * @note The owner key represents absolute control over the account. Generally, the only time the owner key is required
 * is to update the active key.
 * @note The active key represents the hot key of the account. This key has control over nearly all
 * operations the account may perform.
 * @see suggest_brain_key()
 * @param name the name of the account, must be unique on the blockchain and contains at least 5 characters
 * @param owner the owner key for the new account
 * @param active the active key for the new account
 * @param registrar_account the account which will pay the fee to register the user
 * @param broadcast \c true to broadcast the transaction on the network
 * @return the signed transaction registering the account
 * @ingroup WalletAPI_Account
 */
signed_transaction register_account(const string& name,
                                    public_key_type owner,
                                    public_key_type active,
                                    const string& registrar_account,
                                    bool broadcast = false);

/**
 * @brief Registers a third party's multisignature account on the blockckain.
 * This function is used to register an account for which you do not own the private keys.
 * When acting as a registrar, an end user will generate their own private keys and send
 * you the public keys or account ID/IDs.  The registrar will use this function to register the account
 * on behalf of the end user.
 * @note The owner authority represents absolute control over the account. Generally, the only time the owner authority
 * is required is to update the active authority.
 * @note The active authority represents the hot key/keys of the account. This authority has control over nearly all
 * operations the account may perform.
 * @note The memo key is the key this account will typically use to encrypt/sign transaction memos and other non-
   validated account activities.
 * @see suggest_brain_key()
 * @param name the name of the account, must be unique on the blockchain and contains at least 5 characters
 * @param owner the owner authority for the new account
 * @param active the active authority for the new account
 * @param memo the memo public_key_type for the new account
 * @param registrar_account the account which will pay the fee to register the user
 * @param broadcast \c true to broadcast the transaction on the network
 * @return the signed transaction registering the account
 * @ingroup WalletAPI_Account
 */
signed_transaction register_multisig_account(const string& name,
                                             authority owner,
                                             authority active,
                                             public_key_type memo,
                                             const string& registrar_account,
                                             bool broadcast = false);

/**
 * @brief Creates a new account and registers it on the blockchain.
 * @see suggest_brain_key()
 * @see register_account()
 * @param brain_key the brain key used for generating the account's private keys
 * @param account_name the name of the account, must be unique on the blockchain and contains at least 5 characters
 * @param registrar_account the account which will pay the fee to register the user
 * @param broadcast \c true to broadcast the transaction on the network
 * @return the signed transaction registering the account
 * @ingroup WalletAPI_Account
 */
signed_transaction create_account_with_brain_key(const string& brain_key,
                                                 const string& account_name,
                                                 const string& registrar_account,
                                                 bool broadcast = false);

/**
 * @brief Updates an account keys.
 * This function is used to update an account keys.
 * At least one account key needs to be specified. Use empty string to specify keys you do not want to update.
 * @note The owner key represents absolute control over the account. Generally, the only time the owner key is required
 * is to update the active key.
 * @note The active key represents the hot key of the account. This key has control over nearly all
 * operations the account may perform.
 * @note The memo key is the key this account will typically use to encrypt/sign transaction memos and other non-
   validated account activities.
 * @see suggest_brain_key()
 * @param name the name of the account to update
 * @param owner the new owner key for the account
 * @param active the new active key for the account
 * @param memo the new memo key for the account
 * @param broadcast \c true to broadcast the transaction on the network
 * @return the signed transaction registering the account
 * @ingroup WalletAPI_Account
 */
signed_transaction update_account_keys(const string& name,
                                       const string& owner,
                                       const string& active,
                                       const string& memo,
                                       bool broadcast = false);

/**
 * @brief Updates an account keys.
 * This function is used to update an account authorities.
 * At least one account key needs to be specified. Use empty string to specify keys you do not want to update.
 * @note The owner authority represents absolute control over the account. Generally, the only time the owner authority
 * is required is to update the active authority.
 * @note The active authority represents the hot key/keys of the account. This authority has control over nearly all
 * operations the account may perform.
 * @note The memo key is the key this account will typically use to encrypt/sign transaction memos and other non-
   validated account activities.
 * @see suggest_brain_key()
 * @param name the name of the account to update
 * @param owner the new owner authority for the account
 * @param active the new active authority for the account
 * @param memo the new memo key for the account
 * @param broadcast \c true to broadcast the transaction on the network
 * @return the signed transaction registering the account
 * @ingroup WalletAPI_Account
 */
signed_transaction update_account_keys_to_multisig(const string& name,
                                                   authority owner,
                                                   authority active,
                                                   public_key_type memo,
                                                   bool broadcast = false);

/**
 * @brief Transfer an amount from one account to another account or to content.
 * In the case of transferring to a content, amount is transferred to author and co-authors of the content,
 * if they are specified.
 * @param from the name or id of the account sending the funds
 * @param to the name or id of the account or id of the content receiving the funds
 * @param amount the amount to send (in nominal units -- to send half of a DCT, specify 0.5)
 * @param asset_symbol the symbol or id of the asset to send
 * @param memo a memo to attach to the transaction.  The memo will be encrypted in the
 *             transaction and readable for the receiver. There is no length limit
 *             other than the limit imposed by maximum transaction size.
 * @note transaction fee is fixed and does not depend on the length of the memo
 * @param broadcast \c true to broadcast the transaction on the network
 * @return the signed transaction transferring funds
 * @ingroup WalletAPI_Account
 */
signed_transaction transfer(const string& from,
                            const string& to,
                            const string& amount,
                            const string& asset_symbol,
                            const string& memo,
                            bool broadcast = false);

/**
 *  @brief This method works just like transfer, except it always broadcasts and
 *  returns the transaction ID along with the signed transaction.
 *  @param from the name or id of the account sending the funds
 *  @param to the name or id of the account or id of the content receiving the funds
 *  @param amount the amount to send (in nominal units -- to send half of a DCT, specify 0.5)
 *  @param asset_symbol the symbol or id of the asset to send
 *  @param memo a memo to attach to the transaction.  The memo will be encrypted in the
 *             transaction and readable for the receiver.  There is no length limit
 *             other than the limit imposed by maximum transaction size, but transaction
 *             increase with transaction size
 * @note transaction fee is fixed and does not depend on the length of the memo
 * @return the transaction ID along with the signed transaction
 * @ingroup WalletAPI_Account
 */
pair<transaction_id_type,signed_transaction> transfer2(const string& from,
                                                       const string& to,
                                                       const string& amount,
                                                       const string& asset_symbol,
                                                       const string& memo );

/**
 * @brief Generates private El Gamal key and corresponding public key.
 * @return pair of El Gamal keys
 * @ingroup WalletAPI_Account
 */
el_gamal_key_pair generate_el_gamal_keys() const;

/**
 * @brief Gets unique El Gamal key pair for consumer.
 * @return pair of El Gamal keys
 * @ingroup WalletAPI_Account
 */
el_gamal_key_pair_str get_el_gammal_key(const string& consumer) const;


#endif //DECENT_WALLET_ACCOUNT_H
