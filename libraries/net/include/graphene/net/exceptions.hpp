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

namespace graphene { namespace net {

   enum net_exception_code
   {
      already_connected_to_requested_peer_code  = 1,
      block_older_than_undo_history_code        = 2,
      peer_is_on_an_unreachable_fork_code       = 3,
      unlinkable_block_resync_peer_code         = 4
   };

   FC_DECLARE_EXCEPTION(net_exception, 300, "P2P Networking Exception")

#define FC_DECLARE_NET_EXCEPTION(TYPE, OFFSET, WHAT) \
   FC_DECLARE_DERIVED_EXCEPTION(TYPE, net_exception, OFFSET, WHAT)

   FC_DECLARE_NET_EXCEPTION(already_connected_to_requested_peer_exception, already_connected_to_requested_peer_code, "Already connected to requested peer.")
   FC_DECLARE_NET_EXCEPTION(block_older_than_undo_history_exception, block_older_than_undo_history_code, "Block is older than our undo history allows us to process.")
   FC_DECLARE_NET_EXCEPTION(peer_is_on_an_unreachable_fork_exception, peer_is_on_an_unreachable_fork_code, "Peer is on another fork.")
   FC_DECLARE_NET_EXCEPTION(unlinkable_block_resync_peer_exception, unlinkable_block_resync_peer_code, "Need of resync with peer due to unlinkable block.")

} } // graphene::net
