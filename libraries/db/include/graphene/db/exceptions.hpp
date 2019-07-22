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

namespace graphene { namespace db {


   enum db_exception_code {
      
      invalid_space_id_code                     = 1,
      invalid_type_id_code                      = 2,
      object_not_found_code                     = 3, 
      account_does_not_exist_code               = 4,
      block_not_found_code                      = 5,
      block_does_not_contain_requested_trx_code = 6,
      limit_exceeded_code                       = 7,
      buying_object_does_not_exist_code         = 8,
      content_object_does_not_exist_code        = 9,
      decryption_of_key_particle_failed_code    = 10,
      seeder_not_found_code                     = 11,
      
   };

   FC_DECLARE_EXCEPTION(db_exception, fc::db_exception_base_code, "database exception")

   
   FC_DECLARE_DERIVED_EXCEPTION(invalid_space_id_exception, db_exception, fc::db_exception_base_code + invalid_space_id_code, "Invalid space id in object identifier.")
   FC_DECLARE_DERIVED_EXCEPTION(invalid_type_id_exception, db_exception, fc::db_exception_base_code + invalid_type_id_code, "Invalid type id in object identifier.")
   FC_DECLARE_DERIVED_EXCEPTION(object_not_found_exception, db_exception, fc::db_exception_base_code + object_not_found_code, "Object not found in database.")
   FC_DECLARE_DERIVED_EXCEPTION(account_does_not_exist_exception, db_exception, fc::db_exception_base_code + account_does_not_exist_code, "Account does not exist.")
   FC_DECLARE_DERIVED_EXCEPTION(block_not_found_exception, db_exception, fc::db_exception_base_code + block_not_found_code, "Block not found.")
   FC_DECLARE_DERIVED_EXCEPTION(block_does_not_contain_requested_trx_exception, db_exception, fc::db_exception_base_code + block_does_not_contain_requested_trx_code, "Block does not contain requested transaction.");
   FC_DECLARE_DERIVED_EXCEPTION(limit_exceeded_exception, db_exception, fc::db_exception_base_code + limit_exceeded_code, "Limit exceeded.");
   FC_DECLARE_DERIVED_EXCEPTION(buying_object_does_not_exist_exception, db_exception, fc::db_exception_base_code + buying_object_does_not_exist_code, "Buying object does not exist.");
   FC_DECLARE_DERIVED_EXCEPTION(content_object_does_not_exist_exception, db_exception, fc::db_exception_base_code + content_object_does_not_exist_code, "Content object does not exist.");
   FC_DECLARE_DERIVED_EXCEPTION(decryption_of_key_particle_failed_exception, db_exception, fc::db_exception_base_code + decryption_of_key_particle_failed_code, "Decryption of key particle failed.");
   FC_DECLARE_DERIVED_EXCEPTION(seeder_not_found_exception, db_exception, fc::db_exception_base_code + seeder_not_found_code, "Seeder not found.");

} } // graphene::db
