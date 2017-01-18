
#pragma once

#include <fc/optional.hpp>
#include <fc/signals.hpp>
#include <fc/time.hpp>

#include <fc/crypto/ripemd160.hpp>
#include <fc/crypto/sha512.hpp>

#include <boost/filesystem.hpp>
#include <decent/encrypt/crypto_types.hpp>
#include <decent/encrypt/custodyutils.hpp>

#include <graphene/package/package.hpp>


namespace graphene { 
namespace package {

class torrent_transfer_listener: public package_transfer_interface::transfer_listener {
public:
	virtual void on_download_started(package_transfer_interface::transfer_id id) {

	}
	virtual void on_download_finished(package_transfer_interface::transfer_id id, package_object downloaded_package) { 

	}
	virtual void on_download_progress(package_transfer_interface::transfer_id id, package_transfer_interface::transfer_progress progress) { 

	}

	virtual void on_upload_started(package_transfer_interface::transfer_id id) { 

	}
	virtual void on_upload_finished(package_transfer_interface::transfer_id id) { 

	}
	virtual void on_upload_progress(package_transfer_interface::transfer_id id, package_transfer_interface::transfer_progress progress) { 

	}
};



class torrent_transfer: public package_transfer_interface {

public:
	virtual package_transfer_interface::transfer_id upload_package(const package_object& package, transfer_listener& listener) {
		return 0;
	}

	virtual package_transfer_interface::transfer_id download_package(const package_object& package, transfer_listener& listener) {
		return 0;
	}
};





} 
}
