/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */

#pragma once

#include <atomic>
#include <mutex>
#include <fc/thread/thread.hpp>

namespace boost { namespace filesystem { class path; } }

namespace fc
{
   class thread;
}

namespace graphene { namespace wallet {
   struct server_data;
   class wallet_api;

   class WalletAPI
   {
   public:
      WalletAPI();
      ~WalletAPI();

      void Connect(std::atomic_bool& cancellation_token, const boost::filesystem::path &wallet_file, const graphene::wallet::server_data &ws);
      std::string RunTask(std::string const& str_command);

      bool is_connected() { std::lock_guard<std::mutex> lock(m_mutex); return m_pimpl != nullptr; }

      template<typename Result, typename ...Args, typename ...Values>
      Result exec(Result (wallet_api::* func)(Args...), Values... values)
      {
         std::lock_guard<std::mutex> lock(m_mutex);
         fc::future<Result> f = m_pthread->async([&]() -> Result { return (get_api().get()->*func)(values...); });
         return f.wait();
      }

      template<typename Result, typename ...Args, typename ...Values>
      Result exec(Result (wallet_api::* func)(Args...) const, Values... values)
      {
         std::lock_guard<std::mutex> lock(m_mutex);
         fc::future<Result> f = m_pthread->async([&]() -> Result { return (get_api().get()->*func)(values...); });
         return f.wait();
      }

   private:
      std::shared_ptr<wallet_api> get_api();

      // wallet_api does not like to be accessed from several threads
      // so all the access is encapsulated inside m_pthread :(
      std::mutex m_mutex;
      std::unique_ptr<fc::thread> m_pthread;

      struct Impl;
      std::unique_ptr<Impl> m_pimpl;
   };

} }
