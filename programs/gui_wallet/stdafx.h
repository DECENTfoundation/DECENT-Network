#pragma once
#define STDAFX_H

#include <QtWidgets/QtWidgets>

#include <iostream>

#include <nlohmann/json.hpp>

#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <boost/program_options.hpp>

#include <fc/log/file_appender.hpp>
#include <fc/interprocess/signals.hpp>
#include <fc/thread/thread.hpp>

#include <graphene/account_history/account_history_plugin.hpp>
#include <graphene/transaction_history/transaction_history_plugin.hpp>
#include <graphene/chain/content_object.hpp>
#include <graphene/chain/transaction_detail_object.hpp>
#include <graphene/messaging/messaging.hpp>
#include <graphene/miner/miner.hpp>
#include <graphene/seeding/seeding.hpp>
#include <graphene/utilities/dirhelper.hpp>
#include <graphene/utilities/git_revision.hpp>
#include <graphene/utilities/key_conversion.hpp>
#include <graphene/utilities/keys_generator.hpp>

#include <decent/about.hpp>
#include <decent/decent_config.hpp>
#include <decent/wallet_utility/wallet_utility.hpp>

#ifdef _MSC_VER
#include <windows.h>
#endif
