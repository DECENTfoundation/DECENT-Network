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
#pragma once

#include <fc/exception/exception.hpp>

namespace graphene { namespace wallet {


   enum wallet_exception_code {
      account_name_or_id_cannot_be_empty_code   = 1,
      account_in_wallet_not_on_blockchain_code  = 2,

      nft_already_exist_code                    = 3,

      need_buffer_and_brainkey_code             = 4,
      need_buffer_pubkey_and_signature_code     = 5,

      wallet_filename_cannot_be_empty_code      = 6,
      wallet_is_locked_code                     = 7,
      wallet_is_already_locked_code             = 8,
      wallet_is_already_unlocked_code           = 9,
      password_cannot_be_empty_code             = 10,
      account_already_exist_code                = 11,
      invalid_wif_private_key_code              = 12,
      new_auth_needs_to_be_different_from_existing_code = 13,

      asset_not_found_code                      = 14,
      asset_not_monitored_code                  = 15,
      asset_already_exists_code                 = 16,

      invalid_transaction_handle_code           = 17,
      fees_can_be_paid_in_core_asset_code       = 18,
      unsupported_operation_code                = 19,

      no_miner_is_registered_for_this_owner_id_code = 20,
      owner_account_is_already_a_miner_code      = 21,
      account_is_not_registered_as_miner_code   = 22,
      account_was_already_voting_for_miner_code = 23,
      account_is_already_not_voting_for_miner_code = 24,
      voting_proxy_is_already_set_to_voter_code = 25,
      account_was_already_voting_for_itself_code = 26,
      account_was_already_voting_for_miners_code = 27,

      cannot_find_package_code                  = 28,
      package_is_not_in_valid_state_code        = 29,
      no_such_content_at_this_url_code          = 30,
      duplicity_at_the_list_of_coauthors_not_allowed_code = 31,
      the_prices_of_the_content_per_region_cannot_be_empty_code = 32,
      invalid_content_expiration_code           = 33,
      invalid_content_uri_code                  = 34,
      content_not_available_for_this_region_code = 35,
      cannot_find_download_object_code          = 36,

      could_not_find_matching_subscription_code = 37,

   };

   FC_DECLARE_EXCEPTION(wallet_exception, fc::wallet_exception_base_code, "wallet exception")

   FC_DECLARE_DERIVED_EXCEPTION(account_name_or_id_cannot_be_empty_exception, wallet_exception, fc::wallet_exception_base_code + account_name_or_id_cannot_be_empty_code, "Account name or id cannot be empty string.")
   FC_DECLARE_DERIVED_EXCEPTION(account_in_wallet_not_on_blockchain_exception, wallet_exception, fc::wallet_exception_base_code + account_in_wallet_not_on_blockchain_code, "Account present in the wallet but does not exist on the blockchain.")

