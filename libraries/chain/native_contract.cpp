#include <graphene/chain/native_contract.hpp>
#include <graphene/chain/contract_evaluate.hpp>
#include <graphene/utilities/ordered_json.hpp>

#include <boost/algorithm/string.hpp>

namespace graphene {
	namespace chain {
		void abstract_native_contract::set_contract_storage(const address& contract_address, const string& storage_name, const StorageDataType& value)
		{
			if (_contract_invoke_result.storage_changes.find(string(contract_address)) == _contract_invoke_result.storage_changes.end())
			{
				_contract_invoke_result.storage_changes[string(contract_address)] = contract_storage_changes_type();
			}
			auto& storage_changes = _contract_invoke_result.storage_changes[string(contract_address)];
			if (storage_changes.find(storage_name) == storage_changes.end())
			{
				StorageDataChangeType change;
				change.after = value;
				const auto &before = _evaluate->get_storage(string(contract_address), storage_name);
				jsondiff::JsonDiff differ;
				auto before_json_str = before.as<string>();
				auto after_json_str = change.after.as<string>();
				auto diff = differ.diff_by_string(before_json_str, after_json_str);
				change.storage_diff = graphene::utilities::json_ordered_dumps(diff->value());
				change.before = before;
				storage_changes[storage_name] = change;
			}
			else
			{
				auto& change = storage_changes[storage_name];
				auto before = change.before;
				auto after = value;
				change.after = after;
				jsondiff::JsonDiff differ;
				auto before_json_str = before.as<string>();
				auto after_json_str = after.as<string>();
				auto diff = differ.diff_by_string(before_json_str, after_json_str);
				change.storage_diff = graphene::utilities::json_ordered_dumps(diff->value());
			}
		}
		StorageDataType abstract_native_contract::get_contract_storage(const address& contract_address, const string& storage_name)
		{
			if (_contract_invoke_result.storage_changes.find(contract_address.operator fc::string()) == _contract_invoke_result.storage_changes.end())
			{
				return _evaluate->get_storage(contract_address.operator fc::string(), storage_name);
			}
			auto& storage_changes = _contract_invoke_result.storage_changes[contract_address.operator fc::string()];
			if (storage_changes.find(storage_name) == storage_changes.end())
			{
				return _evaluate->get_storage(contract_address.operator fc::string(), storage_name);
			}
			return storage_changes[storage_name].after;
		}

		void abstract_native_contract::emit_event(const address& contract_address, const string& event_name, const string& event_arg)
		{
			FC_ASSERT(!event_name.empty());
			contract_event_notify_info info;
            info.op_num = _evaluate->get_gen_eval()->get_trx_eval_state()->op_num;
            info.event_name = event_name;
            info.event_arg = event_arg;
            //todo
		    //info.caller_addr = caller_address->address_to_string();
            info.block_num = 1 + _evaluate->get_db().head_block_num();

			_contract_invoke_result.events.push_back(info);
		}

		bool abstract_native_contract::has_api(const string& api_name)
		{
			const auto& api_names = apis();
			return api_names.find(api_name) != api_names.end();
		}

		std::string demo_native_contract::contract_key() const
		{
			return demo_native_contract::native_contract_key();
		}
        address demo_native_contract::contract_address() const {
			return contract_id;
		}
		std::set<std::string> demo_native_contract::apis() const {
			return { "init", "hello", "contract_balance", "withdraw", "on_deposit_asset" };
		}
		std::set<std::string> demo_native_contract::offline_apis() const {
			return {};
		}
		std::set<std::string> demo_native_contract::events() const {
			return {};
		}

		contract_invoke_result demo_native_contract::invoke(const std::string& api_name, const std::string& api_arg) {
			contract_invoke_result result;
			printf("demo native contract called\n");
			printf("api %s called with arg %s\n", api_name.c_str(), api_arg.c_str());

            result.invoker = *(_evaluate->get_caller_address());
			if (api_name == "contract_balance")
			{
				auto system_asset_id = _evaluate->asset_from_string(string(GRAPHENE_SYMBOL), string("0")).asset_id;
				auto balance = _evaluate->get_contract_balance(contract_id, system_asset_id);
				result.api_result = std::to_string(balance.value);
			}
			else if (api_name == "withdraw")
			{
				auto system_asset_id = _evaluate->asset_from_string(string(GRAPHENE_SYMBOL), string("0")).asset_id;
				auto balance = _evaluate->get_contract_balance(contract_id, system_asset_id);
				if(balance.value <= 0)
					THROW_CONTRACT_ERROR("can't withdraw because of empty balance");
				_evaluate->transfer_to_address(contract_id, balance, *(_evaluate->get_caller_address()));
			}
			return result;
		}

