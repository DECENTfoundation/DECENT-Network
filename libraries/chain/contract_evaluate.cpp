#include <graphene/chain/contract_evaluate.hpp>
#include <graphene/chain/contract.hpp>
#include <graphene/chain/storage.hpp>
#include <graphene/chain/contract_entry.hpp>
#include <graphene/chain/contract_engine_builder.hpp>
#include <graphene/chain/uvm_chain_api.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/transaction_object.hpp>
#include <iostream>
#include <fc/array.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/crypto/elliptic.hpp>
#include <fc/crypto/base58.hpp>
#include <boost/uuid/sha1.hpp>
#include <exception>
namespace graphene {
	namespace chain {

		using uvm::lua::api::global_uvm_chain_api;

		static contract_register_evaluate* get_register_contract_evaluator(lua_State *L) {
			return (contract_register_evaluate*)uvm::lua::lib::get_lua_state_value(L, "register_evaluate_state").pointer_value;
		}

		static contract_invoke_evaluate* get_invoke_contract_evaluator(lua_State *L) {
			return (contract_invoke_evaluate*)uvm::lua::lib::get_lua_state_value(L, "invoke_evaluate_state").pointer_value;
		}

		static contract_upgrade_evaluate* get_upgrade_contract_evaluator(lua_State *L) {
			return (contract_upgrade_evaluate*)uvm::lua::lib::get_lua_state_value(L, "upgrade_evaluate_state").pointer_value;
		}

		static contract_transfer_evaluate* get_contract_transfer_evaluator(lua_State *L) {
			return (contract_transfer_evaluate*)uvm::lua::lib::get_lua_state_value(L, "transfer_evaluate_state").pointer_value;
		}

        contract_common_evaluate* contract_common_evaluate::get_contract_evaluator(lua_State *L) {
			auto register_contract_evaluator = get_register_contract_evaluator(L);
			if (register_contract_evaluator) {
                return register_contract_evaluator;
			}
			auto invoke_contract_evaluator = get_invoke_contract_evaluator(L);
			if (invoke_contract_evaluator) {
                return invoke_contract_evaluator;
			}
			auto upgrade_contract_evaluator = get_upgrade_contract_evaluator(L);
			if (upgrade_contract_evaluator) {
                return upgrade_contract_evaluator;
			}
			auto contract_transfer_evaluator = get_contract_transfer_evaluator(L);
			if (contract_transfer_evaluator) {
				return contract_transfer_evaluator;
			}
			return nullptr; // FIXME: ��ֱ�ӱ�����Ϊ��ljsonlib.cpp�����ڻ��õ�
		}

	    share_type count_gas_fee(gas_price_type gas_price, gas_count_type gas_count) {
			// every 100 gas cost 1 min-precision base-asset

			share_type fee = ((gas_count / 100) + ((gas_count % 100) == 0 ? 0 : 1)) * gas_price;
			FC_ASSERT(fee >= 0, "gas fee wrong");
			return fee;
		}

		share_type count_contract_register_fee(const uvm::blockchain::Code& code) {
			return 10; // contract register fee
		}

        contract_operation_result_info contract_register_evaluate::do_evaluate(const operation_type& o) {
			auto &d = db();
			// check contract id unique
            if (d.get_node_properties().skip_flags&database::validation_steps::check_gas_price)
            {
                FC_ASSERT(o.gas_price >= d.get_min_gas_price(),"gas is too cheap");
            }
			FC_ASSERT(o.contract_id.version == addressVersion::CONTRACT);
			address fid = contract_register_operation::get_first_contract_id();

            //FC_ASSERT(check_fee_for_gas(o.owner_addr, o.init_cost, o.gas_price));
			FC_ASSERT(!d.has_contract(o.contract_id), "contract address must be unique");
            total_fee = o.fee.amount;
			if (!global_uvm_chain_api)
				global_uvm_chain_api = new UvmChainApi();
            FC_ASSERT(o.contract_code != uvm::blockchain::Code() || o.inherit_from != address());
            if (!(o.contract_code != uvm::blockchain::Code()) && o.inherit_from != address())
            {
                FC_ASSERT(d.has_contract(o.inherit_from));
                const auto& base_contract=d.get_contract(o.inherit_from);
                FC_ASSERT(base_contract.type_of_contract == normal_contract);
            }
			this->caller_address = std::make_shared<address>(o.owner_addr);
			this->caller_pubkey = std::make_shared<fc::ecc::public_key>(o.owner_pubkey);
			::blockchain::contract_engine::ContractEngineBuilder builder;
			auto engine = builder.build();
			int exception_code = 0;
			string exception_msg;
            gas_count = o.init_cost;
			try {
				origin_op = o;
				if (!d.has_contract(fid))
					origin_op.contract_id = fid;
				engine->set_caller(o.owner_pubkey.to_base58(), (string)(o.owner_addr));
				engine->set_state_pointer_value("register_evaluate_state", this);
				engine->clear_exceptions();
				auto limit = o.init_cost;
				if (limit < 0 || limit == 0)
					FC_CAPTURE_AND_THROW(blockchain::contract_engine::invalid_contract_gas_limit);
				gas_limit = limit;
				engine->set_gas_limit(limit);
				invoke_contract_result.reset();
				try
				{
					engine->execute_contract_init_by_address(origin_op.contract_id.operator fc::string(), "", nullptr);
				}
				catch (std::exception &e)
				{
					FC_CAPTURE_AND_THROW(blockchain::contract_engine::uvm_executor_internal_error, (e.what()));
				}

				gas_used_counts = engine->gas_used();
				FC_ASSERT(gas_used_counts <= o.init_cost && gas_used_counts > 0, "costs of execution can be only between 0 and init_cost");
				auto register_fee = count_contract_register_fee(o.contract_code);

				
                gas_count = gas_used_counts;	
				new_contract.contract_address = origin_op.contract_id;
				string fid_str = string(new_contract.contract_address);
				std::cout << fid_str << "\n";
                new_contract.code = o.contract_code;
                new_contract.owner_address = o.owner_addr;
                new_contract.create_time = o.register_time;
                new_contract.inherit_from = o.inherit_from;
                new_contract.registered_block = d.head_block_num() + 1;
                if ((!(o.contract_code != uvm::blockchain::Code())) || o.inherit_from != address())
                    new_contract.type_of_contract = contract_based_on_template;
                unspent_fee = count_gas_fee(o.gas_price, o.init_cost) -count_gas_fee(o.gas_price, gas_used_counts);

                invoke_contract_result.acctual_fee = total_fee - unspent_fee;
                invoke_contract_result.exec_succeed = true;
			}
			catch (::blockchain::contract_engine::contract_run_out_of_money& e)
			{
				undo_contract_effected(total_fee);
				unspent_fee = 0;
			}
			catch (const ::blockchain::contract_engine::contract_error& e)
			{
				FC_CAPTURE_AND_THROW(::blockchain::contract_engine::contract_error, (e.what()));
			}
			catch (std::exception &e)
			{
				FC_CAPTURE_AND_THROW(::blockchain::contract_engine::uvm_executor_internal_error, (e.what()));
			}

			return contract_operation_result_info(invoke_contract_result.ordered_digest(),gas_count, invoke_contract_result.api_result);
		}

