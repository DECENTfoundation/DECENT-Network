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

#ifndef DECENT_WALLET_WALLET_FILE_H
#define DECENT_WALLET_WALLET_FILE_H

/**
 * @brief Lists all accounts controlled by this wallet.
 * This returns a list of the full account objects for all accounts whose private keys
 * we possess.
 * @return a list of accounts imported in the wallet
 * @ingroup WalletAPI_Wallet
 */
vector<account_object> list_my_accounts();

/** @brief Returns the current wallet filename.
 * @note This is the filename that will be used when automatically saving the wallet.
 * @return the wallet filename
 * @ingroup WalletAPI_Wallet
 */
string get_wallet_filename() const;

/**
 * @brief Get the WIF private key corresponding to a public key.  The
 * private key must already be imported in the wallet.
 * @param pubkey public key
 * @return WIF private key corresponding to a public key
 * @ingroup WalletAPI_Wallet
 */
string get_private_key( public_key_type pubkey) const;

/**
 * @brief Checks whether the wallet has just been created and has not yet had a password set.
 * Calling \c set_password() will transition the wallet to the locked state.
 * @return \c true if the wallet is new
 * @ingroup WalletAPI_Wallet
 */
bool is_new() const;

/**
 * @brief Checks whether the wallet is locked (is unable to use its private keys).
 * This state can be changed by calling \c lock() or \c unlock().
 * @see \c unlock()
 * @return \c true if the wallet is locked
 * @ingroup WalletAPI_Wallet
 */
bool is_locked() const;

/**
 * @brief Locks the wallet immediately.
 * @see \c unlock()
 * @ingroup WalletAPI_Wallet
 */
void lock();

/**
 * @brief Unlocks the wallet.
 * The wallet remain unlocked until the \c lock() is called
 * or the program exits.
 * @param password the password previously set with \c set_password()
 * @ingroup WalletAPI_Wallet
 */
void unlock(const string& password);

/**
 * @brief Sets a new password on the wallet.
 * The wallet must be either \c new or \c unlocked to execute this command.
 * @param password
 * @ingroup WalletAPI_Wallet
 */
void set_password(const string& password);

/**
 * @brief Loads a specified wallet file.
 * The current wallet is closed before the new wallet is loaded.
 * @warning This changes the filename that will be used for future wallet writes.
 * @param wallet_filename the filename of the wallet JSON file to load.
 *                        If \c wallet_filename is empty, it reloads the
 *                        existing wallet file
 * @return \c true if the specified wallet is loaded
 * @ingroup WalletAPI_Wallet
 */
bool load_wallet_file(const string& wallet_filename = string());

/**
 * @brief Saves the current wallet to the given filename.
 * @warning This does not change the wallet filename that will be used for future
 * writes, so think of this function as 'Save a Copy As...' instead of 'Save As...'.
 * @param wallet_filename the filename of the new wallet JSON file to create
 *                        or overwrite.  If \c wallet_filename is empty,
 *                        save to the current filename.
 * @ingroup WalletAPI_Wallet
 */
void save_wallet_file(const string& wallet_filename = string());

/**
 * @brief Imports the private key for an existing account.
 * The private key should match either an owner key or an active key for the
 * named account.
 * @see dump_private_keys()
 * @see list_my_accounts()
 * @param account_name_or_id the account owning the key
 * @param wif_key the private key in WIF format
 * @return \c true if the key was imported
 * @ingroup WalletAPI_Wallet
 */
bool import_key(const string& account_name_or_id, const string& wif_key);

/**
 * @brief Dumps all private keys successfully imported in the wallet.
 * @note The keys are printed in WIF format.  You can import these keys into another wallet
 * using \c import_key()
 * @return a map containing the private keys and corresponding public keys
 * @ingroup WalletAPI_Wallet
 */
variant dump_private_keys();


#endif //DECENT_WALLET_WALLET_FILE_H
