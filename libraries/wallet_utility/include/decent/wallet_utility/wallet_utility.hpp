
#pragma once

#include <memory>
#include <string>
#include <mutex>
#include <vector>

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
      WalletAPI();
      ~WalletAPI();

      void Connent();
      bool Connected();
      bool IsNew();
      bool IsLocked();
      void SetPassword(string const& str_password);
      void Unlock(string const& str_password);
      void SaveWalletFile();
      std::vector<graphene::chain::content_summary> SearchContent(string const& str_term, uint32_t iCount);

      string RunTaskImpl(string const& str_command);

   private:
      // wallet_api does not like to be accessed from several threads
      // so all the access is encapsulated inside m_pthread :(
      std::unique_ptr<fc::thread> m_pthread;
      std::unique_ptr<detail::WalletAPIHelper> m_pimpl;
      std::mutex m_mutex;
   };
}
}
