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

#ifndef DECENT_WALLET_GENERAL_H
#define DECENT_WALLET_GENERAL_H

/**
 * @brief Returns info such as client version, git version of graphene/fc, version of boost, openssl.
 * @return compile time info and client and dependencies versions
 * @ingroup WalletAPI_General
 */
variant_object                    about() const;

/**
 * @brief Retrieve a full, signed block with info.
 * @param num ID/height of the block
 * @return the referenced block with info, or \c null if no matching block was found
 * @ingroup WalletAPI_General
 */
optional<signed_block_with_info>    get_block( uint32_t num );

/**
 * @brief Returns the blockchain's slowly-changing properties.
 * This object contains all of the properties of the blockchain that are fixed
 * or that change only once per maintenance interval such as the
 * current list of miners, block interval, etc.
 * @see \c get_dynamic_global_properties() for frequently changing properties
 * @return the global properties
 * @ingroup WalletAPI_General
 */
global_property_object            get_global_properties() const;

/**
 * @brief Returns the blockchain's rapidly-changing properties.
 * The returned object contains information that changes every block interval
 * such as the head block number, the next miner, etc.
 * @see \c get_global_properties() for less-frequently changing properties
 * @return the dynamic global properties
 * @ingroup WalletAPI_General
 */
dynamic_global_property_object    get_dynamic_global_properties() const;

/**
 * @brief Returns the blockchain object corresponding to the given id.
 * This generic function can be used to retrieve any object from the blockchain
 * that is assigned an ID.  Certain types of objects have specialized convenience
 * functions to return their objects -- e.g., assets have \c get_asset(), accounts
 * have \c get_account(), but this function will work for any object.
 * @param id the id of the object to return
 * @return the requested object
 * @ingroup WalletAPI_General
 */
variant                           get_object(object_id_type id) const;

/**
 * @brief Query the last local block.
 * @return the block time
 * @ingroup WalletAPI_General
 */
fc::time_point_sec head_block_time() const;

/**
 * @brief Get information about current state of the blockchain,
 * such as head block number, chain_id, list of active miners,...
 * @return information about current state of the blockchain
 * @ingroup WalletAPI_General
 */
variant                           info();

/**
 * @brief Returns a list of all commands supported by the wallet API.
 * This lists each command, along with its arguments and return types.
 * For more detailed help on a single command, use \c get_help()
 * @return a multi-line string suitable for displaying on a terminal
 * @ingroup WalletAPI_General
 */
string  help() const;

/**
 * @brief Returns detailed help on a single API command.
 * @param method the name of the API command you want help with
 * @return a multi-line string suitable for displaying on a terminal
 * @ingroup WalletAPI_General
 */
string  get_help(const string& method) const;

/**
 * @brief Sign a buffer.
 * @param str_buffer the buffer to be signed
 * @param str_brainkey derives the private key used for signature
 * @return the signed buffer
 * @ingroup WalletAPI_General
 */
std::string sign_buffer(const std::string& str_buffer,
                        const std::string& str_brainkey) const;

/**
 * @brief Verify if the signature is valid.
 * @param str_buffer the original buffer
 * @param str_publickey the public key used for verification
 * @param str_signature the signed buffer
 * @return \c true if valid, otherwise \c false
 * @ingroup WalletAPI_General
 */
bool verify_signature(const std::string& str_buffer,
                      const std::string& str_publickey,
                      const std::string& str_signature) const;

/**
 * @brief
 * @param nodes
 * @ingroup WalletAPI_General
 */
void network_add_nodes( const vector<string>& nodes );

/**
 * @brief
 * @ingroup WalletAPI_General
 */
vector< variant > network_get_connected_peers();

/**
 * @brief This method is used to convert a JSON transaction to its transaction ID.
 * @param trx Signed transaction
 * @return The transaction ID
 * @ingroup WalletAPI_General
 */
transaction_id_type get_transaction_id( const signed_transaction& trx ) const;

/**
 * @brief This method will return the transaction for the given ID (transaction hash) or
 * it will return \c null if it is not known. Just because it is not known does not mean it wasn't
 * included in the blockchain.
 * @note By default these objects are not tracked, the transaction_history_plugin must
 * be loaded for these objects to be maintained.
 * @param id ID (transaction hash) of the transaction to retrieve
 * @return The transaction
 * @ingroup WalletAPI_General
 */
optional<signed_transaction> get_transaction_by_id( const transaction_id_type& id ) const;

/**
 * @brief Listing the operations available.
 * @return a vector of operations with ids, names and fees
 * @ingroup WalletAPI_General
 */
vector<operation_info> list_operations();

/**
 * @brief This method encapsulates the functionality of running a sequence of calls
 * loaded from a text file.
 * @param command_file_name The name of the command file to load
 * @ingroup WalletAPI_General
 */
void from_command_file( const std::string& command_file_name ) const;

/**
 * @brief Exits the CLI Wallet
 * @ingroup WalletAPI_General
 */
void exit() const;

/**
 * @brief Exits the CLI Wallet
 * @ingroup WalletAPI_General
 */
void quit() const;

#endif //DECENT_WALLET_GENERAL_H
