#ifndef DECENT_WALLET_CONTENT_H
#define DECENT_WALLET_CONTENT_H

/**
 * @brief Submits or resubmits content to the blockchain. In a case of resubmit, co-authors, price and synopsis fields
 * can be modified.
 * @see submit_content_new()
 * @param author The author of the content
 * @param co_authors The co-authors' account name or ID mapped to corresponding payment split based on basis points
 * @param URI The URI of the content
 * @param price_amounts The price of the content per regions
 * @param size The size of the content
 * @param hash The Hash of the package
 * @param seeders List of the seeders, which will publish the content
 * @param quorum Defines number of seeders needed to restore the encryption key
 * @param expiration The expiration time of the content. The content is available to buy till it's expiration time
 * @param publishing_fee_asset Ticker symbol of the asset which will be used to publish content
 * @param publishing_fee_amount Publishing price
 * @param synopsis The description of the content
 * @param secret The AES key used to encrypt and decrypt the content
 * @param cd Custody data
 * @param broadcast true to broadcast the transaction on the network
 * @return The signed transaction submitting the content
 * @ingroup WalletCLI
 */
signed_transaction
submit_content(string const& author,
               vector< pair< string, uint32_t>> co_authors,
               string const& URI,
               vector <regional_price_info> const& price_amounts,
               uint64_t size,
               fc::ripemd160 const& hash,
               vector<account_id_type> const& seeders,
               uint32_t quorum,
               fc::time_point_sec const& expiration,
               string const& publishing_fee_asset,
               string const& publishing_fee_amount,
               string const& synopsis,
               DInteger const& secret,
               decent::encrypt::CustodyData const& cd,
               bool broadcast);

/**
 * @brief This function is used to create package, upload package and submit content in one step.
 * @see create_package()
 * @see upload_package()
 * @see submit_content()
 * @param author The author of the content
 * @param co_authors The co-authors' account name or ID mapped to corresponding payment split based on basis points
 * @param content_dir Path to the directory containing all content that should be packed
 * @param samples_dir Path to the directory containing samples of content
 * @param protocol Protocol for uploading package( ipfs )
 * @param price_amounts The prices of the content per regions
 * @param seeders List of the seeders, which will publish the content
 * @param expiration The expiration time of the content. The content is available to buy till it's expiration time
 * @param synopsis The description of the content
 * @return The signed transaction submitting the content
 * @ingroup WalletCLI
 */

void submit_content_async( string const &author,
                                    vector< pair< string, uint32_t>> co_authors,
                                    string const &content_dir,
                                    string const &samples_dir,
                                    string const &protocol,
                                    vector<regional_price_info> const &price_amounts,
                                    vector<account_id_type> const &seeders,
                                    fc::time_point_sec const &expiration,
                                    string const &synopsis);


/**
 * @brief This function can be used to cancel submitted content. This content is immediately not available to purchase.
 * Seeders keep seeding this content in next 24 hours.
 * @param author The author of the content
 * @param URI The URI of the content
 * @param broadcast True to broadcast the transaction on the network
 * @ingroup WalletCLI
 * @return signed transaction
 */
signed_transaction content_cancellation(string author,
                                        string URI,
                                        bool broadcast);

/**
 * @brief Downloads encrypted content specified by provided URI.
 * @param consumer Consumer of the content
 * @param URI The URI of the content
 * @param region_code_from Two letter region code
 * @param broadcast true to broadcast the transaction on the network
 * @ingroup WalletCLI
 */
void download_content(string const& consumer, string const& URI, string const& region_code_from, bool broadcast = false);

/**
 * @brief Get status about particular download process specified by provided URI.
 * @param consumer Consumer of the content
 * @param URI The URI of the content
 * @return Download status, or null if no matching download process was found
 * @ingroup WalletCLI
 */
optional<content_download_status> get_download_status(string consumer, string URI) const;

