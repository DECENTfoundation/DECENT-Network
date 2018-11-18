#include <graphene/chain/database.hpp>
#include <graphene/chain/balance_object.hpp>
#include <graphene/chain/contract.hpp>
#include <graphene/chain/storage.hpp>
#include <graphene/chain/contract_entry.hpp>
#include <graphene/chain/transaction_object.hpp>
#include <graphene/chain/witness_object.hpp>
#include <graphene/chain/contract_object.hpp>
#include <uvm/uvm_api.h>

namespace graphene {
	namespace chain {
		StorageDataType database::get_contract_storage(const address& contract_id, const string& name)
		{
			try {
				auto& storage_index = get_index_type<contract_storage_object_index>().indices().get<by_contract_id_storage_name>();
				auto storage_iter = storage_index.find(boost::make_tuple(contract_id, name));
				if (storage_iter == storage_index.end())
				{
					std::string null_jsonstr("null");
					return StorageDataType(null_jsonstr);
				}
				else
				{
					const auto &storage_data = *storage_iter;
					StorageDataType storage;
					storage.storage_data = storage_data.storage_value;
					return storage;
				}
			} FC_CAPTURE_AND_RETHROW((contract_id)(name));
		}

		void database::set_contract_storage(const address& contract_id, const string& name, const StorageDataType &value)
		{
			try {
				/*auto& index = get_index_type<contract_object_index>().indices().get<by_contract_id>();
				auto itr = index.find(contract_id);
				FC_ASSERT(itr != index.end());*/

				auto& storage_index = get_index_type<contract_storage_object_index>().indices().get<by_contract_id_storage_name>();
				auto storage_iter = storage_index.find(boost::make_tuple(contract_id, name));
				if (storage_iter == storage_index.end()) {
					create<contract_storage_object>([&](contract_storage_object & obj) {
						obj.contract_address = contract_id;
						obj.storage_name = name;
						obj.storage_value = value.storage_data;
					});
				}
				else {
					modify(*storage_iter, [&](contract_storage_object& obj) {
						obj.storage_value = value.storage_data;
					});
				}
			} FC_CAPTURE_AND_RETHROW((contract_id)(name)(value));
		}

		void database::set_contract_storage_in_contract(const contract_object& contract, const string& name, const StorageDataType& value)
		{
			try {
				set_contract_storage(contract.contract_address, name, value);
			} FC_CAPTURE_AND_RETHROW((contract.contract_address)(name)(value));
		}

