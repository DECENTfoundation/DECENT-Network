#include <decent/wallet_utility/wallet_utility.hpp>
#include <graphene/utilities/dirhelper.hpp>
#include <fc/rpc/websocket_api.hpp>
#include <fc/network/http/websocket.hpp>
#include <fc/filesystem.hpp>
#include <graphene/wallet/wallet.hpp>
#include <fc/rpc/api_connection.hpp>

namespace decent
{
namespace wallet_utility
{
   using wallet_api = graphene::wallet::wallet_api;
   using wallet_api_ptr = std::shared_ptr<wallet_api>;
   using wallet_data = graphene::wallet::wallet_data;
   using websocket_client = fc::http::websocket_client;
   using websocket_client_ptr = std::shared_ptr<websocket_client>;
   using websocket_connection_ptr = fc::http::websocket_connection_ptr;
   using websocket_api_connection = fc::rpc::websocket_api_connection;
   using websocket_api_connection_ptr = std::shared_ptr<websocket_api_connection>;
   using fc_api = fc::api<wallet_api>;
   using fc_api_ptr = std::shared_ptr<fc_api>;

   namespace
   {
      class wallet_exception : public std::exception
      {
      public:
         wallet_exception(std::string const& str_info) noexcept : m_str_info(str_info) {}
         virtual ~wallet_exception() {}

         char const* what() const noexcept override
         {
            return m_str_info.c_str();
         }
      private:
         std::string m_str_info;
      };

   }

   namespace detail
   {
      class WalletAPIConnection : public fc::api_connection
      {  // no idea yet why deriving
      public:
         WalletAPIConnection() {}
         virtual ~WalletAPIConnection() {}

         virtual fc::variant send_call(fc::api_id_type api_id, string method_name, fc::variants args = fc::variants() ) {FC_ASSERT(false);}
         virtual fc::variant send_callback(uint64_t callback_id, fc::variants args = fc::variants() ) {FC_ASSERT(false);}
         virtual void send_notice(uint64_t callback_id, fc::variants args = fc::variants() ) {FC_ASSERT(false);}
      };

      using WalletAPIConnectionPtr = std::shared_ptr<WalletAPIConnection>;

      class WalletAPIHelper
      {
      public:
         WalletAPIHelper()
         : m_ptr_wallet_api(nullptr)
         , m_ptr_fc_api(nullptr)
         , m_ptr_fc_api_connection(nullptr)
         , m_mutex()
         {
            wallet_data wdata;
            fc::path wallet_file(decent_path_finder::instance().get_decent_home() / "wallet.json");
            if (fc::exists(wallet_file))
               wdata = fc::json::from_file(wallet_file).as<wallet_data>();
            else
               wdata.chain_id = chain_id_type("0000000000000000000000000000000000000000000000000000000000000000");

            //  probably need to open this when this usage of wallet_api is universally used
            //  most probably this needs to get out to somewhere else
            //package_manager::instance().set_libtorrent_config(wdata.libtorrent_config_path);

            websocket_client_ptr ptr_ws_client(new websocket_client());
            websocket_connection_ptr ptr_ws_connection = ptr_ws_client->connect(wdata.ws_server);

            //  capture ptr_ws_connection, own the lifetime
            websocket_api_connection_ptr ptr_api_connection =
            websocket_api_connection_ptr(new websocket_api_connection(*ptr_ws_connection),
                                         [ptr_ws_connection, ptr_ws_client](websocket_api_connection* &p_api_connection) mutable
                                         {
                                            delete p_api_connection;
                                            p_api_connection = nullptr;
                                            ptr_ws_connection.reset();
                                            ptr_ws_client.reset();
                                         });

            fc::api<graphene::app::login_api> remote_api = ptr_api_connection->get_remote_api<graphene::app::login_api>(1);
            remote_api->login(wdata.ws_user, wdata.ws_password);

            //  capture ptr_api_connection too. encapsulate all inside wallet_api
            m_ptr_wallet_api.reset(new wallet_api(wdata, remote_api),
                                   [ptr_api_connection](wallet_api* &p_wallet_api) mutable
                                   {
                                      delete p_wallet_api;
                                      p_wallet_api = nullptr;
                                      ptr_api_connection.reset();
                                   });

            m_ptr_wallet_api->set_wallet_filename(wallet_file.generic_string());
            m_ptr_wallet_api->load_wallet_file();

            m_ptr_fc_api = fc_api_ptr(new fc_api(m_ptr_wallet_api.get()));

            for (auto& name_formatter : m_ptr_wallet_api->get_result_formatters())
               m_result_formatters[name_formatter.first] = name_formatter.second;

            m_ptr_fc_api_connection = WalletAPIConnectionPtr(new WalletAPIConnection());
            m_ptr_fc_api_connection->register_api(*m_ptr_fc_api);
         }

         wallet_api_ptr m_ptr_wallet_api;
         fc_api_ptr m_ptr_fc_api;
         WalletAPIConnectionPtr m_ptr_fc_api_connection;
         std::mutex m_mutex;
         std::map<string, std::function<string(fc::variant,const fc::variants&)> > m_result_formatters;
      };
   }
   //
   //  WalletAPI
   //
   WalletAPI::WalletAPI()
   : m_pimpl(nullptr)
   {

   }

   WalletAPI::~WalletAPI()
   {

   }

   void WalletAPI::Connent()
   {
      if (m_pimpl)
         throw wallet_exception("already connected");

      while (true)
      {
         try
         {
            m_pimpl.reset(new detail::WalletAPIHelper());
            break;
         }
         catch(fc::exception const& ex)
         {
            std::string error = ex.what();
         }
         catch(fc::exception const& ex)
         {
            std::string error = ex.what();
         }
         catch(...)
         {

         }
      }
   }

   bool WalletAPI::Connected()
   {
      return nullptr != m_pimpl;
   }

   graphene::wallet::wallet_api* WalletAPI::operator -> ()
   {
      if (nullptr == m_pimpl)
         throw wallet_exception("not yet connected");

      std::lock_guard<std::mutex> lock(m_pimpl->m_mutex);

      return m_pimpl->m_ptr_wallet_api.get();
   }

   string WalletAPI::RunTask(string& str_command)
   {
      if (nullptr == m_pimpl)
         throw wallet_exception("not yet connected");

      std::lock_guard<std::mutex> lock(m_pimpl->m_mutex);

      std::string line = str_command;
      line += char(EOF);
      fc::variants args = fc::json::variants_from_string(line);

      if (false == args.empty())
      {
         const string& method = args.front().get_string();

         auto result = m_pimpl->m_ptr_fc_api_connection->receive_call(0, method, fc::variants(args.begin()+1, args.end()));

         auto it = m_pimpl->m_result_formatters.find(method);

         string str_result;
         if (it == m_pimpl->m_result_formatters.end())
            str_result = fc::json::to_pretty_string(result);
         else
            str_result = it->second(result, args);

         return str_result;
      }
      // may as well throw an exception?

      return string();
   }
}
}
