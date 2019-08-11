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
#include <fc/thread/thread.hpp>
#include <fc/thread/mutex.hpp>
#include <fc/thread/scoped_lock.hpp>
#include <fc/thread/future.hpp>
#include <fc/log/logger.hpp>
#include <fc/io/enum_type.hpp>

#include <graphene/net/message_oriented_connection.hpp>
#include <graphene/net/stcp_socket.hpp>
#include <graphene/net/config.hpp>

#ifdef DEFAULT_LOGGER
# undef DEFAULT_LOGGER
#endif
#define DEFAULT_LOGGER "p2p"

#ifndef NDEBUG
# define VERIFY_CORRECT_THREAD() assert(_thread->is_current())
#else
# define VERIFY_CORRECT_THREAD() do {} while (0)
#endif

namespace graphene { namespace net {
  namespace detail
  {
    class message_oriented_connection_impl
    {
    private:
      message_oriented_connection* _self;
      message_oriented_connection_delegate *_delegate;
      stcp_socket _sock;
      fc::future<void> _read_loop_done;
      size_t _max_message_size;
      uint64_t _bytes_received;
      uint64_t _bytes_sent;
      bool _send_message_in_progress;
      bool _run_loop;

      fc::time_point _connected_time;
      fc::time_point _last_message_received_time;
      fc::time_point _last_message_sent_time;

#ifndef NDEBUG
      fc::thread* _thread;
#endif

      void read_loop();
      void start_read_loop();
    public:
      fc::tcp_socket& get_socket();
      void accept();
      void connect_to(const fc::ip::endpoint& remote_endpoint);
      void set_block_size(uint32_t block_size);
      void bind(const fc::ip::endpoint& local_endpoint);

      message_oriented_connection_impl(message_oriented_connection* self,
                                       message_oriented_connection_delegate* delegate = nullptr);
      ~message_oriented_connection_impl();

      void send_message(const message& message_to_send);
      void close_connection();
      void destroy_connection();

      uint64_t get_total_bytes_sent() const;
      uint64_t get_total_bytes_received() const;

      fc::time_point get_last_message_sent_time() const;
      fc::time_point get_last_message_received_time() const;
      fc::time_point get_connection_time() const { return _connected_time; }
      fc::sha512 get_shared_secret() const;
    };

    message_oriented_connection_impl::message_oriented_connection_impl(message_oriented_connection* self,
                                                                       message_oriented_connection_delegate* delegate)
    : _self(self),
      _delegate(delegate),
      _max_message_size(0),
      _bytes_received(0),
      _bytes_sent(0),
      _send_message_in_progress(false),
      _run_loop(true)
#ifndef NDEBUG
      ,_thread(&fc::thread::current())
#endif
    {
    }
    message_oriented_connection_impl::~message_oriented_connection_impl()
    {
      VERIFY_CORRECT_THREAD();
      destroy_connection();
    }

    fc::tcp_socket& message_oriented_connection_impl::get_socket()
    {
      VERIFY_CORRECT_THREAD();
      return _sock.get_socket();
    }

    void message_oriented_connection_impl::accept()
    {
      VERIFY_CORRECT_THREAD();
      _sock.accept();
      assert(!_read_loop_done.valid()); // check to be sure we never launch two read loops
      _read_loop_done = fc::async([=](){ read_loop(); }, "message read_loop");
    }

    void message_oriented_connection_impl::connect_to(const fc::ip::endpoint& remote_endpoint)
    {
      VERIFY_CORRECT_THREAD();
      _sock.connect_to(remote_endpoint);
      assert(!_read_loop_done.valid()); // check to be sure we never launch two read loops
      _read_loop_done = fc::async([=](){ read_loop(); }, "message read_loop");
    }

    void message_oriented_connection_impl::set_block_size(uint32_t block_size)
    {
      VERIFY_CORRECT_THREAD();
      _max_message_size = block_size + sizeof(message_header);
      ilog("Set max message size: ${s}", ("s", _max_message_size));
    }

    void message_oriented_connection_impl::bind(const fc::ip::endpoint& local_endpoint)
    {
      VERIFY_CORRECT_THREAD();
      _sock.bind(local_endpoint);
    }

