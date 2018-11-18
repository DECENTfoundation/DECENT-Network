#include <graphene/chain/storage.hpp>
#include <uvm/uvm_lib.h>

#include <fc/array.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/crypto/elliptic.hpp>
#include <fc/crypto/base58.hpp>
#include <boost/uuid/sha1.hpp>
#include <exception>

namespace graphene {
	namespace chain {
		using namespace uvm::blockchain;

		void            storage_operation::validate()const
		{
		}
		share_type      storage_operation::calculate_fee(const fee_parameters_type& schedule)const
		{
			// base fee
			share_type core_fee_required = schedule.fee;
			// bytes size fee
			for (const auto &p : contract_change_storages) {
				core_fee_required += calculate_data_fee(fc::raw::pack_size(p.first), schedule.price_per_kbyte);
				core_fee_required += calculate_data_fee(fc::raw::pack_size(p.second), schedule.price_per_kbyte); // FIXME: if p.second is pointer, change to data pointed to's size
			}
			return core_fee_required;
		}

		std::map <StorageValueTypes, std::string> storage_type_map = \
		{
			make_pair(storage_value_null, std::string("nil")),
				make_pair(storage_value_int, std::string("int")),
				make_pair(storage_value_number, std::string("number")),
				make_pair(storage_value_bool, std::string("bool")),
				make_pair(storage_value_string, std::string("string")),
				make_pair(storage_value_unknown_table, std::string("Map<unknown type>")),
				make_pair(storage_value_int_table, std::string("Map<int>")),
				make_pair(storage_value_number_table, std::string("Map<number>")),
				make_pair(storage_value_bool_table, std::string("Map<bool>")),
				make_pair(storage_value_string_table, std::string("Map<string>")),
				make_pair(storage_value_unknown_array, std::string("Array<unknown type>")),
				make_pair(storage_value_int_array, std::string("Array<int>")),
				make_pair(storage_value_number_array, std::string("Array<number>")),
				make_pair(storage_value_bool_array, std::string("Array<bool>")),
				make_pair(storage_value_string_array, std::string("Array<string>"))
		};
		const uint8_t StorageNullType::type = storage_value_null;
		const uint8_t StorageIntType::type = storage_value_int;
		const uint8_t StorageNumberType::type = storage_value_number;
		const uint8_t StorageBoolType::type = storage_value_bool;
		const uint8_t StorageStringType::type = storage_value_string;
		const uint8_t StorageIntTableType::type = storage_value_int_table;
		const uint8_t StorageNumberTableType::type = storage_value_number_table;
		const uint8_t StorageBoolTableType::type = storage_value_bool_table;
		const uint8_t StorageStringTableType::type = storage_value_string_table;
		const uint8_t StorageIntArrayType::type = storage_value_int_array;
		const uint8_t StorageNumberArrayType::type = storage_value_number_array;
		const uint8_t StorageBoolArrayType::type = storage_value_bool_array;
		const uint8_t StorageStringArrayType::type = storage_value_string_array;

		StorageDataType StorageDataType::get_storage_data_from_lua_storage(const UvmStorageValue& lua_storage)
		{
			auto storage_json = uvm_storage_value_to_json(lua_storage);
			StorageDataType storage_data(jsondiff::json_dumps(storage_json));
			return storage_data;
		}

		static jsondiff::JsonValue json_from_chars(std::vector<char> data_chars)
		{
			std::vector<char> data(data_chars.size() + 1);
			memcpy(data.data(), data_chars.data(), sizeof(char) * data_chars.size());
			data[data_chars.size()] = '\0';
			std::string storage_json_str(data.data());
			return fc::json::from_string(storage_json_str);
		}

		static jsondiff::JsonValue json_from_str(const std::string &str)
		{
			return fc::json::from_string(str);
		}

		UvmStorageValue StorageDataType::create_lua_storage_from_storage_data(lua_State *L, const StorageDataType& storage)
		{
			auto json_value = json_from_str(storage.as<std::string>());
			auto value = json_to_uvm_storage_value(L, json_value);
			return value;
		}
	}
}