		// token contract
		std::string token_native_contract::contract_key() const
		{
			return token_native_contract::native_contract_key();
		}
        address token_native_contract::contract_address() const {
			return contract_id;
		}
		std::set<std::string> token_native_contract::apis() const {
			return { "init", "init_token", "transfer", "transferFrom", "balanceOf", "approve", "approvedBalanceFrom", "allApprovedFromUser", "state", "supply", "precision" };
		}
		std::set<std::string> token_native_contract::offline_apis() const {
			return { "balanceOf", "approvedBalanceFrom", "allApprovedFromUser", "state", "supply", "precision" };
		}
		std::set<std::string> token_native_contract::events() const {
			return { "Inited", "Transfer", "Approved" };
		}

		static const string not_inited_state_of_token_contract = "NOT_INITED";
		static const string common_state_of_token_contract = "COMMON";

		contract_invoke_result token_native_contract::init_api(const std::string& api_name, const std::string& api_arg)
		{
			set_contract_storage(contract_id, string("name"), string("\"\""));
			set_contract_storage(contract_id, string("supply"), string("0"));
			set_contract_storage(contract_id, string("precision"), string("0"));
			set_contract_storage(contract_id, string("users"), string("{}"));
			set_contract_storage(contract_id, string("allowed"), string("{}"));
			set_contract_storage(contract_id, string("state"), string("\"") + not_inited_state_of_token_contract + "\"");
			auto caller_addr = _evaluate->get_caller_address();
			FC_ASSERT(caller_addr);
			set_contract_storage(contract_id, string("admin"), graphene::utilities::json_ordered_dumps(string(*caller_addr)));
			set_contract_storage(contract_id, string("users"), string("{}"));
			return _contract_invoke_result;
		}

		string token_native_contract::check_admin()
		{
			auto caller_addr = _evaluate->get_caller_address();
			if (!caller_addr)
				THROW_CONTRACT_ERROR("only admin can call this api");
			auto admin_storage = get_contract_storage(contract_id, string("admin"));
			auto admin = jsondiff::json_loads(admin_storage.as<string>());
			if (admin.is_string() && admin.as_string() == string(*caller_addr))
				return admin.as_string();
			THROW_CONTRACT_ERROR("only admin can call this api");
		}

		string token_native_contract::get_storage_state()
		{
			auto state_storage = get_contract_storage(contract_id, string("state"));
			auto state = jsondiff::json_loads(state_storage.as<string>());
			return state.as_string();
		}

		int64_t token_native_contract::get_storage_supply()
		{
			auto supply_storage = get_contract_storage(contract_id, string("supply"));
			auto supply = jsondiff::json_loads(supply_storage.as<string>());
			return supply.as_int64();
		}
		int64_t token_native_contract::get_storage_precision()
		{
			auto precision_storage = get_contract_storage(contract_id, string("precision"));
			auto precision = jsondiff::json_loads(precision_storage.as<string>());
			return precision.as_int64();
		}

		jsondiff::JsonObject token_native_contract::get_storage_users()
		{
			auto users_storage = get_contract_storage(contract_id, string("users"));
			auto users = jsondiff::json_loads(users_storage.as<string>());
			return users.as<jsondiff::JsonObject>();
		}

		jsondiff::JsonObject token_native_contract::get_storage_allowed()
		{
			auto allowed_storage = get_contract_storage(contract_id, string("allowed"));
			auto allowed = jsondiff::json_loads(allowed_storage.as<string>());
			return allowed.as<jsondiff::JsonObject>();
		}

		int64_t token_native_contract::get_balance_of_user(const string& owner_addr)
		{
			const auto& users = get_storage_users();
			if (users.find(owner_addr) == users.end())
				return 0;
			return users[owner_addr].as_int64();
		}

		std::string token_native_contract::get_from_address()
		{
			return string(*(_evaluate->get_caller_address())); // FIXME: when get from_address, caller maybe other contract
		}

		static bool is_numeric(std::string number)
		{
			char* end = 0;
			std::strtod(number.c_str(), &end);

			return end != 0 && *end == 0;
		}


