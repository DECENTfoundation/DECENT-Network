#pragma once
#include <graphene/chain/protocol/base.hpp>
#include <graphene/chain/protocol/memo.hpp>
#include <graphene/chain/protocol/address.hpp>
#include <graphene/chain/contract_entry.hpp>
#include <jsondiff/jsondiff.h>
#include <jsondiff/exceptions.h>
#include <uvm/uvm_lib.h>

namespace graphene {
	namespace chain {

		using namespace jsondiff;

		struct StorageDataType
		{
			std::vector<char> storage_data;
			StorageDataType() {}
			template<typename StorageType>
			StorageDataType(const StorageType& t)
			{
				storage_data = fc::raw::pack(t);
			}
			template<typename StorageType>
			StorageType as()const
			{
				return fc::raw::unpack<StorageType>(storage_data);
			}
			static StorageDataType get_storage_data_from_lua_storage(const UvmStorageValue& lua_storage);
			static UvmStorageValue create_lua_storage_from_storage_data(lua_State *L, const StorageDataType& storage_data);
		};


		struct StorageDataChangeType
		{
			StorageDataType storage_diff;
			StorageDataType before;
			StorageDataType after;
		};

		struct storage_operation : public base_operation
		{
			struct fee_parameters_type {
				uint64_t fee = 0.001 * GRAPHENE_HXCHAIN_PRECISION;
				uint32_t price_per_kbyte = 10 * GRAPHENE_BLOCKCHAIN_PRECISION; /// only required for large memos.
			};

			address contract_id;
			std::map<std::string, StorageDataChangeType> contract_change_storages;
			asset fee;
			address caller_addr;

			extensions_type   extensions;

			address fee_payer()const { return caller_addr; }
			void            validate()const;
			share_type      calculate_fee(const fee_parameters_type& k)const;

			optional<guarantee_object_id_type> guarantee_id;
			optional<guarantee_object_id_type> get_guarantee_id()const { return guarantee_id; }
		};

		struct StorageNullType
		{
			StorageNullType() : raw_storage(0) {}
			static const uint8_t    type;
			LUA_INTEGER raw_storage;
		};
		struct StorageIntType
		{
			StorageIntType() {}
			StorageIntType(LUA_INTEGER value) :raw_storage(value) {}
			static const uint8_t    type;
			LUA_INTEGER raw_storage;
		};
		struct StorageNumberType
		{
			StorageNumberType() {}
			StorageNumberType(double value) :raw_storage(value) {}
			static const uint8_t    type;
			double raw_storage;
		};
		struct StorageBoolType
		{
			StorageBoolType() {}
			StorageBoolType(bool value) :raw_storage(value) {}
			static const uint8_t    type;
			bool raw_storage;
		};
		struct StorageStringType
		{
			StorageStringType() {}
			StorageStringType(string value) :raw_storage(value) {}
			static const uint8_t    type;
			string raw_storage;
		};
		//table
		struct StorageIntTableType
		{
			StorageIntTableType() {}
			StorageIntTableType(const std::map<std::string, LUA_INTEGER>& value_map) :raw_storage_map(value_map) {}
			static const uint8_t    type;
			std::map<std::string, LUA_INTEGER> raw_storage_map;
		};
		struct StorageNumberTableType
		{
			StorageNumberTableType() {}
			StorageNumberTableType(const std::map<std::string, double>& value_map) :raw_storage_map(value_map) {}
			static const uint8_t    type;
			std::map<std::string, double> raw_storage_map;
		};
		struct StorageBoolTableType
		{
			StorageBoolTableType() {}
			StorageBoolTableType(const std::map<std::string, bool>& value_map) :raw_storage_map(value_map) {}
			static const uint8_t    type;
			std::map<std::string, bool> raw_storage_map;
		};
		struct StorageStringTableType
		{
			StorageStringTableType() {}
			StorageStringTableType(const std::map<std::string, string>& value_map) :raw_storage_map(value_map) {}
			static const uint8_t    type;
			std::map<std::string, string> raw_storage_map;
		};
		//array
		struct StorageIntArrayType
		{
			StorageIntArrayType() {}
			StorageIntArrayType(const std::map<std::string, LUA_INTEGER>& value_map) :raw_storage_map(value_map) {}
			static const uint8_t    type;
			std::map<std::string, LUA_INTEGER> raw_storage_map;
		};
		struct StorageNumberArrayType
		{
			StorageNumberArrayType() {}
			StorageNumberArrayType(const std::map<std::string, double>& value_map) :raw_storage_map(value_map) {}
			static const uint8_t    type;
			std::map<std::string, double> raw_storage_map;
		};
		struct StorageBoolArrayType
		{
			StorageBoolArrayType() {}
			StorageBoolArrayType(const std::map<std::string, bool>& value_map) :raw_storage_map(value_map) {}
			static const uint8_t    type;
			std::map<std::string, bool> raw_storage_map;
		};
		struct StorageStringArrayType
		{
			StorageStringArrayType() {}
			StorageStringArrayType(const std::map<std::string, string>& value_map) :raw_storage_map(value_map) {}
			static const uint8_t    type;
			std::map<std::string, string> raw_storage_map;
		};
	}
}


FC_REFLECT_ENUM(uvm::blockchain::StorageValueTypes,
(storage_value_null)
(storage_value_int)
(storage_value_number)
(storage_value_bool)
(storage_value_string)
(storage_value_unknown_table)
(storage_value_int_table)
(storage_value_number_table)
(storage_value_bool_table)
(storage_value_string_table)
(storage_value_unknown_array)
(storage_value_int_array)
(storage_value_number_array)
(storage_value_bool_array)
(storage_value_string_array)
)
FC_REFLECT(graphene::chain::StorageDataType,
(storage_data)
)
FC_REFLECT(graphene::chain::StorageNullType,
(raw_storage)
)
FC_REFLECT(graphene::chain::StorageIntType,
(raw_storage)
)
FC_REFLECT(graphene::chain::StorageBoolType,
(raw_storage)
)
FC_REFLECT(graphene::chain::StorageNumberType,
(raw_storage)
)
FC_REFLECT(graphene::chain::StorageStringType,
(raw_storage)
)
FC_REFLECT(graphene::chain::StorageIntTableType,
(raw_storage_map)
)
FC_REFLECT(graphene::chain::StorageBoolTableType,
(raw_storage_map)
)
FC_REFLECT(graphene::chain::StorageNumberTableType,
(raw_storage_map)
)
FC_REFLECT(graphene::chain::StorageStringTableType,
(raw_storage_map)
)
FC_REFLECT(graphene::chain::StorageIntArrayType,
(raw_storage_map)
)
FC_REFLECT(graphene::chain::StorageBoolArrayType,
(raw_storage_map)
)
FC_REFLECT(graphene::chain::StorageNumberArrayType,
(raw_storage_map)
)
FC_REFLECT(graphene::chain::StorageStringArrayType,
(raw_storage_map)
)

FC_REFLECT(graphene::chain::StorageDataChangeType, (storage_diff));

FC_REFLECT(graphene::chain::storage_operation::fee_parameters_type, (fee)(price_per_kbyte));
FC_REFLECT(graphene::chain::storage_operation, (fee)(caller_addr)(contract_id)(contract_change_storages));