        contract_operation_result_info native_contract_register_evaluate::do_evaluate(const operation_type& o) {
			auto &d = db();
            if (d.get_node_properties().skip_flags&database::validation_steps::check_gas_price)
            {
                FC_ASSERT(o.gas_price >= d.get_min_gas_price(), "gas is too cheap");
            }

			FC_ASSERT(o.contract_id.version == addressVersion::CONTRACT);
			// check contract id unique

            //FC_ASSERT(check_fee_for_gas(o.owner_addr, o.init_cost, o.gas_price));

            invoke_contract_result.invoker = o.owner_addr;
			FC_ASSERT(!d.has_contract(o.contract_id), "contract address must be unique");
			this->caller_address = std::make_shared<address>(o.owner_addr);
			this->caller_pubkey = std::make_shared<fc::ecc::public_key>(o.owner_pubkey);
            total_fee = o.fee.amount;
            gas_count = o.init_cost;
			try {
				FC_ASSERT(native_contract_finder::has_native_contract_with_key(o.native_contract_key));
				auto limit = o.init_cost;
				if (limit < 0 || limit == 0)
					FC_CAPTURE_AND_THROW(blockchain::contract_engine::invalid_contract_gas_limit);
				gas_limit = limit;
				auto native_contract = native_contract_finder::create_native_contract_by_key(this, o.native_contract_key, o.contract_id);
				FC_ASSERT(native_contract);

				auto invoke_result = native_contract->invoke("init", "");

				gas_used_counts = native_contract->gas_count_for_api_invoke("init");
				FC_ASSERT(gas_used_counts <= limit && gas_used_counts > 0, "costs of execution can be only between 0 and init_cost");
				auto register_fee = native_contract_register_fee;

				// TODO: deposit margin balance to contract

                gas_count = gas_used_counts;

				this->invoke_contract_result = invoke_result;

				new_contract.contract_address = o.calculate_contract_id();
				new_contract.type_of_contract = contract_type::native_contract;
				new_contract.native_contract_key = o.native_contract_key;
				new_contract.owner_address = o.owner_addr;
				new_contract.create_time = o.register_time;
                new_contract.inherit_from = address();
                new_contract.registered_block = d.head_block_num() + 1;
                unspent_fee = count_gas_fee(o.gas_price, o.init_cost) - count_gas_fee(o.gas_price, gas_used_counts);
                invoke_result.acctual_fee = total_fee - unspent_fee;
                invoke_result.exec_succeed = true;
			}
			catch (::blockchain::contract_engine::contract_run_out_of_money& e)
			{
				undo_contract_effected(total_fee);
				unspent_fee = 0;
			}
			catch (const ::blockchain::contract_engine::contract_error& e)
			{
				FC_CAPTURE_AND_THROW(::blockchain::contract_engine::contract_error, (e.what()));
			}
			catch (std::exception &e)
			{
				FC_CAPTURE_AND_THROW(::blockchain::contract_engine::uvm_executor_internal_error, (e.what()));
			}


            return contract_operation_result_info(invoke_contract_result.ordered_digest(), gas_count, invoke_contract_result.api_result);
		}

        contract_operation_result_info contract_invoke_evaluate::do_evaluate(const operation_type& o) {
			auto &d = db();
            if (d.get_node_properties().skip_flags&database::validation_steps::check_gas_price)
            {
                FC_ASSERT(o.gas_price >= d.get_min_gas_price(), "gas is too cheap");
            }
            //FC_ASSERT(check_fee_for_gas(o.caller_addr,o.invoke_cost,o.gas_price));

			FC_ASSERT(o.contract_id.version == addressVersion::CONTRACT);
            invoke_contract_result.invoker = o.caller_addr;
			FC_ASSERT(d.has_contract(o.contract_id,o.contract_api));
			FC_ASSERT(operation_type::contract_api_check(o));
			
			const auto &contract = d.get_contract(o.contract_id);
			this->caller_address = std::make_shared<address>(o.caller_addr);
			this->caller_pubkey = std::make_shared<fc::ecc::public_key>(o.caller_pubkey);
            total_fee = o.fee.amount;
            gas_count = o.invoke_cost;
			try {
				if (contract.type_of_contract==contract_type::native_contract)
				{
					FC_ASSERT(native_contract_finder::has_native_contract_with_key(contract.native_contract_key));
					auto limit = o.invoke_cost;
 					if ((limit < 0 || limit == 0))
						FC_CAPTURE_AND_THROW(blockchain::contract_engine::contract_error);
					gas_limit = limit;
					auto native_contract = native_contract_finder::create_native_contract_by_key(this, contract.native_contract_key, o.contract_id);
					FC_ASSERT(native_contract);
					auto invoke_result = native_contract->invoke(o.contract_api, o.contract_arg);
					this->invoke_contract_result = invoke_result;
					gas_used_counts = native_contract->gas_count_for_api_invoke(o.contract_api);
                    gas_count = gas_used_counts;
					FC_ASSERT(gas_used_counts <= limit && gas_used_counts > 0, "costs of execution can be only between 0 and invoke_cost");
                    //gas_fees.push_back(asset(required, asset_id_type(0)));
                    unspent_fee = count_gas_fee(o.gas_price, o.invoke_cost) - count_gas_fee(o.gas_price, gas_used_counts);
				}
				else
				{
					if (!global_uvm_chain_api)
						global_uvm_chain_api = new UvmChainApi();

					::blockchain::contract_engine::ContractEngineBuilder builder;
					auto engine = builder.build();
					int exception_code = 0;
				
					origin_op = o;
					engine->set_caller(o.caller_pubkey.to_base58(), (string)(o.caller_addr));
					engine->set_state_pointer_value("invoke_evaluate_state", this);
					engine->clear_exceptions();
					auto limit = o.invoke_cost;
					if (!offline && (limit < 0 || limit == 0))
						FC_CAPTURE_AND_THROW(blockchain::contract_engine::invalid_contract_gas_limit);
					gas_limit = limit;
					engine->set_gas_limit(limit);
					invoke_contract_result.storage_changes.clear();
					std::string contract_result_str;
					try
					{
						engine->execute_contract_api_by_address(o.contract_id.operator fc::string(), o.contract_api, o.contract_arg, &contract_result_str);
						this->invoke_contract_result.api_result = contract_result_str;
					}
					catch (std::exception &e)
					{
						FC_CAPTURE_AND_THROW(::blockchain::contract_engine::uvm_executor_internal_error, (e.what()));
					}

					gas_used_counts = engine->gas_used();
                    gas_count = gas_used_counts;
					if(!offline)
						FC_ASSERT(gas_used_counts <= o.invoke_cost && gas_used_counts > 0, "costs of execution can be only between 0 and invoke_cost");

                    unspent_fee = count_gas_fee(o.gas_price, o.invoke_cost) - count_gas_fee(o.gas_price, gas_used_counts);
				}
                invoke_contract_result.acctual_fee = total_fee - unspent_fee;
                invoke_contract_result.exec_succeed = true;
			}
			catch (::blockchain::contract_engine::contract_run_out_of_money& e)
			{
				undo_contract_effected(total_fee);
				invoke_contract_result.api_result = string("gas ran out");
				unspent_fee = 0;
			}
			catch (const ::blockchain::contract_engine::contract_error& e)
			{
				FC_CAPTURE_AND_THROW(::blockchain::contract_engine::contract_error, (e.what()));
			}
			catch (std::exception &e)
			{
				FC_CAPTURE_AND_THROW(::blockchain::contract_engine::uvm_executor_internal_error, (e.what()));
			}


            return contract_operation_result_info(invoke_contract_result.ordered_digest(), gas_count, invoke_contract_result.api_result);
		}