		static bool is_integral(std::string number)
		{
			return is_numeric(number.c_str()) && std::strchr(number.c_str(), '.') == 0;
		}

		// arg format: name,symbol,supply,precision
		contract_invoke_result token_native_contract::init_token_api(const std::string& api_name, const std::string& api_arg)
		{
			check_admin();
			if(get_storage_state()!= not_inited_state_of_token_contract)
				THROW_CONTRACT_ERROR("this token contract inited before");
			std::vector<string> parsed_args;
			boost::split(parsed_args, api_arg, boost::is_any_of(","));
			if (parsed_args.size() < 3)
				THROW_CONTRACT_ERROR("argument format error, need format: name,supply,precision");
			string name = parsed_args[0];
			boost::trim(name);
			string supply_str = parsed_args[1];
			if (!is_integral(supply_str))
				THROW_CONTRACT_ERROR("argument format error, need format: name,supply,precision");
			int64_t supply = std::stoll(supply_str);
			if(supply <= 0)
				THROW_CONTRACT_ERROR("argument format error, supply must be positive integer");
			string precision_str = parsed_args[2];
			if(!is_integral(precision_str))
				THROW_CONTRACT_ERROR("argument format error, need format: name,supply,precision");
			int64_t precision = std::stoll(precision_str);
			if(precision <= 0)
				THROW_CONTRACT_ERROR("argument format error, precision must be positive integer");
			// allowedPrecisions = [1,10,100,1000,10000,100000,1000000,10000000,100000000]
			std::vector<int64_t> allowed_precisions = { 1,10,100,1000,10000,100000,1000000,10000000,100000000 };
			if(std::find(allowed_precisions.begin(), allowed_precisions.end(), precision) == allowed_precisions.end())
				THROW_CONTRACT_ERROR("argument format error, precision must be any one of [1,10,100,1000,10000,100000,1000000,10000000,100000000]");
			set_contract_storage(contract_id, string("state"), string("\"") + common_state_of_token_contract + "\"");
			set_contract_storage(contract_id, string("precision"), string("") + std::to_string(precision));
			set_contract_storage(contract_id, string("supply"), string("") + std::to_string(supply));
			set_contract_storage(contract_id, string("name"), string("\"") + name + "\"");

			jsondiff::JsonObject users;
			auto caller_addr = string(*(_evaluate->get_caller_address()));
			users[caller_addr] = supply;
			set_contract_storage(contract_id, string("users"), graphene::utilities::json_ordered_dumps(users));
			emit_event(contract_id, "Inited", supply_str);
			return _contract_invoke_result;
		}

		contract_invoke_result token_native_contract::balance_of_api(const std::string& api_name, const std::string& api_arg)
		{
			if (get_storage_state() != common_state_of_token_contract)
				THROW_CONTRACT_ERROR("this token contract state doesn't allow transfer");
			std::string owner_addr = api_arg;
			if(!address::is_valid(owner_addr))
				THROW_CONTRACT_ERROR("owner address is not valid address format");
			auto amount = get_balance_of_user(owner_addr);
			_contract_invoke_result.api_result = std::to_string(amount);
			return _contract_invoke_result;
		}

		contract_invoke_result token_native_contract::state_api(const std::string& api_name, const std::string& api_arg)
		{
			auto state = get_storage_state();
			_contract_invoke_result.api_result = state;
			return _contract_invoke_result;
		}


		contract_invoke_result token_native_contract::supply_api(const std::string& api_name, const std::string& api_arg)
		{
			auto supply = get_storage_supply();
			_contract_invoke_result.api_result = std::to_string(supply);
			return _contract_invoke_result;
		}
		contract_invoke_result token_native_contract::precision_api(const std::string& api_name, const std::string& api_arg)
		{
			auto precision = get_storage_precision();
			_contract_invoke_result.api_result = std::to_string(precision);
			return _contract_invoke_result;
		}

