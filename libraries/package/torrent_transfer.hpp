/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */

#pragma once

#include "detail.hpp"

#include <decent/package/package.hpp>

#include <libtorrent/session.hpp>

#include <boost/filesystem.hpp>

#include <fc/log/logger.hpp>
#include <fc/thread/thread.hpp>

#include <memory>


namespace decent { namespace package {


    namespace detail {


        struct upload_torrent_data {
            std::string creator     = "Decent";
            int piece_size          = 0;
            bool priv               = false;
            bool upload_mode        = true;
            bool super_seeding_mode = false;
            bool share_mode         = false;
            bool auto_managed       = true;
            bool announce_on_add    = true;
            bool scrape_on_add      = true;
            int max_uploads         = -1;
            int max_connections     = -1;
            int upload_limit        = -1;
            int download_limit      = -1;
            std::vector<std::string>                 url_seeds;
            std::vector<std::string>                 http_seeds;
            std::vector<std::pair<std::string, int>> dht_nodes{
                {"dht.transmissionbt.com", 6881},
                {"router.utorrent.com",    6881},
                {"router.bittorrent.com",  6881},
                {"router.bitcomet.com",    6881},
                {"dht.aelitis.com",        6881},
                {"dht.libtorrent.org",     25401}
            };
            std::vector<std::string>                 trackers{
//              "udp://tracker.opentrackr.org:1337"
            };
        };

        struct download_torrent_data {
            bool upload_mode        = true;
            bool share_mode         = false;
            bool auto_managed       = true;
            bool announce_on_add    = true;
            bool scrape_on_add      = true;
            int max_uploads         = -1;
            int max_connections     = -1;
            int upload_limit        = -1;
            int download_limit      = -1;
            std::vector<std::string>                 url_seeds;
            std::vector<std::string>                 http_seeds;
            std::vector<std::pair<std::string, int>> dht_nodes{
                {"dht.transmissionbt.com", 6881},
                {"router.utorrent.com",    6881},
                {"router.bittorrent.com",  6881},
                {"router.bitcomet.com",    6881},
                {"dht.aelitis.com",        6881},
                {"dht.libtorrent.org",     25401}
            };
            std::vector<std::string>                 trackers{
//              "udp://tracker.opentrackr.org:1337"
            };
        };


        struct libtorrent_config_data
        {
            libtorrent_config_data();

            std::map<std::string, fc::variant> settings;
            libtorrent::dht_settings dht_settings;
            upload_torrent_data upload_torrent;
            download_torrent_data download_torrent;
        };


    } // namespace detail


    class TorrentTransferEngine;


    class TorrentPackageTask : public detail::PackageTask {
    public:
        TorrentPackageTask(PackageInfo& package, TorrentTransferEngine& engine);
        virtual ~TorrentPackageTask();

    protected:
        void print_status();
        void reset_torrent_by_handle();
        void initialize_handle(const bool seed_node, const boost::filesystem::path& temp_dir_path = boost::filesystem::path());

    protected:
        TorrentTransferEngine& _engine;
        libtorrent::torrent_handle _torrent_handle;
    };


    class TorrentDownloadPackageTask : public TorrentPackageTask {
    public:
        using TorrentPackageTask::TorrentPackageTask;

    protected:
        virtual void task() override;
    };


    class TorrentStartSeedingPackageTask : public TorrentPackageTask {
    public:
        using TorrentPackageTask::TorrentPackageTask;

    protected:
        virtual void task() override;
    };


    class TorrentStopSeedingPackageTask : public TorrentPackageTask {
    public:
        using TorrentPackageTask::TorrentPackageTask;

    protected:
        virtual void task() override;
    };


    class TorrentTransferEngine : public TransferEngineInterface {
    public:
        friend class TorrentPackageTask;
        friend class TorrentDownloadPackageTask;
        friend class TorrentStartSeedingPackageTask;
        friend class TorrentStopSeedingPackageTask;

        TorrentTransferEngine();
        virtual ~TorrentTransferEngine();

        virtual std::shared_ptr<detail::PackageTask> create_download_task(PackageInfo& package) override;
        virtual std::shared_ptr<detail::PackageTask> create_start_seeding_task(PackageInfo& package) override;
        virtual std::shared_ptr<detail::PackageTask> create_stop_seeding_task(PackageInfo& package) override;

        void handle_torrent_alerts();
        void reconfigure(const boost::filesystem::path& config_file);
        void dump_config(const boost::filesystem::path& config_file);

    private:
        fc::logger                      _transfer_logger;
        detail::libtorrent_config_data  _config_data;
        mutable std::recursive_mutex    _mutex;
        fc::thread                      _thread;
        libtorrent::session             _session;
    };
    
    
} } // namespace decent::package









