
#pragma once

#include <memory>
#include <graphene/wallet/wallet.hpp>

namespace decent
{
namespace wallet_utility
{
    using wallet_api = graphene::wallet::wallet_api;
    using wallet_api_ptr = std::shared_ptr<wallet_api>;
    
    wallet_api_ptr create_wallet_api();
}
}
