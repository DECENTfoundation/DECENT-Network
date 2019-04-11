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

#ifndef DECENT_WALLET_SUBSCRIPTION_H
#define DECENT_WALLET_SUBSCRIPTION_H

/**
 * @brief Creates a subscription to author. This function is used by consumers.
 * @note The wallet needs to be unlocked and a required key/s needs to be imported.
 * @param from account who wants subscription to author
 * @param to the author you wish to subscribe to
 * @param price_amount price for the subscription
 * @param price_asset_symbol ticker symbol of the asset which will be used to buy subscription
 * @param broadcast \c true if you wish to broadcast the transaction
 * @return the signed transaction subscribing the consumer to the author
 * @ingroup WalletAPI_Subscription
 */
signed_transaction subscribe_to_author( const string& from,
                                        const string& to,
                                        const string& price_amount,
                                        const string& price_asset_symbol,
                                        bool broadcast/* = false */);

/**
 * @brief Creates a subscription to author. This function is used by author.
 * @note The wallet needs to be unlocked and a required key/s needs to be imported.
 * @param from the account obtaining subscription from the author
 * @param to the name or id of the author
 * @param broadcast \c true if you wish to broadcast the transaction
 * @return the signed transaction subscribing the consumer to the author
 * @ingroup WalletAPI_Subscription
 */
signed_transaction subscribe_by_author( const string& from,
                                        const string& to,
                                        bool broadcast/* = false */);

/**
 * @brief This function can be used to allow/disallow subscription.
 * @note The wallet needs to be unlocked and a required key/s needs to be imported.
 * @param account the name or id of the account to update
 * @param allow_subscription \c true if account (author) wants to allow subscription, \c false otherwise
 * @param subscription_period duration of subscription in days
 * @param price_amount price for subscription per one subscription period
 * @param price_asset_symbol ticker symbol of the asset which will be used to buy subscription
 * @param broadcast \c true if you wish to broadcast the transaction
 * @return the signed transaction updating the account
 * @ingroup WalletAPI_Subscription
 */
signed_transaction set_subscription( const string& account,
                                     bool allow_subscription,
                                     uint32_t subscription_period,
                                     const string& price_amount,
                                     const string& price_asset_symbol,
                                     bool broadcast/* = false */);

/**
 * @brief This function can be used to allow/disallow automatic renewal of expired subscription.
 * @note The wallet needs to be unlocked and a required key/s needs to be imported.
 * @param account_id_or_name the name or id of the account to update
 * @param subscription_id the ID of the subscription.
 * @param automatic_renewal \c true if account (consumer) wants to allow automatic renewal of subscription, \c false otherwise
 * @param broadcast \c true if you wish to broadcast the transaction
 * @return the signed transaction allowing/disallowing renewal of the subscription
 * @ingroup WalletAPI_Subscription
 */
signed_transaction set_automatic_renewal_of_subscription( const string& account_id_or_name,
                                                          subscription_id_type subscription_id,
                                                          bool automatic_renewal,
                                                          bool broadcast/* = false */);

/**
 * @brief Get a list of consumer's active (not expired) subscriptions.
 * @param account_id_or_name the name or id of the consumer
 * @param count maximum number of subscriptions to fetch (must not exceed 100)
 * @return list of active subscription objects corresponding to the provided consumer
 * @ingroup WalletAPI_Subscription
 */
vector< subscription_object > list_active_subscriptions_by_consumer( const string& account_id_or_name, const uint32_t count)const;

/**
 * @brief Get a list of consumer's subscriptions.
 * @param account_id_or_name the name or id of the consumer
 * @param count maximum number of subscriptions to fetch (must not exceed 100)
 * @return list of subscription objects corresponding to the provided consumer
 * @ingroup WalletAPI_Subscription
 */
vector< subscription_object > list_subscriptions_by_consumer( const string& account_id_or_name, const uint32_t count)const;

/**
 * @brief Get a list of active (not expired) subscriptions to author.
 * @param account_id_or_name the name or id of the author
 * @param count maximum number of subscriptions to fetch (must not exceed 100)
 * @return list of active subscription objects corresponding to the provided author
 * @ingroup WalletAPI_Subscription
 */
vector< subscription_object > list_active_subscriptions_by_author( const string& account_id_or_name, const uint32_t count)const;

/**
 * @brief Get a list of subscriptions to author.
 * @param account_id_or_name the name or id of the author
 * @param count maximum number of subscriptions to fetch (must not exceed 100)
 * @return list of subscription objects corresponding to the provided author
 * @ingroup WalletAPI_Subscription
 */
vector< subscription_object > list_subscriptions_by_author( const string& account_id_or_name, const uint32_t count)const;


#endif //DECENT_WALLET_SUBSCRIPTION_H
