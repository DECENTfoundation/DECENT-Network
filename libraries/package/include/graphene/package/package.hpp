
#pragma once

#include <decent/encrypt/custodyutils.hpp>
#include <graphene/utilities/dirhelper.hpp>

#include <fc/thread/thread.hpp>
#include <fc/thread/mutex.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/crypto/sha512.hpp>
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


namespace decent { namespace package {


    class PackageManager;
    class PackageInfo;
    class EventListenerInterface;
    class TransferEngineInterface;

    typedef std::shared_ptr<PackageInfo>                package_handle_t;
    typedef std::set<package_handle_t>                  package_handle_set_t;
    typedef std::shared_ptr<EventListenerInterface>     event_listener_handle_t;
    typedef std::list<event_listener_handle_t>          event_listener_handle_list_t;
    typedef std::shared_ptr<TransferEngineInterface>    transfer_engine_t;
    typedef std::map<std::string, transfer_engine_t>    proto_to_transfer_engine_map_t;


    namespace detail {


        class PackageTask;
        class CreatePackageTask;
        class DownloadPackageTask;
        class RemovePackageTask;
        class UnpackPackageTask;
        class SeedPackageTask;


    } // namespace detail


    class PackageInfo {
    public:
        enum DataState {
            DS_NONE = 0,
            INVALID,
            PARTIAL,
            UNCHECKED,
            CHECKED
        };

        enum TransferState {
            TS_NONE = 0,
            DOWNLOADING,
            SEEDING,
        };

        enum ManipulationState {
            MS_NONE = 0,
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
        friend class detail::CreatePackageTask;
        friend class detail::DownloadPackageTask;
        friend class detail::RemovePackageTask;
        friend class detail::UnpackPackageTask;
        friend class detail::SeedPackageTask;


        PackageInfo(PackageManager& manager,
                    const boost::filesystem::path& content_dir_path,
                    const boost::filesystem::path& samples_dir_path,
                    const fc::sha512& key);

        PackageInfo(PackageManager& manager,const fc::ripemd160& package_hash);

        PackageInfo(PackageManager& manager, const fc::url& url);

    public:
        PackageInfo(const PackageInfo&)             = delete;
        PackageInfo(PackageInfo&&)                  = delete;
        PackageInfo& operator=(const PackageInfo&)  = delete;
        PackageInfo& operator=(PackageInfo&&)       = delete;

        ~PackageInfo();

    public:
        void add_event_listener(const event_listener_handle_t& event_listener);
        void remove_event_listener(const event_listener_handle_t& event_listener);

        DataState get_data_state() const;
        TransferState get_transfer_state() const;
        ManipulationState get_manipulation_state() const;

        void create();
        void unpack(const boost::filesystem::path& dir_path, const fc::sha512& key);
        void download(const boost::filesystem::path& dir_path);
        void seed(const std::string& proto = "");
        void remove(bool block = false);

        void cancel_current_task(bool block = false);

    private:
        static boost::filesystem::path get_package_state_dir(const boost::filesystem::path& package_dir)  { return package_dir / ".state" ; }
        static boost::filesystem::path get_lock_file_path(const boost::filesystem::path& package_dir)     { return get_package_state_dir(package_dir) / "lock"; }

        boost::filesystem::path get_package_dir() const        { return _parent_dir / _hash.str(); }
        boost::filesystem::path get_package_state_dir() const  { return get_package_state_dir(get_package_dir()); }
        boost::filesystem::path get_lock_file_path() const     { return get_lock_file_path(get_package_dir()); }

        void lock_dir();
        void unlock_dir();

    private:
        mutable std::recursive_mutex  _mutex;
        std::shared_ptr<fc::thread>   _thread;

        DataState                     _data_state;
        TransferState                 _transfer_state;
        ManipulationState             _manipulation_state;

        fc::ripemd160                 _hash;
        fc::url                       _url;
        boost::filesystem::path       _parent_dir;

        std::shared_ptr<boost::interprocess::file_lock> _file_lock;
        std::shared_ptr<boost::interprocess::scoped_lock<boost::interprocess::file_lock>> _file_lock_guard;

        mutable std::recursive_mutex  _event_mutex;
        event_listener_handle_list_t  _event_listeners;

        mutable std::recursive_mutex                  _task_mutex;
        std::shared_ptr<detail::CreatePackageTask>    _create_task;
        std::shared_ptr<detail::DownloadPackageTask>  _download_task;
        std::shared_ptr<detail::PackageTask>          _current_task;
    };


