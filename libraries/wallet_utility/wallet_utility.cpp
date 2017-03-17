#include <decent/wallet_utility/wallet_utility.hpp>
#include <graphene/utilities/dirhelper.hpp>
#include <fc/rpc/websocket_api.hpp>
#include <fc/network/http/websocket.hpp>
#include <fc/filesystem.hpp>

namespace decent
{
namespace wallet_utility
{
    using wallet_data = graphene::wallet::wallet_data;
    using websocket_client = fc::http::websocket_client;
    using websocket_client_ptr = std::shared_ptr<websocket_client>;
    using websocket_connection_ptr = fc::http::websocket_connection_ptr;
    using websocket_api_connection = fc::rpc::websocket_api_connection;
    using websocket_api_connection_ptr = std::shared_ptr<websocket_api_connection>;
    
    wallet_api_ptr create_wallet_api()
    {
        wallet_api_ptr ptr_wallet_api;
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
        ptr_wallet_api.reset(new wallet_api(wdata, remote_api),
                                      [ptr_api_connection](wallet_api* &p_wallet_api) mutable
                                      {
                                          delete p_wallet_api;
                                          p_wallet_api = nullptr;
                                          ptr_api_connection.reset();
                                      });
        
        ptr_wallet_api->set_wallet_filename(wallet_file.generic_string());
        ptr_wallet_api->load_wallet_file();
        
        return ptr_wallet_api;
    }
}
}
