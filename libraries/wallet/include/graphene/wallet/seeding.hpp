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
