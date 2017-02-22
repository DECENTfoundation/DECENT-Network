
#pragma once

#include <fc/optional.hpp>
#include <fc/signals.hpp>
#include <fc/time.hpp>

#include <fc/thread/thread.hpp>

#include <fc/crypto/ripemd160.hpp>
#include <fc/crypto/sha512.hpp>

#include <boost/filesystem.hpp>
#include <decent/encrypt/crypto_types.hpp>
#include <decent/encrypt/custodyutils.hpp>

#include <graphene/package/package.hpp>

#include <ipfs/client.h>


namespace graphene { 
namespace package {



class ipfs_transfer: public package_transfer_interface {
	ipfs_transfer(fc::thread* tthread) : _my_thread(tthread) {
		_client = new ipfs::Client("localhost", 5001);
	}

public:
	ipfs_transfer();
	~ipfs_transfer();

public:
	virtual void upload_package(transfer_id id, const package_object& package, transfer_listener* listener);
	virtual void download_package(transfer_id id, const std::string& url, transfer_listener* listener);

	virtual std::string       get_transfer_url();
	virtual void print_status();
	virtual transfer_progress get_progress();

	virtual package_transfer_interface* clone() {
		return new ipfs_transfer();
	}

private:
	
	package::package_object check_and_install_package();
	
private:
	std::string 		 _url;
	transfer_id    		 _id;
	transfer_listener*   _listener;
	std::ofstream		 _transfer_log;
	bool 				 _is_upload;
	ipfs::Client* 		 _client;
	fc::thread* 		 _my_thread;
	transfer_progress    _last_progress;
};





} 
}
