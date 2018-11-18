#pragma once

#include <graphene/chain/protocol/base.hpp>
#include <graphene/chain/protocol/memo.hpp>
#include <graphene/chain/protocol/address.hpp>
#include <graphene/chain/storage.hpp>
#include <graphene/chain/contract_entry.hpp>
#include <graphene/chain/protocol/types.hpp>
#include <jsondiff/jsondiff.h>
#include <jsondiff/exceptions.h>
#include <uvm/uvm_lib.h>

#include <map>
#include <vector>
#include <unordered_map>

namespace graphene {
	namespace chain {
		struct contract_event_notify_info
		{
			address contract_address;
			string event_name;
			string event_arg;
            string caller_addr;
            uint64_t block_num;
            uint32_t op_num;
		};

		struct comparator_for_contract_invoke_result_balance {
            bool operator() (const std::pair<address, asset_id_type>& x, const std::pair<address, asset_id_type>& y) const
            {
                string x_addr_str = x.first.operator fc::string();
                string y_addr_str = y.first.operator fc::string();
                if (x_addr_str < y_addr_str) {
                    return true;
                }
                if (x_addr_str > y_addr_str) {
                    return false;
                }
                return (int64_t)x.second.instance < (int64_t)(y.second.instance);
            }			
		};

		struct comparator_for_string {
			bool operator() (const string& x, const string& y) const
			{
				return x < y;
			}
		};

		typedef std::map<std::string, StorageDataChangeType, comparator_for_string> contract_storage_changes_type;

		struct contract_invoke_result
		{
			std::string api_result;
			std::map<std::string, contract_storage_changes_type, std::less<std::string>> storage_changes;
				
			std::map<std::pair<address, asset_id_type>, share_type, comparator_for_contract_invoke_result_balance> contract_withdraw;
			std::map<std::pair<address, asset_id_type>, share_type, comparator_for_contract_invoke_result_balance> contract_balances;
			std::map<std::pair<address, asset_id_type>, share_type, comparator_for_contract_invoke_result_balance> deposit_to_address;
			std::map<std::pair<address, asset_id_type>, share_type, comparator_for_contract_invoke_result_balance> deposit_contract;
            std::map<asset_id_type,share_type, std::less<asset_id_type>> transfer_fees;
			std::vector<contract_event_notify_info> events;
            bool exec_succeed = true;
            share_type acctual_fee;
            address invoker;
			void reset();
            void set_failed(const share_type & fee);
			// recursive_ordered_dumps to like-json(something looks like json), and digest to string
			string ordered_digest() const;
		};

		struct contract_register_operation : public base_operation
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
			uvm::blockchain::Code  contract_code;
            address inherit_from;
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
			static address get_first_contract_id();
		};

		struct contract_upgrade_operation : public base_operation
		{
			struct fee_parameters_type {
				uint64_t fee = 0.001 * GRAPHENE_HXCHAIN_PRECISION;
				uint32_t price_per_kbyte = 10 * GRAPHENE_BLOCKCHAIN_PRECISION; /// only required for large fields.
			};


			asset fee; // transaction fee limit
			gas_count_type invoke_cost; // contract init limit
			gas_price_type gas_price; // gas price of this contract transaction
			address caller_addr;
			fc::ecc::public_key caller_pubkey;
            address contract_id;
			string contract_name;
			string contract_desc;

			extensions_type   extensions;
			optional<guarantee_object_id_type> guarantee_id;
			optional<guarantee_object_id_type> get_guarantee_id()const { return guarantee_id; }
            static bool contract_name_check(const string& contract);
			address fee_payer()const { return caller_addr; }
			void            validate()const;
			share_type      calculate_fee(const fee_parameters_type& k)const;
			void get_required_authorities(vector<authority>& a)const
			{
				a.push_back(authority(1, caller_addr, 1));
			}
		};

		struct contract_invoke_operation : public base_operation
		{
			struct fee_parameters_type {
				uint64_t fee = 0.001 * GRAPHENE_HXCHAIN_PRECISION;
				uint32_t price_per_kbyte = 10 * GRAPHENE_BLOCKCHAIN_PRECISION; /// only required for large fields.
			};


			asset fee; // transaction fee limit
			gas_count_type invoke_cost; // contract invoke gas limit
			gas_price_type gas_price; // gas price of this contract transaction
			address caller_addr;
			fc::ecc::public_key caller_pubkey;
            address contract_id;
			string contract_api;
			string contract_arg;

