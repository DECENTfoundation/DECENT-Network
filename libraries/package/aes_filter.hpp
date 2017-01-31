#ifndef AES_FILTER_HPP_INCLUDED
#define AES_FILTER_HPP_INCLUDED

#include <boost/cstdint.hpp> // uint*_t
#include <boost/iostreams/constants.hpp>   // buffer size.
#include <boost/iostreams/detail/config/dyn_link.hpp>
#include <boost/iostreams/filter/symmetric.hpp>
#include <fc/crypto/aes.hpp>

#include <stdexcept>
#include <vector>
#include <iostream>
#include <stdio.h>
#include <string.h>



namespace boost { namespace iostreams { 


const unsigned int BUFFER_SIZE  = 8*1024*1024; // 8 MB

struct aes_block_header {
	char size[4];
};

template<typename Alloc = std::allocator<char> >
class aes_encryptor_impl {
	fc::sha512			  _key;

public:
	typedef char         char_type;
public:
    aes_encryptor_impl(const fc::sha512 &key) : _key(key) {}
    ~aes_encryptor_impl() {}
    bool filter( const char*& src_begin, const char* src_end, char*& dest_begin, char* dest_end, bool flush ) {

    	std::cout << "ENCRYPTOR: SRC_SIZE=" << src_end - src_begin << " DST_SIZE=" << dest_end - dest_begin << std::endl;

    	if ( src_begin == src_end ) {
        	return false;
        }

    	std::vector<char> plain_data(src_begin, src_end);

		const std::vector<char>& encrypted_data = aes_encrypt(_key, plain_data);

		std::cout << "ENCRYPTOR: ENC_DATA_SIZE=" << encrypted_data.size() << std::endl;

		aes_block_header header;

		*(int*)header.size = encrypted_data.size();
		
		std::strncpy(dest_begin, (char*)&header, sizeof(header));

		int size_to_copy = std::min((int)encrypted_data.size(), (int)(dest_end - dest_begin - sizeof(header)));

		std::strncpy(dest_begin + sizeof(header), encrypted_data.data(), size_to_copy);

		src_begin = src_end;
		dest_begin = dest_begin + size_to_copy + sizeof(header);
    	return true;
    }
    void close() {
    	std::cout << "ENCRYPTOR::CLOSE()" << std::endl;
    }
};



template<typename Alloc = std::allocator<char> >
class aes_decrypter_impl {
	fc::sha512			  _key;

public:
	typedef char         char_type;
public:
    aes_decrypter_impl(const fc::sha512 &key) : _key(key) {}
    ~aes_decrypter_impl() {}
    bool filter( const char*& src_begin, const char* src_end, char*& dest_begin, char* dest_end, bool flush ) {
    	std::cout << "DECRYPTER: SRC_SIZE=" << src_end - src_begin << " DST_SIZE=" << dest_end - dest_begin << std::endl;


    	if ( src_begin == src_end ) {
        	return false;
        }

        aes_block_header header;
        std::strncpy((char*)&header, src_begin, sizeof(header));

		std::cout << "DECRYPTER: SIZE_TO_DECRYPT=" << *(int*)header.size << std::endl;

		const char* data_begin = src_begin + sizeof(header);
		const char* data_end = src_begin + sizeof(header) + *(int*)header.size;

    	std::vector<char> encrypted_data(data_begin, data_end);

		const std::vector<char>& plain_data = aes_decrypt(_key, encrypted_data);

		std::cout << "DECRYPTER: DEC_DATA_SIZE=" << plain_data.size() << std::endl;
		
		int size_to_copy = std::min((int)plain_data.size(), (int)(dest_end - dest_begin - sizeof(header)));
		
		std::strncpy(dest_begin, plain_data.data(), size_to_copy);

		src_begin = src_begin + sizeof(header) + *(int*)header.size;
		dest_begin = dest_begin + size_to_copy + sizeof(header);
    	return true;
    }
    void close() {
    	std::cout << "DECRYPTER::CLOSE()" << std::endl;
    }
};


template<typename Alloc = std::allocator<char> >
struct basic_aes_encryptor : symmetric_filter<aes_encryptor_impl<Alloc>, Alloc>
{
private:
	typedef aes_encryptor_impl<Alloc>  impl_type;
	typedef symmetric_filter<impl_type, Alloc>  base_type;
public:
	struct category : dual_use, filter_tag, multichar_tag, closable_tag, optimally_buffered_tag { };
	std::streamsize optimal_buffer_size() const { 
		return BUFFER_SIZE;
	}

	typedef typename base_type::char_type               char_type;
	basic_aes_encryptor(const fc::sha512& key) : base_type(BUFFER_SIZE, key) {}
};
BOOST_IOSTREAMS_PIPABLE(basic_aes_encryptor, 1)

typedef basic_aes_encryptor<> aes_encryptor;




template<typename Alloc = std::allocator<char> >
struct basic_aes_decrypter : symmetric_filter<aes_decrypter_impl<Alloc>, Alloc>
{
private:
	typedef aes_decrypter_impl<Alloc> impl_type;
	typedef symmetric_filter<impl_type, Alloc>   base_type;
public:
	struct category : dual_use, filter_tag, multichar_tag, closable_tag, optimally_buffered_tag { };
	std::streamsize optimal_buffer_size() const
	{ 
		return BUFFER_SIZE;
	}

	typedef typename base_type::char_type        char_type;
	basic_aes_decrypter(const fc::sha512& key) : base_type(BUFFER_SIZE, key) {}
};
BOOST_IOSTREAMS_PIPABLE(basic_aes_decrypter, 1)

typedef basic_aes_decrypter<> aes_decrypter;


}
}

#endif