		contract_invoke_result token_native_contract::approved_balance_from_api(const std::string& api_name, const std::string& api_arg)
		{
			if (get_storage_state() != common_state_of_token_contract)
				THROW_CONTRACT_ERROR("this token contract state doesn't allow this api");
			auto allowed = get_storage_allowed();
			std::vector<string> parsed_args;
			boost::split(parsed_args, api_arg, boost::is_any_of(","));
			if (parsed_args.size() < 2)
				THROW_CONTRACT_ERROR("argument format error, need format: spenderAddress, authorizerAddress");
			string spender_address = parsed_args[0];
			boost::trim(spender_address);
			if (!address::is_valid(spender_address))
				THROW_CONTRACT_ERROR("argument format error, spender address format error");
			string authorizer_address = parsed_args[1];
			boost::trim(authorizer_address);
			if (!address::is_valid(authorizer_address))
				THROW_CONTRACT_ERROR("argument format error, authorizer address format error");
			int64_t approved_amount = 0;
			if (allowed.find(authorizer_address) != allowed.end())
			{
				jsondiff::JsonObject allowed_data = jsondiff::json_loads(allowed[authorizer_address].as_string()).as<jsondiff::JsonObject>();
				if (allowed_data.find(spender_address) != allowed_data.end())
				{
					approved_amount = allowed_data[spender_address].as_int64();
				}
			}

			_contract_invoke_result.api_result = std::to_string(approved_amount);
			return _contract_invoke_result;
		}
		contract_invoke_result token_native_contract::all_approved_from_user_api(const std::string& api_name, const std::string& api_arg)
		{
			if (get_storage_state() != common_state_of_token_contract)
				THROW_CONTRACT_ERROR("this token contract state doesn't allow this api");
			auto allowed = get_storage_allowed();
			string from_address = api_arg;
			boost::trim(from_address);
			if (!address::is_valid(from_address))
				THROW_CONTRACT_ERROR("argument format error, from address format error");
			
			jsondiff::JsonObject allowed_data;
			if (allowed.find(from_address) != allowed.end())
			{
				allowed_data = jsondiff::json_loads(allowed[from_address].as_string()).as<jsondiff::JsonObject>();
			}
			auto allowed_data_str = graphene::utilities::json_ordered_dumps(allowed_data);
			_contract_invoke_result.api_result = allowed_data_str;
			return _contract_invoke_result;
		}

		contract_invoke_result token_native_contract::transfer_api(const std::string& api_name, const std::string& api_arg)
		{
			if (get_storage_state() != common_state_of_token_contract)
				THROW_CONTRACT_ERROR("this token contract state doesn't allow transfer");
			std::vector<string> parsed_args;
			boost::split(parsed_args, api_arg, boost::is_any_of(","));
			if (parsed_args.size() < 2)
				THROW_CONTRACT_ERROR("argument format error, need format: toAddress,amount(with precision, integer)");
			string to_address = parsed_args[0];
			boost::trim(to_address);
			if(!address::is_valid(to_address))
				THROW_CONTRACT_ERROR("argument format error, to address format error");
			string amount_str = parsed_args[1];
			boost::trim(amount_str);
			if(!is_integral(amount_str))
				THROW_CONTRACT_ERROR("argument format error, amount must be positive integer");
			int64_t amount = std::stoll(amount_str);
			if(amount <= 0)
				THROW_CONTRACT_ERROR("argument format error, amount must be positive integer");
			
			string from_addr = get_from_address();
			auto users = get_storage_users();
			if(users.find(from_addr)==users.end() || users[from_addr].as_int64()<amount)
				THROW_CONTRACT_ERROR("you have not enoungh amount to transfer out");
			auto from_addr_remain = users[from_addr].as_int64() - amount;
			if (from_addr_remain > 0)
				users[from_addr] = from_addr_remain;
			else
				users.erase(from_addr);
			users[to_address] = users[to_address].as_int64() + amount;
			set_contract_storage(contract_id, string("users"), graphene::utilities::json_ordered_dumps(users));
			jsondiff::JsonObject event_arg;
			event_arg["from"] = from_addr;
			event_arg["to"] = to_address;
			event_arg["amount"] = amount;
			emit_event(contract_id, "Transfer", graphene::utilities::json_ordered_dumps(event_arg));
			return _contract_invoke_result;
		}

