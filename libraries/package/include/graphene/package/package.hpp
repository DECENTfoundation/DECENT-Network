
#pragma once

#include <decent/encrypt/custodyutils.hpp>
#include <graphene/utilities/dirhelper.hpp>

#include <fc/thread/thread.hpp>
#include <fc/thread/mutex.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/crypto/sha256.hpp>
#include <fc/network/url.hpp>

#include <boost/filesystem.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>


/*
Quick package user guide
1. you as before can't construct the PackageManager instance, but have to access the singleton via
 `decent::package::PackageManager::instance();`
2. you can't create package handlers manuall but have to ask package manager to allocate one for you via
 `package_manager_instance.get_package(...)`
3. package handlers are shared pointers around PackageInfo class
4. package handlers can be explicitly released via `package_manager_instance.release_package(...)` but this is not
 nececarry, unless you really need to free any unused resource and "unlock" the package data folder
5. to perform a task, one of `create()`, `unpack()`, `download()`, `start_seeding()`, `stop_seeding()`, `check()`, and
 `remove()` can be called on a valid package handle. Each of these can be either blocking or non-blocking
6. you can wait for current (blocking or non-blocking) task completeion with `wait_for_current_task()` called on a
 valid package handle
7. you can cancel current task via `cancel_current_task()` called on a valid package handle (each task at certain
 points check for cancellation request, hence some operations must finish to task be able to self-terminate)
8. you can query data, transfer, and package manipulation states via `get_data_state()`, `get_transfer_state()`, and
 `get_manipulation_state()` called on a valid package handle
9. these and other task-related events are delivered to events listeners, that can be registered at package handle via
 `add_event_listener()` (info that is delivered with each callbacks can and should be tuned in the code as required --
 this is just a facility yet)
10. `create()`, and `download()` can be called only on those handles which were created by the appropriate
 `get_package()`
11. `recover_all_packages()` called at package manager instance tries to create handles for each package that it will
 be able to detect in current package root folder
 */

 
namespace decent { namespace package {


    class PackageManager;
    class PackageInfo;
    class EventListenerInterface;
    class TransferEngineInterface;
    class IPFSTransferEngine;
    class IPFSDownloadPackageTask;
    class IPFSStartSeedingPackageTask;
    class IPFSStopSeedingPackageTask;
    class TorrentTransferEngine;
    class TorrentPackageTask;
    class TorrentDownloadPackageTask;
    class TorrentStartSeedingPackageTask;
    class TorrentStopSeedingPackageTask;


    namespace detail {


        class PackageTask;
        class CreatePackageTask;
        class RemovePackageTask;
        class UnpackPackageTask;
        class CheckPackageTask;


    } // namespace detail


    typedef std::shared_ptr<PackageInfo>                package_handle_t;
    typedef std::set<package_handle_t>                  package_handle_set_t;
    typedef std::shared_ptr<EventListenerInterface>     event_listener_handle_t;
    typedef std::list<event_listener_handle_t>          event_listener_handle_list_t;
    typedef std::shared_ptr<TransferEngineInterface>    transfer_engine_t;
    typedef std::map<std::string, transfer_engine_t>    proto_to_transfer_engine_map_t;


    class PackageInfo {
    public:
        enum DataState {
            DS_UNINITIALIZED = 0,
            INVALID,
            PARTIAL,
            UNCHECKED,
            CHECKED
        };

        enum TransferState {
            TS_IDLE = 0,
            DOWNLOADING,
            SEEDING,
        };

        enum ManipulationState {
            MS_IDLE = 0,
            PACKING,
            ENCRYPTING,
            STAGING,
            CHECKING,
            DECRYPTING,
            UNPACKING,
            DELETTING
        };

    private:
        friend class PackageManager;

        friend class IPFSDownloadPackageTask;
        friend class IPFSStartSeedingPackageTask;
        friend class IPFSStopSeedingPackageTask;
        friend class TorrentPackageTask;
        friend class TorrentDownloadPackageTask;
        friend class TorrentStartSeedingPackageTask;
        friend class TorrentStopSeedingPackageTask;