    void message_oriented_connection_impl::read_loop()
    {
      VERIFY_CORRECT_THREAD();
      const int BUFFER_SIZE = 16;
      const int LEFTOVER = BUFFER_SIZE - sizeof(message_header);
      static_assert(BUFFER_SIZE >= sizeof(message_header), "insufficient buffer");

      _connected_time = fc::time_point::now();

      fc::optional<fc::exception> exception_to_rethrow;
      bool call_on_connection_closed = false;
      fc::ip::endpoint rep = _sock.get_socket().remote_endpoint();
      std::string rep_addr_str = rep.operator std::string();

      try
      {
        message m;
        _run_loop = true;
        while(true)
        {
          char buffer[BUFFER_SIZE];
          _sock.read(buffer, BUFFER_SIZE);
          _bytes_received += BUFFER_SIZE;
          memcpy((char*)&m, buffer, sizeof(message_header));

          FC_ASSERT( m.size <= _max_message_size, "Max message size exceeded: ${s} <= ${m}", ("s",m.size)("m",_max_message_size) );

          size_t remaining_bytes_with_padding = 16 * ((m.size - LEFTOVER + 15) / 16);
          m.data.resize(LEFTOVER + remaining_bytes_with_padding); //give extra 16 bytes to allow for padding added in send call
          std::copy(buffer + sizeof(message_header), buffer + sizeof(buffer), m.data.begin());
          if (remaining_bytes_with_padding)
          {
            _sock.read(&m.data[LEFTOVER], remaining_bytes_with_padding);
            _bytes_received += remaining_bytes_with_padding;
          }
          m.data.resize(m.size); // truncate off the padding bytes

          _last_message_received_time = fc::time_point::now();

          try
          {
            // message handling errors are warnings...
            _delegate->on_message(_self, m);
          }
          /// Dedicated catches needed to distinguish from general fc::exception
          catch ( const fc::canceled_exception& e ) { throw e; }
          catch ( const fc::eof_exception& e ) { throw e; }
          catch ( const fc::exception& e)
          {
            /// Here loop should be continued so exception should be just caught locally.
            wlog( "message transmission failed ${er}", ("er", e.to_detail_string() ) );
            throw;
          }
        }
      }
      catch ( const fc::canceled_exception& e )
      {
        wlog( "caught a canceled_exception in read_loop.  this should mean we're in the process of deleting this object already, so there's no need to notify the delegate: ${e}", ("e", e.to_detail_string() ) );
        throw;
      }
      catch ( const fc::eof_exception& e )
      {
         call_on_connection_closed = true;
         if (_run_loop)
            dlog( "disconnected ${a} ${e}", ("a", rep_addr_str)("e", e.to_detail_string() ));
         else {
            wlog("disconnected ${a} ${e}", ("a", rep_addr_str)("e", e.to_detail_string()));
         }
      }
      catch ( const fc::exception& e )
      {
        call_on_connection_closed = true;
        if (_run_loop)
           dlog("disconnected ${a} ${e}", ("a", rep_addr_str)("e", e.to_detail_string()));
        else {
           elog("disconnected ${a} ${e}", ("a", rep_addr_str)("e", e.to_detail_string()));
           exception_to_rethrow = fc::unhandled_exception(FC_LOG_MESSAGE(warn, "disconnected: ${a} ${e}", ("a", rep_addr_str)("e", e.to_detail_string())));
        }
      }
      catch ( const std::exception& e )
      {
        call_on_connection_closed = true;
        if (_run_loop)
         dlog( "disconnected ${a} ${er}", ("a", rep_addr_str)("er", e.what() ));
        else {
           elog("disconnected ${a} ${er}", ("a", rep_addr_str)("er", e.what()));
           exception_to_rethrow = fc::unhandled_exception(FC_LOG_MESSAGE(warn, "disconnected: $a{} ${e}", ("a", rep_addr_str)("e", e.what())));
        }
      }
      catch ( ... )
      {
        elog( "unexpected exception" );
        call_on_connection_closed = true;
        exception_to_rethrow = fc::unhandled_exception(FC_LOG_MESSAGE(warn, "disconnected: ${a} ${e}", ("a", rep_addr_str)("e", fc::except_str())));
      }

      if (call_on_connection_closed || _run_loop == false)
        _delegate->on_connection_closed(_self);

      if (exception_to_rethrow)
        throw *exception_to_rethrow;
    }

    void message_oriented_connection_impl::send_message(const message& message_to_send)
    {
      VERIFY_CORRECT_THREAD();
#if 0 // this gets too verbose
#ifndef NDEBUG
      fc::optional<fc::ip::endpoint> remote_endpoint;
      if (_sock.get_socket().is_open())
        remote_endpoint = _sock.get_socket().remote_endpoint();
      struct scope_logger {
        const fc::optional<fc::ip::endpoint>& endpoint;
        scope_logger(const fc::optional<fc::ip::endpoint>& endpoint) : endpoint(endpoint) { dlog("entering message_oriented_connection::send_message() for peer ${endpoint}", ("endpoint", endpoint)); }
        ~scope_logger() { dlog("leaving message_oriented_connection::send_message() for peer ${endpoint}", ("endpoint", endpoint)); }
      } send_message_scope_logger(remote_endpoint);
#endif
#endif
      struct verify_no_send_in_progress {
        bool& var;
        verify_no_send_in_progress(bool& var) : var(var)
        {
          if (var)
            elog("Error: two tasks are calling message_oriented_connection::send_message() at the same time");
          assert(!var);
          var = true;
        }
        ~verify_no_send_in_progress() { var = false; }
      } _verify_no_send_in_progress(_send_message_in_progress);

      try
      {
        size_t size_of_message_and_header = sizeof(message_header) + message_to_send.size;
        FC_ASSERT( size_of_message_and_header <= _max_message_size,
          "Trying to send a message larger than max message size: ${s} <= ${m}", ("s",size_of_message_and_header)("m",_max_message_size));
        //pad the message we send to a multiple of 16 bytes
        size_t size_with_padding = 16 * ((size_of_message_and_header + 15) / 16);
        std::unique_ptr<char[]> padded_message(new char[size_with_padding]);
        memcpy(padded_message.get(), (char*)&message_to_send, sizeof(message_header));
        memcpy(padded_message.get() + sizeof(message_header), message_to_send.data.data(), message_to_send.size );
        _sock.write(padded_message.get(), size_with_padding);
        _sock.flush();
        _bytes_sent += size_with_padding;
        _last_message_sent_time = fc::time_point::now();
      } FC_LOG_AND_RETHROW( )
    }

