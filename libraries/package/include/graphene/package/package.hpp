
#pragma once

#include <fc/optional.hpp>
#include <fc/signals.hpp>
#include <fc/time.hpp>

#include <fc/crypto/ripemd160.hpp>
#include <fc/crypto/sha512.hpp>

#include <boost/filesystem.hpp>


namespace graphene { 
namespace package {



class PackageManager {
public:
	typedef std::vector<boost::filesystem::path>		PathList;
	typedef boost::filesystem::path 					Path;


public:

	PackageManager(const Path& contentPath, const Path& samples, const fc::sha512& key); //creates the package out of submitted files; encrypts the content and generates hash, generate custody file 
	PackageManager(const Path& packagePath); //creates the package object out of downloaded package 

	
	bool unpackPackage(const Path& destinationDirectory, const fc::sha512& key, std::string* error); 
	bool createPackage(const Path& destinationDirectory, std::string* error); 
	
	const Path& getPackagePath() const; 

	const Path& getCustodyFile();  // content.cus 
	const Path& getContentFile();  // content.zip.aes
	const Path& getSamplesPath(); 	   // Samples/

	bool verifyHash() const;
	fc::ripemd160 getHash() const;

private:
	fc::ripemd160 	_hash; 

	Path 			_contentPath;
	Path 			_samples;
	fc::sha512		_packageKey;

	Path 			_custodyFile;
	Path 			_contentFile;
	Path 			_packagePath;
};






} 
}
