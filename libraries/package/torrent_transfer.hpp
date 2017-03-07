
#pragma once

#include <decent/encrypt/crypto_types.hpp>
#include <decent/encrypt/custodyutils.hpp>
#include <graphene/package/package.hpp>

#include <fc/crypto/ripemd160.hpp>
#include <fc/crypto/sha512.hpp>
#include <fc/optional.hpp>
#include <fc/signals.hpp>
#include <fc/thread/thread.hpp>
#include <fc/time.hpp>

#include <libtorrent/session.hpp>

#include <boost/filesystem.hpp>

#include <utility>
#include <vector>
#include <string>
#include <mutex>


namespace graphene { namespace package {

namespace detail {
        struct libtorrent_config_data;
}

class torrent_transfer : public package_transfer_interface {
private:
    torrent_transfer(const torrent_transfer& orig);

public:
    torrent_transfer(torrent_transfer&&)                 = delete;
    torrent_transfer& operator=(const torrent_transfer&) = delete;
    torrent_transfer& operator=(torrent_transfer&&)      = delete;

    torrent_transfer();
    virtual ~torrent_transfer();

public:
    virtual void upload_package(transfer_id id, const package_object& package, transfer_listener* listener);
    virtual void download_package(transfer_id id, const std::string& url, transfer_listener* listener, report_stats_listener_base& stats_listener);

    virtual std::string get_transfer_url();
    virtual void        print_status();
    virtual transfer_progress get_progress();

    virtual std::shared_ptr<package_transfer_interface> clone() {
        return std::shared_ptr<torrent_transfer>(new torrent_transfer(*this));
    }

    virtual void reconfigure(const boost::filesystem::path& config_file);
    virtual void dump_config(const boost::filesystem::path& config_file);

private:
    void handle_torrent_alerts();
    void update_torrent_status();
    package::package_object check_and_install_package();

private: // These will be shared by all clones (via clone()) of the initial instance, which in its turn is constructed only by the default c-tor.
    std::shared_ptr<fc::thread>                      _thread;
    std::shared_ptr<libtorrent::session>             _session;
    std::shared_ptr<detail::libtorrent_config_data>  _config_data;

private: // These are used to maintain instance lifetime info, and will be shared by all async callbacks that access this instance.
    std::shared_ptr<fc::mutex>                 _lifetime_info_mutex;
    std::shared_ptr<std::atomic<bool>>         _instance_exists;

private:
    std::string                 _url;
    transfer_id                 _id;
    transfer_listener*          _listener;
    std::ofstream               _transfer_log;
    bool                        _is_upload;
    libtorrent::torrent_handle  _torrent_handle;
};


} } // graphene::package
