/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */

#pragma once

#include <mutex>
#include <graphene/wallet/wallet.hpp>

namespace graphene
{
namespace wallet
{
    class wallet_api;
}
namespace chain
{
   struct content_summary;
}
}
namespace fc
{
   class thread;
}
namespace decent
{
namespace wallet_utility
{
   using string = std::string;

   namespace detail
   {
      class WalletAPIHelper;
   }
   class WalletAPI
   {
   public:
      WalletAPI(const fc::path &wallet_file, const graphene::wallet::server_data &ws);
      ~WalletAPI();

      void Connect(std::atomic_bool& cancellation_token);
      bool IsConnected();
      bool IsNew();
      bool IsLocked();
      std::chrono::system_clock::time_point HeadBlockTime();
      void SetPassword(string const& str_password);
      void Unlock(string const& str_password);
      void SaveWalletFile();
      //std::vector<graphene::chain::content_summary> SearchContent(string const& str_term, uint32_t iCount);

      string RunTask(string const& str_command);

   private:
      fc::path m_wallet_file;
      graphene::wallet::server_data m_ws;
      // wallet_api does not like to be accessed from several threads
      // so all the access is encapsulated inside m_pthread :(
      std::unique_ptr<fc::thread> m_pthread;
      std::unique_ptr<detail::WalletAPIHelper> m_pimpl;
      std::mutex m_mutex;
   };
}
}
