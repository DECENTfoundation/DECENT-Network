
#pragma once

#include "detail.hpp"

#include <graphene/package/package.hpp>

#include <ipfs/client.h>

#include <memory>


namespace decent { namespace package {


    class IPFSTransferEngine;


    class IPFSDownloadPackageTask : public detail::PackageTask {
    public:
        explicit IPFSDownloadPackageTask(PackageInfo& package);

    protected:
        virtual void task() override;

    private:
        void ipfs_recursive_get(const std::string& url, const boost::filesystem::path & dest_path );
        ipfs::Client _client;
    };


    class IPFSStartSeedingPackageTask : public detail::PackageTask {
    public:
        explicit IPFSStartSeedingPackageTask(PackageInfo& package);

    protected:
        virtual void task() override;

    private:
        ipfs::Client _client;
    };


    class IPFSStopSeedingPackageTask : public detail::PackageTask {
    public:
        explicit IPFSStopSeedingPackageTask(PackageInfo& package);

    protected:
        virtual void task() override;

    private:
        ipfs::Client _client;
    };


    class IPFSTransferEngine : public TransferEngineInterface {
    public:
        virtual std::shared_ptr<detail::PackageTask> create_download_task(PackageInfo& package) override;
        virtual std::shared_ptr<detail::PackageTask> create_start_seeding_task(PackageInfo& package) override;
        virtual std::shared_ptr<detail::PackageTask> create_stop_seeding_task(PackageInfo& package) override;
    };
    
    
} } // namespace decent::package









// ====================================================








#include <decent/encrypt/crypto_types.hpp>
#include <decent/encrypt/custodyutils.hpp>

#include <fc/crypto/ripemd160.hpp>
#include <fc/crypto/sha512.hpp>
#include <fc/log/logger.hpp>
#include <fc/optional.hpp>
#include <fc/signals.hpp>
#include <fc/time.hpp>
#include <fc/thread/thread.hpp>
#include <fc/thread/mutex.hpp>


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
	virtual void download_package(transfer_id id, const std::string& url, transfer_listener* listener, report_stats_listener_base& stats_listener);

	virtual std::string        get_transfer_url();
	virtual void               print_status();
	virtual transfer_progress  get_progress();
   virtual fc::ripemd160      hash_from_url(const std::string& url);

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
    fc::logger          _transfer_logger;
    std::string         _url;
    transfer_id         _id;
    transfer_listener*  _listener;
    bool                _is_upload;
    transfer_progress   _last_progress;
};


} } // graphene::package
