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
#include <assert.h>
#include <algorithm>
#include <fc/crypto/elliptic.hpp>
#include <fc/network/ip.hpp>

#include <graphene/net/stcp_socket.hpp>

namespace graphene { namespace net {

stcp_socket::stcp_socket(const std::string& cert_file)
   : _sock(cert_file)
{
}

stcp_socket::stcp_socket(const std::string& cert_file, const std::string& key_file, const std::string& key_password)
   : _sock(cert_file, key_file, key_password)
{
}

stcp_socket::~stcp_socket()
{
}

void stcp_socket::do_key_exchange()
{
  _priv_key = fc::ecc::private_key::generate();
  fc::ecc::public_key_data s = _priv_key.get_public_key().serialize();
  _sock.writesome( s.data, sizeof(fc::ecc::public_key_data) );
  fc::ecc::public_key_data rpub;
  _sock.readsome( rpub.data, sizeof(fc::ecc::public_key_data) );
  _shared_secret = _priv_key.get_shared_secret( rpub );
  _send_aes.init( fc::sha256::hash( (char*)&_shared_secret, sizeof(_shared_secret) ),
                  fc::city_hash_crc_128((char*)&_shared_secret,sizeof(_shared_secret) ) );
  _recv_aes.init( fc::sha256::hash( (char*)&_shared_secret, sizeof(_shared_secret) ),
                  fc::city_hash_crc_128((char*)&_shared_secret,sizeof(_shared_secret) ) );
}

void stcp_socket::connect_to( const fc::ip::endpoint& remote_endpoint )
{
  _sock.connect_to( remote_endpoint );
  do_key_exchange();
}

void stcp_socket::bind( const fc::ip::endpoint& local_endpoint )
{
  _sock.bind(local_endpoint);
}

/**
 *   This method must read at least 16 bytes at a time from
 *   the underlying TCP socket so that it can decrypt them. It
 *   will buffer any left-over.
 */
size_t stcp_socket::readsome( char* buffer, size_t len )
{ try {
    assert( len > 0 && (len % 16) == 0 );

    const size_t read_buffer_length = 4096;
    if (!_read_buffer)
      _read_buffer.reset(new char[read_buffer_length]);

    len = std::min(read_buffer_length, len);
    size_t s = _sock.readsome(_read_buffer.get(), len);
    if( s % 16 )
    {
      _sock.read(_read_buffer.get() + s, 16 - (s%16));
      s += 16-(s%16);
    }

    if (_sock.uses_ssl())
      memcpy(buffer, _read_buffer.get(), std::min(s, len));
    else
      _recv_aes.decode(_read_buffer.get(), static_cast<uint32_t>(s), buffer);
    return s;
} FC_RETHROW_EXCEPTIONS( warn, "", ("len",len) ) }

bool stcp_socket::eof()const
{
  return _sock.eof();
}

size_t stcp_socket::writesome( const char* buffer, size_t len )
{ try {
    assert( len > 0 && (len % 16) == 0 );
    if (_sock.uses_ssl())
    {
      _sock.write(buffer, len);
      return len;
    }

    const std::size_t write_buffer_length = 4096;
    if (!_write_buffer)
      _write_buffer.reset(new char[write_buffer_length]);

    len = std::min<size_t>(write_buffer_length, len);
    memset(_write_buffer.get(), 0, len); // just in case aes.encode screws up
    uint32_t ciphertext_len = _send_aes.encode(buffer, static_cast<uint32_t>(len), _write_buffer.get());
    assert(ciphertext_len == len);
    _sock.write(_write_buffer.get(), ciphertext_len);
    return ciphertext_len;
} FC_RETHROW_EXCEPTIONS( warn, "", ("len",len) ) }

void stcp_socket::flush()
{
  _sock.flush();
}

void stcp_socket::close()
{
  try
  {
    _sock.close();
  }FC_RETHROW_EXCEPTIONS( warn, "error closing stcp socket" );
}

void stcp_socket::accept()
{
  do_key_exchange();
}

}} // namespace graphene::net
