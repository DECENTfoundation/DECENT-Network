#pragma once
#define STDAFX_APP_H

#include <iostream>
#include <cfenv>
#include <memory>

#include <boost/algorithm/string.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/program_options.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/range/algorithm/reverse.hpp>
#include <boost/range/iterator_range.hpp>

#include <fc/bloom_filter.hpp>
#include <fc/crypto/elliptic.hpp>
#include <fc/crypto/hex.hpp>
#include <fc/io/fstream.hpp>
#include <fc/network/resolve.hpp>
#include <fc/rpc/websocket_api.hpp>

#include <graphene/chain/database.hpp>
#include <graphene/chain/get_config.hpp>
#include <graphene/chain/protocol/asset.hpp>
#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/content_object.hpp>
#include <graphene/chain/message_object.hpp>
#include <graphene/chain/transaction_object.hpp>
#include <graphene/chain/transaction_detail_object.hpp>
#include <graphene/chain/transaction_history_object.hpp>
#include <graphene/chain/operation_history_object.hpp>
#include <graphene/chain/chain_property_object.hpp>
#include <graphene/chain/proposal_object.hpp>
#include <graphene/chain/buying_object.hpp>
#include <graphene/chain/miner_object.hpp>
#include <graphene/chain/seeder_object.hpp>
#include <graphene/chain/subscription_object.hpp>
#include <graphene/chain/vesting_balance_object.hpp>
#include <graphene/chain/custom_evaluator.hpp>
#include <graphene/egenesis/egenesis.hpp>
#include <graphene/net/exceptions.hpp>
#include <graphene/net/node.hpp>
#include <graphene/time/time.hpp>
#include <graphene/utilities/dirhelper.hpp>
#include <graphene/utilities/key_conversion.hpp>

#include <decent/encrypt/encryptionutils.hpp>
#include <decent/ipfs_check.hpp>
#include <decent/monitoring/monitoring_fc.hpp>
#include <decent/package/package_config.hpp>

#include "json.hpp"
