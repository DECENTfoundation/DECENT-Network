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
#include <graphene/chain/protocol/operations.hpp>
#include <fc/uint128.hpp>

namespace graphene { namespace chain {

template<>
uint64_t base_operation<>::calculate_data_fee( uint64_t bytes, uint64_t price_per_kbyte )
{
   auto result = (fc::uint128(bytes) * price_per_kbyte) / 1024;
   FC_ASSERT( result <= GRAPHENE_MAX_SHARE_SUPPLY );
   return result.to_uint64();
}

/**
 * @brief Used to validate operations in a polymorphic manner
 */
struct operation_validator
{
   typedef void result_type;
   template<typename T>
   void operator()( const T& v )const { v.validate(); }
};

struct operation_get_required_auth
{
   typedef void result_type;

   boost::container::flat_set<account_id_type>& active;
   boost::container::flat_set<account_id_type>& owner;
   std::vector<authority>& other;


   operation_get_required_auth( boost::container::flat_set<account_id_type>& a,
     boost::container::flat_set<account_id_type>& own,
     std::vector<authority>&  oth ):active(a),owner(own),other(oth){}

   template<typename T>
   void operator()( const T& v )const
   {
      active.insert( v.fee_payer() );
      v.get_required_active_authorities( active );
      v.get_required_owner_authorities( owner );
      v.get_required_authorities( other );
   }
};

void operation_validate( const operation& op )
{
   op.visit( operation_validator() );
}

void operation_get_required_authorities( const operation& op,
                                         boost::container::flat_set<account_id_type>& active,
                                         boost::container::flat_set<account_id_type>& owner,
                                         std::vector<authority>& other )
{
   op.visit( operation_get_required_auth( active, owner, other ) );
}

} } // namespace graphene::chain