		contract_invoke_result token_native_contract::approve_api(const std::string& api_name, const std::string& api_arg)
		{
			if (get_storage_state() != common_state_of_token_contract)
				THROW_CONTRACT_ERROR("this token contract state doesn't allow approve");
			std::vector<string> parsed_args;
			boost::split(parsed_args, api_arg, boost::is_any_of(","));
			if (parsed_args.size() < 2)
				THROW_CONTRACT_ERROR("argument format error, need format: spenderAddress, amount(with precision, integer)");
			string spender_address = parsed_args[0];
			boost::trim(spender_address);
			if (!address::is_valid(spender_address))
				THROW_CONTRACT_ERROR("argument format error, spender address format error");
			string amount_str = parsed_args[1];
			boost::trim(amount_str);
			if (!is_integral(amount_str))
				THROW_CONTRACT_ERROR("argument format error, amount must be positive integer");
			int64_t amount = std::stoll(amount_str);
			if (amount <= 0)
				THROW_CONTRACT_ERROR("argument format error, amount must be positive integer");
			auto allowed = get_storage_allowed();
			jsondiff::JsonObject allowed_data;
			std::string contract_caller = get_from_address();
			if (allowed.find(contract_caller) == allowed.end())
				allowed_data = jsondiff::JsonObject();
			else
			{
				allowed_data = jsondiff::json_loads(allowed[contract_caller].as_string()).as<jsondiff::JsonObject>();
			}
			allowed_data[spender_address] = amount;
			allowed[contract_caller] = graphene::utilities::json_ordered_dumps(allowed_data);
			set_contract_storage(contract_id, string("allowed"), graphene::utilities::json_ordered_dumps(allowed));
			jsondiff::JsonObject event_arg;
			event_arg["from"] = contract_caller;
			event_arg["spender"] = spender_address;
			event_arg["amount"] = amount;
			emit_event(contract_id, "Approved", graphene::utilities::json_ordered_dumps(event_arg));
			return _contract_invoke_result;
		}

		contract_invoke_result token_native_contract::transfer_from_api(const std::string& api_name, const std::string& api_arg)
		{
			if (get_storage_state() != common_state_of_token_contract)
				THROW_CONTRACT_ERROR("this token contract state doesn't allow transferFrom");
			std::vector<string> parsed_args;
			boost::split(parsed_args, api_arg, boost::is_any_of(","));
			if (parsed_args.size() < 3)
				THROW_CONTRACT_ERROR("argument format error, need format:fromAddress, toAddress, amount(with precision, integer)");
			string from_address = parsed_args[0];
			boost::trim(from_address);
			if (!address::is_valid(from_address))
			{
				THROW_CONTRACT_ERROR("argument format error, from address format error");
			}
			string to_address = parsed_args[1];
			boost::trim(to_address);
			if (!address::is_valid(to_address))
				THROW_CONTRACT_ERROR("argument format error, to address format error");
			string amount_str = parsed_args[2];
			boost::trim(amount_str);
			if (!is_integral(amount_str))
				THROW_CONTRACT_ERROR("argument format error, amount must be positive integer");
			int64_t amount = std::stoll(amount_str);
			if (amount <= 0)
				THROW_CONTRACT_ERROR("argument format error, amount must be positive integer");

			auto users = get_storage_users();
			auto allowed = get_storage_allowed();
			if (get_balance_of_user(from_address) < amount)
			{
				THROW_CONTRACT_ERROR("fromAddress not have enough token to withdraw");
			}
			jsondiff::JsonObject allowed_data;
			if (allowed.find(from_address) == allowed.end())
				THROW_CONTRACT_ERROR("not enough approved amount to withdraw");
			else
			{
				allowed_data = jsondiff::json_loads(allowed[from_address].as_string()).as<jsondiff::JsonObject>();
			}
			auto contract_caller = get_from_address();
			if(allowed_data.find(contract_caller)==allowed_data.end())
				THROW_CONTRACT_ERROR("not enough approved amount to withdraw");
			auto approved_amount = allowed_data[contract_caller].as_int64();
			if(approved_amount < amount)
				THROW_CONTRACT_ERROR("not enough approved amount to withdraw");
			auto from_addr_remain = users[from_address].as_int64() - amount;
			if (from_addr_remain > 0)
				users[from_address] = from_addr_remain;
			else
				users.erase(from_address);
			users[to_address] = users[to_address].as_int64() + amount;
			set_contract_storage(contract_id, string("users"), graphene::utilities::json_ordered_dumps(users));
							
			allowed_data[contract_caller] = approved_amount - amount;
			if (allowed_data[contract_caller].as_int64() == 0)
				allowed_data.erase(contract_caller);
			allowed[from_address] = graphene::utilities::json_ordered_dumps(allowed_data);
			set_contract_storage(contract_id, string("allowed"), graphene::utilities::json_ordered_dumps(allowed));
									
			jsondiff::JsonObject event_arg;
			event_arg["from"] = from_address;
			event_arg["to"] = to_address;
			event_arg["amount"] = amount;
			emit_event(contract_id, "Transfer", graphene::utilities::json_ordered_dumps(event_arg));
			
			return _contract_invoke_result;
		}