        friend class detail::CreatePackageTask;
        friend class detail::RemovePackageTask;
        friend class detail::UnpackPackageTask;
        friend class detail::CheckPackageTask;


        PackageInfo(PackageManager& manager,
                    const boost::filesystem::path& content_dir_path,
                    const boost::filesystem::path& samples_dir_path,
                    const fc::sha256& key);

        PackageInfo(PackageManager& manager,const fc::ripemd160& package_hash);

        PackageInfo(PackageManager& manager, const std::string& url);

    public:
        PackageInfo(const PackageInfo&)             = delete;
        PackageInfo(PackageInfo&&)                  = delete;
        PackageInfo& operator=(const PackageInfo&)  = delete;
        PackageInfo& operator=(PackageInfo&&)       = delete;

        ~PackageInfo();

    public:
        void add_event_listener(const event_listener_handle_t& event_listener);
        void remove_event_listener(const event_listener_handle_t& event_listener);
        void remove_all_event_listeners();

        DataState          get_data_state() const;
        TransferState      get_transfer_state() const;
        ManipulationState  get_manipulation_state() const;

        void create(bool block = false);
        void unpack(const boost::filesystem::path& dir_path, const fc::sha256& key, bool block = false);
        void download(bool block = false);
        void start_seeding(std::string proto = "", bool block = false);
        void stop_seeding(std::string proto = "", bool block = false);
        void check(bool block = false);
        void remove(bool block = false);
        void create_proof_of_custody(const decent::encrypt::CustodyData& cd, decent::encrypt::CustodyProof& proof)const;

        void wait_for_current_task();
        void cancel_current_task(bool block = false);
        std::exception_ptr get_task_last_error() const;

    private:
        static boost::filesystem::path get_package_state_dir(const boost::filesystem::path& package_dir)  { return package_dir / ".state" ; }
        static boost::filesystem::path get_lock_file_path(const boost::filesystem::path& package_dir)     { return get_package_state_dir(package_dir) / "lock"; }

        void lock_dir();
        void unlock_dir();

        boost::filesystem::path get_package_state_dir() const  { return get_package_state_dir(get_package_dir()); }
        boost::filesystem::path get_lock_file_path() const     { return get_lock_file_path(get_package_dir()); }
        boost::filesystem::path get_custody_file() const       { return get_package_dir() / "content.cus"; }
        boost::filesystem::path get_content_file() const       { return get_package_dir() / "content.zip.aes"; }
        boost::filesystem::path get_samples_path() const       { return get_package_dir() / "samples"; }

    public:
        boost::filesystem::path get_package_dir() const        { return _parent_dir / _hash.str(); }
        std::string             get_url() const                { return _url; }
        fc::ripemd160           get_hash() const               { return _hash; }
        uint64_t                get_size() const;
        uint64_t                get_downloaded_size() const    { return _downloaded_size; }
        uint64_t                get_total_size() const         { return _size; }
        decent::encrypt::CustodyData get_custody_data() const  { return _custody_data; };


    private:
        mutable std::recursive_mutex  _mutex;

        DataState                     _data_state;
        TransferState                 _transfer_state;
        ManipulationState             _manipulation_state;

        fc::ripemd160                 _hash;
        std::string                   _url;
        boost::filesystem::path       _parent_dir;
        decent::encrypt::CustodyData  _custody_data;
        uint64_t                      _size;
        uint64_t                      _downloaded_size;


        std::shared_ptr<boost::interprocess::file_lock> _file_lock;
        std::shared_ptr<boost::interprocess::scoped_lock<boost::interprocess::file_lock>> _file_lock_guard;

        mutable std::recursive_mutex  _event_mutex;
        event_listener_handle_list_t  _event_listeners;

        mutable std::recursive_mutex                  _task_mutex;
        std::shared_ptr<detail::CreatePackageTask>    _create_task;
        std::shared_ptr<detail::PackageTask>          _download_task;
        std::shared_ptr<detail::PackageTask>          _current_task;
    };


