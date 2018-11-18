#pragma once
#include <stdint.h>
#include <string>
#include <memory>
#include <graphene/chain/contract_entry.hpp>

namespace blockchain
{
	namespace contract_engine
	{

		typedef UvmModuleByteStream VMModuleByteStream;

		class ContractEngine
		{
		private:
		public:
			virtual ~ContractEngine();

			virtual bool has_gas_limit() const = 0;
			virtual int64_t gas_limit() const = 0;
			virtual int64_t gas_used() const = 0;
			virtual void set_gas_limit(int64_t gas_limit) = 0;
			virtual void set_no_gas_limit() = 0;
			virtual void set_gas_used(int64_t gas_used) = 0;
			virtual void add_gas_used(int64_t delta_used) = 0;

			virtual void stop()=0;

			virtual void add_global_bool_variable(std::string name, bool value) = 0;
			virtual void add_global_int_variable(std::string name, int64_t value) = 0;
			virtual void add_global_string_variable(std::string name, std::string value) = 0;

			virtual void set_caller(std::string caller, std::string caller_address) = 0;

			virtual void set_state_pointer_value(std::string name, void *addr) = 0;

			virtual void clear_exceptions() = 0;

			// @throws exception
			virtual void execute_contract_api_by_address(std::string contract_id, std::string method, std::string argument, std::string *result_json_string) = 0;

			// @throws exception
			virtual void execute_contract_init_by_address(std::string contract_id, std::string argument, std::string *result_json_string) = 0;

			virtual void load_and_run_stream(void *stream) = 0;

			virtual std::shared_ptr<VMModuleByteStream> get_bytestream_from_code(const uvm::blockchain::Code& code) = 0;
		};
	}
}
