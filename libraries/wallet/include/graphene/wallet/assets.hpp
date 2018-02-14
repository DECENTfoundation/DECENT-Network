#ifndef DECENT_WALLET_ASSETS_H
#define DECENT_WALLET_ASSETS_H

/**
 * @brief Lists all assets registered on the blockchain.
 *
 * To list all assets, pass the empty string \c "" for the lowerbound to start
 * at the beginning of the list, and iterate as necessary.
 *
 * @param lowerbound  the symbol of the first asset to include in the list.
 * @param limit the maximum number of assets to return (max: 100)
 * @returns the list of asset objects, ordered by symbol
 * @ingroup WalletCLI
 */
vector<asset_object>              list_assets(const string& lowerbound, uint32_t limit)const;

/**
 * @brief Returns information about the given asset.
 * @param asset_name_or_id the symbol or id of the asset in question
 * @returns the information about the asset stored in the block chain
 * @ingroup WalletCLI
 */
asset_object                      get_asset(string asset_name_or_id) const;

/**
 * @brief Returns the BitAsset-specific data for a given asset.
 * Market-issued assets's behavior are determined both by their "BitAsset Data" and
 * their basic asset data, as returned by \c get_asset().
 * @param asset_name_or_id the symbol or id of the BitAsset in question
 * @returns the BitAsset-specific data for this asset
 * @ingroup WalletCLI
 */
monitored_asset_options        get_monitored_asset_data(string asset_name_or_id)const;

/**
 * @brief Lookup the id of a named asset.
 * @param asset_name_or_id the symbol of an asset to look up
 * @returns the id of the given asset
 * @ingroup WalletCLI
 */
asset_id_type                     get_asset_id(string asset_name_or_id) const;

/**
 * @brief Creates a new monitored asset.
 *
 * Options can be changed later using \c update_monitored_asset()
 *
 * @param issuer the name or id of the account who will pay the fee and become the
 *               issuer of the new asset.  This can be updated later
 * @param symbol the ticker symbol of the new asset
 * @param precision the number of digits of precision to the right of the decimal point,
 *                  must be less than or equal to 12
 * @param description asset description. Maximal length is 1000 chars.
 * @param feed_lifetime_sec time before a price feed expires
 * @param minimum_feeds minimum number of unexpired feeds required to extract a median feed from
 * @param broadcast true to broadcast the transaction on the network
 * @returns the signed transaction creating a new asset
 * @ingroup WalletCLI
 */
signed_transaction create_monitored_asset(string issuer,
                                          string symbol,
                                          uint8_t precision,
                                          string description,
                                          uint32_t feed_lifetime_sec,
                                          uint8_t minimum_feeds,
                                          bool broadcast = false);

/**
 * @brief Update the options specific to a monitored asset.
 *
 * Monitored assets have some options which are not relevant to other asset types. This operation is used to update those
 * options and an existing monitored asset.
 *
 * @param symbol the name or id of the asset to update, which must be a market-issued asset
 * @param description asset description
 * @param feed_lifetime_sec time before a price feed expires
 * @param minimum_feeds minimum number of unexpired feeds required to extract a median feed from
 * @param broadcast true to broadcast the transaction on the network
 * @returns the signed transaction updating the bitasset
 * @ingroup WalletCLI
 */
signed_transaction update_monitored_asset(string symbol,
                                          string description,
                                          uint32_t feed_lifetime_sec,
                                          uint8_t minimum_feeds,
                                          bool broadcast = false);

/**
 * @brief Creates a new user-issued asset.
 *
 * Options can be changed later using \c update_asset()
 *
 * @param issuer the name or id of the account who will pay the fee and become the
 *               issuer of the new asset.  This can be updated later
 * @param symbol the ticker symbol of the new asset
 * @param precision the number of digits of precision to the right of the decimal point,
 *                  must be less than or equal to 12
 * @param description asset description. Maximal length is 1000 chars
 * @param max_supply the maximum supply of this asset which may exist at any given time
 * @param core_exchange_rate Core_exchange_rate technically needs to store the asset ID of
 *               this new asset. Since this ID is not known at the time this operation is
 *               created, create this price as though the new asset has instance ID 1, and
 *               the chain will overwrite it with the new asset's ID
 * @param is_exchangeable True to allow implicit conversion of this asset to/from core asset
 * @param broadcast true to broadcast the transaction on the network
 * @returns the signed transaction creating a new asset
 * @ingroup WalletCLI
 */
signed_transaction create_user_issued_asset(string issuer,
                                            string symbol,
                                            uint8_t precision,
                                            string description,
                                            uint64_t max_supply,
                                            price core_exchange_rate,
                                            bool is_exchangeable,
                                            bool broadcast = false);

/** Issue new shares of an asset.
 *
 * @param to_account the name or id of the account to receive the new shares
 * @param amount the amount to issue, in nominal units
 * @param symbol the ticker symbol of the asset to issue
 * @param memo a memo to include in the transaction, readable by the recipient
 * @param broadcast true to broadcast the transaction on the network
 * @returns the signed transaction issuing the new shares
 */
   signed_transaction issue_asset(string to_account,
                                  string amount,
                                  string symbol,
                                  string memo,
                                  bool broadcast = false);

