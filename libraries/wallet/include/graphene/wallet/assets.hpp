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

#ifndef DECENT_WALLET_ASSETS_H
#define DECENT_WALLET_ASSETS_H

/**
 * @brief Lists all assets registered on the blockchain.
 * To list all assets, pass the empty string \c "" for the \c lowerbound to start
 * at the beginning of the list, and iterate as necessary.
 * @param lowerbound  the symbol of the first asset to include in the list
 * @param limit the maximum number of assets to return (max: 100)
 * @return the list of asset objects, ordered by symbol
 * @ingroup WalletAPI_Asset
 */
vector<asset_object>              list_assets(const string& lowerbound, uint32_t limit)const;

/**
 * @brief Returns information about the given asset.
 * @param asset_name_or_id the symbol or id of the asset in question
 * @return the information about the asset stored in the block chain
 * @ingroup WalletAPI_Asset
 */
asset_object                      get_asset(const string& asset_name_or_id) const;

/**
 * @brief Returns the specific data for a given monitored asset.
 * @see \c get_asset()
 * @param asset_name_or_id the symbol or id of the monitored asset in question
 * @return the specific data for this monitored asset
 * @ingroup WalletAPI_Asset
 */
monitored_asset_options        get_monitored_asset_data(const string& asset_name_or_id)const;

/**
 * @brief Creates a new monitored asset.
 * @note some parameters can be changed later using \c update_monitored_asset()
 * @note The wallet needs to be unlocked and a required key/s needs to be imported.
 * @param issuer the name or id of the account who will pay the fee and become the
 *               issuer of the new asset.  This can be updated later
 * @param symbol the ticker symbol of the new asset
 * @param precision the number of digits of precision to the right of the decimal point,
 *                  must be less than or equal to 12
 * @param description asset description. Maximal length is 1000 chars
 * @param feed_lifetime_sec time before a price feed expires
 * @param minimum_feeds minimum number of unexpired feeds required to extract a median feed from
 * @param broadcast \c true to broadcast the transaction on the network
 * @return the signed transaction creating a new asset
 * @ingroup WalletAPI_Asset
 */
pair<transaction_id_type,signed_transaction> create_monitored_asset(const string& issuer,
                                                                    const string& symbol,
                                                                    uint8_t precision,
                                                                    const string& description,
                                                                    uint32_t feed_lifetime_sec,
                                                                    uint8_t minimum_feeds,
                                                                    bool broadcast = false);

/**
 * @brief Update the parameters specific to a monitored asset.
 * @note The wallet needs to be unlocked and a required key/s needs to be imported.
 * @param symbol the name or id of the asset to update, which must be a monitored asset
 * @param description asset description
 * @param feed_lifetime_sec time before a price feed expires
 * @param minimum_feeds minimum number of unexpired feeds required to extract a median feed from
 * @param broadcast \c true to broadcast the transaction on the network
 * @return the signed transaction updating the monitored asset
 * @ingroup WalletAPI_Asset
 */
pair<transaction_id_type,signed_transaction> update_monitored_asset(const string& symbol,
                                                                    const string& description,
                                                                    uint32_t feed_lifetime_sec,
                                                                    uint8_t minimum_feeds,
                                                                    bool broadcast = false);

/**
 * @brief Creates a new user-issued asset.
 * @note Some parameters can be changed later using \c update_user_issued_asset()
 * @note The wallet needs to be unlocked and a required key/s needs to be imported.
 * @see \c issue_asset()
 * @param issuer the name or id of the account who will pay the fee and become the
 *               issuer of the new asset.  This can be updated later
 * @param symbol the ticker symbol of the new asset
 * @param precision the number of digits of precision to the right of the decimal point,
 *               must be less than or equal to 12
 * @param description asset description. Maximal length is 1000 chars
 * @param max_supply the maximum supply of this asset which may exist at any given time
 * @param core_exchange_rate core_exchange_rate is a price struct which consist from base asset
 *               and quote asset (see price). One of the asset has to be core asset.
 *               Technically core_exchange_rate needs to store the asset id of
 *               this new asset. Since this id is not known at the time this operation is
 *               created, create this price as though the new asset id has instance 1, and
 *               the chain will overwrite it with the new asset's id
 * @param is_exchangeable \c true to allow implicit conversion when buing content of this asset to/from core asset
 * @param is_fixed_max_supply true to deny future modifications of 'max_supply' otherwise false
 * @param broadcast \c true to broadcast the transaction on the network
 * @return the signed transaction creating a new asset
 * @ingroup WalletAPI_Asset
 */
pair<transaction_id_type,signed_transaction> create_user_issued_asset(const string& issuer,
                                                                      const string& symbol,
                                                                      uint8_t precision,
                                                                      const string& description,
                                                                      uint64_t max_supply,
                                                                      price core_exchange_rate,
                                                                      bool is_exchangeable,
                                                                      bool is_fixed_max_supply,
                                                                      bool broadcast = false);

/**
 * @brief Issue new shares of an asset.
 * @note The wallet needs to be unlocked and a required key/s needs to be imported.
 * @param to_account the name or id of the account to receive the new shares
 * @param amount the amount to issue, in nominal units
 * @param symbol the ticker symbol of the asset to issue
 * @param memo a memo to include in the transaction, readable by the recipient
 * @param broadcast \c true to broadcast the transaction on the network
 * @return the signed transaction issuing the new shares
 * @ingroup WalletAPI_Asset
 */
pair<transaction_id_type,signed_transaction> issue_asset(const string& to_account,
                                                         const string& amount,
                                                         const string& symbol,
                                                         const string& memo,
                                                         bool broadcast = false);

