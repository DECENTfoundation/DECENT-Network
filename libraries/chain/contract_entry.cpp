#include <graphene/chain/contract_entry.hpp>

#include <fc/array.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/crypto/elliptic.hpp>
#include <fc/crypto/base58.hpp>
#include <boost/uuid/sha1.hpp>
#include <exception>

#define PRINTABLE_CHAR(chr) \
if (chr >= 0 && chr <= 9)  \
    chr = chr + '0'; \
else \
    chr = chr + 'a' - 10; 

static std::string to_printable_hex(unsigned char chr)
{
	unsigned char high = chr >> 4;
	unsigned char low = chr & 0x0F;
	char tmp[16];

	PRINTABLE_CHAR(high);
	PRINTABLE_CHAR(low);

	snprintf(tmp, sizeof(tmp), "%c%c", high, low);
	return std::string(tmp);
}

namespace uvm {
	namespace blockchain {

		using namespace graphene::chain;

		std::string Code::GetHash() const
		{
			std::string hashstr = "";
			boost::uuids::detail::sha1 sha;
			unsigned int check_digest[5];
			sha.process_bytes(code.data(), code.size());
			sha.get_digest(check_digest);
			for (int i = 0; i < 5; ++i)
			{
				unsigned char chr1 = (check_digest[i] & 0xFF000000) >> 24;
				unsigned char chr2 = (check_digest[i] & 0x00FF0000) >> 16;
				unsigned char chr3 = (check_digest[i] & 0x0000FF00) >> 8;
				unsigned char chr4 = (check_digest[i] & 0x000000FF);

				hashstr = hashstr + to_printable_hex(chr1) + to_printable_hex(chr2) +
					to_printable_hex(chr3) + to_printable_hex(chr4);
			}
			return hashstr;
		}

        bool Code::operator!=(const Code& it) const
        {
            if (abi != it.abi)
                return true;
            if (offline_abi != it.offline_abi)
                return true;
            if (events != it.events)
                return true;
            if (storage_properties != it.storage_properties)
                return true;
            if (code != it.code)
                return true;
            if (code_hash != code_hash)
                return true;
            return false;
        }

        void Code::SetApis(char* module_apis[], int count, int api_type)
		{
			if (api_type == ContractApiType::chain)
			{
				abi.clear();
				for (int i = 0; i < count; i++)
					abi.insert(module_apis[i]);
			}
			else if (api_type == ContractApiType::offline)
			{
				offline_abi.clear();
				for (int i = 0; i < count; i++)
					offline_abi.insert(module_apis[i]);
			}
			else if (api_type == ContractApiType::event)
			{
				events.clear();
				for (int i = 0; i < count; i++)
					events.insert(module_apis[i]);
			}
		}

		bool Code::valid() const
		{
			//FC_ASSERT(0);
			//return false;
			return true;
		}

	}
}
#include <graphene/chain/contract_object.hpp>
namespace graphene {
	namespace chain {
		CodePrintAble::CodePrintAble(const uvm::blockchain::Code& code) : abi(code.abi), offline_abi(code.offline_abi), events(code.events), code_hash(code.code_hash)
		{
			unsigned char* code_buf = (unsigned char*)malloc(code.code.size());
			FC_ASSERT(code_buf, "malloc failed");

			for (int i = 0; i < code.code.size(); ++i)
			{
				code_buf[i] = code.code[i];
				printable_code = printable_code + to_printable_hex(code_buf[i]);
			}

			free(code_buf);
		}

	    ContractEntryPrintable::ContractEntryPrintable(const contract_object& obj)
	    {
            id = obj.contract_address.operator fc::string();
            owner_address = obj.owner_address;
            name = obj.contract_name;
            description = obj.contract_desc;
            type_of_contract = obj.type_of_contract;
            code_printable = obj.code;
            registered_block = obj.registered_block;
            if (obj.inherit_from != address())
            {
                inherit_from = obj.inherit_from.operator fc::string();
            }
            for (auto& addr : obj.derived)
            {
                derived.push_back(addr.operator fc::string());
            }
            createtime = obj.create_time;
	    }
	}
}
