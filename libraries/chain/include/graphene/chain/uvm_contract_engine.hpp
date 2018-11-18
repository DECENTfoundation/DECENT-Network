#pragma once

#include <graphene/chain/contract_engine.hpp>
#include <uvm/uvm_lib.h>
#include <uvm/exceptions.h>

namespace uvm
{
	using namespace graphene::chain;

	class UvmContractEngine : public ::blockchain::contract_engine::ContractEngine
	{
	private:
		std::shared_ptr<uvm::lua::lib::UvmStateScope> _scope;
	public:
		UvmContractEngine(bool use_contract=true);
		virtual ~UvmContractEngine();

		virtual bool has_gas_limit() const;
		virtual int64_t gas_limit() const;
		virtual int64_t gas_used() const;
		virtual void set_gas_limit(int64_t gas_limit);
		virtual void set_no_gas_limit();
		virtual void set_gas_used(int64_t gas_used);
		virtual void add_gas_used(int64_t delta_used);

		virtual void stop();

		virtual void add_global_bool_variable(std::string name, bool value);
		virtual void add_global_int_variable(std::string name, int64_t value);
		virtual void add_global_string_variable(std::string name, std::string value);

		virtual void set_caller(std::string caller, std::string caller_address);

		virtual void set_state_pointer_value(std::string name, void *addr);

		virtual void clear_exceptions();

		virtual void execute_contract_api_by_address(std::string contract_id, std::string method, std::string argument, std::string *result_json_string);

		virtual void execute_contract_init_by_address(std::string contract_id, std::string argument, std::string *result_json_string);

		virtual void load_and_run_stream(void *stream);

		virtual std::shared_ptr<::blockchain::contract_engine::VMModuleByteStream> get_bytestream_from_code(const uvm::blockchain::Code& code);
	};
}