   // general
   FC_DECLARE_DERIVED_EXCEPTION(nft_already_exist_exception, wallet_exception, fc::wallet_exception_base_code + nft_already_exist_code, "Non fungible token with that symbol already exists.")   
   FC_DECLARE_DERIVED_EXCEPTION(need_buffer_and_brainkey_exception, wallet_exception, fc::wallet_exception_base_code + need_buffer_and_brainkey_code, "You need buffer and brainkey to sign.")
   FC_DECLARE_DERIVED_EXCEPTION(need_buffer_pubkey_and_signature_exception, wallet_exception, fc::wallet_exception_base_code + need_buffer_pubkey_and_signature_code, "You need buffer, public key and signature to verify.")
   // wallet_filename
   FC_DECLARE_DERIVED_EXCEPTION(wallet_filename_cannot_be_empty_exception, wallet_exception, fc::wallet_exception_base_code + wallet_filename_cannot_be_empty_code, "Wallet filename cannot be empty.")
   FC_DECLARE_DERIVED_EXCEPTION(wallet_is_locked_exception, wallet_exception, fc::wallet_exception_base_code + wallet_is_locked_code, "The wallet is locked and needs to be unlocked.")
   FC_DECLARE_DERIVED_EXCEPTION(wallet_is_already_locked_exception, wallet_exception, fc::wallet_exception_base_code + wallet_is_already_locked_code, "The wallet is already locked.")
   //FC_DECLARE_DERIVED_EXCEPTION(wallet_is_already_unlocked_exception, wallet_exception, fc::wallet_exception_base_code + wallet_is_already_unlocked_code, "The wallet is already unlocked.")
   FC_DECLARE_DERIVED_EXCEPTION(password_cannot_be_empty_exception, wallet_exception, fc::wallet_exception_base_code + password_cannot_be_empty_code, "Password cannot be empty.")
   //account
   FC_DECLARE_DERIVED_EXCEPTION(account_already_exist_exception, wallet_exception, fc::wallet_exception_base_code + account_already_exist_code, "Account already exists.");
   FC_DECLARE_DERIVED_EXCEPTION(invalid_wif_private_key_exception, wallet_exception, fc::wallet_exception_base_code + invalid_wif_private_key_code, "Invalid wif private key.");
   FC_DECLARE_DERIVED_EXCEPTION(new_auth_needs_to_be_different_from_existing_exception, wallet_exception, fc::wallet_exception_base_code + new_auth_needs_to_be_different_from_existing_code, "New authority needs to be different from the existing one.")
   // asset
   FC_DECLARE_DERIVED_EXCEPTION(asset_not_found_exception, wallet_exception, fc::wallet_exception_base_code + asset_not_found_code, "Asset not found.")
   FC_DECLARE_DERIVED_EXCEPTION(asset_not_monitored_exception, wallet_exception, fc::wallet_exception_base_code + asset_not_monitored_code, "Asset not found.")
   FC_DECLARE_DERIVED_EXCEPTION(asset_already_exists_exception, wallet_exception, fc::wallet_exception_base_code + asset_already_exists_code, "Asset already exists.")
   // transaction builder
   FC_DECLARE_DERIVED_EXCEPTION(invalid_transaction_handle_exception, wallet_exception, fc::wallet_exception_base_code + invalid_transaction_handle_code, "Invalid transaction handle.")
   FC_DECLARE_DERIVED_EXCEPTION(fees_can_be_paid_in_core_asset_exception, wallet_exception, fc::wallet_exception_base_code + fees_can_be_paid_in_core_asset_code, "Fees can be paid in core asset.");
   FC_DECLARE_DERIVED_EXCEPTION(unsupported_operation_exception, wallet_exception, fc::wallet_exception_base_code + unsupported_operation_code, "Unsupported operation.");
   // mining
   FC_DECLARE_DERIVED_EXCEPTION(no_miner_is_registered_for_this_owner_id_exception, wallet_exception, fc::wallet_exception_base_code + no_miner_is_registered_for_this_owner_id_code, "No miner is registered for this owner id.");
   FC_DECLARE_DERIVED_EXCEPTION(owner_account_is_already_a_miner_exception, wallet_exception, fc::wallet_exception_base_code + owner_account_is_already_a_miner_code, "Owner account is already a miner.");
   FC_DECLARE_DERIVED_EXCEPTION(account_is_not_registered_as_miner_exception, wallet_exception, fc::wallet_exception_base_code + account_is_not_registered_as_miner_code, "Account is not registered as miner.");
   FC_DECLARE_DERIVED_EXCEPTION(account_was_already_voting_for_miner_exception, wallet_exception, fc::wallet_exception_base_code + account_was_already_voting_for_miner_code, "Account was already voting for miner.");
   FC_DECLARE_DERIVED_EXCEPTION(account_is_already_not_voting_miner_exception, wallet_exception, fc::wallet_exception_base_code + account_is_already_not_voting_for_miner_code, "Account is already not voting for miner.");
   FC_DECLARE_DERIVED_EXCEPTION(voting_proxy_is_already_set_to_voter_exception, wallet_exception, fc::wallet_exception_base_code + voting_proxy_is_already_set_to_voter_code, "Voting proxy is already set to voter.");
   FC_DECLARE_DERIVED_EXCEPTION(account_was_already_voting_for_itself_exception, wallet_exception, fc::wallet_exception_base_code + account_was_already_voting_for_itself_code, "Account was already voting for itself.");
   FC_DECLARE_DERIVED_EXCEPTION(account_was_already_voting_for_miners_exception, wallet_exception, fc::wallet_exception_base_code + account_was_already_voting_for_miners_code, "Account was already voting for miners.");   
   // content
   FC_DECLARE_DERIVED_EXCEPTION(cannot_find_package_exception, wallet_exception, fc::wallet_exception_base_code + cannot_find_package_code, "Cannot find package.");
   FC_DECLARE_DERIVED_EXCEPTION(package_is_not_in_valid_state_exception, wallet_exception, fc::wallet_exception_base_code + package_is_not_in_valid_state_code, "Package is not in valid state.");
   FC_DECLARE_DERIVED_EXCEPTION(no_such_content_at_this_url_exception, wallet_exception, fc::wallet_exception_base_code + no_such_content_at_this_url_code, "No such content at this URL.");
   FC_DECLARE_DERIVED_EXCEPTION(duplicity_at_the_list_of_coauthors_not_allowed_exception, wallet_exception, fc::wallet_exception_base_code + duplicity_at_the_list_of_coauthors_not_allowed_code, "Duplicity in the list of co-authors is not allowed.");
   FC_DECLARE_DERIVED_EXCEPTION(the_prices_of_the_content_per_region_cannot_be_empty_exception, wallet_exception, fc::wallet_exception_base_code + the_prices_of_the_content_per_region_cannot_be_empty_code, "Parameter: the prices of the content per regions cannot be empty.");
   FC_DECLARE_DERIVED_EXCEPTION(invalid_content_expiration_exception, wallet_exception, fc::wallet_exception_base_code + invalid_content_expiration_code, "Invalid content expiration.");
   FC_DECLARE_DERIVED_EXCEPTION(invalid_content_uri_exception, wallet_exception, fc::wallet_exception_base_code + invalid_content_uri_code, "Invalid content URI.");
   FC_DECLARE_DERIVED_EXCEPTION(content_not_available_for_this_region_exception, wallet_exception, fc::wallet_exception_base_code + content_not_available_for_this_region_code, "Content not available for this region.");
   FC_DECLARE_DERIVED_EXCEPTION(cannot_find_download_object_exception, wallet_exception, fc::wallet_exception_base_code + cannot_find_download_object_code, "Cannot find download object.");
   // suscription
   FC_DECLARE_DERIVED_EXCEPTION(could_not_find_matching_subcription_exception, wallet_exception, fc::wallet_exception_base_code + could_not_find_matching_subscription_code, "Could not find matching subscription.");


}}