        contract_operation_result_info contract_upgrade_evaluate::do_evaluate(const operation_type& o) {
			auto &d = db();
            if (d.get_node_properties().skip_flags&database::validation_steps::check_gas_price)
            {
                FC_ASSERT(o.gas_price >= d.get_min_gas_price(), "gas is too cheap");
            }

            //FC_ASSERT(check_fee_for_gas(o.caller_addr, o.invoke_cost, o.gas_price));

			FC_ASSERT(o.contract_id.version == addressVersion::CONTRACT);
            invoke_contract_result.invoker = o.caller_addr;
			FC_ASSERT(d.has_contract(o.contract_id));
			FC_ASSERT(!d.has_contract_of_name(o.contract_name));
			const auto &contract = d.get_contract(o.contract_id);
			FC_ASSERT(contract.contract_name.empty());
			this->caller_address = std::make_shared<address>(o.caller_addr);
            total_fee = o.fee.amount;
			this->caller_pubkey = std::make_shared<fc::ecc::public_key>(o.caller_pubkey);
            
            if(contract.code.abi.find("on_upgrade") != contract.code.abi.end())
            { 
			try {
                gas_count = o.invoke_cost;
				if (contract.type_of_contract == contract_type::native_contract)
				{
					FC_ASSERT(native_contract_finder::has_native_contract_with_key(contract.native_contract_key));
					auto limit = o.invoke_cost;
					if (limit < 0 || limit == 0)
						FC_CAPTURE_AND_THROW(blockchain::contract_engine::invalid_contract_gas_limit);
					gas_limit = limit;
					auto native_contract = native_contract_finder::create_native_contract_by_key(this, contract.native_contract_key, o.contract_id);
					FC_ASSERT(native_contract);
					auto invoke_result = native_contract->invoke("on_upgrade", o.contract_name);
					this->invoke_contract_result = invoke_result;
					gas_used_counts = native_contract->gas_count_for_api_invoke("on_upgrade");
					FC_ASSERT(gas_used_counts <= limit && gas_used_counts > 0, "costs of execution can be only between 0 and invoke_cost");
                    //gas_fees.push_back(asset(required, asset_id_type(0)));
                    unspent_fee = count_gas_fee(o.gas_price, o.invoke_cost) - count_gas_fee(o.gas_price, gas_used_counts);
                    gas_count = gas_used_counts;
				}
				else
				{
					if (!global_uvm_chain_api)
						global_uvm_chain_api = new UvmChainApi();

					::blockchain::contract_engine::ContractEngineBuilder builder;
					auto engine = builder.build();
					int exception_code = 0;

					origin_op = o;
					engine->set_caller(o.caller_pubkey.to_base58(), (string)(o.caller_addr));
					engine->set_state_pointer_value("upgrade_evaluate_state", this);
					engine->clear_exceptions();
					auto limit = o.invoke_cost;
					if (limit < 0 || limit == 0)
						FC_CAPTURE_AND_THROW(blockchain::contract_engine::invalid_contract_gas_limit);
					gas_limit = limit;
					engine->set_gas_limit(limit);
					invoke_contract_result.storage_changes.clear();
					std::string contract_result_str;
					try
					{
						engine->execute_contract_api_by_address(o.contract_id.operator fc::string(), "on_upgrade", o.contract_name, &contract_result_str);
					}
					catch (std::exception &e)
					{
						FC_CAPTURE_AND_THROW(::blockchain::contract_engine::uvm_executor_internal_error, (e.what()));
					}

					gas_used_counts = engine->gas_used();
                    gas_count = gas_used_counts;
					FC_ASSERT(gas_used_counts <= o.invoke_cost && gas_used_counts > 0, "costs of execution can be only between 0 and invoke_cost");

                    //gas_fees.push_back(asset(required, asset_id_type(0)));
                    unspent_fee = count_gas_fee(o.gas_price, o.invoke_cost) - count_gas_fee(o.gas_price, gas_used_counts);
				}
                invoke_contract_result.acctual_fee = total_fee - unspent_fee;
                invoke_contract_result.exec_succeed = true;
			}
			catch (::blockchain::contract_engine::contract_run_out_of_money& e)
			{
				undo_contract_effected(total_fee);
				unspent_fee = 0;
			}
			catch (const ::blockchain::contract_engine::contract_error& e)
			{
				FC_CAPTURE_AND_THROW(::blockchain::contract_engine::contract_error, (e.what()));
			}
			catch (std::exception &e)
			{
				FC_CAPTURE_AND_THROW(::blockchain::contract_engine::uvm_executor_internal_error, (e.what()));
			}
            }
            else
            {
                unspent_fee = count_gas_fee(o.gas_price, o.invoke_cost);
            }

            return contract_operation_result_info(invoke_contract_result.ordered_digest(), gas_count, invoke_contract_result.api_result);
		}

        contract_operation_result_info contract_register_evaluate::do_apply(const operation_type& o) {
			database& d = db();

            auto  trx_id = get_current_trx_id();
			// commit contract result to db
            if (invoke_contract_result.exec_succeed)
            {
                
                d.store_contract(new_contract);
                if (new_contract.inherit_from != address())
                {
                    auto base_contract = d.get_contract(new_contract.inherit_from);
                    base_contract.derived.push_back(new_contract.contract_address);
                    d.update_contract(base_contract);
                }
                apply_storage_change(d,new_contract.registered_block, trx_id);
                do_apply_contract_event_notifies();
                //do_apply_fees_balance(origin_op.owner_addr);
                do_apply_balance();
            }

            d.store_invoke_result(trx_id, gen_eval->get_trx_eval_state()->op_num,invoke_contract_result);
            return contract_operation_result_info(invoke_contract_result.ordered_digest(), gas_count, invoke_contract_result.api_result);
		}