			extensions_type   extensions;
			optional<guarantee_object_id_type> guarantee_id;
			optional<guarantee_object_id_type> get_guarantee_id()const { return guarantee_id; }
			address fee_payer()const { return caller_addr; }
			void            validate()const;
			share_type      calculate_fee(const fee_parameters_type& k)const;
			void get_required_authorities(vector<authority>& a)const
			{
				a.push_back(authority(1, caller_addr, 1));
			}
			static inline bool contract_api_check(const contract_invoke_operation& op)
			{
				string api_name = op.contract_api;
				for (auto it : uvm::lua::lib::contract_special_api_names)
				{
					if (api_name == it)
						return false;

				}
				for (auto it : uvm::lua::lib::contract_int_argument_special_api_names)
				{
					if (api_name == it)
						return false;
				}
				for (auto it : uvm::lua::lib::contract_string_argument_special_api_names)
				{
					if (api_name == it)
						return false;
				}
				return true;
			}
		};

		struct transfer_contract_operation : public base_operation
        {
            struct fee_parameters_type {
                uint64_t fee = 0.001 * GRAPHENE_HXCHAIN_PRECISION;
                uint32_t price_per_kbyte = 10 * GRAPHENE_BLOCKCHAIN_PRECISION; /// only required for large fields.
            };

            struct transfer_param
            {
                share_type num;
                string symbol;
                string param;
            };
            asset fee; // transaction fee limit
            gas_count_type invoke_cost; // contract invoke gas limit
            gas_price_type gas_price; // gas price of this contract transaction
            address caller_addr;
            fc::ecc::public_key caller_pubkey;
            address contract_id;
            asset amount;
            std::string param;
            extensions_type   extensions;
			optional<guarantee_object_id_type> guarantee_id;
			optional<guarantee_object_id_type> get_guarantee_id()const { return guarantee_id; }
            address fee_payer()const { return caller_addr; }
            void            validate()const;
            share_type      calculate_fee(const fee_parameters_type& k)const;
            void get_required_authorities(vector<authority>& a)const
            {
                a.push_back(authority(1, caller_addr, 1));
            }
        };
		struct contract_transfer_fee_proposal_operation:public base_operation
		{

			
			struct fee_parameters_type {
				uint64_t fee = 0.001 * GRAPHENE_HXCHAIN_PRECISION;
			};
			asset fee;
			string memo;
			address guard;
			share_type fee_rate;
			guard_member_id_type guard_id;
			address fee_payer()const {
				return guard;
			}
			void            validate()const;
			share_type      calculate_fee(const fee_parameters_type& k)const;

			optional<guarantee_object_id_type> guarantee_id;
			optional<guarantee_object_id_type> get_guarantee_id()const { return guarantee_id; }

		};
		class ContractHelper
		{
		public:
			static int common_fread_int(FILE* fp, int* dst_int);
			static int common_fwrite_int(FILE* fp, const int* src_int);
			static int common_fwrite_stream(FILE* fp, const void* src_stream, int len);
			static int common_fread_octets(FILE* fp, void* dst_stream, int len);
			static std::string to_printable_hex(unsigned char chr);
			static int save_code_to_file(const string& name, UvmModuleByteStream *stream, char* err_msg);
			static uvm::blockchain::Code load_contract_from_file(const fc::path &path);
		};
		

	}
}
FC_REFLECT(graphene::chain::contract_event_notify_info,(contract_address)(caller_addr)(event_name)(event_arg)(block_num)(op_num))
FC_REFLECT(graphene::chain::contract_register_operation::fee_parameters_type, (fee)(price_per_kbyte))
FC_REFLECT(graphene::chain::contract_register_operation, (fee)(init_cost)(gas_price)(owner_addr)(owner_pubkey)(register_time)(contract_id)(contract_code)(inherit_from)(guarantee_id))
FC_REFLECT(graphene::chain::contract_invoke_operation::fee_parameters_type, (fee)(price_per_kbyte))
FC_REFLECT(graphene::chain::contract_invoke_operation, (fee)(invoke_cost)(gas_price)(caller_addr)(caller_pubkey)(contract_id)(contract_api)(contract_arg)(guarantee_id))
FC_REFLECT(graphene::chain::contract_upgrade_operation::fee_parameters_type, (fee)(price_per_kbyte))
FC_REFLECT(graphene::chain::contract_upgrade_operation, (fee)(invoke_cost)(gas_price)(caller_addr)(caller_pubkey)(contract_id)(contract_name)(contract_desc)(guarantee_id))
FC_REFLECT(graphene::chain::transfer_contract_operation::fee_parameters_type, (fee)(price_per_kbyte))
FC_REFLECT(graphene::chain::transfer_contract_operation::transfer_param, (num)(symbol)(param))
FC_REFLECT(graphene::chain::transfer_contract_operation, (fee)(invoke_cost)(gas_price)(caller_addr)(caller_pubkey)(contract_id)(amount)(param)(guarantee_id))
FC_REFLECT(graphene::chain::contract_transfer_fee_proposal_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::contract_transfer_fee_proposal_operation, (fee_rate)(memo)(guard)(guard_id))