    class TransferListenerInterface {
    public:
        virtual ~TransferListenerInterface() {}

        virtual void package_seed_start() {}
        virtual void package_seed_progress() {}
        virtual void package_seed_error(const std::string&) {}
        virtual void package_seed_complete() {}

        virtual void package_download_start() {}
        virtual void package_download_progress() {}
        virtual void package_download_error(const std::string&) {}
        virtual void package_download_complete() {}
    };


    class EventListenerInterface : public TransferListenerInterface {
    public:
        virtual ~EventListenerInterface() {}

        virtual void package_data_state_change(PackageInfo::DataState) {}
        virtual void package_transfer_state_change(PackageInfo::TransferState) {}
        virtual void package_manipulation_state_change(PackageInfo::ManipulationState) {}

        virtual void package_creation_start() {};
        virtual void package_creation_progress() {}
        virtual void package_creation_error(const std::string&) {}
        virtual void package_creation_complete() {}

        virtual void package_restoration_start() {}
        virtual void package_restoration_progress() {}
        virtual void package_restoration_error(const std::string&) {}
        virtual void package_restoration_complete() {}

        virtual void package_extraction_start() {}
        virtual void package_extraction_progress() {}
        virtual void package_extraction_error(const std::string&) {}
        virtual void package_extraction_complete() {}

        virtual void package_check_start() {}
        virtual void package_check_progress() {}
        virtual void package_check_error(const std::string&) {}
        virtual void package_check_complete() {}
    };


    class TransferEngineInterface {
    public:
        TransferEngineInterface(const TransferEngineInterface& orig)        = delete;
        TransferEngineInterface(TransferEngineInterface&&)                  = delete;
        TransferEngineInterface& operator=(const TransferEngineInterface&)  = delete;
        TransferEngineInterface& operator=(TransferEngineInterface&&)       = delete;

        TransferEngineInterface() {}
        virtual ~TransferEngineInterface() {}

    public:
        virtual std::shared_ptr<detail::PackageTask> create_download_task(PackageInfo& package) = 0;
        virtual std::shared_ptr<detail::PackageTask> create_start_seeding_task(PackageInfo& package) = 0;
        virtual std::shared_ptr<detail::PackageTask> create_stop_seeding_task(PackageInfo& package) = 0;

    protected:
        fc::mutex   _mutex;
        fc::thread  _thread;
    };


    class PackageManager {
    private:
        explicit PackageManager(const boost::filesystem::path& packages_path);

    public:
        PackageManager()                                  = delete;
        PackageManager(const PackageManager&)             = delete;
        PackageManager(PackageManager&&)                  = delete;
        PackageManager& operator=(const PackageManager&)  = delete;
        PackageManager& operator=(PackageManager&&)       = delete;

        ~PackageManager();

        static PackageManager& instance() {
            static PackageManager the_package_manager(graphene::utilities::decent_path_finder::instance().get_decent_data() / "packages");
            return the_package_manager;
        }

    public:
        package_handle_t get_package(const boost::filesystem::path& content_dir_path,
                                     const boost::filesystem::path& samples_dir_path,
                                     const fc::sha256& key);
        package_handle_t get_package(const std::string& url);
        package_handle_t get_package(const fc::ripemd160& hash);

        package_handle_t find_package(const std::string& url);
        package_handle_t find_package(const fc::ripemd160& hash);

        package_handle_set_t get_all_known_packages() const;
        void recover_all_packages(const event_listener_handle_t& event_listener = event_listener_handle_t());
        bool release_all_packages();

        bool release_package(const fc::ripemd160& hash);
        bool release_package(package_handle_t& package);

        boost::filesystem::path get_packages_path() const;
        void set_libtorrent_config(const boost::filesystem::path& libtorrent_config_file);

        TransferEngineInterface& get_proto_transfer_engine(const std::string& proto) const;

    private:
        mutable std::recursive_mutex    _mutex;
        boost::filesystem::path         _packages_path;
        package_handle_set_t            _packages;
        proto_to_transfer_engine_map_t  _proto_transfer_engines;
    };


} } // namespace decent::package