    void message_oriented_connection_impl::close_connection()
    {
      VERIFY_CORRECT_THREAD();
      _run_loop = false;
      _sock.close();
    }

    void message_oriented_connection_impl::destroy_connection()
    {
      VERIFY_CORRECT_THREAD();

      _run_loop = false;
      fc::optional<fc::ip::endpoint> remote_endpoint;
      if (_sock.get_socket().is_open())
        remote_endpoint = _sock.get_socket().remote_endpoint();
      ilog( "in destroy_connection() for ${endpoint}", ("endpoint", remote_endpoint) );

      if (_send_message_in_progress)
        elog("Error: message_oriented_connection is being destroyed while a send_message is in progress.  "
             "The task calling send_message() should have been canceled already");
      assert(!_send_message_in_progress);

      try
      {
        _read_loop_done.cancel_and_wait(__FUNCTION__);
      }
      catch ( const fc::exception& e )
      {
        wlog( "Exception thrown while canceling message_oriented_connection's read_loop, ignoring: ${e}", ("e",e) );
      }
      catch (...)
      {
        wlog( "Exception thrown while canceling message_oriented_connection's read_loop, ignoring" );
      }
    }

    uint64_t message_oriented_connection_impl::get_total_bytes_sent() const
    {
      VERIFY_CORRECT_THREAD();
      return _bytes_sent;
    }

    uint64_t message_oriented_connection_impl::get_total_bytes_received() const
    {
      VERIFY_CORRECT_THREAD();
      return _bytes_received;
    }

    fc::time_point message_oriented_connection_impl::get_last_message_sent_time() const
    {
      VERIFY_CORRECT_THREAD();
      return _last_message_sent_time;
    }

    fc::time_point message_oriented_connection_impl::get_last_message_received_time() const
    {
      VERIFY_CORRECT_THREAD();
      return _last_message_received_time;
    }

    fc::sha512 message_oriented_connection_impl::get_shared_secret() const
    {
      VERIFY_CORRECT_THREAD();
      return _sock.get_shared_secret();
    }

  } // end namespace graphene::net::detail


  message_oriented_connection::message_oriented_connection(message_oriented_connection_delegate* delegate) :
    my(new detail::message_oriented_connection_impl(this, delegate))
  {
  }

  message_oriented_connection::~message_oriented_connection()
  {
  }

  fc::tcp_socket& message_oriented_connection::get_socket()
  {
    return my->get_socket();
  }

  void message_oriented_connection::accept()
  {
    my->accept();
  }

  void message_oriented_connection::connect_to(const fc::ip::endpoint& remote_endpoint)
  {
    my->connect_to(remote_endpoint);
  }

  void message_oriented_connection::bind(const fc::ip::endpoint& local_endpoint)
  {
    my->bind(local_endpoint);
  }

  void message_oriented_connection::set_block_size(uint32_t block_size)
  {
    my->set_block_size(block_size);
  }

  void message_oriented_connection::send_message(const message& message_to_send)
  {
    my->send_message(message_to_send);
  }

  void message_oriented_connection::close_connection()
  {
    my->close_connection();
  }

  void message_oriented_connection::destroy_connection()
  {
    my->destroy_connection();
  }

  uint64_t message_oriented_connection::get_total_bytes_sent() const
  {
    return my->get_total_bytes_sent();
  }

  uint64_t message_oriented_connection::get_total_bytes_received() const
  {
    return my->get_total_bytes_received();
  }

  fc::time_point message_oriented_connection::get_last_message_sent_time() const
  {
    return my->get_last_message_sent_time();
  }

  fc::time_point message_oriented_connection::get_last_message_received_time() const
  {
    return my->get_last_message_received_time();
  }
  fc::time_point message_oriented_connection::get_connection_time() const
  {
    return my->get_connection_time();
  }
  fc::sha512 message_oriented_connection::get_shared_secret() const
  {
    return my->get_shared_secret();
  }

} } // end namespace graphene::net