/**
 * @brief Update the parameters specific to a user issued asset.
 * User issued assets have some options which are not relevant to other asset types. This operation is used to update those
 * options an an existing user issues asset.
 * @note The wallet needs to be unlocked and a required key/s needs to be imported.
 * @param symbol the name or id of the asset to update, which must be a user-issued asset
 * @param new_issuer if the asset is to be given a new issuer, specify his ID here
 * @param description asset description
 * @param max_supply the maximum supply of this asset which may exist at any given time
 * @param core_exchange_rate price used to convert non-core asset to core asset
 * @param is_exchangeable \c true to allow implicit conversion of this asset to/from core asset
 * @param broadcast \c true to broadcast the transaction on the network
 * @return the signed transaction updating the user-issued asset
 * @ingroup WalletAPI_Asset
 */
pair<transaction_id_type,signed_transaction> update_user_issued_asset(const string& symbol,
                                                                      const string& new_issuer,
                                                                      const string& description,
                                                                      uint64_t max_supply,
                                                                      price core_exchange_rate,
                                                                      bool is_exchangeable,
                                                                      bool broadcast = false);

/**
 * @brief Pay into the pools for the given asset. Allows anyone to deposit core/asset into pools.
 * @note User-issued assets can optionally have two asset pools.
 * This pools are used when conversion between assets is needed (paying fees, paying for a content in different asset ).
 * @note The wallet needs to be unlocked and a required key/s needs to be imported.
 * @param from the name or id of the account sending the core asset
 * @param uia_amount the amount of "this" asset to deposit
 * @param uia_symbol the name or id of the asset whose pool you wish to fund
 * @param dct_amount the amount of the core asset to deposit
 * @param dct_symbol the name or id of the DCT asset
 * @param broadcast \c true to broadcast the transaction on the network
 * @return the signed transaction funding the asset pools
 * @ingroup WalletAPI_Asset
 */
pair<transaction_id_type,signed_transaction> fund_asset_pools(const string& from,
                                                              const string& uia_amount,
                                                              const string& uia_symbol,
                                                              const string& dct_amount,
                                                              const string& dct_symbol,
                                                              bool broadcast = false);

/**
 * @brief Burns the given user-issued asset.
 * This command burns the user-issued asset to reduce the amount in circulation.
 * @note you cannot burn monitored asset.
 * @note The wallet needs to be unlocked and a required key/s needs to be imported.
 * @param from the account containing the asset you wish to burn
 * @param amount the amount to burn, in nominal units
 * @param symbol the name or id of the asset to burn
 * @param broadcast \c true to broadcast the transaction on the network
 * @return the signed transaction burning the asset
 * @ingroup WalletAPI_Asset
 */
pair<transaction_id_type,signed_transaction> reserve_asset(const string& from,
                                                           const string& amount,
                                                           const string& symbol,
                                                           bool broadcast = false);

/**
 * @brief Transfers accumulated assets from pools back to the issuer's balance.
 * @note You cannot claim assets from pools of monitored asset.
 * @note The wallet needs to be unlocked and a required key/s needs to be imported.
 * @param uia_amount the amount of "this" asset to claim, in nominal units
 * @param uia_symbol the name or id of the asset to claim
 * @param dct_amount the amount of DCT asset to claim, in nominal units
 * @param dct_symbol the name or id of the DCT asset to claim
 * @param broadcast \c true to broadcast the transaction on the network
 * @return the signed transaction claiming the fees
 * @ingroup WalletAPI_Asset
 */
pair<transaction_id_type,signed_transaction> claim_fees(const string& uia_amount,
                                                        const string& uia_symbol,
                                                        const string& dct_amount,
                                                        const string& dct_symbol,
                                                        bool broadcast = false);

/**
 * @brief Converts asset into DCT, using actual price feed.
 * @param amount the amount to convert in nominal units
 * @param asset_symbol_or_id the symbol or id of the asset to convert
 * @return price in DCT
 * @ingroup WalletAPI_Asset
 */
string price_to_dct(const string& amount, const string& asset_symbol_or_id);

/**
 * @brief Publishes a price feed for the named asset.
 * Price feed providers use this command to publish their price feeds for monitored assets. A price feed is
 * used to tune the market for a particular monitored asset. For each value in the feed, the median across all
 * miner feeds for that asset is calculated and the market for the asset is configured with the median of that
 * value.
 * @note The wallet needs to be unlocked and a required key/s needs to be imported.
 * @param publishing_account the account publishing the price feed
 * @param symbol the name or id of the asset whose feed we're publishing
 * @param feed the price feed object for particular monitored asset
 * @param broadcast \c true to broadcast the transaction on the network
 * @return the signed transaction updating the price feed for the given asset
 * @ingroup WalletAPI_Asset
 */
pair<transaction_id_type,signed_transaction> publish_asset_feed(const string& publishing_account,
                                                                const string& symbol,
                                                                price_feed feed,
                                                                bool broadcast = false);

/**
 * @brief Get a list of published price feeds by a miner.
 * @param account_name_or_id the name or id of the account
 * @param count maximum number of price feeds to fetch (must not exceed 100)
 * @return list of price feeds published by the miner
 * @ingroup WalletAPI_Asset
 */
multimap<time_point_sec, price_feed> get_feeds_by_miner(const string& account_name_or_id,
                                                        const uint32_t count);

/**
 * @brief Get current supply of the core asset
 * @return the number of shares currently in existence in account and vesting balances, escrows and pools
 * @ingroup WalletAPI_Asset
 */
real_supply get_real_supply() const;


#endif //DECENT_WALLET_ASSETS_H
