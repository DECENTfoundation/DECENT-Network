#ifndef DECENT_WALLET_ACCOUNT_H
#define DECENT_WALLET_ACCOUNT_H

/**
 * @brief Returns the number of accounts registered on the blockchain
 * @returns the number of registered accounts
 * @ingroup WalletCLI
 */
uint64_t                          get_account_count()const;

/**
 * @brief Lists all accounts registered in the blockchain.
 * This returns a list of all account names and their account ids, sorted by account name.
 *
 * Use the \c lowerbound and limit parameters to page through the list.  To retrieve all accounts,
 * start by setting \c lowerbound to the empty string \c "", and then each iteration, pass
 * the last account name returned as the \c lowerbound for the next \c list_accounts() call.
 *
 * @param lowerbound the name of the first account to return.  If the named account does not exist,
 *                   the list will start at the account that comes after \c lowerbound
 * @param limit the maximum number of accounts to return (max: 1000)
 * @returns a list of accounts mapping account names to account ids
 * @ingroup WalletCLI
 */
map<string,account_id_type>       list_accounts(const string& lowerbound, uint32_t limit);

/**
 * @brief Get names and IDs for registered accounts that match search term
 * @param term will try to partially match account name or id
 * @param limit Maximum number of results to return -- must not exceed 1000
 * @param order Sort data by field
 * @param id object_id to start searching from
 * @return Map of account names to corresponding IDs
 * @ingroup WalletCLI
 */
vector<account_object>       search_accounts(const string& term, const string& order, const string& id, uint32_t limit);

/**
 * @brief List the balances of an account.
 * Each account can have multiple balances, one for each type of asset owned by that
 * account.  The returned list will only contain assets for which the account has a
 * nonzero balance
 * @param id the name or id of the account whose balances you want
 * @returns a list of the given account's balances
 * @ingroup WalletCLI
 */
vector<asset>                     list_account_balances(const string& id);

/**
 * @brief Returns the operations on the named account.
 *
 * This returns a list of transaction detail object, which describe activity on the account.
 *
 * @param account_name the name or id of the account
 * @param order Sort data by field
 * @param id object_id to start searching from
 * @param limit the number of entries to return (starting from the most recent) (max 100)
 * @returns a list of \c transaction_detail_object
 * @ingroup WalletCLI
 */
vector<class transaction_detail_object> search_account_history(string const& account_name,
                                                               string const& order,
                                                               string const& id,
                                                               int limit) const;


/** Returns the most recent operations on the named account.
 *
 * This returns a list of operation history objects, which describe activity on the account.
 *
 * @note this API doesn't give a way to retrieve more than the most recent 100 transactions,
 *       you can interface directly with the blockchain to get more history
 * @param name the name or id of the account
 * @param limit the number of entries to return (starting from the most recent) (max 100)
 * @returns a list of \c operation_history_objects
 */
vector<operation_detail>  get_account_history(string name, int limit)const;

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
 * @ingroup WalletCLI
 */
vector<operation_detail>  get_relative_account_history(string name,
                                                       uint32_t stop,
                                                       int limit,
                                                       uint32_t start)const;


/**
 * @brief Returns information about the given account.
 *
 * @param account_name_or_id the name or id of the account to provide information about
 * @returns the public account data stored in the blockchain
 * @ingroup WalletCLI
 */
account_object                    get_account(string account_name_or_id) const;

/**
 * @brief Derive private key from given prefix and sequence
 * @param prefix_string
 * @param sequence_number
 * @return private_key Derived private key
 * @ingroup WalletCLI
 */
fc::ecc::private_key derive_private_key(const std::string& prefix_string, int sequence_number) const;

/**
 * @brief Suggests a safe brain key to use for creating your account.
 * \c create_account_with_brain_key() requires you to specify a 'brain key',
 * a long passphrase that provides enough entropy to generate cyrptographic
 * keys.  This function will suggest a suitably random string that should
 * be easy to write down (and, with effort, memorize).
 * @returns a suggested brain_key
 * @ingroup WalletCLI
 */