/**
 * @brief This function is used to send a request to buy a content. This request is caught by seeders.
 * @param consumer Consumer of the content
 * @param URI The URI of the content
 * @param price_asset_name Ticker symbol of the asset which will be used to buy content
 * @param price_amount The price of the content
 * @param str_region_code_from Two letter region code
 * @param broadcast true to broadcast the transaction on the network
 * @return The signed transaction requesting buying of the content
 * @ingroup WalletCLI
 */
signed_transaction request_to_buy(string consumer,
                                  string URI,
                                  string price_asset_name,
                                  string price_amount,
                                  string str_region_code_from,
                                  bool broadcast);

/**
 * @brief This method allows user to start seeding plugin from running application
 * @param account_id_type_or_name Name or ID of account controlling this seeder
 * @param content_private_key El Gamal content private key
 * @param seeder_private_key Private key of the account controlling this seeder
 * @param free_space Allocated disk space, in MegaBytes
 * @param seeding_price price per MegaByte
 * @param seeding_symbol seeding price asset, e.g. DCT
 * @param packages_path Packages storage path
 * @param region_code Optional ISO 3166-1 alpha-2 two-letter region code
 * @ingroup WalletCLI
 */
void seeding_startup( string account_id_type_or_name,
                      DInteger content_private_key,
                      string seeder_private_key,
                      uint64_t free_space,
                      uint32_t seeding_price,
                      string seeding_symbol,
                      string packages_path,
                      string region_code = "" );

/**
 * @brief Rates and comments a content.
 * @param consumer Consumer giving the rating
 * @param URI The URI of the content
 * @param rating Rating
 * @param comment Comment
 * @param broadcast true to broadcast the transaction on the network
 * @ingroup WalletCLI
 */
void leave_rating_and_comment(string consumer,
                              string URI,
                              uint64_t rating,
                              string comment,
                              bool broadcast = false);

/**
 * @brief Get a list of open buyings
 * @return Open buying objects
 * @ingroup WalletCLI
 */
vector<buying_object> get_open_buyings()const;

/**
 * @brief Get a list of open buyings by URI
 * @param URI URI of the buyings to retrieve
 * @return Open buyings corresponding to the provided URI
 * @ingroup WalletCLI
 */
vector<buying_object> get_open_buyings_by_URI( const string& URI )const;

/**
 * @brief Get a list of open buyings by consumer
 * @param account_id_or_name Consumer of the buyings to retrieve
 * @return Open buyings corresponding to the provided consumer
 * @ingroup WalletCLI
 */
vector<buying_object> get_open_buyings_by_consumer( const string& account_id_or_name )const;

/**
 * @brief Get history buyings by consumer
 * @param account_id_or_name Consumer of the buyings to retrieve
 * @return History buying objects corresponding to the provided consumer
 * @ingroup WalletCLI
 */
vector<buying_object> get_buying_history_objects_by_consumer( const string& account_id_or_name )const;


/**
 * @brief Get history buying objects by consumer that match search term
 * @param account_id_or_name Consumer of the buyings to retrieve
 * @param term Search term to look up in Title and Description
 * @param order Sort data by field
 * @param id Object_id to start searching from
 * @param count Maximum number of contents to fetch (must not exceed 100)
 * @return History buying objects corresponding to the provided consumer and matching search term
 * @ingroup WalletCLI
 */
vector<buying_object_ex> search_my_purchases(const string& account_id_or_name,
                                             const string& term,
                                             const string& order,
                                             const string& id,
                                             uint32_t count) const;

/**
* @brief Get buying (open or history) by consumer and URI
* @param account_id_or_name Consumer of the buying to retrieve
* @param URI URI of the buying to retrieve
* @return Buying_objects corresponding to the provided consumer, or null if no matching buying was found
* @ingroup WalletCLI
*/
optional<buying_object> get_buying_by_consumer_URI( const string& account_id_or_name, const string & URI )const;

