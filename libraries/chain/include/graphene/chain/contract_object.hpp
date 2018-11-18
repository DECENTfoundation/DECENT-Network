#pragma once
#include <graphene/chain/protocol/operations.hpp>
#include <graphene/db/generic_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <graphene/chain/contract_entry.hpp>
#include <graphene/chain/vesting_balance_object.hpp>
#include <vector>
#include <graphene/chain/contract_entry.hpp>
#include <graphene/chain/contract.hpp>


namespace graphene {
    namespace chain {

        struct by_contract_id;

		struct by_contract_name {};
        class contract_object : public abstract_object<contract_object> {
        public:
            static const uint8_t space_id = protocol_ids;
            static const uint8_t type_id = contract_object_type;
            uint32_t  registered_block;
            uvm::blockchain::Code code;
            address owner_address;
            time_point_sec create_time;
            string name;
            address contract_address;
			string contract_name;
			string contract_desc;
            contract_type type_of_contract = normal_contract;
			string native_contract_key; // key to find native contract code
            vector<address> derived;
            address inherit_from;

        };
        struct by_owner{};
        struct by_contract_obj_id {};
        struct by_registered_block {};
        typedef multi_index_container<
            contract_object,
            indexed_by<
            ordered_unique<tag<by_id>, member<object, object_id_type, &object::id>>,
            ordered_unique<tag<by_contract_id>, member<contract_object, address, &contract_object::contract_address>>,
			ordered_non_unique<tag<by_contract_name>, member<contract_object, string, &contract_object::contract_name>>,
            ordered_non_unique<tag<by_owner>, member<contract_object, address, &contract_object::owner_address>>,
            ordered_non_unique<tag<by_registered_block>, member<contract_object, uint32_t, &contract_object::registered_block>>
            >> contract_object_multi_index_type;
        typedef generic_index<contract_object, contract_object_multi_index_type> contract_object_index;
        class contract_storage_change_object : public abstract_object<contract_storage_change_object> {
        public:
            static const uint8_t space_id = protocol_ids;
            static const uint8_t type_id = contract_storage_change_object_type;
            address contract_address;
            uint32_t block_num;
        };

        struct by_block_num {};
        typedef multi_index_container<
            contract_storage_change_object,
        indexed_by< 
            ordered_unique<tag<by_id>, member<object, object_id_type, &object::id>>,
            ordered_unique<tag<by_contract_id>, member<contract_storage_change_object, address, &contract_storage_change_object::contract_address>>,
            ordered_non_unique<tag<by_block_num>, member<contract_storage_change_object, uint32_t, &contract_storage_change_object::block_num>>
        >> contract_storage_change_object_multi_index_type;

        typedef generic_index<contract_storage_change_object, contract_storage_change_object_multi_index_type> contract_storage_change_index;
		class contract_storage_object : public abstract_object<contract_storage_object> {
		public:
			static const uint8_t space_id = protocol_ids;
			static const uint8_t type_id = contract_storage_object_type;

            address contract_address;
			string storage_name;
			std::vector<char> storage_value;
		};
		struct by_contract_id_storage_name {};
		typedef multi_index_container<
			contract_storage_object,
			indexed_by<
			ordered_unique<tag<by_id>, member<object, object_id_type, &object::id>>,
			ordered_unique< tag<by_contract_id_storage_name>,
			composite_key<
			contract_storage_object,
			member<contract_storage_object, address, &contract_storage_object::contract_address>,
			member<contract_storage_object, string, &contract_storage_object::storage_name>
			>
			>
			>> contract_storage_object_multi_index_type;
		typedef generic_index<contract_storage_object, contract_storage_object_multi_index_type> contract_storage_object_index;

		class contract_event_notify_object : public abstract_object<contract_event_notify_object>
		{
		public:
			static const uint8_t space_id = protocol_ids;
			static const uint8_t type_id = contract_event_notify_object_type;

            address contract_address;
			string event_name;
			string event_arg;
			transaction_id_type trx_id;
            uint64_t block_num;
            uint64_t op_num;
		};