		void database::add_contract_storage_change(const transaction_id_type& trx_id, const address& contract_id, const string& name, const StorageDataType &diff)
		{
			try {
				transaction_contract_storage_diff_object obj;
				obj.contract_address = contract_id;
				obj.storage_name = name;
				obj.diff = diff.storage_data;
				obj.trx_id = trx_id;
				create<transaction_contract_storage_diff_object>([&](transaction_contract_storage_diff_object & o) {
					o.contract_address = obj.contract_address;
					o.diff = obj.diff;
					o.storage_name = obj.storage_name;
					o.trx_id = obj.trx_id;
				});
			} FC_CAPTURE_AND_RETHROW((trx_id)(contract_id)(name)(diff));
		}
        void  database::store_contract_storage_change_obj(const address& contract, uint32_t block_num)
		{
            try {
                auto& conn_db = get_index_type<contract_storage_change_index>().indices().get<by_contract_id>();
                auto it = conn_db.find(contract);
                if (it == conn_db.end())
                {
                    create<contract_storage_change_object>([&](contract_storage_change_object& o)
                    {
                        o.contract_address = contract;
                        o.block_num = block_num;
                    });
                }else
                {
                    modify<contract_storage_change_object>(*it, [&](contract_storage_change_object& obj) {obj.block_num = block_num; });
                }
            }FC_CAPTURE_AND_RETHROW((contract)(block_num));
		}
        vector<contract_blocknum_pair> database::get_contract_changed(uint32_t block_num, uint32_t duration)
		{
            try
            {
                contract_blocknum_pair tmp;
                vector<contract_blocknum_pair> res;
                auto& conn_db = get_index_type<contract_storage_change_index>().indices().get<by_block_num>();
                auto lb = conn_db.lower_bound(block_num);
                uint32_t last_block_num = block_num + duration;
                if(duration==0)
                {
                    last_block_num = head_block_num();
                }
                auto ub = conn_db.lower_bound(last_block_num);
                while(lb!=ub)
                {
                    tmp.block_num = lb->block_num;
                    tmp.contract_address = lb->contract_address.operator fc::string();
                    res.push_back(tmp);
                    lb++;
                }
                return res;
            }FC_CAPTURE_AND_RETHROW((block_num)(duration));
		}
		void database::add_contract_event_notify(const transaction_id_type& trx_id, const address& contract_id, const string& event_name, const string& event_arg, uint64_t block_num, uint64_t
		                                         op_num)
		{
			try {
				contract_event_notify_object obj;
				obj.trx_id = trx_id;
				obj.contract_address = contract_id;
				obj.event_name = event_name;
				obj.event_arg = event_arg;
                obj.block_num = block_num;
                obj.op_num = op_num;
				auto& conn_db = get_index_type<contract_event_notify_index>().indices().get<by_contract_id>();
				create<contract_event_notify_object>([&](contract_event_notify_object & o) {
					o.contract_address = obj.contract_address;
					o.trx_id = obj.trx_id;
					o.event_name = obj.event_name;
					o.event_arg = obj.event_arg;
                    o.block_num = obj.block_num;
                    o.op_num = obj.op_num;

				});
			} FC_CAPTURE_AND_RETHROW((trx_id)(contract_id)(event_name)(event_arg));
		}
        bool object_id_type_comp(const object& obj1, const object& obj2)
        {
            return obj1.id < obj2.id;
        }
        vector<contract_event_notify_object> database::get_contract_event_notify(const address & contract_id, const transaction_id_type & trx_id, const string& event_name)
        {
            try {
                bool trx_id_check = true;
                if (trx_id == transaction_id_type())
                    trx_id_check = false;
                bool event_check = true;
                if (event_name == "")
                    event_check = false;
                vector<contract_event_notify_object> res;
                auto& conn_db=get_index_type<contract_event_notify_index>().indices().get<by_contract_id>();
                auto lb=conn_db.lower_bound(contract_id);
                auto ub = conn_db.upper_bound(contract_id);
                while (lb!=ub)
                {
                    if (trx_id_check&&lb->trx_id != trx_id)
                    {
                        lb++;
                        continue;
                    }
                    if (event_check&&lb->event_name != event_name)
                    {
                        lb++;
                        continue;
                    }
                    res.push_back(*lb);
                    lb++;
                }
                std::sort(res.begin(), res.end(), object_id_type_comp);
                return res;
            } FC_CAPTURE_AND_RETHROW((contract_id)(trx_id)(event_name));
        }
        vector<contract_invoke_result_object> database::get_contract_invoke_result(const transaction_id_type& trx_id)const
        {
            try {
                vector<contract_invoke_result_object> res;
                auto& res_db = get_index_type<contract_invoke_result_index>().indices().get<by_trxid>();
                auto it = res_db.lower_bound(trx_id);
                auto end_it= res_db.upper_bound(trx_id);
                while(it!=end_it)
                {
                    res.push_back(*it);
                    it++;
                }
                std::sort(res.begin(), res.end());
                return res;
            }FC_CAPTURE_AND_RETHROW((trx_id))
        }
		void graphene::chain::database::store_contract_related_transaction(const transaction_id_type& tid, const address& contract_id)
		{
			auto block_num = head_block_num()+1;
			create<contract_history_object>([tid, contract_id, block_num](contract_history_object& obj) {
				obj.block_num = block_num;
				obj.contract_id = contract_id;
				obj.trx_id = tid;
			});
		}

