
#pragma once

#include <fc/optional.hpp>
#include <fc/signals.hpp>
#include <fc/time.hpp>

#include <fc/crypto/ripemd160.hpp>
#include <fc/crypto/sha512.hpp>

#include <boost/filesystem.hpp>


namespace graphene { 
namespace package {



class package_manager {
public:
	typedef boost::filesystem::path 					path;


public:

	package_manager(const path& content_path, const path& samples, const fc::sha512& key); //creates the package out of submitted files; encrypts the content and generates hash, generate custody file 
	package_manager(const path& package_path); //creates the package object out of downloaded package 

	
	bool unpack_package(const path& destination_directory, const fc::sha512& key); 
	bool create_package(const path& destination_directory); 
	
	const path& get_package_path() const; 

	const path& get_custody_file();  // content.cus 
	const path& get_content_file();  // content.zip.aes
	const path& get_samples_path(); 	   // Samples/

	bool verify_hash() const;
	fc::ripemd160 get_hash() const;

private:
	fc::ripemd160 	_hash; 

	path 			_content_path;
	path 			_samples;
	fc::sha512		_package_key;

	path 			_custody_file;
	path 			_content_file;
	path 			_package_path;
};






} 
}