		contract_invoke_result token_native_contract::invoke(const std::string& api_name, const std::string& api_arg) {
			std::map<std::string, std::function<contract_invoke_result(const std::string&, const std::string&)>> apis = {
				{"init", std::bind(&token_native_contract::init_api, this, std::placeholders::_1, std::placeholders::_2)},
				{"init_token", std::bind(&token_native_contract::init_token_api, this, std::placeholders::_1, std::placeholders::_2)},
				{"transfer", std::bind(&token_native_contract::transfer_api, this, std::placeholders::_1, std::placeholders::_2)},
				{"transferFrom", std::bind(&token_native_contract::transfer_from_api, this, std::placeholders::_1, std::placeholders::_2)},
				{"balanceOf", std::bind(&token_native_contract::balance_of_api, this, std::placeholders::_1, std::placeholders::_2)},
				{"approve", std::bind(&token_native_contract::approve_api, this, std::placeholders::_1, std::placeholders::_2)},
				{"approvedBalanceFrom", std::bind(&token_native_contract::approved_balance_from_api, this, std::placeholders::_1, std::placeholders::_2)},
				{"allApprovedFromUser", std::bind(&token_native_contract::all_approved_from_user_api, this, std::placeholders::_1, std::placeholders::_2)},
				{"state", std::bind(&token_native_contract::state_api, this, std::placeholders::_1, std::placeholders::_2)},
				{"supply", std::bind(&token_native_contract::supply_api, this, std::placeholders::_1, std::placeholders::_2)},
				{"precision", std::bind(&token_native_contract::precision_api, this, std::placeholders::_1, std::placeholders::_2)}
			};
            if (apis.find(api_name) != apis.end())
            {
                contract_invoke_result res= apis[api_name](api_name, api_arg);
                res.invoker = *(_evaluate->get_caller_address());
                return res;
            }
			THROW_CONTRACT_ERROR("token api not found");
		}

		bool native_contract_finder::has_native_contract_with_key(const std::string& key)
		{
			std::vector<std::string> native_contract_keys = {
				// demo_native_contract::native_contract_key(),
				token_native_contract::native_contract_key()
			};
			return std::find(native_contract_keys.begin(), native_contract_keys.end(), key) != native_contract_keys.end();
		}
		shared_ptr<abstract_native_contract> native_contract_finder::create_native_contract_by_key(contract_common_evaluate* evaluate, const std::string& key, const address& contract_address)
		{
			/*if (key == demo_native_contract::native_contract_key())
			{
				return std::make_shared<demo_native_contract>(evaluate, contract_address);
			}
			else */
			if (key == token_native_contract::native_contract_key())
			{
				return std::make_shared<token_native_contract>(evaluate, contract_address);
			}
			else
			{
				return nullptr;
			}
		}

		void            native_contract_register_operation::validate()const
		{
			FC_ASSERT(init_cost > 0 && init_cost <= BLOCKLINK_MAX_GAS_LIMIT);
			// FC_ASSERT(fee.amount == 0 & fee.asset_id == asset_id_type(0));
			FC_ASSERT(gas_price >= BLOCKLINK_MIN_GAS_PRICE);
			FC_ASSERT(contract_id == calculate_contract_id());
			FC_ASSERT(native_contract_finder::has_native_contract_with_key(native_contract_key));
			FC_ASSERT(contract_id.version == addressVersion::CONTRACT);
		}
		share_type      native_contract_register_operation::calculate_fee(const fee_parameters_type& schedule)const
		{
			// base fee
			share_type core_fee_required = schedule.fee;
			core_fee_required += calculate_data_fee(100, schedule.price_per_kbyte); // native contract base fee
            core_fee_required += count_gas_fee(gas_price, init_cost);
			return core_fee_required;
		}
        address native_contract_register_operation::calculate_contract_id() const
		{
            address id;
			fc::sha512::encoder enc;
			std::pair<address, fc::time_point> info_to_digest(owner_addr, register_time);
			fc::raw::pack(enc, info_to_digest);
			id.addr = fc::ripemd160::hash(enc.result());
			return id;
		}
	}
}