		std::vector<graphene::chain::transaction_id_type> database::get_contract_related_transactions(const address& contract_id, uint64_t start, uint64_t end)
		{
			std::vector<graphene::chain::transaction_id_type> res;
			if (end < start)
				return res;
			auto head_num = head_block_num();
			if (head_num<end)
			{
				end = head_num;
			}
			auto& res_db = get_index_type<contract_history_object_index>().indices().get<by_contract_id_and_block_num>();
			auto start_it = res_db.lower_bound(std::make_tuple(contract_id,start));
			auto end_it = res_db.upper_bound(std::make_tuple(contract_id, end));
			while (start_it != end_it)
			{
				res.push_back(start_it->trx_id);
				start_it++;
			}
			return res;

		}
        void database::store_invoke_result(const transaction_id_type& trx_id, int op_num, const contract_invoke_result& res)
        {
            try {
                auto& con_db = get_index_type<contract_invoke_result_index>().indices().get<by_trxid_and_opnum>();
                auto con = con_db.find(boost::make_tuple(trx_id,op_num));
                auto block_num=head_block_num();
                address invoker = res.invoker;
                if (con == con_db.end())
                {
                    create<contract_invoke_result_object>([res,trx_id, op_num, block_num, invoker](contract_invoke_result_object & obj) {
                        obj.acctual_fee = res.acctual_fee;
                        obj.api_result = res.api_result;
                        obj.exec_succeed = res.exec_succeed;
                        obj.events = res.events;
                        obj.trx_id = trx_id;
                        obj.block_num = block_num+1;
                        obj.op_num = op_num;
                        obj.invoker = invoker;
                        for (auto it = res.contract_withdraw.begin(); it != res.contract_withdraw.end(); it++)
                        {
                            obj.contract_withdraw.insert( make_pair(it->first,it->second));
                        }
                        for (auto it = res.contract_balances.begin(); it != res.contract_balances.end(); it++)
                        {
                            obj.contract_balances.insert(make_pair(it->first, it->second));
                        }
                        for (auto it = res.deposit_contract.begin(); it != res.deposit_contract.end(); it++)
                        {
                            obj.deposit_contract.insert(make_pair(it->first, it->second));
                        }
                        for (auto it = res.deposit_to_address.begin(); it != res.deposit_to_address.end(); it++)
                        {
                            obj.deposit_to_address.insert(make_pair(it->first, it->second));
                        }
						for (auto it = res.transfer_fees.begin(); it != res.transfer_fees.end(); it++)
						{
							obj.transfer_fees.insert(make_pair(it->first, it->second));
						}
                    });     
                }
                else
                {
                    FC_ASSERT(false, "result exsited");
                }
            }FC_CAPTURE_AND_RETHROW((trx_id))
        }
        class event_compare
        {
		public:
            const database* _db;
            event_compare(const database* _db):_db(_db){}
            bool operator()(const contract_event_notify_object& obj1,const contract_event_notify_object& obj2)
            {
                if (obj1.block_num < obj2.block_num)
                    return true;
                if (obj1.block_num > obj2.block_num)
                    return false;
                auto blk=_db->fetch_block_by_number(obj1.block_num);
                FC_ASSERT(blk.valid());
                for(auto trx:blk->transactions)
                {
                    if (trx.id() == obj1.trx_id)
                        return true;
                    if (trx.id() == obj2.trx_id)
                        return false;
                }
                return false;
            }
        };
        vector<contract_event_notify_object> database::get_contract_events_by_contract_ordered(const address &addr) const
		{
            vector<contract_event_notify_object> res;
            auto& con_db = get_index_type<contract_event_notify_index>().indices().get<by_contract_id>();

            auto evit = con_db.lower_bound(addr);
            auto ubit = con_db.upper_bound(addr);
            while(evit != ubit )
            {
                res.push_back(*evit);
                evit++;
            }
            event_compare cmp(this);
            sort(res.begin(),res.end(), cmp);
            return res;
		}
		vector<contract_event_notify_object> database::get_contract_events_by_block_and_addr_ordered(const address &addr, uint64_t start, uint64_t range) const
		{
			vector<contract_event_notify_object> res;
			auto& con_db = get_index_type<contract_event_notify_index>().indices().get<by_block_num>();

			auto evit = con_db.lower_bound(start);
			auto ubit = con_db.upper_bound(start+ range);
			while (evit != ubit)
			{
				if(evit->contract_address==addr)
					res.push_back(*evit);
				evit++;
			}
			event_compare cmp(this);
			sort(res.begin(), res.end(), cmp);
			return res;
		}

        vector<contract_object> database::get_registered_contract_according_block(const uint32_t start_with, const uint32_t num)const
        {
            vector<contract_object> res;
            auto& con_db = get_index_type<contract_object_index>().indices().get<by_registered_block>();
            auto evit = con_db.lower_bound(start_with);

            auto ubit = con_db.upper_bound(start_with + num);
            if (num == 0|| start_with + num<start_with)
                ubit = con_db.upper_bound(head_block_num());
            while (evit != ubit)
            {
                res.push_back(*evit);
                evit++;
            }
            return res;
        }
        void database::store_contract(const contract_object & contract)
        {
            try {
            auto& con_db = get_index_type<contract_object_index>().indices().get<by_contract_id>();
            auto con = con_db.find(contract.contract_address);
            if (con == con_db.end())
            {
                create<contract_object>([contract](contract_object & obj) {
                    obj.create_time = contract.create_time;
                    obj.code = contract.code; 
                    obj.name = contract.name;
                    obj.owner_address = contract.owner_address;
					obj.contract_name = contract.contract_name;
					obj.contract_desc = contract.contract_desc;
                    obj.contract_address = contract.contract_address;
					obj.type_of_contract = contract.type_of_contract;
					obj.native_contract_key = contract.native_contract_key;
                    obj.inherit_from = contract.inherit_from;
                    obj.derived = contract.derived;
                    obj.registered_block = contract.registered_block;
                });
            }
            else
            {
                FC_ASSERT( false,"contract exsited");
            }
            }FC_CAPTURE_AND_RETHROW((contract))
        }