    class EventListenerInterface {
    public:
        virtual ~EventListenerInterface() {}

        virtual void package_data_state_change(PackageInfo::DataState) {};
        virtual void package_transfer_state_change(PackageInfo::TransferState) {};
        virtual void package_manipulation_state_change(PackageInfo::ManipulationState) {};

        virtual void package_creation_start() {};
        virtual void package_creation_progress(const std::string&) {};
        virtual void package_creation_error(const std::string&) {};
        virtual void package_creation_complete() {};

        virtual void package_restoration_start() {};
        virtual void package_restoration_progress() {};
        virtual void package_restoration_error(const std::string&) {};
        virtual void package_restoration_complete() {};

        virtual void package_upload_start() {};
        virtual void package_upload_progress() {};
        virtual void package_upload_error(const std::string&) {};
        virtual void package_upload_complete() {};

        virtual void package_download_start() {};
        virtual void package_download_progress() {};
        virtual void package_download_error(const std::string&) {};
        virtual void package_download_complete() {};
    };


    class TransferEngineInterface {
    public:
        virtual ~TransferEngineInterface() {}

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
                                     const fc::sha512& key);
        package_handle_t get_package(const fc::url& url);
        package_handle_t get_package(const fc::ripemd160& hash);

        package_handle_set_t get_all_known_packages() const;
        void recover_all_packages(const event_listener_handle_t& event_listener = event_listener_handle_t());

        bool release_package(const fc::ripemd160& hash);
        bool release_package(package_handle_t& package);

        boost::filesystem::path get_packages_path() const;
        std::shared_ptr<fc::thread> get_thread() const;
        void set_libtorrent_config(const boost::filesystem::path& libtorrent_config_file);

    private:
        mutable std::recursive_mutex    _mutex;
        std::shared_ptr<fc::thread>     _thread;
        boost::filesystem::path         _packages_path;
        package_handle_set_t            _packages;
        proto_to_transfer_engine_map_t  _proto_transfer_engines;
    };


} } // decent::package







#pragma once

#include <decent/encrypt/crypto_types.hpp>
#include <decent/encrypt/custodyutils.hpp>

#include <fc/optional.hpp>
#include <fc/signals.hpp>
#include <fc/time.hpp>
#include <fc/thread/mutex.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/crypto/sha512.hpp>
#include <fc/network/url.hpp>

#include <boost/filesystem.hpp>

#include <vector>
#include <map>


namespace graphene { namespace package {


class report_stats_listener_base {
public:
    std::map<std::string, std::vector<std::string>> ipfs_IDs;

    virtual void report_stats( std::map<std::string,uint64_t> stats )=0;
};


class empty_report_stats_listener : public report_stats_listener_base {
public:
    static empty_report_stats_listener& instance() {
        static empty_report_stats_listener the_empty_report_stats_listener;
        return the_empty_report_stats_listener;
    }

    virtual void report_stats( std::map<std::string,uint64_t> stats ){};
};


class package_object {
public:
    package_object() = delete;
    explicit package_object(const boost::filesystem::path& package_path);

    boost::filesystem::path get_custody_file() const { return _package_path / "content.cus"; }
    boost::filesystem::path get_content_file() const { return _package_path / "content.zip.aes"; }
    boost::filesystem::path get_samples_path() const { return _package_path / "samples"; }
    const boost::filesystem::path& get_path() const { return _package_path; }

    void get_all_files(std::vector<boost::filesystem::path>& all_files) const;
	bool verify_hash() const;
	fc::ripemd160 get_hash() const { return _hash; }
	int get_size() const;
	bool is_valid() const { return _hash != fc::ripemd160(); }
	uint32_t create_proof_of_custody(const decent::encrypt::CustodyData& cd, decent::encrypt::CustodyProof& proof) const;

private:
    boost::filesystem::path  _package_path;
    fc::ripemd160            _hash;
};


    class package_transfer_interface : public decent::package::TransferEngineInterface {
public:
    typedef int transfer_id;

    struct transfer_progress {
        transfer_progress() : total_bytes(0), current_bytes(0), current_speed(0), str_status("No Status") { }
        transfer_progress(int tb, int cb, int cs, const std::string& status) : total_bytes(tb), current_bytes(cb), current_speed(cs), str_status(status) { }

        int total_bytes;
        int current_bytes;
        int current_speed; // Bytes per second
        std::string str_status;
    };

