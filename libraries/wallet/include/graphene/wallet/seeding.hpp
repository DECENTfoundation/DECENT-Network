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

#ifndef DECENT_WALLET_SEEDING_H
#define DECENT_WALLET_SEEDING_H


/**
 * @brief Get a list of seeders by price, in increasing order
 * @param count Maximum number of seeders to retrieve
 * @return The seeders found
 * @ingroup WalletCLI
 */
vector<seeder_object> list_seeders_by_price( uint32_t count )const;

/**
 * @brief Get a list of seeders ordered by total upload, in decreasing order
 * @param count Maximum number of seeders to retrieve
 * @return The seeders found
 * @ingroup WalletCLI
 */
optional<vector<seeder_object>> list_seeders_by_upload( const uint32_t count )const;

/**
 * @brief Get a list of seeders by region code
 * @param region_code Region code of seeders to retrieve
 * @return The seeders found
 * @ingroup WalletCLI
 */
vector<seeder_object> list_seeders_by_region( const string region_code )const;

/**
 * @brief Get a list of seeders ordered by rating, in decreasing order
 * @param count Maximum number of seeders to retrieve
 * @return The seeders found
 * @ingroup WalletCLI
 */
vector<seeder_object> list_seeders_by_rating( const uint32_t count )const;


#endif //DECENT_WALLET_SEEDING_H
