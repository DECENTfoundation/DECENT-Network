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

#ifndef DECENT_WALLET_CONTENT_H
#define DECENT_WALLET_CONTENT_H

/**
 * @brief Submits or resubmits a content to the blockchain. In a case of resubmit, co-authors, price and synopsis fields
 * can be modified.
 * @see \c generate_encryption_key()
 * @see \c submit_content_async()
 * @param author the author of the content
 * @param co_authors the co-authors' account name or ID mapped to corresponding payment split based on basis points. The maximum number of co-authors is 10
 * @param URI the URI of the content
 * @param price_amounts the price of the content per regions
 * @param size the size of the content
 * @param hash the Hash of the package
 * @param seeders list of the seeders, which will publish the content
 * @param quorum defines number of seeders needed to restore the encryption key
 * @param expiration the expiration time of the content. The content is available to buy till it's expiration time
 * @param publishing_fee_asset ticker symbol of the asset which will be used to publish content
 * @param publishing_fee_amount publishing price
 * @param synopsis the description of the content
 * @param secret the AES key used to encrypt and decrypt the content
 * @param cd custody data
 * @param broadcast \c true to broadcast the transaction on the network
 * @return the signed transaction submitting the content
 * @ingroup WalletAPI_Content
 */
signed_transaction submit_content(const string& author,
                                  const vector< pair< string, uint32_t>>& co_authors,
                                  const string& URI,
                                  const vector <regional_price_info>& price_amounts,
                                  uint64_t size,
                                  const fc::ripemd160& hash,
                                  const vector<account_id_type>& seeders,
                                  uint32_t quorum,
                                  const fc::time_point_sec& expiration,
                                  const string& publishing_fee_asset,
                                  const string& publishing_fee_amount,
                                  const string& synopsis,
                                  const DInteger& secret,
                                  const decent::encrypt::CustodyData& cd,
                                  bool broadcast);

/**
 * @brief This function is used to create and upload a package and submit content in one step.
 * @see create_package()
 * @see upload_package()
 * @see submit_content()
 * @param author the author of the content
 * @param co_authors the co-authors' account name or ID mapped to corresponding payment split based on basis points. The maximum number of co-authors is 10
 * @param content_dir path to the directory containing all content that should be packed
 * @param samples_dir path to the directory containing samples of content
 * @param protocol protocol for uploading package( ipfs )
 * @param price_amounts the prices of the content per regions
 * @param seeders list of the seeders, which will publish the content
 * @param expiration the expiration time of the content. The content is available to buy till it's expiration time
 * @param synopsis the description of the content
 * @ingroup WalletAPI_Content
 */
void submit_content_async( const string& author,
                           const vector< pair< string, uint32_t>>& co_authors,
                           const string& content_dir,
                           const string& samples_dir,
                           const string& protocol,
                           const vector<regional_price_info>& price_amounts,
                           const vector<account_id_type>& seeders,
                           const fc::time_point_sec& expiration,
                           const string& synopsis);

/**
 * @brief This function can be used to cancel submitted content. This content is immediately not available to purchase.
 * Seeders keep seeding this content up to next 24 hours.
 * @param author the author of the content
 * @param URI the URI of the content
 * @param broadcast \c true to broadcast the transaction on the network
 * @return the signed transaction cancelling the content
 * @ingroup WalletAPI_Content
 */
signed_transaction content_cancellation(const string& author,
                                        const string& URI,
                                        bool broadcast);

/**
 * @brief Downloads encrypted content specified by provided URI.
 * @param consumer consumer of the content
 * @param URI the URI of the content
 * @param region_code_from two letter region code
 * @param broadcast \c true to broadcast the transaction on the network
 * @ingroup WalletAPI_Content
 */
void download_content(const string& consumer, const string& URI, const string& region_code_from, bool broadcast = false);

/**
 * @brief Get status about particular download process specified by provided URI.
 * @param consumer consumer of the content
 * @param URI the URI of the content
 * @return download status, or \c null if no matching download process was found
 * @ingroup WalletAPI_Content
 */
optional<content_download_status> get_download_status(const string& consumer, const string& URI) const;

