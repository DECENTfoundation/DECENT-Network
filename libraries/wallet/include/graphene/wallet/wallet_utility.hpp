/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */

#pragma once

#include <mutex>
#include <boost/filesystem/path.hpp>

namespace graphene
{
namespace wallet
{
    struct server_data;
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
   namespace detail
   {
      class WalletAPIHelper;
   }
   class WalletAPI
   {
   public:
      WalletAPI(const boost::filesystem::path &wallet_file);
      ~WalletAPI();

      void Connect(std::atomic_bool& cancellation_token, const graphene::wallet::server_data &ws);
      bool IsConnected();
      bool IsNew();
      bool IsLocked();
      std::chrono::system_clock::time_point HeadBlockTime();
      void SetPassword(std::string const& str_password);
      void Unlock(std::string const& str_password);
      void SaveWalletFile();
      bool IsPackageManagerTaskWaiting();
      //std::vector<graphene::chain::content_summary> SearchContent(string const& str_term, uint32_t iCount);

      std::string RunTask(std::string const& str_command);

   private:
      boost::filesystem::path m_wallet_file;
      // wallet_api does not like to be accessed from several threads
      // so all the access is encapsulated inside m_pthread :(
      std::unique_ptr<fc::thread> m_pthread;
      std::unique_ptr<detail::WalletAPIHelper> m_pimpl;
      std::mutex m_mutex;
   };
}
}
