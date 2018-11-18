/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/protocol/address.hpp>
#include <fc/crypto/elliptic.hpp>
#include <fc/crypto/base58.hpp>
#include <algorithm>

namespace graphene {
	namespace chain {

		bool address::testnet_mode =false;
		address::address() {}

		address::address(const std::string& base58str, const char *prefix_str)
		{
			if (base58str == ADDRESS_NOT_INITED)
				return;
			std::string prefix(prefix_str);
			if (testnet_mode)
				prefix += GRAPHENE_ADDRESS_TESTNET_PREFIX;
			FC_ASSERT(is_valid(base58str, prefix), "${str}", ("str", base58str + " -- " + prefix));

			std::vector<char> v = fc::from_base58(base58str.substr(prefix.size()));
			version = *(unsigned char *)v.data();
			memcpy((char*)addr._hash, v.data() + 1, std::min<size_t>(v.size() - 5, sizeof(addr)));
		}

		bool address::is_valid(const std::string& base58str, const std::string& prefix)
		{
			if (prefix.empty())
			{
				if (testnet_mode)
					return is_valid(base58str, std::string(GRAPHENE_ADDRESS_PREFIX)+ GRAPHENE_ADDRESS_TESTNET_PREFIX);
				else
					return is_valid(base58str, GRAPHENE_ADDRESS_PREFIX);
			}
				
			const size_t prefix_len = prefix.size();
			if (base58str.size() <= prefix_len)
				return false;
			if (base58str.substr(0, prefix_len) != prefix)
				return false;
			std::vector<char> v;
			try
			{
				v = fc::from_base58(base58str.substr(prefix_len));
			}
			catch (const fc::parse_error_exception& e)
			{
				return false;
			}
			if (v[0] != addressVersion::CONTRACT && v[0] != addressVersion::MULTISIG && v[0] != addressVersion::NORMAL)
				return false;
			if (v.size() != sizeof(fc::ripemd160) + 5)
				return false;

			const fc::ripemd160 checksum = fc::ripemd160::hash(v.data(), v.size() - 4);
			if (memcmp(v.data() + 21, (char*)checksum._hash, 4) != 0)
				return false;

			return true;
		}

		address::address(const fc::ecc::public_key& pub, unsigned char version)
		{
			auto dat = pub.serialize();
			addr = fc::ripemd160::hash(fc::sha512::hash(dat.data, sizeof(dat)));
			this->version = version;
		}

		std::string address::address_to_string() const
		{
			string prefix = GRAPHENE_ADDRESS_PREFIX;
			if (testnet_mode)
				prefix += GRAPHENE_ADDRESS_TESTNET_PREFIX;
			if (*this == address())
				return ADDRESS_NOT_INITED;
			fc::array<char, 25> bin_addr;
			memcpy((char*)&bin_addr, (char*)&version, sizeof(version));
			memcpy((char*)&bin_addr + 1, (char*)&addr, sizeof(addr));
			auto checksum = fc::ripemd160::hash((char*)&bin_addr, bin_addr.size() - 4);
			memcpy(((char*)&bin_addr) + 21, (char*)&checksum._hash[0], 4);
			return prefix + fc::to_base58(bin_addr.data, sizeof(bin_addr));
		}
		address::address(const pts_address& ptsaddr)
		{
			addr = fc::ripemd160::hash((char*)&ptsaddr, sizeof(ptsaddr));
		}

		address::address(const fc::ecc::public_key_data& pub, unsigned char version)
		{
			addr = fc::ripemd160::hash(fc::sha512::hash(pub.data, sizeof(pub)));
			this->version = version;
		}

		address::address(const graphene::chain::public_key_type& pub, unsigned char version)
		{
			addr = fc::ripemd160::hash(fc::sha512::hash(pub.key_data.data, sizeof(pub.key_data)));
			this->version = version;
		}

		address::operator std::string()const
		{
			return address_to_string();
		}
	}
}
namespace fc
{
    void to_variant( const graphene::chain::address& var,  variant& vo )
    {
        vo = std::string(var);
    }
	void from_variant(const variant& var, graphene::chain::address& vo)
	{
		vo = graphene::chain::address(var.as_string());
	}
}
