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

namespace graphene { namespace db {

   enum db_exception_code
   {
      invalid_space_id_code                     = 1,
      invalid_type_id_code                      = 2,
      object_not_found_code                     = 3,
      invalid_index_code                        = 4
   };

   FC_DECLARE_EXCEPTION(db_exception, 100, "database exception")

#define FC_DECLARE_DB_EXCEPTION(TYPE, OFFSET, WHAT) \
   FC_DECLARE_DERIVED_EXCEPTION(TYPE, db_exception, OFFSET, WHAT)

   FC_DECLARE_DB_EXCEPTION(invalid_space_id_exception, invalid_space_id_code, "Invalid space id in object identifier.")
   FC_DECLARE_DB_EXCEPTION(invalid_type_id_exception, invalid_type_id_code, "Invalid type id in object identifier.")
   FC_DECLARE_DB_EXCEPTION(object_not_found_exception, object_not_found_code, "Object not found in database.")
   FC_DECLARE_DB_EXCEPTION(invalid_index_exception, invalid_index_code, "Invalid database index type.")
 
} } // graphene::db
