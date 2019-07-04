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

#define wallet_exception_base_code 150
   enum wallet_exception_code {
      account_name_or_id_cannot_be_empty_code   = 1,
      account_in_wallet_not_on_blockchain_code  = 2,

   };

   //account_does_not_exist_code           ,   // wallet exception
   //public_key_not_found_in_wallet_code   ,
   //private_key_not_imported_code         ,
    
   //FC_DECLARE_EXCEPTION(account_in_wallet_not_on_blockchain_exception, account_in_wallet_not_on_blockchain_code, "Account present in the wallet but does not exist on the blockchain.");

   FC_DECLARE_EXCEPTION(wallet_exception, wallet_exception_base_code, "database exception")

   FC_DECLARE_DERIVED_EXCEPTION(account_name_or_id_cannot_be_empty_exception, wallet_exception, wallet_exception_base_code + account_name_or_id_cannot_be_empty_code, "Account name or id cannot be empty string.")
   FC_DECLARE_DERIVED_EXCEPTION(account_in_wallet_not_on_blockchain_exception, wallet_exception, wallet_exception_base_code + account_in_wallet_not_on_blockchain_code, "Account present in the wallet but does not exist on the blockchain.")
      

}}