
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
        uint64_t ipfs_recursive_get_size(const std::string &url);
        void     ipfs_recursive_get(const std::string &url, const boost::filesystem::path &dest_path);
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