		void database::update_contract(const contract_object& contract)
		{
			try {
				auto& con_db = get_index_type<contract_object_index>().indices().get<by_contract_id>();
				auto con = con_db.find(contract.contract_address);
				if (con != con_db.end())
				{
					modify(*con, [&](contract_object& obj) {
						obj.create_time = contract.create_time;
						obj.code = contract.code;
						obj.name = contract.name;
						obj.owner_address = contract.owner_address;
						obj.contract_name = contract.contract_name;
						obj.contract_desc = contract.contract_desc;
						obj.contract_address = contract.contract_address;
						obj.type_of_contract = contract.type_of_contract;
						obj.native_contract_key = contract.native_contract_key;
                        obj.inherit_from = contract.inherit_from;
                        obj.derived = contract.derived;
                        obj.registered_block = contract.registered_block;
					});
				}
				else
				{
					FC_ASSERT(false, "contract not exsited");
				}
			}FC_CAPTURE_AND_RETHROW((contract))
		}

        contract_object database::get_contract(const address & contract_address)
        {
            contract_object res;
            auto& index = get_index_type<contract_object_index>().indices().get<by_contract_id>();
            auto itr = index.find(contract_address);
            FC_ASSERT(itr != index.end());
            res =*itr;
            if(res.inherit_from!= address())
            {
                res.code = get_contract(res.inherit_from).code;
            }
            return res;
        }

        contract_object database::get_contract(const string& name_or_id)
        {
            auto& index_name = get_index_type<contract_object_index>().indices().get<by_contract_name>();
            auto itr = index_name.find(name_or_id);
            if(itr != index_name.end())
            {
                contract_object res;
                res = *itr;
                if (res.inherit_from != address())
                {
                    res.code = get_contract(res.inherit_from).code;
                }
                return res;
            }
            return get_contract(address(name_or_id));
        }
        contract_object database::get_contract(const contract_id_type & id)
        {
            auto& index = get_index_type<contract_object_index>().indices().get<by_id>();
            auto itr = index.find(id);
            FC_ASSERT(itr != index.end());
            return *itr;
        }

		contract_object database::get_contract_of_name(const string& contract_name)
		{
			auto& index = get_index_type<contract_object_index>().indices().get<by_contract_name>();
			auto itr = index.find(contract_name);
			FC_ASSERT(itr != index.end());
			return *itr;
		}
        vector<contract_object> database::get_contract_by_owner(const address& owner)
		{
            vector<contract_object> res;
            auto& index = get_index_type<contract_object_index>().indices().get<by_owner>();
            auto itr = index.lower_bound(owner);
            auto itr_end = index.upper_bound(owner);
            while(itr!= itr_end)
            {
                res.push_back(*itr);
                itr++;
            }
            return res;
		}
        vector<address> database::get_contract_address_by_owner(const address& owner)
		{
            vector<address> res;
            auto& index = get_index_type<contract_object_index>().indices().get<by_owner>();
            auto itr = index.lower_bound(owner);
            auto itr_end = index.upper_bound(owner);
            while (itr != itr_end)
            {
                res.push_back(itr->contract_address);
                itr++;
            }
            return res;
		}
		bool database::has_contract(const address& contract_address, const string& method/*=""*/)
		{
			auto& index = get_index_type<contract_object_index>().indices().get<by_contract_id>();
			auto itr = index.find(contract_address);
			if(method=="")
				return itr != index.end();
			auto& apis = itr->code.abi;
			auto& offline_apis = itr->code.offline_abi;
			return apis.find(method) != apis.end()|| offline_apis.find(method)!= offline_apis.end();
		}

        void database::set_min_gas_price(const share_type min_price)
        {
            _min_gas_price = min_price;
        }
        share_type database::get_min_gas_price() const
        {
            return _min_gas_price;
        }
		bool database::has_contract_of_name(const string& contract_name)
		{
			auto& index = get_index_type<contract_object_index>().indices().get<by_contract_name>();
			auto itr = index.find(contract_name);
			return itr != index.end();
		}

        address database::get_account_address(const string& name)
        {
            auto& db = get_index_type<account_index>().indices().get<by_name>();
            auto it = db.find(name);
            if (it != db.end())
            {
                return it->addr;
            }
            return address();
        }


	}
}
