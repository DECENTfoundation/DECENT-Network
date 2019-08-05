/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */

#pragma once

#include <mutex>

namespace boost { namespace filesystem { class path; } }

namespace fc
{
   class thread;
}

namespace graphene { namespace wallet {
   struct server_data;

   class WalletAPI
   {
   public:
      WalletAPI();
      ~WalletAPI();

      void Connect(std::atomic_bool& cancellation_token, const boost::filesystem::path &wallet_file, const graphene::wallet::server_data &ws);
      bool IsConnected();
      bool IsNew();
      bool IsLocked();
      std::chrono::system_clock::time_point HeadBlockTime();
      void SetPassword(std::string const& str_password);
      bool Unlock(std::string const& str_password);
      void SaveWalletFile();
      bool IsPackageManagerTaskWaiting();

      std::string RunTask(std::string const& str_command);

   private:
      // wallet_api does not like to be accessed from several threads
      // so all the access is encapsulated inside m_pthread :(
      std::mutex m_mutex;
      std::unique_ptr<fc::thread> m_pthread;

      struct Impl;
      std::unique_ptr<Impl> m_pimpl;
   };

} }
