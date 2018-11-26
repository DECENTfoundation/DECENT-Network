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

#ifndef STDAFX_APP_H
#include <fc/reflect/reflect.hpp>
#endif

namespace graphene { namespace app {

class api_access_info
{
public:
   api_access_info() {}
   api_access_info( std::string pw_hash, std::string pw_salt, std::vector<std::string> apis)
   : password_hash_b64( pw_hash), password_salt_b64( pw_salt), allowed_apis( apis){}

   std::string password_hash_b64 = "*";
   std::string password_salt_b64 = "*";
   std::vector< std::string > allowed_apis = { "database_api",
                                               "network_broadcast_api",
                                               "history_api",
                                               "crypto_api",
                                               "messaging_api",
                                               "monitoring_api" };
};

class api_access
{
public:
   api_access() {}
   api_access( std::string username, api_access_info access)
   {
      permission_map.insert( std::make_pair(username, access));
   }

   std::map< std::string, api_access_info > permission_map = {{"*", api_access_info()}};
};

} } // graphene::app

FC_REFLECT( graphene::app::api_access_info,
    (password_hash_b64)
    (password_salt_b64)
    (allowed_apis)
   )

FC_REFLECT( graphene::app::api_access,
    (permission_map)
   )