        contract_operation_result_info native_contract_register_evaluate::do_apply(const operation_type& o) {

            auto trx_id = get_current_trx_id();
            if (invoke_contract_result.exec_succeed)
            {
			database& d = db();
			// commit contract result to db
			d.store_contract(new_contract);
            apply_storage_change(d, new_contract.registered_block, trx_id);
			//do_apply_fees_balance(o.owner_addr);
			do_apply_contract_event_notifies();

            }

            db().store_invoke_result(trx_id, gen_eval->get_trx_eval_state()->op_num, invoke_contract_result);
            return contract_operation_result_info(invoke_contract_result.ordered_digest(), gas_count, invoke_contract_result.api_result);
		}

        contract_operation_result_info contract_invoke_evaluate::do_apply(const operation_type& o) {
            if (invoke_contract_result.exec_succeed)
            {
                database& d = db();
                FC_ASSERT(d.has_contract(o.contract_id));
                auto trx_id = get_current_trx_id();
                // commit contract result to db

                apply_storage_change(d, d.head_block_num(), trx_id);
                do_apply_contract_event_notifies();
                //do_apply_fees_balance(origin_op.caller_addr);
                do_apply_balance();
            }
            db().store_invoke_result(get_current_trx_id(), gen_eval->get_trx_eval_state()->op_num, invoke_contract_result);
            return contract_operation_result_info(invoke_contract_result.ordered_digest(), gas_count, invoke_contract_result.api_result);
		}

        contract_operation_result_info contract_upgrade_evaluate::do_apply(const operation_type& o) {
            if (invoke_contract_result.exec_succeed)
            {
                database& d = db();
                // save contract name
                FC_ASSERT(d.has_contract(o.contract_id));
                auto contract = d.get_contract(o.contract_id);
                contract.contract_name = o.contract_name;
                contract.contract_desc = o.contract_desc;
                d.update_contract(contract);
                auto trx_id = get_current_trx_id();
                // commit contract result to db
                apply_storage_change(d, d.head_block_num(), trx_id);
                do_apply_contract_event_notifies();
                //do_apply_fees_balance(origin_op.caller_addr);
                do_apply_balance();

            }
            db().store_invoke_result(get_current_trx_id(), gen_eval->get_trx_eval_state()->op_num, invoke_contract_result);
            return contract_operation_result_info(invoke_contract_result.ordered_digest(), gas_count, invoke_contract_result.api_result);
		}

		void contract_register_evaluate::pay_fee() {
            pay_fee_and_refund();
		}

		void native_contract_register_evaluate::pay_fee() {
            pay_fee_and_refund();
		}

		void contract_invoke_evaluate::pay_fee() {
            pay_fee_and_refund();
		}

		void contract_upgrade_evaluate::pay_fee() {
            pay_fee_and_refund();
		}

		std::shared_ptr<UvmContractInfo> contract_register_evaluate::get_contract_by_id(const string &contract_id) const
		{
            FC_ASSERT((origin_op.contract_code != uvm::blockchain::Code()) || (origin_op.inherit_from != address()));
			if (origin_op.contract_id.operator fc::string() == contract_id)
			{
				auto contract_info = std::make_shared<UvmContractInfo>();
                if (origin_op.contract_code != uvm::blockchain::Code())
                {
                    const auto &code = origin_op.contract_code;
                    for (const auto & api : code.abi) {
                        contract_info->contract_apis.push_back(api);
                    }
                }
                else
                {
                    if (!db().has_contract(origin_op.inherit_from))
                        return nullptr;
                    const auto &contract =db().get_contract(origin_op.inherit_from);
                    if (contract.type_of_contract == contract_type::native_contract)
                        return nullptr;
                    const auto &code = contract.code;
                    for (const auto & api : code.abi) {
                        contract_info->contract_apis.push_back(api);
                    }
                    return contract_info;

                }
         
				return contract_info;
			}
			else
			{
				address contract_addr(contract_id);
				if (!db().has_contract(contract_addr))
					return nullptr;
				auto contract_info = std::make_shared<UvmContractInfo>();
				const auto &contract = db().get_contract(contract_addr);
				if (contract.type_of_contract == contract_type::native_contract)
				{
					auto native_contract = native_contract_finder::create_native_contract_by_key(const_cast<contract_register_evaluate*>(this), contract.native_contract_key, contract.contract_address);
					if (!native_contract)
						return nullptr;
					for (const auto & api : native_contract->apis()) {
						contract_info->contract_apis.push_back(api);
					}
					return contract_info;
				}
				const auto &code = contract.code;
				for (const auto & api : code.abi) {
					contract_info->contract_apis.push_back(api);
				}
				return contract_info;
			}
		}

		std::shared_ptr<UvmContractInfo> native_contract_register_evaluate::get_contract_by_id(const string &contract_id) const
		{
			address contract_addr;
			try {
				auto temp = graphene::chain::address(contract_id);
				if (temp.version != addressVersion::CONTRACT)
					return nullptr;
				contract_addr = temp;
			}
			catch (fc::exception& e)
			{
				return nullptr;
			}
			if (origin_op.contract_id == contract_addr)
			{
				auto contract_info = std::make_shared<UvmContractInfo>();
				auto native_contract = native_contract_finder::create_native_contract_by_key(const_cast<native_contract_register_evaluate*>(this), origin_op.native_contract_key, address(contract_id));
				if (!native_contract)
					return nullptr;
				for (const auto & api : native_contract->apis()) {
					contract_info->contract_apis.push_back(api);
				}
				return contract_info;
			}
			else
			{
				if (!db().has_contract(contract_addr))
					return nullptr;
				auto contract_info = std::make_shared<UvmContractInfo>();
				const auto &contract = db().get_contract(contract_addr);
				if (contract.type_of_contract == contract_type::native_contract)
				{
					auto native_contract = native_contract_finder::create_native_contract_by_key(const_cast<native_contract_register_evaluate*>(this), contract.native_contract_key, contract.contract_address);
					if (!native_contract)
						return nullptr;
					for (const auto & api : native_contract->apis()) {
						contract_info->contract_apis.push_back(api);
					}
					return contract_info;
				}
				const auto &code = contract.code;
				for (const auto & api : code.abi) {
					contract_info->contract_apis.push_back(api);
				}
				return contract_info;
			}
		}