/**
 * @brief Update the options specific to a user issued asset.
 *
 * User issued assets have some options which are not relevant to other asset types. This operation is used to update those
 * options an an existing user issues asset.
 *
 *
 * @param symbol the name or id of the asset to update, which must be a market-issued asset
 * @param new_issuer if the asset is to be given a new issuer, specify his ID here
 * @param description asset description
 * @param max_supply The maximum supply of this asset which may exist at any given time
 * @param core_exchange_rate Price used to convert non-core asset to core asset
 * @param is_exchangeable True to allow implicit conversion of this asset to/from core asset
 * @param broadcast true to broadcast the transaction on the network
 * @returns the signed transaction updating the bitasset
 * @ingroup WalletCLI
 */
signed_transaction update_user_issued_asset(string symbol,
                                            string new_issuer,
                                            string description,
                                            uint64_t max_supply,
                                            price core_exchange_rate,
                                            bool is_exchangeable,
                                            bool broadcast = false);

/**
 * Pay into the pools for the given asset.
 *
 * User-issued assets can optionally have a pool of the core asset which is
 * automatically used to pay transaction fees for any transaction using that
 * asset (using the asset's core exchange rate).
 *
 * Allows anyone to deposit core/asset into pools.
 * This pool are used when conversion between assets is needed (paying fees, paying for a content in different asset).
 *
 * @param from the name or id of the account sending the core asset
 * @param uia_amount the amount of "this" asset to deposit
 * @param uia_symbol the name or id of the asset whose pool you wish to fund
 * @param dct_amount the amount of the core asset to deposit
 * @param dct_symbol the name or id of the DCT asset
 * @param broadcast true to broadcast the transaction on the network
 * @returns the signed transaction funding the fee pool
 */
signed_transaction fund_asset_pools(string from,
                                    string uia_amount,
                                    string uia_symbol,
                                    string dct_amount,
                                    string dct_symbol,
                                    bool broadcast = false);

/**
 * Burns the given user-issued asset.
 *
 * This command burns the user-issued asset to reduce the amount in circulation.
 * @note you cannot burn market-issued assets.
 * @param from the account containing the asset you wish to burn
 * @param amount the amount to burn, in nominal units
 * @param symbol the name or id of the asset to burn
 * @param broadcast true to broadcast the transaction on the network
 * @returns the signed transaction burning the asset
 */
signed_transaction reserve_asset(string from,
                                 string amount,
                                 string symbol,
                                 bool broadcast = false);

/**
 * Transfers accumulated assets from pools back to the issuer's balance.
 *
 * @note you cannot claim assets from pools of market-issued asset.
 * @param uia_amount the amount of "this" asset to claim, in nominal units
 * @param uia_symbol the name or id of the asset to claim
 * @param dct_amount the amount of DCT asset to claim, in nominal units
 * @param dct_symbol the name or id of the DCT asset to claim
 * @param broadcast true to broadcast the transaction on the network
 * @returns the signed transaction claiming the fees
 */
signed_transaction claim_fees(string uia_amount,
                              string uia_symbol,
                              string dct_amount,
                              string dct_symbol,
                              bool broadcast = false);

/**
 * @brief Converts asset into DCT, using actual price feed.
 * @param amount the amount to convert in nominal units
 * @param asset_symbol_or_id the symbol or id of the asset to convert
 * @return price in DCT
 */
string price_to_dct(const string& amount, const string& asset_symbol_or_id);

/**
 * @brief Publishes a price feed for the named asset.
 *
 * Price feed providers use this command to publish their price feeds for market-issued assets. A price feed is
 * used to tune the market for a particular market-issued asset. For each value in the feed, the median across all
 * miner feeds for that asset is calculated and the market for the asset is configured with the median of that
 * value.
 *
 * @param publishing_account the account publishing the price feed
 * @param symbol the name or id of the asset whose feed we're publishing
 * @param feed the price_feed object for particular market-issued asset
 * @param broadcast true to broadcast the transaction on the network
 * @returns the signed transaction updating the price feed for the given asset
 * @ingroup WalletCLI
 */
signed_transaction publish_asset_feed(string publishing_account,
                                      string symbol,
                                      price_feed feed,
                                      bool broadcast = false);

/**
 * @brief Get a list of published price feeds by a miner.
 *
 * @param account_name_or_id the name or id of the account
 * @param count Maximum number of price feeds to fetch (must not exceed 100)
 * @returns list of price feeds published by the miner
 * @ingroup WalletCLI
 */
multimap<time_point_sec, price_feed> get_feeds_by_miner(const string& account_name_or_id,
                                                        const uint32_t count);

/**
 * Get current supply of the core asset
 * @ingroup WalletCLI
 */
real_supply get_real_supply()const;


#endif //DECENT_WALLET_ASSETS_H
