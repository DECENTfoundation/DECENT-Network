#ifndef DECENT_WALLET_WALLET_FILE_H
#define DECENT_WALLET_WALLET_FILE_H

/**
 * @brief Copy wallet file to a new file
 * @param destination_filename
 * @return true if the wallet is copied, false otherwise
 * @ingroup WalletCLI
 */
bool copy_wallet_file( string destination_filename );  //obsolete

/**
 * @brief Lists all accounts controlled by this wallet.
 * This returns a list of the full account objects for all accounts whose private keys
 * we possess.
 * @returns a list of account objects
 * @ingroup WalletCLI
 */
vector<account_object>            list_my_accounts();

/**
 * @brief Returns the current wallet filename.
 *
 * This is the filename that will be used when automatically saving the wallet.
 *
 * @see set_wallet_filename()
 * @return the wallet filename
 * @ingroup WalletCLI
 */
string                            get_wallet_filename() const;

/**
 * @brief Get the WIF private key corresponding to a public key.  The
 * private key must already be in the wallet.
 * @param pubkey Public key
 * @return WIF private key corresponding to a public key
 * @ingroup WalletCLI
 */
string                            get_private_key( public_key_type pubkey )const;

/**
 * @brief Checks whether the wallet has just been created and has not yet had a password set.
 *
 * Calling \c set_password will transition the wallet to the locked state.
 * @return true if the wallet is new
 * @ingroup Wallet Management
 * @ingroup WalletCLI
 */
bool    is_new()const;

/**
 * @brief Checks whether the wallet is locked (is unable to use its private keys).
 *
 * This state can be changed by calling \c lock() or \c unlock().
 * @return true if the wallet is locked
 * @ingroup Wallet Management
 * @ingroup WalletCLI
 */
bool    is_locked()const;

/**
 * @brief Locks the wallet immediately.
 * @ingroup Wallet Management
 * @ingroup WalletCLI
 */
void    lock();

/**
 * @brief Unlocks the wallet.
 *
 * The wallet remain unlocked until the \c lock is called
 * or the program exits.
 * @param password the password previously set with \c set_password()
 * @ingroup Wallet Management
 * @ingroup WalletCLI
 */
void    unlock(string password);

/**
 * @brief Sets a new password on the wallet.
 *
 * The wallet must be either 'new' or 'unlocked' to
 * execute this command.
 * @param password
 * @ingroup Wallet Management
 * @ingroup WalletCLI
 */
void    set_password(string password);

/**
 * @brief Loads a specified Graphene wallet.
 *
 * The current wallet is closed before the new wallet is loaded.
 *
 * @warning This does not change the filename that will be used for future
 * wallet writes, so this may cause you to overwrite your original
 * wallet unless you also call \c set_wallet_filename()
 *
 * @param wallet_filename the filename of the wallet JSON file to load.
 *                        If \c wallet_filename is empty, it reloads the
 *                        existing wallet file
 * @returns true if the specified wallet is loaded
 * @ingroup WalletCLI
 */
bool    load_wallet_file(string wallet_filename = "");

/**
 * @brief Saves the current wallet to the given filename.
 *
 * @warning This does not change the wallet filename that will be used for future
 * writes, so think of this function as 'Save a Copy As...' instead of
 * 'Save As...'.  Use \c set_wallet_filename() to make the filename
 * persist.
 * @param wallet_filename the filename of the new wallet JSON file to create
 *                        or overwrite.  If \c wallet_filename is empty,
 *                        save to the current filename.
 * @ingroup WalletCLI
 */
void    save_wallet_file(string wallet_filename = "");

/**
 * @brief Sets the wallet filename used for future writes.
 *
 * This does not trigger a save, it only changes the default filename
 * that will be used the next time a save is triggered.
 *
 * @param wallet_filename the new filename to use for future saves
 * @ingroup WalletCLI
 */
void    set_wallet_filename(string wallet_filename);



/**
 * @brief Imports the private key for an existing account.
 *
 * The private key must match either an owner key or an active key for the
 * named account.
 *
 * @see dump_private_keys()
 *
 * @param account_name_or_id the account owning the key
 * @param wif_key the private key in WIF format
 * @returns true if the key was imported
 * @ingroup WalletCLI
 */
bool import_key(string account_name_or_id, string wif_key);

/**
 * @brief Imports accounts from the other wallet file
 * @param filename The filename of the wallet JSON file
 * @param password User's password to the wallet
 * @return mapped account names to boolean values indicating whether the account was successfully imported
 * @ingroup WalletCLI
 */
map<string, bool> import_accounts( string filename, string password );   //obsolete

/**
 * @brief Imports account keys from particular account from another wallet file to desired account located in wallet file currently used
 * @param filename The filename of the wallet JSON file
 * @param password User's password to the wallet
 * @param src_account_name Name of the source account
 * @param dest_account_name Name of the destination account
 * @return true if the keys were imported
 * @ingroup WalletCLI
 */
bool import_account_keys( string filename, string password, string src_account_name, string dest_account_name );   //obsolete

/**
 * @brief Dumps all private keys owned by the wallet.
 *
 * The keys are printed in WIF format.  You can import these keys into another wallet
 * using \c import_key()
 * @returns a map containing the private keys, indexed by their public key
 * @ingroup WalletCLI
 */
map<public_key_type, string> dump_private_keys();


#endif //DECENT_WALLET_FILE_H
