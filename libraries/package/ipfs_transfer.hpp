
#pragma once

#include <decent/encrypt/crypto_types.hpp>
#include <decent/encrypt/custodyutils.hpp>
#include <graphene/package/package.hpp>

#include <fc/crypto/ripemd160.hpp>
#include <fc/crypto/sha512.hpp>
#include <fc/optional.hpp>
#include <fc/signals.hpp>
#include <fc/time.hpp>
#include <fc/thread/thread.hpp>
#include <fc/thread/mutex.hpp>

#include <ipfs/client.h>

#include <boost/filesystem.hpp>

#include <string>
#include <mutex>
#include <utility>
#include <vector>


namespace graphene { namespace package {


class ipfs_transfer: public package_transfer_interface {
private:
    ipfs_transfer(const ipfs_transfer& orig);

public:
    ipfs_transfer(ipfs_transfer&&)                  = delete;
    ipfs_transfer& operator=(const ipfs_transfer&)  = delete;
    ipfs_transfer& operator=(ipfs_transfer&&)       = delete;

    ipfs_transfer();
    virtual ~ipfs_transfer();

public:
	virtual void upload_package(transfer_id id, const package_object& package, transfer_listener* listener);
	virtual void download_package(transfer_id id, const std::string& url, transfer_listener* listener);

	virtual std::string        get_transfer_url();
	virtual void               print_status();
	virtual transfer_progress  get_progress();

    virtual std::shared_ptr<package_transfer_interface> clone() {
        return std::shared_ptr<ipfs_transfer>(new ipfs_transfer(*this));
    }

private:
	package::package_object check_and_install_package();

private: // These will be shared by all clones (via clone()) of the initial instance, which in its turn is constructed only by the default c-tor.
    std::shared_ptr<fc::thread>    _thread;
    std::shared_ptr<fc::mutex>     _mutex;
    std::shared_ptr<ipfs::Client>  _client;

private:
    std::string         _url;
    transfer_id         _id;
    transfer_listener*  _listener;
    std::ofstream       _transfer_log;
    bool                _is_upload;
    transfer_progress   _last_progress;
};


} } // graphene::package