brain_key_info suggest_brain_key()const;

/**
 * @brief Calculates the private key and public key corresponding to any brain key
 * @param brain_key the brain key to be used for calculation
 * @returns the corresponding brain_key_info
 * @ingroup WalletCLI
 */
brain_key_info get_brain_key_info(string const& brain_key) const;

/**
 * @brief Suggests a safe brain key to use for creating your account also
 * generates the el_gamal_key_pair corresponding to the brain key
 * \c create_account_with_brain_key() requires you to specify a 'brain key',
 * a long passphrase that provides enough entropy to generate cyrptographic
 * keys.  This function will suggest a suitably random string that should
 * be easy to write down (and, with effort, memorize).
 * @returns a suggested brain_key
 * @ingroup WalletCLI
 */
pair<brain_key_info, el_gamal_key_pair> generate_brain_key_el_gamal_key() const;

/**
 * @brief Registers a third party's account on the blockckain.
 *
 * This function is used to register an account for which you do not own the private keys.
 * When acting as a registrar, an end user will generate their own private keys and send
 * you the public keys.  The registrar will use this function to register the account
 * on behalf of the end user.
 *
 * @see create_account_with_brain_key()
 *
 * @param name the name of the account, must be unique on the blockchain.  Shorter names
 *             are more expensive to register; the rules are still in flux, but in general
 *             names of more than 8 characters with at least one digit will be cheap.
 * @param owner the owner key for the new account
 * @param active the active key for the new account
 * @param registrar_account the account which will pay the fee to register the user
 * @param broadcast true to broadcast the transaction on the network
 * @returns the signed transaction registering the account
 * @ingroup WalletCLI
 */
signed_transaction register_account(string name,
                                    public_key_type owner,
                                    public_key_type active,
                                    string  registrar_account,
                                    bool broadcast = false);

/**
 * @brief Creates a new account and registers it on the blockchain.
 *
 * @todo why no referrer_percent here?
 *
 * @see suggest_brain_key()
 * @see register_account()
 *
 * @param brain_key the brain key used for generating the account's private keys
 * @param account_name the name of the account, must be unique on the blockchain.  Shorter names
 *                     are more expensive to register; the rules are still in flux, but in general
 *                     names of more than 8 characters with at least one digit will be cheap.
 * @param registrar_account the account which will pay the fee to register the user
 * @param broadcast true to broadcast the transaction on the network
 * @returns the signed transaction registering the account
 * @ingroup WalletCLI
 */
signed_transaction create_account_with_brain_key(string brain_key,
                                                 string account_name,
                                                 string registrar_account,
                                                 bool broadcast = false);

/**
 * @brief Transfer an amount from one account to another.
 * @param from the name or id of the account sending the funds
 * @param to the name or id of the account receiving the funds
 * @param amount the amount to send (in nominal units -- to send half of a BTS, specify 0.5)
 * @param asset_symbol the symbol or id of the asset to send
 * @param memo a memo to attach to the transaction.  The memo will be encrypted in the
 *             transaction and readable for the receiver.  There is no length limit
 *             other than the limit imposed by maximum transaction size, but transaction
 *             increase with transaction size
 * @param broadcast true to broadcast the transaction on the network
 * @returns the signed transaction transferring funds
 * @ingroup WalletCLI
 */
signed_transaction transfer(string from,
                            string to,
                            string amount,
                            string asset_symbol,
                            string memo,
                            bool broadcast = false);

/**
 * @brief Generates private ElGamal key and corresponding public key.
 * @return Pair of ElGamal keys
 * @ingroup WalletCLI
 */
el_gamal_key_pair generate_el_gamal_keys() const;

/**
 * @brief Gets unique ElGamal key pair for consumer.
 * @return Pair of ElGamal keys
 * @ingroup WalletCLI
 */
el_gamal_key_pair_str get_el_gammal_key(string const& consumer) const;


#endif //DECENT_WALLET_ACCOUNT_H