		std::shared_ptr<uvm::blockchain::Code> contract_register_evaluate::get_contract_code_by_id(const string &contract_id) const
		{
			if (origin_op.contract_id.operator fc::string() == contract_id)
			{
				auto code = std::make_shared<uvm::blockchain::Code>();
				if(origin_op.contract_code!=uvm::blockchain::Code())
                {
                    *code = origin_op.contract_code;
                }
                else
                {
                    FC_ASSERT(origin_op.inherit_from != address());
                    if (!db().has_contract(origin_op.inherit_from))
                        return nullptr;
                    const auto &contract = db().get_contract(origin_op.inherit_from);
                    if (contract.type_of_contract == contract_type::native_contract)
                        return nullptr;
                    *code = contract.code;
                }
				return code;
			}
			else
			{
				return get_contract_code_from_db_by_id(contract_id);
			}
		}


		address contract_register_evaluate::origin_op_contract_id() const
		{
			return origin_op.contract_id;
		}

		share_type contract_register_evaluate::origin_op_fee() const
		{
			return calculate_fee_for_operation(origin_op);
		}
        optional<guarantee_object_id_type> contract_register_evaluate::get_guarantee_id()const
        {
            return origin_op.get_guarantee_id();
        }
		share_type native_contract_register_evaluate::origin_op_fee() const
		{
			return calculate_fee_for_operation(origin_op);
		}
        optional<guarantee_object_id_type> native_contract_register_evaluate::get_guarantee_id()const
        {
            return origin_op.get_guarantee_id();
        }
		share_type contract_invoke_evaluate::origin_op_fee() const
		{
			return calculate_fee_for_operation(origin_op);
		}
        optional<guarantee_object_id_type> contract_invoke_evaluate::get_guarantee_id()const
        {
            return origin_op.get_guarantee_id();
        }
		share_type contract_upgrade_evaluate::origin_op_fee() const
		{
			return calculate_fee_for_operation(origin_op);
		}
        optional<guarantee_object_id_type> contract_upgrade_evaluate::get_guarantee_id()const
        {
            return origin_op.get_guarantee_id();
        }
		share_type contract_transfer_evaluate::origin_op_fee() const
		{
			return calculate_fee_for_operation(origin_op);
		}
        optional<guarantee_object_id_type> contract_transfer_evaluate::get_guarantee_id()const
        {
            return origin_op.get_guarantee_id();
        }
        address native_contract_register_evaluate::origin_op_contract_id() const
		{
			return origin_op.contract_id;
		}


        contract_operation_result_info contract_transfer_evaluate::do_evaluate(const operation_type & o)
        {
            auto &d = db();
            
            if (d.get_node_properties().skip_flags&database::validation_steps::check_gas_price)
            {
                FC_ASSERT(o.gas_price >= d.get_min_gas_price(), "gas is too cheap");
            }

			FC_ASSERT(o.contract_id.version == addressVersion::CONTRACT);
            //FC_ASSERT(check_fee_for_gas(o.caller_addr, o.invoke_cost, o.gas_price));

            invoke_contract_result.invoker = o.caller_addr;
            FC_ASSERT(d.has_contract(o.contract_id));
            const auto &contract = d.get_contract(o.contract_id);
            deposit_to_contract(o.contract_id, o.amount);
            this->caller_address = std::make_shared<address>(o.caller_addr);
            this->caller_pubkey = std::make_shared<fc::ecc::public_key>(o.caller_pubkey);
            total_fee = o.fee.amount;
            
            try {
                if (contract.type_of_contract == contract_type::native_contract)
                {
                    FC_ASSERT(native_contract_finder::has_native_contract_with_key(contract.native_contract_key));
					auto limit = o.invoke_cost;
					if (limit < 0 || limit == 0)
						FC_CAPTURE_AND_THROW(blockchain::contract_engine::invalid_contract_gas_limit);
					gas_limit = limit;
                    auto native_contract = native_contract_finder::create_native_contract_by_key(this, contract.native_contract_key, o.contract_id);
                    FC_ASSERT(native_contract);
					if (native_contract->has_api("on_deposit_asset"))
					{
                        gas_count = o.invoke_cost;
                        transfer_contract_operation::transfer_param param;
                        
                        param.num= o.amount.amount;
                        param.symbol = o.amount.asset_id(d).symbol;
                        param.param = o.param;

                        auto invoke_result = native_contract->invoke("on_deposit_asset", fc::json::to_string(param));

						gas_used_counts = native_contract->gas_count_for_api_invoke("on_deposit_asset");
                        gas_count = gas_used_counts;
                        FC_ASSERT(gas_used_counts <= limit && gas_used_counts > 0, "costs of execution can be only between 0 and invoke_cost");
						auto register_fee = native_contract_register_fee;

                        //gas_fees.push_back(asset(required, asset_id_type(0)));
                        unspent_fee = count_gas_fee(o.gas_price, o.invoke_cost) - count_gas_fee(o.gas_price, gas_used_counts);
					}
					else
					{
						unspent_fee = count_gas_fee(o.gas_price, o.invoke_cost);
					}
                }
                else
                {
					if (contract.code.abi.find("on_deposit_asset") != contract.code.abi.end())
					{

                        gas_count = o.invoke_cost;
						if (!global_uvm_chain_api)
							global_uvm_chain_api = new UvmChainApi();

						::blockchain::contract_engine::ContractEngineBuilder builder;
						auto engine = builder.build();
						int exception_code = 0;

						origin_op = o;
						engine->set_caller(o.caller_pubkey.to_base58(), (string)(o.caller_addr));
						engine->set_state_pointer_value("transfer_evaluate_state", this);
						engine->clear_exceptions();
						auto limit = o.invoke_cost;
						if (limit < 0 || limit == 0)
							FC_CAPTURE_AND_THROW(blockchain::contract_engine::invalid_contract_gas_limit);
						gas_limit = limit;
						engine->set_gas_limit(limit);
						invoke_contract_result.reset();

                        deposit_to_contract(o.contract_id, o.amount);
						std::string contract_result_str;
						try
						{
                            transfer_contract_operation::transfer_param param;
                            param.num = o.amount.amount;
                            param.symbol = o.amount.asset_id(d).symbol;
                            param.param = o.param;
							engine->execute_contract_api_by_address(o.contract_id.operator fc::string(), "on_deposit_asset", fc::json::to_string(param), &contract_result_str);
						}
						catch (std::exception &e)
						{
							FC_CAPTURE_AND_THROW(::blockchain::contract_engine::uvm_executor_internal_error, (e.what()));
						}

						gas_used_counts = engine->gas_used();

                        gas_count = gas_used_counts;
						FC_ASSERT(gas_used_counts <= limit && gas_used_counts > 0, "costs of execution can be only between 0 and invoke_cost");

                        unspent_fee = count_gas_fee(o.gas_price, o.invoke_cost) - count_gas_fee(o.gas_price, gas_used_counts);
					}
					else
					{
						unspent_fee = count_gas_fee(o.gas_price, o.invoke_cost);
					}
                }
                invoke_contract_result.acctual_fee = total_fee - unspent_fee;
                invoke_contract_result.exec_succeed = true;
            }
            catch (::blockchain::contract_engine::contract_run_out_of_money& e)
            {
				undo_contract_effected(total_fee);
                unspent_fee = 0;
            }
            catch (const ::blockchain::contract_engine::contract_error& e)
            {
                FC_CAPTURE_AND_THROW(::blockchain::contract_engine::contract_error, (e.what()));
            }
			catch (std::exception &e)
			{
				FC_CAPTURE_AND_THROW(::blockchain::contract_engine::uvm_executor_internal_error, (e.what()));
			}

            return contract_operation_result_info(invoke_contract_result.ordered_digest(), gas_count, invoke_contract_result.api_result);
        }

