
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

#include <libtorrent/session.hpp>


namespace graphene { 
namespace package {



class torrent_transfer: public package_transfer_interface {

	torrent_transfer(fc::thread* tthread) : _my_thread(tthread) { }

public:
	torrent_transfer();
	~torrent_transfer();

public:
	virtual void upload_package(transfer_id id, const package_object& package, transfer_listener* listener);
	virtual void download_package(transfer_id id, const std::string& url, transfer_listener* listener);

	virtual std::string       get_transfer_url(transfer_id id);
	virtual transfer_progress get_transfer_progress(transfer_id id);


	virtual package_transfer_interface* clone() {
		return new torrent_transfer(_my_thread);
	}

private:
	void handle_torrent_alerts();
	void update_torrent_status();
	
private:
	fc::thread* 		 _my_thread;
	std::string 		 _url;
	transfer_id    		 _id;
	transfer_listener*   _listener;

	libtorrent::session         _session;
	libtorrent::torrent_handle  _torrent_handle;
};





} 
}
