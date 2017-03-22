
#pragma once

#include <memory>
#include <string>

namespace graphene
{
namespace wallet
{
    class wallet_api;
}
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
      graphene::wallet::wallet_api* operator -> ();
      string RunTask(string& str_command);

   private:

      std::unique_ptr<detail::WalletAPIHelper> m_pimpl;
   };
}
}