        contract_operation_result_info contract_transfer_evaluate::do_apply(const operation_type & o)
        {
            if (invoke_contract_result.exec_succeed)
            {
                database& d = db();
                FC_ASSERT(d.has_contract(o.contract_id));
                // commit contract result to db
                auto trx_id = get_current_trx_id();
                apply_storage_change(d, d.head_block_num(), trx_id);

                do_apply_contract_event_notifies();
                do_apply_balance();
                if(!gen_eval->get_trx_eval_state()->testing)
                    db_adjust_balance(o.caller_addr, asset(-o.amount.amount, o.amount.asset_id));
            }

            db().store_invoke_result(get_current_trx_id(), gen_eval->get_trx_eval_state()->op_num, invoke_contract_result);
            return contract_operation_result_info(invoke_contract_result.ordered_digest(), gas_count, invoke_contract_result.api_result);
        }

        void contract_transfer_evaluate::pay_fee()
        {
            pay_fee_and_refund();
        }


         contract_common_evaluate::contract_common_evaluate(generic_evaluator * gen_eval) :gen_eval(gen_eval)
        {
        }
        contract_common_evaluate::~contract_common_evaluate() {}

		void contract_common_evaluate::set_contract_storage_changes(const string& contract_id, const contract_storage_changes_type& changes)
		{
			related_contract.insert(address(contract_id));
			invoke_contract_result.storage_changes[contract_id] = changes;
		}
         std::shared_ptr<address> contract_common_evaluate::get_caller_address() const
        {
            return caller_address;
        }
         std::shared_ptr<fc::ecc::public_key> contract_common_evaluate::get_caller_pubkey() const
        {
            return caller_pubkey;
        }
         database & contract_common_evaluate::get_db() const { return gen_eval->db(); }
         StorageDataType contract_common_evaluate::get_storage(const string & contract_id, const string & storage_name) const
        {
            database& d = get_db();
            std::cout<<contract_id<<"___"<<storage_name<<std::endl;
            auto storage_data = d.get_contract_storage(address(contract_id), storage_name);
            return storage_data;
        }
        std::shared_ptr<uvm::blockchain::Code> contract_common_evaluate::get_contract_code_by_name(const string & contract_name) const
        {
            if (!get_db().has_contract_of_name(contract_name))
                return nullptr;
            if (contract_name.empty())
                return nullptr;
            auto contract_info = std::make_shared<UvmContractInfo>();
            const auto &contract = get_db().get_contract_of_name(contract_name);
            // TODO: when contract is native contract
            const auto &code = contract.code;
            for (const auto & api : code.abi) {
                contract_info->contract_apis.push_back(api);
            }
            auto ccode = std::make_shared<uvm::blockchain::Code>();
            *ccode = code;
            return ccode;
        }
        asset contract_common_evaluate::asset_from_string(const string & symbol, const string & amount)
        {
            auto& asset_indx = get_db().get_index_type<asset_index>().indices().get<by_symbol>();
            auto asset_symbol_itr = asset_indx.find(symbol);
            if (asset_symbol_itr == asset_indx.end())
            {
                FC_CAPTURE_AND_THROW(blockchain::contract_engine::invalid_asset_symbol, (symbol));
            }
            else
            {
                return asset_symbol_itr->amount_from_string(amount);
            }
        }
        std::shared_ptr<uvm::blockchain::Code> contract_common_evaluate::get_contract_code_from_db_by_id(const string & contract_id) const
        {
            address contract_addr(contract_id);
            if (!get_db().has_contract(contract_addr))
                return nullptr;
            //auto contract_info = std::make_shared<UvmContractInfo>();
            const auto &contract = get_db().get_contract(contract_addr);
            // TODO: when contract is native contract
            FC_ASSERT(contract.code != uvm::blockchain::Code() || contract.inherit_from != address());
            auto code = contract.code;
            if (!(contract.code != uvm::blockchain::Code()))
            {
                FC_ASSERT(contract.inherit_from != address());
                if (!get_db().has_contract(contract.inherit_from))
                    return nullptr;
                const auto &base_contract = get_db().get_contract(contract.inherit_from);
                if (base_contract.type_of_contract == contract_type::native_contract)
                    return nullptr;
                code = base_contract.code;

            }
            auto ccode = std::make_shared<uvm::blockchain::Code>();
            *ccode = code;
            return ccode;
        }
        //void contract_common_evaluate::add_gas_fee(const asset & fee)
        //{
        //    for (auto fee_it : gas_fees)
        //    {
        //        if (fee_it.asset_id == fee.asset_id)
        //        {
        //            fee_it.amount += fee.amount;
        //            return;
        //        }
        //    }
        //    gas_fees.push_back(fee);
        //}
        void contract_common_evaluate::undo_contract_effected(const share_type& fee)
{
			invoke_contract_result.set_failed(fee);
        }
        void contract_common_evaluate::deposit_to_contract(const address & contract, const asset & amount)
        {
			related_contract.insert(contract);
            share_type to_deposit = amount.amount;
            auto index = std::make_pair(contract, amount.asset_id);
            if (!get_db().has_contract(contract))
                FC_CAPTURE_AND_THROW(blockchain::contract_engine::contract_not_exsited, (contract));
            auto withdraw = invoke_contract_result.contract_withdraw.find(index);
            if (withdraw != invoke_contract_result.contract_withdraw.end())
            {
                if (withdraw->second >= to_deposit)
                {
                    withdraw->second -= to_deposit;
                    to_deposit = 0;
                }
                else
                {
                    to_deposit -= withdraw->second;
                    withdraw->second = 0;
                }
            }
            if (to_deposit == 0)
                return;
            auto deposit = invoke_contract_result.deposit_contract.find(index);
            if (deposit == invoke_contract_result.deposit_contract.end())
            {
                auto res = invoke_contract_result.deposit_contract.insert(std::make_pair(index, 0));
                if (res.second)
                {
                    deposit = res.first;
                }
            }
			invoke_contract_result.deposit_contract[index] += to_deposit;
        }
        //void contract_common_evaluate::do_apply_fees_balance(const address & caller_addr)
        //{
        //    for (auto fee : gas_fees)
        //    {
        //        FC_ASSERT(fee.amount >= 0);
        //        asset fee_to_cost;
        //        fee_to_cost.asset_id = fee.asset_id;
        //        fee_to_cost.amount = -fee.amount;
        //        get_db().adjust_balance(caller_addr, fee_to_cost); // FIXME: now account have no money
        //    }
        //}
        void contract_common_evaluate::do_apply_balance()
        {
			auto trx_id = get_current_trx_id();
            for (auto to_contract = invoke_contract_result.deposit_contract.begin(); to_contract != invoke_contract_result.deposit_contract.end(); to_contract++)
            {
				if (to_contract->second != 0)
				{
					get_db().adjust_balance(to_contract->first.first, asset(to_contract->second, to_contract->first.second));
				}
            }
            for (auto to_withraw = invoke_contract_result.contract_withdraw.begin(); to_withraw != invoke_contract_result.contract_withdraw.end(); to_withraw++)
            {
                if (to_withraw->second != 0)
				{
					get_db().adjust_balance(to_withraw->first.first, asset(0 - to_withraw->second, to_withraw->first.second));
				}

            }
            for (auto to_deposit = invoke_contract_result.deposit_to_address.begin(); to_deposit != invoke_contract_result.deposit_to_address.end(); to_deposit++)
            {
                if (to_deposit->second != 0)
                    get_db().adjust_balance(to_deposit->first.first, asset(to_deposit->second, to_deposit->first.second));
            }
			for (auto addr : related_contract)
			{
				get_db().store_contract_related_transaction(trx_id, addr);
			}
        }
        transaction_id_type contract_common_evaluate::get_current_trx_id() const
        {
			FC_ASSERT(gen_eval->get_trx_eval_state()->_trx != nullptr);
            return gen_eval->get_trx_eval_state()->_trx->id();
        }
         void contract_common_evaluate::do_apply_contract_event_notifies()
        {
            auto trx_id = get_current_trx_id();
            for (const auto &obj : invoke_contract_result.events)
            {
                get_db().add_contract_event_notify(trx_id, obj.contract_address, obj.event_name, obj.event_arg, obj.block_num,obj.op_num);
            }
        }
         void contract_common_evaluate::transfer_to_address(const address & contract, const asset & amount, const address & to)
        {

			 related_contract.insert(contract);
            //withdraw
			share_type to_withdraw = amount.amount;
			auto con = get_db().get_contract(contract);
			bool fee_needed=(con.owner_address!=to);
            std::pair<address, asset_id_type> index = std::make_pair(contract, amount.asset_id);
            auto balance = invoke_contract_result.contract_balances.find(index);
            if (balance == invoke_contract_result.contract_balances.end())
            {
                auto res = invoke_contract_result.contract_balances.insert(std::make_pair(index, get_db().get_balance(index.first, index.second).amount));
                if (res.second)
                {
                    balance = res.first;
                }
            }
            share_type all_balance = balance->second;
            auto deposit = invoke_contract_result.deposit_contract.find(index);
            if (deposit != invoke_contract_result.deposit_contract.end())
            {
                all_balance += deposit->second;
            }
            auto withdraw = invoke_contract_result.contract_withdraw.find(index);
            if (withdraw != invoke_contract_result.contract_withdraw.end())
            {
                all_balance -= withdraw->second;
            }
            if (all_balance<to_withdraw)
                FC_CAPTURE_AND_THROW(blockchain::contract_engine::contract_insufficient_balance, ("insufficient contract balance"));

            if (deposit != invoke_contract_result.deposit_contract.end())
            {
                if (deposit->second >= to_withdraw)
                {
                    deposit->second -= to_withdraw;
                    to_withdraw = 0;
                }
                else
                {
                    to_withdraw -= deposit->second;
                    deposit->second = 0;
                }
            }
            if (withdraw != invoke_contract_result.contract_withdraw.end())
            {
                withdraw->second += to_withdraw;
            }
            else
            {
				invoke_contract_result.contract_withdraw.insert(std::make_pair(index, to_withdraw));
            }

            //deposit
            std::pair<address, asset_id_type> deposit_index;
            deposit_index.first = to;
			deposit_index.second = amount.asset_id;
			share_type transfer_fee_amount = 0;

			if (fee_needed)
			{
				auto fee_rate = get_contract_transfer_fee_rate();
				FC_ASSERT((fee_rate >= 0) && (fee_rate < CONTRACT_MAX_TRASACTION_FEE_RATE));

				transfer_fee_amount = ((double)(amount.amount.value))*(((double)(get_contract_transfer_fee_rate().value)) / ((double)(CONTRACT_MAX_TRASACTION_FEE_RATE)));
			}
            share_type transfer_amount = amount.amount- transfer_fee_amount;

            if (invoke_contract_result.deposit_to_address.find(deposit_index) != invoke_contract_result.deposit_to_address.end())
				invoke_contract_result.deposit_to_address[deposit_index] += transfer_amount;
            else
				invoke_contract_result.deposit_to_address[deposit_index] = transfer_amount;
            bool transfer_fee_exsited = false;
            if (transfer_fee_amount > 0)
            {
                for (auto it = invoke_contract_result.transfer_fees.begin(); it != invoke_contract_result.transfer_fees.end(); it++)
                {
                    if (it->first == amount.asset_id)
                    {
                        it->second += transfer_fee_amount;
                        transfer_fee_exsited = true;
                        break;
                    }
                }
                if (!transfer_fee_exsited)
                {
                    invoke_contract_result.transfer_fees.insert(make_pair(amount.asset_id, transfer_fee_amount));
                }
            }
        }
         share_type contract_common_evaluate::get_contract_balance(const address & contract, const asset_id_type & asset_id)
        {
            //balance= db_balance+deposit-withdraw
            share_type running_balance;
            //db_balance
            std::pair<address, asset_id_type> index = std::make_pair(contract, asset_id);
            auto balance = invoke_contract_result.contract_balances.find(index);
            if (balance == invoke_contract_result.contract_balances.end())
            {
                auto res = invoke_contract_result.contract_balances.insert(std::make_pair(index, get_db().get_balance(index.first, index.second).amount));
                if (res.second)
                {
                    balance = res.first;
                }
            }
            running_balance = balance->second;

            //deposit
            auto deposit = invoke_contract_result.deposit_contract.find(index);
            if (deposit != invoke_contract_result.deposit_contract.end())
            {
                running_balance += deposit->second;
            }

            //withdraw
            auto withdraw = invoke_contract_result.contract_withdraw.find(index);
            if (withdraw != invoke_contract_result.contract_withdraw.end())
            {
                running_balance -= withdraw->second;
            }
            return running_balance;
        }
		 void contract_common_evaluate::emit_event(const address& contract_addr, const string& event_name, const string& event_arg)
		 {
			 contract_event_notify_info info;
             info.op_num = gen_eval->get_trx_eval_state()->op_num;
			 info.contract_address = contract_addr;
			 info.event_name = event_name;
			 info.event_arg = event_arg;
             info.caller_addr=caller_address->address_to_string();
             info.block_num=1+ get_db().head_block_num();
			 invoke_contract_result.events.push_back(info);
		 }
         std::shared_ptr<UvmContractInfo> contract_common_evaluate::get_contract_by_id(const string &contract_id) const
         {
             address contract_addr(contract_id);
             if (!get_db().has_contract(contract_addr))
                 return nullptr;
             auto contract_info = std::make_shared<UvmContractInfo>();
             const auto &contract = get_db().get_contract(contract_addr);
             if (contract.type_of_contract == contract_type::native_contract)
             {
                 auto native_contract = native_contract_finder::create_native_contract_by_key(const_cast<contract_common_evaluate*>(this), contract.native_contract_key, contract.contract_address);
                 if (!native_contract)
                     return nullptr;
                 for (const auto & api : native_contract->apis()) {
                     contract_info->contract_apis.push_back(api);
                 }
                 return contract_info;
             }
             FC_ASSERT(contract.code != uvm::blockchain::Code() || contract.inherit_from != address());
             if (contract.code != uvm::blockchain::Code())
             {
                 const auto &code = contract.code;
                 for (const auto & api : code.abi) {
                     contract_info->contract_apis.push_back(api);
                 }
             }
             else
             {
                 if (!get_db().has_contract(contract.inherit_from))
                 {
                     return nullptr;
                 }

                 const auto& base_contract = get_db().get_contract(contract.inherit_from);
                 if (base_contract.type_of_contract != contract_type::normal_contract)
                     return nullptr;
                 for (const auto & api : base_contract.code.abi) {
                     contract_info->contract_apis.push_back(api);
                 }
             }
             return contract_info;
         };
         contract_object contract_common_evaluate::get_contract_by_name(const string& contract_name) const
         {
             FC_ASSERT(!contract_name.empty());
             FC_ASSERT(get_db().has_contract_of_name(contract_name));
             auto contract_info = std::make_shared<UvmContractInfo>();
             const auto &contract = get_db().get_contract_of_name(contract_name);
             return contract;
         }