    class transfer_listener {
    public:
        virtual ~transfer_listener() = default;
        virtual void on_download_started(transfer_id id) = 0;
        virtual void on_download_finished(transfer_id id, package_object downloaded_package) = 0;
        virtual void on_download_progress(transfer_id id, transfer_progress progress) = 0;
        virtual void on_upload_started(transfer_id id, const std::string& url) = 0;
        virtual void on_error(transfer_id id, std::string error) = 0;
    };

    virtual ~package_transfer_interface() = default;
    virtual void upload_package(transfer_id id, const package_object& package, transfer_listener* listener) = 0;
    virtual void download_package(transfer_id id, const std::string& url, transfer_listener* listener, report_stats_listener_base& stats_listener) = 0;
    virtual void print_status() = 0;
    virtual transfer_progress get_progress() = 0;
    virtual std::string get_transfer_url() = 0;
    virtual std::shared_ptr<package_transfer_interface> clone() = 0;
};


class empty_transfer_listener : public package_transfer_interface::transfer_listener {
public:
    static empty_transfer_listener& instance() {
        static empty_transfer_listener the_empty_transfer_listener;
        return the_empty_transfer_listener;
    }

    virtual void on_download_started(package_transfer_interface::transfer_id id) { }
    virtual void on_download_finished(package_transfer_interface::transfer_id id, package_object downloaded_package) { }
    virtual void on_download_progress(package_transfer_interface::transfer_id id, package_transfer_interface::transfer_progress progress) { }
    virtual void on_upload_started(package_transfer_interface::transfer_id id, const std::string& url) { }
    virtual void on_error(package_transfer_interface::transfer_id id, std::string error) { }
};


class package_manager {
private:
    struct transfer_job {
        enum transfer_type {
            DOWNLOAD,
            UPLOAD
        };

        std::shared_ptr<package_transfer_interface>     transport;
        package_transfer_interface::transfer_listener*  listener;
        package_transfer_interface::transfer_id         job_id;
        transfer_type                                   job_type;
    };

private:
    typedef std::map<std::string, std::shared_ptr<package_transfer_interface>>  protocol_handler_map;
    typedef std::map<package_transfer_interface::transfer_id, transfer_job>     transfer_map;
    typedef std::set<package_object>                                            package_set;

private:
    package_manager();

public:
    package_manager(const package_manager&)             = delete;
    package_manager(package_manager&&)                  = delete;
    package_manager& operator=(const package_manager&)  = delete;
    package_manager& operator=(package_manager&&)       = delete;

    ~package_manager();

    static package_manager& instance() {
        static package_manager the_package_manager;
        return the_package_manager;
    }

public:
	package_object create_package( const boost::filesystem::path& content_path,
								   const boost::filesystem::path& samples, 
								   const fc::sha512& key,
                                   decent::encrypt::CustodyData& cd);

    bool unpack_package( const boost::filesystem::path& destination_directory, 
                         const package_object& package,
                         const fc::sha512& key);

    void delete_package(fc::ripemd160 hash);

    package_transfer_interface::transfer_id upload_package( const package_object& package, 
                                                            const std::string& protocol_name,
                                                            package_transfer_interface::transfer_listener& listener );

    package_transfer_interface::transfer_id download_package( const std::string& url,
                                                              package_transfer_interface::transfer_listener& listener,
                                                              report_stats_listener_base& stats_listener );


    std::vector<package_object> get_packages();
    package_object              get_package_object(fc::ripemd160 hash);

    std::string                                   get_transfer_url(package_transfer_interface::transfer_id id);
    package_transfer_interface::transfer_progress get_progress(std::string URI) const;

    void set_packages_path(const boost::filesystem::path& packages_path);
    boost::filesystem::path get_packages_path() const;

    void set_libtorrent_config(const boost::filesystem::path& libtorrent_config_file);
    boost::filesystem::path get_libtorrent_config() const;

    uint32_t create_proof_of_custody(const boost::filesystem::path& content_file, const decent::encrypt::CustodyData& cd, decent::encrypt::CustodyProof& proof);

    void print_all_transfers();

private:
    transfer_job& create_transfer_object();

    void save_state();
    void restore_state();

private:
    mutable fc::mutex                  _mutex;
    boost::filesystem::path            _packages_path;
    boost::filesystem::path            _libtorrent_config_file;
    decent::encrypt::CustodyUtils      _custody_utils;
	protocol_handler_map               _protocol_handlers;
	transfer_map                       _transfers;
    int                                _next_transfer_id;
    package_set                        _packages;
};


} } // graphene::package