		using contract_event_notify_multi_index_type = multi_index_container<
			contract_event_notify_object,
			indexed_by<
			ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
			ordered_non_unique<tag<by_contract_id>, member<contract_event_notify_object, address, &contract_event_notify_object::contract_address>>,
			ordered_non_unique<tag<by_block_num>, member<contract_event_notify_object, uint64_t, &contract_event_notify_object::block_num>>
			>
		>;
		using contract_event_notify_index = generic_index<contract_event_notify_object, contract_event_notify_multi_index_type>;
		class contract_history_object : public abstract_object<contract_history_object>
		{
		public:
			address contract_id;
			transaction_id_type trx_id;
			uint64_t block_num;
		};
		struct by_contract_id_and_block_num {};
		using contract_history_object_multi_index_type = multi_index_container <
			contract_history_object,
			indexed_by <
			ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
			ordered_non_unique<tag<by_contract_id_and_block_num>,
			composite_key<contract_history_object,
			member<contract_history_object, address, &contract_history_object::contract_id>,
			member<contract_history_object, uint64_t, &contract_history_object::block_num>>>
			>
			>;
		using contract_history_object_index = generic_index<contract_history_object, contract_history_object_multi_index_type>;
        class contract_invoke_result_object : public abstract_object<contract_invoke_result_object>
        {
        public:
            static const uint8_t space_id = protocol_ids;
            static const uint8_t type_id = contract_invoke_result_object_type;
            transaction_id_type trx_id;
            uint32_t block_num;
            int op_num;
            std::string api_result;
            std::vector<contract_event_notify_info> events;
            bool exec_succeed = true;
            share_type acctual_fee;
            address invoker;
            std::map<std::string, contract_storage_changes_type, comparator_for_string> storage_changes;

           std::map<std::pair<address, asset_id_type>, share_type> contract_withdraw;
           std::map<std::pair<address, asset_id_type>, share_type> contract_balances;
           std::map<std::pair<address, asset_id_type>, share_type> deposit_to_address;
           std::map<std::pair<address, asset_id_type>, share_type> deposit_contract;
		   std::map<asset_id_type, share_type> transfer_fees;
           
            inline bool operator<(const contract_invoke_result_object& obj) const
            {
                if (block_num < obj.block_num)
                    return true;
                if(block_num == obj.block_num)
                    return op_num < obj.op_num;
                return false;
            }
        };
        struct by_trxid_and_opnum {};
        struct by_trxid {};
        using contract_invoke_result_multi_index_type = multi_index_container <
            contract_invoke_result_object,
            indexed_by <
            ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
            ordered_non_unique<tag<by_trxid>, member<contract_invoke_result_object, transaction_id_type, &contract_invoke_result_object::trx_id>>,
            ordered_unique<tag<by_trxid_and_opnum>, 
            composite_key<contract_invoke_result_object,
                member<contract_invoke_result_object, transaction_id_type, &contract_invoke_result_object::trx_id>,
                member<contract_invoke_result_object, int, &contract_invoke_result_object::op_num>>>
            >
        >;
        using contract_invoke_result_index = generic_index<contract_invoke_result_object, contract_invoke_result_multi_index_type>;
        class contract_hash_entry
        {
        public:
            std::string contract_address;
            std::string hash;
        public:
            contract_hash_entry() {}
            inline contract_hash_entry(const chain::contract_object& cont)
            {
                contract_address = cont.contract_address.operator fc::string();
                hash = cont.code.GetHash();
            }
        };
    }

}
FC_REFLECT_DERIVED(graphene::chain::contract_object, (graphene::db::object),
    (registered_block)(code)(owner_address)(create_time)(name)(contract_address)(type_of_contract)(native_contract_key)(contract_name)(contract_desc)(derived)(inherit_from))
FC_REFLECT_DERIVED(graphene::chain::contract_storage_object, (graphene::db::object),
	(contract_address)(storage_name)(storage_value))
FC_REFLECT_DERIVED(graphene::chain::contract_event_notify_object, (graphene::db::object),
	(contract_address)(event_name)(event_arg)(trx_id)(block_num)(op_num))
FC_REFLECT_DERIVED(graphene::chain::contract_invoke_result_object, (graphene::db::object),
    (trx_id)(block_num)(op_num)(api_result)(events)(exec_succeed)(acctual_fee)(invoker)(contract_withdraw)(contract_balances)(deposit_to_address)(deposit_contract)(transfer_fees))
    //(contract_withdraw)(contract_balances)(deposit_to_address)(deposit_contract)
FC_REFLECT(graphene::chain::contract_hash_entry,(contract_address)(hash))

FC_REFLECT_DERIVED(graphene::chain::contract_storage_change_object, (graphene::db::object), (contract_address)(block_num))
FC_REFLECT_DERIVED(graphene::chain::contract_history_object, (graphene::db::object), (contract_id)(trx_id)(block_num))