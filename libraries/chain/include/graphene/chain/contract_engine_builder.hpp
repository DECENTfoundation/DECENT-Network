#pragma once

#include <graphene/chain/contract_engine.hpp>
#include <graphene/chain/uvm_contract_engine.hpp>

namespace blockchain
{
	namespace contract_engine
	{
		typedef ::uvm::UvmContractEngine ActiveContractEngine;

		class ContractEngineBuilder
		{
		private:
			std::shared_ptr<ActiveContractEngine> _engine;
		public:
			virtual ~ContractEngineBuilder();
			ContractEngineBuilder *set_use_contract(bool use_contract);
			ContractEngineBuilder *set_caller(std::string caller, std::string caller_address);
			std::shared_ptr<ActiveContractEngine> build();
		};
	}
}