/**
 * @brief Search for term in contents (author, title and description)
 * @param user Feedback author
 * @param URI The content object URI
 * @param id The id of feedback object to start searching from
 * @param count Maximum number of feedbacks to fetch
 * @return The feedback found
 * @ingroup WalletCLI
 */
vector<rating_object_ex> search_feedback(const string& user,
                                         const string& URI,
                                         const string& id,
                                         uint32_t count) const;

/**
 * @brief Get a content by URI
 * @param URI URI of the content to retrieve
 * @return The content corresponding to the provided URI, or null if no matching content was found
 * @ingroup WalletCLI
 */
optional<content_object> get_content( const string& URI )const;

/**
 * @brief Get a list of contents ordered alphabetically by search term
 * @param term Search term
 * @param order Order field
 * @param user Content owner
 * @param region_code Two letter region code
 * @param id The id of content object to start searching from
 * @param type The application and content type to be filtered
 * @param count Maximum number of contents to fetch (must not exceed 100)
 * @return The contents found
 * @ingroup WalletCLI
 */
vector<content_summary> search_content(const string& term,
                                       const string& order,
                                       const string& user,
                                       const string& region_code,
                                       const string& id,
                                       const string& type,
                                       uint32_t count )const;
/**
 * @brief Get a list of contents ordered alphabetically by search term
 * @param user Content owner
 * @param term Search term
 * @param order Order field
 * @param region_code Two letter region code
 * @param id The id of content object to start searching from
 * @param type The application and content type to be filtered
 * @param count Maximum number of contents to fetch (must not exceed 100)
 * @return The contents found
 * @ingroup WalletCLI
 */
vector<content_summary> search_user_content(const string& user,
                                            const string& term,
                                            const string& order,
                                            const string& region_code,
                                            const string& id,
                                            const string& type,
                                            uint32_t count )const;

/**
 * @brief Get author and list of co-authors of a content corresponding to the provided URI
 * @param URI URI of the content
 * @return The autor of the content and the list of co-authors, if provided
 */
pair<account_id_type, vector<account_id_type>> get_author_and_co_authors_by_URI( const string& URI )const;

/**
 * @brief Create package from selected files
 * @param content_dir Directory containing all content that should be packed
 * @param samples_dir Directory containing samples of content
 * @param aes_key AES key for encryption
 * @return package hash (ripemd160 hash of package content) and content custody data
 * @ingroup WalletCLI
 */
std::pair<string, decent::encrypt::CustodyData> create_package(const std::string& content_dir, const std::string& samples_dir, const DInteger& aes_key) const;


/**
 * @brief Extract selected package
 * @param package_hash Hash of package that needs to be extracted
 * @param output_dir Directory where extracted files will be created
 * @param aes_key AES key for decryption
 * @ingroup WalletCLI
 */
void extract_package(const std::string& package_hash, const std::string& output_dir, const DInteger& aes_key) const;

/**
 * @brief Download package
 * @param url Magnet or IPFS URL of package
 * @ingroup WalletCLI
 */
void download_package(const std::string& url) const;

/**
 * @brief Start uploading package
 * @param package_hash Hash of package that needs to be extracted
 * @param protocol protocol for uploading package ( ipfs )
 * @ingroup WalletCLI
 */
std::string upload_package(const std::string& package_hash, const std::string& protocol) const;

/**
 * @brief Remove package
 * @param package_hash Hash of package that needs to be removed
 * @ingroup WalletCLI
 */
void remove_package(const std::string& package_hash) const;

/**
 * @brief Restores AES key( used to encrypt and decrypt a content) from key particles stored in a buying object
 * @param account Consumers account id or name
 * @param buying The buying object containing key particles
 * @return restored AES key from particles
 * @ingroup WalletCLI
 */
DInteger restore_encryption_key(std::string account, buying_id_type buying);

/**
 * @brief Generates AES encryption key.
 * @return Random encryption key
 * @ingroup WalletCLI
 */
DInteger generate_encryption_key() const;


#endif //DECENT_WALLET_CONTENT_H
