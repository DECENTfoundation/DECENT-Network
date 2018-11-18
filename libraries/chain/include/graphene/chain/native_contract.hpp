#pragma once
#include <graphene/chain/protocol/base.hpp>
#include <graphene/chain/protocol/memo.hpp>
#include <graphene/db/generic_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <graphene/chain/contract_entry.hpp>
#include <graphene/chain/contract.hpp>
#include <graphene/chain/vesting_balance_object.hpp>

namespace graphene {
	namespace chain {
		class native_contract_register_evaluate;
		class contract_common_evaluate;

		const share_type native_contract_register_fee = 1000;

#define THROW_CONTRACT_ERROR(...) FC_ASSERT(false, __VA_ARGS__)

		class abstract_native_contract
		{
		protected:
			contract_common_evaluate* _evaluate;
			contract_invoke_result _contract_invoke_result;
		public:
            address contract_id;
			abstract_native_contract(contract_common_evaluate* evaluate, const address& _contract_id) : _evaluate(evaluate), contract_id(_contract_id) {}
			virtual ~abstract_native_contract() {}

			// unique key to identify native contract
			virtual std::string contract_key() const = 0;
			virtual address contract_address() const = 0;
			virtual std::set<std::string> apis() const = 0;
			virtual std::set<std::string> offline_apis() const = 0;
			virtual std::set<std::string> events() const = 0;

			virtual contract_invoke_result invoke(const std::string& api_name, const std::string& api_arg) = 0;

			virtual gas_count_type gas_count_for_api_invoke(const std::string& api_name) const
			{
				return 100; // now all native api call requires 100 gas count
			}
			bool has_api(const string& api_name);

			void set_contract_storage(const address& contract_address, const string& storage_name, const StorageDataType& value);
			StorageDataType get_contract_storage(const address& contract_address, const string& storage_name);
			void emit_event(const address& contract_address, const string& event_name, const string& event_arg);
		};

		// FIXME: remove the demo native contract
		class demo_native_contract : public abstract_native_contract
		{
		public:
			static std::string native_contract_key() { return "demo"; }

			demo_native_contract(contract_common_evaluate* evaluate, const address& _contract_id) : abstract_native_contract(evaluate, _contract_id) {}
			virtual ~demo_native_contract() {}
			virtual std::string contract_key() const;
			virtual address contract_address() const;
			virtual std::set<std::string> apis() const;
			virtual std::set<std::string> offline_apis() const;
			virtual std::set<std::string> events() const;

			virtual contract_invoke_result invoke(const std::string& api_name, const std::string& api_arg);
		};

		// native��Լ��storage��json dumps�ͱ�����������ȷ���Ե�

		// this is native contract for token
		class token_native_contract : public abstract_native_contract
		{
		public:
			static std::string native_contract_key() { return "token"; }

			token_native_contract(contract_common_evaluate* evaluate, const address& _contract_id) : abstract_native_contract(evaluate, _contract_id) {}
			virtual ~token_native_contract() {}

			virtual std::string contract_key() const;
			virtual address contract_address() const;
			virtual std::set<std::string> apis() const;
			virtual std::set<std::string> offline_apis() const;
			virtual std::set<std::string> events() const;

			virtual contract_invoke_result invoke(const std::string& api_name, const std::string& api_arg);
			string check_admin();
			string get_storage_state();
			int64_t get_storage_supply();
			int64_t get_storage_precision();
			jsondiff::JsonObject get_storage_users();
			jsondiff::JsonObject get_storage_allowed();
			int64_t get_balance_of_user(const string& owner_addr);
			std::string get_from_address();

			contract_invoke_result init_api(const std::string& api_name, const std::string& api_arg);
			contract_invoke_result init_token_api(const std::string& api_name, const std::string& api_arg);
			contract_invoke_result transfer_api(const std::string& api_name, const std::string& api_arg);
			contract_invoke_result balance_of_api(const std::string& api_name, const std::string& api_arg);
			contract_invoke_result state_api(const std::string& api_name, const std::string& api_arg);
			contract_invoke_result supply_api(const std::string& api_name, const std::string& api_arg);
			contract_invoke_result precision_api(const std::string& api_name, const std::string& api_arg);
			// ��Ȩ��һ���û����Դ��Լ������������
			// arg format : spenderAddress, amount(with precision)
			contract_invoke_result approve_api(const std::string& api_name, const std::string& api_arg);
			// spender�û�����Ȩ����Ȩ�Ľ���з���ת��
			// arg format : fromAddress, toAddress, amount(with precision)
			contract_invoke_result transfer_from_api(const std::string& api_name, const std::string& api_arg);
			// ��ѯһ���û�������ĳ���û���Ȩ�Ľ��
			// arg format : spenderAddress, authorizerAddress
			contract_invoke_result approved_balance_from_api(const std::string& api_name, const std::string& api_arg);
			// ��ѯ�û���Ȩ�������˵����н��
			// arg format : fromAddress
			contract_invoke_result all_approved_from_user_api(const std::string& api_name, const std::string& api_arg);
			
		};

		class native_contract_finder
		{
		public:
			static bool has_native_contract_with_key(const std::string& key);
			static shared_ptr<abstract_native_contract> create_native_contract_by_key(contract_common_evaluate* evaluate, const std::string& key, const address& contract_address);

		};

		struct native_contract_register_operation : public base_operation
		{
			struct fee_parameters_type {
				uint64_t fee = 0.001 * GRAPHENE_HXCHAIN_PRECISION;
				uint32_t price_per_kbyte = 10 * GRAPHENE_BLOCKCHAIN_PRECISION; /// only required for large fields.
			};


			asset fee; // transaction fee limit
			gas_count_type init_cost; // contract init limit
			gas_price_type gas_price; // gas price of this contract transaction
			address owner_addr;
			fc::ecc::public_key owner_pubkey;
			fc::time_point_sec     register_time;
            address contract_id;
			string  native_contract_key;

			extensions_type   extensions;
			optional<guarantee_object_id_type> guarantee_id;
			optional<guarantee_object_id_type> get_guarantee_id()const { return guarantee_id; }

			address fee_payer()const { return owner_addr; }
			void            validate()const;
			share_type      calculate_fee(const fee_parameters_type& k)const;
			void get_required_authorities(vector<authority>& a)const
			{
				a.push_back(authority(1, owner_addr, 1));
			}
            address calculate_contract_id() const;
		};
	}
}

FC_REFLECT(graphene::chain::demo_native_contract, (contract_id))
FC_REFLECT(graphene::chain::native_contract_register_operation::fee_parameters_type, (fee)(price_per_kbyte))
FC_REFLECT(graphene::chain::native_contract_register_operation, (fee)(init_cost)(gas_price)(owner_addr)(owner_pubkey)(register_time)(contract_id)(native_contract_key)(guarantee_id))