         inline std::shared_ptr<uvm::blockchain::Code> contract_common_evaluate::get_contract_code_by_id(const string & contract_id) const
         {
             return get_contract_code_from_db_by_id(contract_id);
         }

		 string contract_common_evaluate::get_api_result() const
		 {
			 return invoke_contract_result.api_result;
		 }
		 gas_count_type contract_common_evaluate::get_gas_limit() const
		 {
			 return gas_limit;
		 }
         void contract_common_evaluate::apply_storage_change(database& d, uint32_t block_num, const transaction_id_type & trx_id) const
         {

             set<address> contracts;
             for (const auto &pair1 : invoke_contract_result.storage_changes)
             {
                 const auto &contract_id = pair1.first;

                 address contract_addr(contract_id);
                 contracts.insert(contract_addr);
                 const auto &contract_storage_changes = pair1.second;
                 for (const auto &pair2 : contract_storage_changes)
                 {
                     const auto &storage_name = pair2.first;
                     const auto &change = pair2.second;
                     d.set_contract_storage(contract_addr, storage_name, change.after);
                     d.add_contract_storage_change(trx_id, contract_addr, storage_name, change.storage_diff);
                 }
             }
             for(auto& addr:contracts)
             {
                 d.store_contract_storage_change_obj(addr, block_num);
             }
         }
         bool contract_common_evaluate::check_fee_for_gas(const address& addr, const gas_count_type& gas_count, const  gas_price_type& gas_price) const
         {
             auto obj=get_db().get_asset(GRAPHENE_SYMBOL);
             FC_ASSERT(obj.valid());
            auto balance= get_db().get_balance(addr, obj->get_id());
            return count_gas_fee(gas_price, gas_count) <= balance.amount;
         }
         void contract_common_evaluate::pay_fee_and_refund() const
		{
             if (!get_guarantee_id().valid())
             {
                 if (unspent_fee != 0)
                     get_db().adjust_balance(*caller_address, asset(unspent_fee, asset_id_type()));
             }
             else {
				 if (unspent_fee != 0)
				 {
					 auto guarantee_obj = get_db().get(*get_guarantee_id());
					 GRAPHENE_ASSERT(guarantee_obj.finished == false, guarantee_order_finished, "guarantee order has been finished", ("guarantee_id", guarantee_obj.id));
					 price p(guarantee_obj.asset_orign, guarantee_obj.asset_target);
					 auto unspent = asset(unspent_fee, asset_id_type());
					 auto unspent_to_return = unspent * p;
					 get_db().adjust_frozen(guarantee_obj.owner_addr, unspent);
					 get_db().adjust_guarantee(*get_guarantee_id(), -unspent_to_return);
					 get_db().adjust_balance(guarantee_obj.owner_addr, -unspent_to_return);
					 get_db().adjust_balance(*caller_address, unspent_to_return);
				 }
				 FC_ASSERT(gen_eval->get_trx_eval_state()->_trx != nullptr);
				 get_db().record_guarantee(*get_guarantee_id(), gen_eval->get_trx_eval_state()->_trx->id());
             }
             get_db().modify_current_collected_fee(asset(total_fee - unspent_fee, asset_id_type()));
			 for (auto fee_iter : invoke_contract_result.transfer_fees)
			 {
				 get_db().modify_current_collected_fee(asset(fee_iter.second, fee_iter.first));
			 }
		}
		 void_result contract_transfer_fee_evaluate::do_evaluate(const operation_type & o)
		 {
			 try
			 {
				 FC_ASSERT(trx_state->_is_proposed_trx);
				 const database& d = db();
				 const auto& guard_db = d.get_index_type<guard_member_index>().indices().get<by_id>();
				 auto guard_iter = guard_db.find(o.guard_id);
				 FC_ASSERT(guard_iter != guard_db.end(), "cant find this guard");
				 const auto& account_db = d.get_index_type<account_index>().indices().get<by_id>();
				 auto account_iter = account_db.find(guard_iter->guard_member_account);
				 FC_ASSERT(account_iter != account_db.end(), "cant find this account");
				 FC_ASSERT(account_iter->addr == o.guard, "guard address error");
				 o.validate();
				 return void_result();
			 }FC_CAPTURE_AND_RETHROW((o));
		 }
		 void_result contract_transfer_fee_evaluate::do_apply(const operation_type & o)
		 {
			 try
			 {
				 const auto& dynamic_properties = db().get_dynamic_global_properties();
				 db().modify(dynamic_properties, [o](dynamic_global_property_object& p) {
					 p.contract_transfer_fee_rate = o.fee_rate;
				 });
			 }FC_CAPTURE_AND_RETHROW((o));
		 }
}
}