/**
 * @brief This function is used to send a request to buy a content. This request is caught by seeders.
 * @param consumer consumer of the content
 * @param URI the URI of the content
 * @param price_asset_name ticker symbol of the asset which will be used to buy content
 * @param price_amount the price of the content
 * @param str_region_code_from two letter region code
 * @param broadcast \c true to broadcast the transaction on the network
 * @return the signed transaction requesting buying of the content
 * @ingroup WalletAPI_Content
 */
signed_transaction request_to_buy(const string& consumer,
                                  const string& URI,
                                  const string& price_asset_name,
                                  const string& price_amount,
                                  const string& str_region_code_from,
                                  bool broadcast);

/**
 * @brief This method allows user to start seeding plugin from running application
 * @param account_id_type_or_name name or ID of account controlling this seeder
 * @param content_private_key El Gamal content private key
 * @param seeder_private_key private key of the account controlling this seeder
 * @param free_space allocated disk space, in MegaBytes
 * @param seeding_price price per MegaByte
 * @param seeding_symbol seeding price asset, e.g. DCT
 * @param packages_path packages storage path
 * @param region_code optional ISO 3166-1 alpha-2 two-letter region code
 * @ingroup WalletAPI_Seeding
 */
void seeding_startup( const string& account_id_type_or_name,
                      DInteger content_private_key,
                      const string& seeder_private_key,
                      uint64_t free_space,
                      uint32_t seeding_price,
                      const string& seeding_symbol,
                      const string& packages_path,
                      const string& region_code = string() );

/**
 * @brief Rates and comments a content.
 * @param consumer consumer giving the rating
 * @param URI the URI of the content
 * @param rating the rating. The available options are 1-5
 * @param comment the maximum length of a comment is 100 characters
 * @param broadcast \c true to broadcast the transaction on the network
 * @ingroup WalletAPI_Content
 */
void leave_rating_and_comment(const string& consumer,
                              const string& URI,
                              uint64_t rating,
                              const string& comment,
                              bool broadcast = false);

/**
 * @brief Get a list of open buyings.
 * @return a list of open buying objects
 * @ingroup WalletAPI_Content
 */
vector<buying_object> get_open_buyings() const;

/**
 * @brief Get a list of open buyings by URI.
 * @param URI URI of the buyings to retrieve
 * @return a list of open buying objects corresponding to the provided URI
 * @ingroup WalletAPI_Content
 */
vector<buying_object> get_open_buyings_by_URI( const string& URI ) const;

/**
 * @brief Get a list of open buyings by consumer.
 * @param account_id_or_name consumer of the buyings to retrieve
 * @return a list of open buying objects corresponding to the provided consumer
 * @ingroup WalletAPI_Content
 */
vector<buying_object> get_open_buyings_by_consumer( const string& account_id_or_name ) const;

/**
 * @brief Get history buyings by consumer.
 * @param account_id_or_name consumer of the buyings to retrieve
 * @return a list of history buying objects corresponding to the provided consumer
 * @ingroup WalletAPI_Content
 */
vector<buying_object> get_buying_history_objects_by_consumer( const string& account_id_or_name ) const;

/**
 * @brief Get history buying objects by consumer that match search term.
 * @param account_id_or_name consumer of the buyings to retrieve
 * @param term search term to look up in \c title and \c description
 * @param order sort data by field. Available options are defined in 'database_api.cpp'
 * @param id the id of buying object to start searching from
 * @param count maximum number of contents to fetch (must not exceed 100)
 * @return a list of history buying objects corresponding to the provided consumer and matching search term
 * @ingroup WalletAPI_Content
 */
vector<buying_object_ex> search_my_purchases(const string& account_id_or_name,
                                             const string& term,
                                             const string& order,
                                             const string& id,
                                             uint32_t count) const;

/**
 * @brief Get buying object (open or history) by consumer and URI.
 * @param account_id_or_name consumer of the buying to retrieve
 * @param URI the URI of the buying to retrieve
 * @return buying objects corresponding to the provided consumer, or null if no matching buying was found
 * @ingroup WalletAPI_Content
 */
optional<buying_object> get_buying_by_consumer_URI( const string& account_id_or_name, const string & URI ) const;

/**
 * @brief Search for term in users' feedbacks.
 * @param user the author of the feedback
 * @param URI the content object URI
 * @param id the id of feedback object to start searching from
 * @param count maximum number of feedbacks to fetch
 * @return the feedback found
 * @ingroup WalletAPI_Content
 */
