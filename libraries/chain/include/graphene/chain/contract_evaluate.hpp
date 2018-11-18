#pragma once
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/contract.hpp>
#include <graphene/chain/contract_entry.hpp>
#include <graphene/chain/storage.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/transaction_object.hpp>
#include <graphene/chain/contract_object.hpp>
#include <graphene/chain/native_contract.hpp>
#include <graphene/chain/storage.hpp>
#include <graphene/chain/contract_engine_builder.hpp>
#include <graphene/chain/uvm_chain_api.hpp>
#include <graphene/chain/database.hpp>
#include <memory>
#include <unordered_map>
#include <map>

namespace graphene {
	namespace chain {
        share_type count_gas_fee(gas_price_type gas_price, gas_count_type gas_count);
        share_type count_contract_register_fee(const uvm::blockchain::Code& code);
		class contract_common_evaluate  {
		protected:
            generic_evaluator* gen_eval=nullptr;
			std::shared_ptr<address> caller_address;
			std::shared_ptr<fc::ecc::public_key> caller_pubkey;

            share_type total_fee;
            share_type unspent_fee;
            gas_count_type gas_used_counts;
			gas_count_type gas_limit;
			contract_invoke_result invoke_contract_result;
			share_type transfer_fee_rate = -1;

			std::set<address> related_contract;
        public:
			inline share_type get_contract_transfer_fee_rate() 
			{
				if(transfer_fee_rate==-1)
					transfer_fee_rate=get_db().get_dynamic_global_properties().contract_transfer_fee_rate; 
				return transfer_fee_rate;
			};
            inline const generic_evaluator* get_gen_eval() { return gen_eval; }
            contract_common_evaluate(generic_evaluator* gen_eval);
            virtual ~contract_common_evaluate();
			void set_contract_storage_changes(const string& contract_id, const contract_storage_changes_type& changes);
            std::shared_ptr<address> get_caller_address() const;
            std::shared_ptr<fc::ecc::public_key> get_caller_pubkey() const;
            database& get_db() const;
            StorageDataType get_storage(const string &contract_id, const string &storage_name) const;
            std::shared_ptr<uvm::blockchain::Code> get_contract_code_by_name(const string &contract_name) const;
            asset asset_from_string(const string& symbol, const string& amount);
            std::shared_ptr<uvm::blockchain::Code> get_contract_code_from_db_by_id(const string &contract_id) const;
            //void add_gas_fee(const asset& fee);
            void undo_contract_effected(const share_type& fee);
            void deposit_to_contract(const address& contract, const asset& amount);
            //void do_apply_fees_balance(const address& caller_addr);
            void do_apply_balance();
            transaction_id_type get_current_trx_id() const;
            void do_apply_contract_event_notifies();
            void transfer_to_address(const address& contract, const asset & amount, const address & to);
            share_type get_contract_balance(const address& contract, const asset_id_type& asset_id);
			void emit_event(const address& contract_addr, const string& event_name, const string& event_arg);
			virtual share_type origin_op_fee() const = 0;
            virtual  std::shared_ptr<UvmContractInfo> get_contract_by_id(const string &contract_id) const;
            virtual contract_object get_contract_by_name(const string& contract_name) const;
            virtual std::shared_ptr<uvm::blockchain::Code> get_contract_code_by_id(const string &contract_id) const;
			string get_api_result() const;
			gas_count_type get_gas_limit() const;
            void pay_fee_and_refund() const; 
            bool check_fee_for_gas(const address& addr, const gas_count_type& gas_count, const  gas_price_type& gas_price) const;
            void apply_storage_change(database& d, uint32_t block_num, const transaction_id_type & trx_id) const;

			static contract_common_evaluate* get_contract_evaluator(lua_State *L); 
            virtual optional<guarantee_object_id_type> get_guarantee_id()const = 0;
		};

		class contract_register_evaluate :public evaluator<contract_register_evaluate>,public contract_common_evaluate{
			contract_register_operation origin_op;
			contract_object new_contract;
		public:
            contract_register_evaluate():contract_common_evaluate(this){}
			typedef contract_register_operation operation_type;

            contract_operation_result_info do_evaluate(const operation_type& o);
            contract_operation_result_info do_apply(const operation_type& o);

			virtual void pay_fee() override;

			std::shared_ptr<UvmContractInfo> get_contract_by_id(const string &contract_id) const;
			std::shared_ptr<uvm::blockchain::Code> get_contract_code_by_id(const string &contract_id) const;
			address origin_op_contract_id() const;
			virtual share_type origin_op_fee() const;
            optional<guarantee_object_id_type> get_guarantee_id()const;
		};

		class native_contract_register_evaluate :public evaluator<native_contract_register_evaluate>, public contract_common_evaluate {
		private:
			native_contract_register_operation origin_op;
			contract_object new_contract;
		public:
            native_contract_register_evaluate(): contract_common_evaluate(this) {}
		public:
			typedef native_contract_register_operation operation_type;

            contract_operation_result_info do_evaluate(const operation_type& o);
            contract_operation_result_info do_apply(const operation_type& o);

			virtual void pay_fee() override;

			std::shared_ptr<UvmContractInfo> get_contract_by_id(const string &contract_id) const;
            address origin_op_contract_id() const;
			virtual share_type origin_op_fee() const;

            optional<guarantee_object_id_type> get_guarantee_id()const;
		};

		class contract_invoke_evaluate : public evaluator<contract_invoke_evaluate>, public contract_common_evaluate {
		private:
			contract_invoke_operation origin_op;
    	public:
			typedef contract_invoke_operation operation_type;
            contract_invoke_evaluate() : contract_common_evaluate(this) {}
            contract_operation_result_info do_evaluate(const operation_type& o);
            contract_operation_result_info do_apply(const operation_type& o);

			virtual void pay_fee() override;

			virtual share_type origin_op_fee() const;

            optional<guarantee_object_id_type> get_guarantee_id()const;
		};

		class contract_upgrade_evaluate : public evaluator<contract_upgrade_evaluate>, public contract_common_evaluate {
		private:
			contract_upgrade_operation origin_op;
		public:
			typedef contract_upgrade_operation operation_type;

            contract_upgrade_evaluate() : contract_common_evaluate(this) {}
            contract_operation_result_info do_evaluate(const operation_type& o);
            contract_operation_result_info do_apply(const operation_type& o);

			virtual void pay_fee() override;
			virtual share_type origin_op_fee() const;
            optional<guarantee_object_id_type> get_guarantee_id()const;
		};

        class contract_transfer_evaluate : public evaluator<contract_transfer_evaluate>, public contract_common_evaluate {
        private:
            transfer_contract_operation origin_op;
            

        public:
            typedef transfer_contract_operation operation_type;

            contract_transfer_evaluate() : contract_common_evaluate(this) {}
            contract_operation_result_info do_evaluate(const operation_type& o);
            contract_operation_result_info do_apply(const operation_type& o);
            void pay_fee();
            
			virtual share_type origin_op_fee() const;
            optional<guarantee_object_id_type> get_guarantee_id()const;
        };

		class contract_transfer_fee_evaluate : public evaluator<contract_transfer_fee_evaluate>
		{
		private:
			contract_transfer_fee_proposal_operation origin_op;


		public:
			typedef contract_transfer_fee_proposal_operation operation_type;

			contract_transfer_fee_evaluate(){}
			void_result do_evaluate(const operation_type& o);
			void_result do_apply(const operation_type& o);
			optional<guarantee_object_id_type> get_guarantee_id()const;
		};

	}
}