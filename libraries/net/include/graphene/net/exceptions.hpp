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

namespace graphene { namespace net {
   // registered in node.cpp 

   enum net_exception_code
   {
      already_connected_to_requested_peer_code  = 1,
      block_older_than_undo_history_code        = 2,
      peer_is_on_an_unreachable_fork_code       = 3,
      unlinkable_block_resync_peer_code         = 4,
      //send_queue_overflow_code = 
      //insufficient_relay_fee_code = 
   };
   
   FC_DECLARE_EXCEPTION( net_exception, fc::net_exception_base_code, "P2P Networking Exception" );

   FC_DECLARE_DERIVED_EXCEPTION( already_connected_to_requested_peer_exception,   net_exception, fc::net_exception_base_code + already_connected_to_requested_peer_code, "Already connected to requested peer." );
   FC_DECLARE_DERIVED_EXCEPTION( block_older_than_undo_history_exception,         net_exception, fc::net_exception_base_code + block_older_than_undo_history_code, "Block is older than our undo history allows us to process." );
   FC_DECLARE_DERIVED_EXCEPTION( peer_is_on_an_unreachable_fork_exception,        net_exception, fc::net_exception_base_code + peer_is_on_an_unreachable_fork_code, "Peer is on another fork." );
   FC_DECLARE_DERIVED_EXCEPTION( unlinkable_block_resync_peer_exception,          net_exception, fc::net_exception_base_code + unlinkable_block_resync_peer_code, "Need of resync with peer due to unlinkable block.");
   //FC_DECLARE_DERIVED_EXCEPTION( send_queue_overflow_exception,                   net_exception, fc::net_exception_base_code + send_queue_overflow_code, "Send queue for this peer exceeded maximum size." );
   //FC_DECLARE_DERIVED_EXCEPTION( insufficient_relay_fee_exception,                net_exception, fc::net_exception_base_code + insufficient_relay_fee_code, "Insufficient relay fee." );
   //

} }