vector<rating_object_ex> search_feedback(const string& user,
                                         const string& URI,
                                         const string& id,
                                         uint32_t count) const;

/**
 * @brief Get a content by URI.
 * @param URI the URI of the content to retrieve
 * @return the content corresponding to the provided URI, or \c null if no matching content was found
 * @ingroup WalletAPI_Content
 */
optional<content_object> get_content( const string& URI ) const;

/**
 * @brief Get a list of contents ordered alphabetically by search term.
 * @param term search term
 * @param order order field. Available options are defined in 'database_api.cpp'
 * @param user content owner
 * @param region_code two letter region code
 * @param id the id of content object to start searching from
 * @param type the application and content type to be filtered, separated by comma.
 * Available options are defined in 'content_object.hpp'
 * @param count maximum number of contents to fetch (must not exceed 100)
 * @return the contents found
 * @ingroup WalletAPI_Content
 */
vector<content_summary> search_content(const string& term,
                                       const string& order,
                                       const string& user,
                                       const string& region_code,
                                       const string& id,
                                       const string& type,
                                       uint32_t count ) const;

/**
 * @brief Get a list of contents ordered alphabetically by search term.
 * @param user content owner
 * @param term search term. Available options are defined in 'database_api.cpp'
 * @param order order field
 * @param region_code two letter region code
 * @param id the id of content object to start searching from
 * @param type the application and content type to be filtered, separated by comma.
 * Available options are defined in 'content_object.hpp'
 * @param count maximum number of contents to fetch (must not exceed 100)
 * @return the contents found
 * @ingroup WalletAPI_Content
 */
vector<content_summary> search_user_content(const string& user,
                                            const string& term,
                                            const string& order,
                                            const string& region_code,
                                            const string& id,
                                            const string& type,
                                            uint32_t count ) const;

/**
 * @brief Get author and list of co-authors of a content corresponding to the provided URI.
 * @param URI the URI of the content
 * @return the autor of the content and the list of co-authors, if provided
 * @ingroup WalletAPI_Content
 */
pair<account_id_type, vector<account_id_type>> get_author_and_co_authors_by_URI( const string& URI ) const;

/**
 * @brief Creates a package from selected files.
 * @see \c upload_package()
 * @param content_dir the directory containing all content that should be packed
 * @param samples_dir the directory containing samples of the content
 * @param aes_key the AES key for encryption
 * @return the package hash and content custody data
 * @ingroup WalletAPI_Content
 */
std::pair<string, decent::encrypt::CustodyData> create_package(const std::string& content_dir,
                                                               const std::string& samples_dir,
                                                               const DInteger& aes_key) const;

/**
 * @brief Extracts selected package.
 * @see \c download_package()
 * @param package_hash hash of the package that needs to be extracted
 * @param output_dir directory where extracted files will be created
 * @param aes_key the AES key for decryption
 * @ingroup WalletAPI_Content
 */
void extract_package(const std::string& package_hash, const std::string& output_dir, const DInteger& aes_key) const;

/**
 * @brief Downloads the package.
 * @param url the URL of the package
 * @ingroup WalletAPI_Content
 */
void download_package(const std::string& url) const;

/**
 * @brief Starts uploading of the package.
 * @see \c create_package()
 * @param package_hash hash of the package that needs to be extracted
 * @param protocol protocol for uploading package ( ipfs )
 * @return URL of package
 * @ingroup WalletAPI_Content
 */
std::string upload_package(const std::string& package_hash, const std::string& protocol) const;

/**
 * @brief Removes the package.
 * @param package_hash hash of the package that needs to be removed
 * @ingroup WalletAPI_Content
 */
void remove_package(const std::string& package_hash) const;

/**
 * @brief Restores AES key( used to encrypt and decrypt a content) from key particles stored in a buying object.
 * @param account consumers account id or name
 * @param buying the buying object containing key particles
 * @return restored AES key from key particles
 * @ingroup WalletAPI_Content
 */
DInteger restore_encryption_key(const string& account, buying_id_type buying);

/**
 * @brief Generates AES encryption key.
 * @return random encryption key
 * @ingroup WalletAPI_Content
 */
DInteger generate_encryption_key() const;


#endif //DECENT_WALLET_CONTENT_H
