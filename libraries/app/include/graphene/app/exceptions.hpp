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

#include <fc/exception.hpp>

namespace graphene { namespace app {

   enum app_exception_code
   {
      database_not_available_code                     = 1,
      at_least_one_account_needs_to_be_specified_code = 2,
      malformed_private_key_code                      = 3,
      api_not_available_code                          = 4,
      database_already_used_code                      = 5,
      block_not_found_code                            = 6,
      block_does_not_contain_requested_trx_code       = 7,
      limit_exceeded_code                             = 8,
      account_does_not_exist_code                     = 9,
      buying_object_does_not_exist_code               = 10,
      content_object_does_not_exist_code              = 11,
      seeder_not_found_code                           = 12,
      decryption_of_key_particle_failed_code          = 13
   };

   FC_DECLARE_EXCEPTION(app_exception, 200, "App exception")

#define FC_DECLARE_APP_EXCEPTION(TYPE, OFFSET, WHAT) \
   FC_DECLARE_DERIVED_EXCEPTION(TYPE, app_exception, OFFSET, WHAT)

   FC_DECLARE_APP_EXCEPTION(database_not_available_exception, database_not_available_code, "Database not available.")
   FC_DECLARE_APP_EXCEPTION(at_least_one_account_needs_to_be_specified_exception, at_least_one_account_needs_to_be_specified_code, "At leas one account needs to be specified.")
   FC_DECLARE_APP_EXCEPTION(malformed_private_key_exception, malformed_private_key_code, "Malformed private_key.")
   FC_DECLARE_APP_EXCEPTION(api_not_available_exception, api_not_available_code, "API not available.")
   FC_DECLARE_APP_EXCEPTION(database_already_used_exception, database_already_used_code, "Database is already used by another process.")
   FC_DECLARE_APP_EXCEPTION(block_not_found_exception, block_not_found_code, "Block not found.")
   FC_DECLARE_APP_EXCEPTION(block_does_not_contain_requested_trx_exception, block_does_not_contain_requested_trx_code, "Block does not contain requested transaction.")
   FC_DECLARE_APP_EXCEPTION(limit_exceeded_exception, limit_exceeded_code, "Limit exceeded.")
   FC_DECLARE_APP_EXCEPTION(account_does_not_exist_exception, account_does_not_exist_code, "Account does not exist.")
   FC_DECLARE_APP_EXCEPTION(buying_object_does_not_exist_exception, buying_object_does_not_exist_code, "Buying object does not exist.")
   FC_DECLARE_APP_EXCEPTION(content_object_does_not_exist_exception, content_object_does_not_exist_code, "Content object does not exist.")
   FC_DECLARE_APP_EXCEPTION(seeder_not_found_exception, seeder_not_found_code, "Seeder not found.")
   FC_DECLARE_APP_EXCEPTION(decryption_of_key_particle_failed_exception, decryption_of_key_particle_failed_code, "Decryption of key particle failed.")

} } // graphene::app
